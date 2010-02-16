#include <cmath>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
using namespace std;

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_marker.h>

#include <QPen>
#include <QBrush>
#include <QColor>
#include <QChar>
#include <QDebug>

#include "networkmanager.h"
#include "ffnetwork.h"
#include "config.h"

NetworkManager::NetworkManager(QwtPlot *_plot)
    : numNetworks(0), networks(NULL), plot(_plot), curves(NULL),
    minEpochMilestone(-1.0), isRunning(false)
{
    legend = new QwtLegend;
    legend->setItemMode(QwtLegend::CheckableItem);
    plot->insertLegend(legend, QwtPlot::RightLegend);
    plot->setCanvasBackground(QColor(255,255,255));
    connect(plot, SIGNAL(legendChecked(QwtPlotItem*, bool)),
            this, SLOT(legendChecked(QwtPlotItem*, bool)));
}

void NetworkManager::networksFromConfig(Config *c)
{
    mutex.lock();
    isRunning = false;

    // stop and delete existing networks
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->quit();
        networks[i]->wait();
        disconnect(networks[i], SIGNAL(epochMilestone(int,int,double)),
                   this, SLOT(epochMilestone(int,int,double)));
        delete networks[i];
        delete epochMilestones[i];
        delete errors[i];
        if(markers[curves[i]] != NULL)
        {
            markers[curves[i]]->detach();
            delete markers[curves[i]];
        }
        curves[i]->detach();
        delete curves[i];
    }
    if(networks != NULL)
    {
        delete[] networks;
        delete[] epochMilestones;
        delete[] errors;
        delete[] curves;
        legend->clear();
        highlightedCurves.clear();
        markers.clear();
        networks = NULL;
    }
    plot->replot();

    vector<vector<double> > inputs;
    vector<vector<double> > expected;

    // create inputs & expected values
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++)
            for(int k = 0; k < 2; k++)
                for(int l = 0; l < 2; l++)
                {
                    double a[4] = {i,j,k,l};
                    vector<double> inputVec(a, a+4);
                    double e[1] = {((i+j+k+l)%2 == 0) ? 0.0 : 1.0};
                    vector<double> expectedVec(e, e+1);
                    inputs.push_back(inputVec);
                    expected.push_back(expectedVec);
                }

    unsigned int l[3] = {4,4,1};
    vector<unsigned int> layers(l, l+3);

    double etaStart = c->getEtaStart();
    double etaEnd = c->getEtaEnd();
    double etaIncrement = c->getEtaIncrement();
    double momentum = c->getMomentum();
    unsigned int averaged = c->getAveraged();
    double stop = c->getStop();

    if(etaEnd < 0.00001)
    {
        numNetworks = 0;
        mutex.unlock();
        return;
    }
    // determine number of networks
    numNetworks = int(floor((etaEnd - etaStart)/etaIncrement)) + 1;
    networks = new FFNetwork*[numNetworks];
    epochMilestones = new QVector<double>*[numNetworks];
    errors = new QVector<double>*[numNetworks];
    curves = new QwtPlotCurve*[numNetworks];

    // for each eta, create a network
    double eta;
    int i;
    int r, g, b;
    for(eta = etaStart, i = 0; i < numNetworks; eta += etaIncrement, i++)
    {
        r = qrand() % 256;
        g = qrand() % 256;
        b = qrand() % 256;
        networks[i] = new FFNetwork(i, layers, eta, momentum, averaged, stop, inputs, expected);
        connect(networks[i], SIGNAL(epochMilestone(int, int, double)),
                this, SLOT(epochMilestone(int, int, double)));
        networks[i]->start(QThread::IdlePriority);
        epochMilestones[i] = new QVector<double>;
        errors[i] = new QVector<double>;
        curves[i] = new QwtPlotCurve;
        curves[i]->setPen(QPen(QBrush(QColor(r,g,b)), 2.0));
        curves[i]->setTitle((QString(QChar(0x03B7))+QString(" = %1, ")+
                             QString(QChar(0x03B1))+QString(" = %2"))
                            .arg(eta, 3, 'f', 2)
                            .arg(momentum, 3, 'f', 2));
        curves[i]->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
        curves[i]->attach(plot);
        highlightedCurves[curves[i]] = false;
        markers[curves[i]] = NULL;
    }
    mutex.unlock();
}

void NetworkManager::epochMilestone(int id, int epoch, double error)
{
    mutex.lock();

    *epochMilestones[id] << double(epoch);
    *errors[id] << error;
    curves[id]->setSamples(*epochMilestones[id], *errors[id]);
    if(highlightedCurves[curves[id]])
    {
        updateMarker(markers[curves[id]], double(epoch), error);
    }
    plot->replot();

    if(isRunning)
    {
        bool someRunning = false;
        for(int i = 0; i < numNetworks; i++)
        {
            if(!networks[i]->isSuccessful())
            {
                someRunning = true;
            }
        }
        if(!someRunning)
        {
            isRunning = false;
            emit stopped();
            mutex.unlock();
            return;
        }

        // pause faster networks
        if(minEpochMilestone < 0.0)
            minEpochMilestone = double(epoch);
        else
        {
            double newMinEpochMilestone = double(epoch);
            for(int i = 0; i < numNetworks; i++)
            {
                if(networks[i]->isSuccessful()) continue;
                if(epochMilestones[i]->isEmpty()) continue;
                if(epochMilestones[i]->last() < newMinEpochMilestone)
                    newMinEpochMilestone = epochMilestones[i]->last();
            }
            minEpochMilestone = newMinEpochMilestone;
            for(int i = 0; i < numNetworks; i++)
            {
                if(networks[i]->isSuccessful()) continue;
                if(!epochMilestones[i]->isEmpty()
                    && epochMilestones[i]->last() > minEpochMilestone)
                    networks[i]->pause();
                else
                    networks[i]->resume();
            }
        }
    }
    mutex.unlock();
}

void NetworkManager::resume()
{
    mutex.lock();
    isRunning = true;
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->resume();
        if(markers[curves[i]] != NULL)
            markers[curves[i]]->show();
    }
    mutex.unlock();
}

void NetworkManager::pause()
{
    mutex.lock();
    isRunning = false;
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->pause();
    }
    mutex.unlock();
}

void NetworkManager::restart()
{
    mutex.lock();
    isRunning = false;
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->restart();
        epochMilestones[i]->clear();
        errors[i]->clear();
        curves[i]->setSamples(*epochMilestones[i], *errors[i]);
        if(markers[curves[i]] != NULL)
            markers[curves[i]]->hide();
    }
    plot->replot();
    mutex.unlock();
}

void NetworkManager::legendChecked(QwtPlotItem *item, bool on)
{
    QwtPlotCurve *curve = dynamic_cast<QwtPlotCurve*>(item);
    // determine id
    int id;
    for(id = 0; id < numNetworks; id++)
    {
        if(curves[id] == curve) break;
    }
    if(id == numNetworks) return; // shouldn't happen

    highlightedCurves[curve] = on;
    QPen pen = curve->pen();
    if(on)
    {
        pen.setWidth(4);
        markers[curve] = new QwtPlotMarker;
        markers[curve]->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        updateMarker(markers[curve], epochMilestones[id]->last(), errors[id]->last());
        markers[curve]->attach(plot);
    }
    else
    {
        pen.setWidth(2);
        markers[curve]->detach();
        delete markers[curve];
        markers[curve] = NULL;
    }
    curve->setPen(pen);
    plot->replot();
}

void NetworkManager::updateMarker(QwtPlotMarker *marker, double epoch, double error)
{
    marker->setXValue(epoch + 500);
    marker->setYValue(error - 0.01);
    marker->setLabel(QwtText(QString("%1 (%2)")
                             .arg(epoch)
                             .arg(error, 3, 'f', 2)));
}
