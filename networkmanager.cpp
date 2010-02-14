#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
using namespace std;

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_legend.h>

#include <QPen>
#include <QBrush>
#include <QColor>
#include <QChar>

#include "networkmanager.h"
#include "ffnetwork.h"
#include "config.h"

NetworkManager::NetworkManager(QwtPlot *_plot)
    : numNetworks(0), networks(NULL), plot(_plot), curves(NULL),
    minEpochMilestone(-1.0), isRunning(false)
{
    legend = new QwtLegend;
    plot->insertLegend(legend, QwtPlot::RightLegend);
    plot->setCanvasBackground(QColor(255,255,255));
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

    if(etaEnd < 0.00001)
    {
        numNetworks = 0;
        mutex.unlock();
        return;
    }
    // determine number of networks
    numNetworks = int(ceil((etaEnd - etaStart + etaIncrement)/etaIncrement));
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
        networks[i] = new FFNetwork(i, layers, eta, momentum, inputs, expected);
        connect(networks[i], SIGNAL(epochMilestone(int, int, double)),
                this, SLOT(epochMilestone(int, int, double)));
        networks[i]->start(QThread::IdlePriority);
        epochMilestones[i] = new QVector<double>;
        errors[i] = new QVector<double>;
        curves[i] = new QwtPlotCurve;
        curves[i]->setPen(QPen(QBrush(QColor(r,g,b)), 2.0));
        curves[i]->setTitle((QString(QChar(0x03B7))+QString(" = %1")).arg(eta, 3, 'f', 2));
        curves[i]->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
        curves[i]->attach(plot);
    }
    mutex.unlock();
}

void NetworkManager::epochMilestone(int id, int epoch, double error)
{
    mutex.lock();
    if(isRunning)
    {
        *epochMilestones[id] << double(epoch);
        *errors[id] << error;
        curves[id]->setSamples(*epochMilestones[id], *errors[id]);
        plot->replot();

        // pause faster networks
        if(minEpochMilestone < 0.0)
            minEpochMilestone = double(epoch);
        else
        {
            double newMinEpochMilestone = double(epoch);
            for(int i = 0; i < numNetworks; i++)
            {
                if(epochMilestones[i]->isEmpty()) continue;
                if(epochMilestones[i]->last() < newMinEpochMilestone)
                    newMinEpochMilestone = epochMilestones[i]->last();
            }
            minEpochMilestone = newMinEpochMilestone;
            for(int i = 0; i < numNetworks; i++)
            {
                if(!epochMilestones[i]->isEmpty() && epochMilestones[i]->last() > minEpochMilestone)
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
    }
    plot->replot();
    mutex.unlock();
}
