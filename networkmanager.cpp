#include <cmath>
#include <vector>
using namespace std;

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>

#include "networkmanager.h"
#include "ffnetwork.h"
#include "config.h"

NetworkManager::NetworkManager(QwtPlot *_plot)
    : numNetworks(0), networks(NULL), plot(_plot), curves(NULL)
{
}

void NetworkManager::networksFromConfig(Config *c)
{
    // stop and delete existing networks
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->pause();
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

    // determine number of networks
    numNetworks = int(floor((etaEnd - etaStart)/etaIncrement));
    networks = new FFNetwork*[numNetworks];
    epochMilestones = new QVector<double>*[numNetworks];
    errors = new QVector<double>*[numNetworks];
    curves = new QwtPlotCurve*[numNetworks];

    // for each eta, create a network
    double eta;
    int i;
    for(eta = etaStart, i = 0; i < numNetworks; eta += etaIncrement, i++)
    {
        networks[i] = new FFNetwork(i, layers, eta, momentum, inputs, expected);
        connect(networks[i], SIGNAL(epochMilestone(int, int, double)),
                this, SLOT(epochMilestone(int, int, double)));
        networks[i]->start();
        epochMilestones[i] = new QVector<double>;
        errors[i] = new QVector<double>;
        curves[i] = new QwtPlotCurve;
        curves[i]->attach(plot);
    }
}

void NetworkManager::epochMilestone(int id, int epoch, double error)
{
    mutex.lock();
    *epochMilestones[id] << double(epoch);
    *errors[id] << error;
    curves[id]->setSamples(*epochMilestones[id], *errors[id]);
    plot->replot();
    mutex.unlock();
}

void NetworkManager::resume()
{
    mutex.lock();
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->resume();
    }
    mutex.unlock();
}

void NetworkManager::pause()
{
    mutex.lock();
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->pause();
    }
    mutex.unlock();
}

void NetworkManager::restart()
{
    mutex.lock();
    for(int i = 0; i < numNetworks; i++)
    {
        networks[i]->restart();
        epochMilestones[i]->clear();
        errors[i]->clear();
        curves[i]->setSamples(*epochMilestones[i], *errors[i]);
    }
    mutex.unlock();
}
