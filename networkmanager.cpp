#include <cmath>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <iostream>
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

#include "networkmanager.h"
#include "ffnetwork.h"
#include "config.h"

NetworkManager::NetworkManager(QwtPlot *_plot)
    : numNetworks(0), averaged(0), networks(NULL), plot(_plot), curves(NULL),
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
        for(unsigned int a = 0; a < averaged; a++)
        {
            networks[i][a]->quit();
            networks[i][a]->wait();
            disconnect(networks[i][a], SIGNAL(epochMilestone(int,int,int,double)),
                       this, SLOT(epochMilestone(int,int,int,double)));
            disconnect(networks[i][a], SIGNAL(epochFinal(int,int,int)),
                       this, SLOT(epochFinal(int,int,int)));
            delete networks[i][a];
            delete epochMilestones[i][a];
            delete errors[i][a];
            if(markers[curves[i][a]] != NULL)
            {
                markers[curves[i][a]]->detach();
                delete markers[curves[i][a]];
            }
            curves[i][a]->detach();
            delete curves[i][a];
        }

        delete[] networks[i];
        delete[] epochMilestones[i];
        delete[] errors[i];
        delete[] curves[i];
        delete[] finals[i];
    }
    if(networks != NULL)
    {
        delete[] networks;
        delete[] epochMilestones;
        delete[] errors;
        delete[] curves;
        delete[] finals;
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
    averaged = c->getAveraged();
    double stop = c->getStop();

    if(etaEnd < 0.00001)
    {
        numNetworks = 0;
        mutex.unlock();
        return;
    }
    // determine number of networks
    numNetworks = int(floor((etaEnd - etaStart)/etaIncrement)) + 1;
    networks = new FFNetwork**[numNetworks];
    finals = new int*[numNetworks];
    epochMilestones = new QVector<double>**[numNetworks];
    errors = new QVector<double>**[numNetworks];
    curves = new QwtPlotCurve**[numNetworks];

    // for each eta, create a network
    double eta;
    int i;
    int r, g, b;
    for(eta = etaStart, i = 0; i < numNetworks; eta += etaIncrement, i++)
    {
        r = qrand() % 256;
        g = qrand() % 256;
        b = qrand() % 256;

        networks[i] = new FFNetwork*[averaged];
        finals[i] = new int[averaged];
        epochMilestones[i] = new QVector<double>*[averaged];
        errors[i] = new QVector<double>*[averaged];
        curves[i] = new QwtPlotCurve*[averaged];

        for(unsigned int a = 0; a < averaged; a++)
        {
            networks[i][a] = new FFNetwork(i, a, layers, eta, momentum, stop, inputs, expected);
            finals[i][a] = -1;
            connect(networks[i][a], SIGNAL(epochMilestone(int,int,int,double)),
                    this, SLOT(epochMilestone(int,int,int,double)));
            connect(networks[i][a], SIGNAL(epochFinal(int,int,int)),
                    this, SLOT(epochFinal(int,int,int)));
            networks[i][a]->start(QThread::IdlePriority);
            epochMilestones[i][a] = new QVector<double>;
            errors[i][a] = new QVector<double>;
            curves[i][a] = new QwtPlotCurve;
            curves[i][a]->setPen(QPen(QBrush(QColor(r,g,b)), 2.0));
            curves[i][a]->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
            curves[i][a]->attach(plot);
            if(a == 0)
            {
                curves[i][a]->setTitle((QString(QChar(0x03B7))+QString(" = %1, ")+
                                     QString(QChar(0x03B1))+QString(" = %2"))
                                    .arg(eta, 3, 'f', 2)
                                    .arg(momentum, 3, 'f', 2));
            }
            else
            {
                legend->remove(curves[i][a]);
            }
            highlightedCurves[curves[i][a]] = false;
            markers[curves[i][a]] = NULL;
        }
    }
    mutex.unlock();
}

void NetworkManager::epochMilestone(int id, int avgId, int epoch, double error)
{
    mutex.lock();

    *epochMilestones[id][avgId] << double(epoch);
    *errors[id][avgId] << error;
    curves[id][avgId]->setSamples(*epochMilestones[id][avgId], *errors[id][avgId]);

    // find mean and stddev for the finals in this network configuration
    // then stop this network (id,avgId) if it's way beyond the finals mean
    int avgFinalEpoch = 0;
    int count = 0;
    for(unsigned int a = 0; a < averaged; a++)
    {
        if(finals[id][a] == -1) continue;
        avgFinalEpoch += finals[id][a];
        count++;
    }
    if(count != 0)
    {
        avgFinalEpoch /= count;
        double stddevsum = 0.0;
        for(unsigned int a = 0; a < averaged; a++)
        {
            if(finals[id][a] == -1) continue;
            stddevsum += pow(avgFinalEpoch - finals[id][a], 2.0);
        }
        double stddev = sqrt(stddevsum / count);
        if(stddev > 0.0 && epoch > 3*stddev + avgFinalEpoch)
        {
            networks[id][avgId]->cancel();
        }
    }

    // continually remove the legend item
    // (legend seems to update and add the item when curve is updated)
    if(avgId != 0)
        legend->remove(curves[id][avgId]);

    plot->replot();

    if(isRunning)
    {
        bool someRunning = false;
        for(int i = 0; i < numNetworks; i++)
        {
            for(unsigned int a = 0; a < averaged; a++)
            {
                if(!networks[i][a]->isSuccessful())
                {
                    someRunning = true;
                }
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
                for(unsigned int a = 0; a < averaged; a++)
                {
                    if(networks[i][a]->isSuccessful()) continue;
                    if(epochMilestones[i][a]->isEmpty()) continue;
                    if(epochMilestones[i][a]->last() < newMinEpochMilestone)
                        newMinEpochMilestone = epochMilestones[i][a]->last();
                }
            }
            minEpochMilestone = newMinEpochMilestone;
            for(int i = 0; i < numNetworks; i++)
            {
                for(unsigned int a = 0; a < averaged; a++)
                {
                    if(networks[i][a]->isSuccessful()) continue;
                    if(!epochMilestones[i][a]->isEmpty()
                        && epochMilestones[i][a]->last() > minEpochMilestone)
                        networks[i][a]->pause();
                    else
                        networks[i][a]->resume();
                }
            }
        }
    }
    mutex.unlock();
}

void NetworkManager::epochFinal(int id, int avgId, int epoch)
{
    finals[id][avgId] = epoch;
    // determine if this network has completely finished
    bool done = true;
    for(unsigned int a = 0; a < averaged; a++)
    {
        done &= (finals[id][a] != -1);
    }
    if(done && highlightedCurves[curves[id][0]])
    {
        updateMarker(id);
    }
}

void NetworkManager::resume()
{
    mutex.lock();
    isRunning = true;
    for(int i = 0; i < numNetworks; i++)
    {
        for(unsigned int a = 0; a < averaged; a++)
        {
            networks[i][a]->resume();
            if(markers[curves[i][a]] != NULL)
                markers[curves[i][a]]->show();
        }
    }
    mutex.unlock();
}

void NetworkManager::pause()
{
    mutex.lock();
    isRunning = false;
    for(int i = 0; i < numNetworks; i++)
    {
        for(unsigned int a = 0; a < averaged; a++)
        {
            networks[i][a]->pause();
        }
    }
    mutex.unlock();
}

void NetworkManager::restart()
{
    mutex.lock();
    isRunning = false;
    for(int i = 0; i < numNetworks; i++)
    {
        for(unsigned int a = 0; a < averaged; a++)
        {
            networks[i][a]->restart();
            finals[i][a] = -1;
            epochMilestones[i][a]->clear();
            errors[i][a]->clear();
            curves[i][a]->setSamples(*epochMilestones[i][a], *errors[i][a]);
            // continually remove the legend item
            // (legend seems to update and add the item when curve is updated)
            if(a != 0)
                legend->remove(curves[i][a]);
        }
        if(markers[curves[i][0]] != NULL)
        {
            markers[curves[i][0]]->detach();
            delete markers[curves[i][0]];
            markers[curves[i][0]] = NULL;
        }
        highlightedCurves[curves[i][0]] = false;
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
        if(curves[id][0] == curve) break;
    }
    if(id == numNetworks) return; // shouldn't happen

    bool networkSuccessful = true;
    for(unsigned int a = 0; a < averaged; a++)
    {
        networkSuccessful &= networks[id][a]->isSuccessful();
    }

    highlightedCurves[curve] = on;
    QPen pen = curve->pen();
    if(on)
    {
        pen.setWidth(4);
        if(networkSuccessful)
        {
            updateMarker(id);
        }
    }
    else
    {
        pen.setWidth(2);
        if(markers[curves[id][0]] != NULL)
        {
            markers[curves[id][0]]->detach();
            delete markers[curves[id][0]];
            markers[curves[id][0]] = NULL;
        }
    }
    for(unsigned int a = 0; a < averaged; a++)
    {
        curves[id][a]->setPen(pen);

        // continually remove the legend item
        // (legend seems to update and add the item when curve is updated)
        if(a != 0)
            legend->remove(curves[id][a]);
    }
    plot->replot();
}

void NetworkManager::updateMarker(int id)
{
    if(markers[curves[id][0]] == NULL)
    {
        markers[curves[id][0]] = new QwtPlotMarker;
        markers[curves[id][0]]->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    }

    int avgEpochs = 0;
    int count = 0;
    for(unsigned int a = 0; a < averaged; a++)
    {
        if(finals[id][a] == -1) continue; // may be true if a network was "canceled"
        avgEpochs += finals[id][a];
        count++;
    }
    avgEpochs /= count;
    double stddevsum = 0.0;
    for(unsigned int a = 0; a < averaged; a++)
    {
        if(finals[id][a] == -1) continue;
        stddevsum += pow((double(finals[id][a]) - double(avgEpochs)), 2.0);
    }
    double stddev = sqrt(stddevsum / double(count));

    // do it again, ignoring any point that's two stddevs away
    int avgEpochs2 = 0;
    count = 0;
    for(unsigned int a = 0; a < averaged; a++)
    {
        if(stddev > 0.0 && double(finals[id][a]) > 2*stddev+avgEpochs) continue;
        avgEpochs2 += finals[id][a];
        count++;
    }
    avgEpochs2 /= count;
    double stddevsum2 = 0.0;
    for(unsigned int a = 0; a < averaged; a++)
    {
        if(stddev > 0.0 && double(finals[id][a]) > 2*stddev+avgEpochs) continue;
        stddevsum2 += pow((double(finals[id][a]) - double(avgEpochs2)), 2.0);
    }
    double stddev2 = sqrt(stddevsum2 / double(count));

    markers[curves[id][0]]->setXValue(avgEpochs2);
    markers[curves[id][0]]->setYValue(0.1);
    if(avgEpochs != avgEpochs2)
    {
        QString text = QString(QChar(0x03BC))+QString("=%1/%2, ")
                       .arg(avgEpochs).arg(avgEpochs2) +
                       QString(QChar(0x03C3))+QString("=%1/%2")
                       .arg(int(stddev)).arg(int(stddev2));
        markers[curves[id][0]]->setLabel(QwtText(text));
        cout << (QString("%1 - %2").arg(networks[id][0]->toString())
                 .arg(text).toAscii().data()) << endl;
    }
    else
    {
        QString text = QString(QChar(0x03BC))+QString("=%1, ").arg(avgEpochs) +
                       QString(QChar(0x03C3))+QString("=%1").arg(int(stddev));
        markers[curves[id][0]]->setLabel(QwtText(text));
        cout << (QString("%1 - %2").arg(networks[id][0]->toString())
                 .arg(text).toAscii().data()) << endl;
    }

    markers[curves[id][0]]->attach(plot);
    plot->replot();
}
