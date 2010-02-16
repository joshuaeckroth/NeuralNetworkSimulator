#include <vector>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <cmath>
using namespace std;

#include "ffnetwork.h"

/**
  * Initializes a feed-foward network with the architecture
  * determined by the layers vector.
  */
FFNetwork::FFNetwork(int _id,
                     std::vector<unsigned int>_layers,
                     double _eta,
                     double _momentum,
                     unsigned int _averaged,
                     double _stop,
                     vector<vector<double> > _inputs,
                     vector<vector<double> > _expected) :
    id(_id), layers(_layers), inputs(_inputs), expected(_expected),
    eta(_eta), momentum(_momentum), averaged(_averaged),
    stop(_stop), quitNow(false), successful(false)
{
    assert(layers.size() > 1);

    running = false;
    epoch = 0;
    error = 0.0;

    weights = new double**[averaged];
    prevWeightUpdates = new double**[averaged];
    neuronVals = new double**[averaged];
    delta = new double**[averaged];
    output = new vector<double>[averaged];
    for(unsigned int a = 0; a < averaged; a++)
    {
        weights[a] = new double*[layers.size() - 1];
        prevWeightUpdates[a] = new double*[layers.size() - 1];

        // need #layers neuron values because neuronVals[0] will hold input values
        neuronVals[a] = new double*[layers.size()];
        neuronVals[a][0] = new double[layers[0]];

        delta[a] = new double*[layers.size()-1];

        for(unsigned int i = 1; i < layers.size(); i++)
        {
            // each layer has n*p + n weights
            // where n = number of neurons on this layer (layers[i])
            // and p = number of neurons on previous layer (layers[i-1]);
            // the (+ n) is for n bias terms;

            // the weights for neuron j (counted from 0) on a layer i are in position
            // weights[i-1][j*(p+1)], weights[i-1][j*(p+1)+1], ..., weights[i-1][j*(p+1)+(p-1)]
            // with bias weights[i-1][j*(p+1)+p]
            weights[a][i-1] = new double[layers[i]*layers[i-1] + layers[i]];
            prevWeightUpdates[a][i-1] = new double[layers[i]*layers[i-1] + layers[i]];

            // since each layer has n neurons, we need n cells to hold the output
            // of each neuron (-1 or 1)
            neuronVals[a][i] = new double[layers[i]];

            delta[a][i-1] = new double[layers[i]];
        }
    }

    ordering = new unsigned int[inputs.size()];

    fillRandomWeights();
}

FFNetwork::~FFNetwork()
{
    delete[] ordering;
}

void FFNetwork::run()
{
    double sampleError;
    double errorSum;
    double *errorAvgPerInput = new double[inputs.size()];
    bool withinError;

    forever
    {
        mutex.lock();
        while(!running)
        {
            runningCond.wait(&mutex);
        }
        if(quitNow)
        {
            mutex.unlock();
            return;
        }

        epoch++;
        error = 0.0;
        ordered = 0;
        while(ordered < inputs.size())
        {
            index = qrand() % inputs.size();

            // check if we've seen this number
            seen = false;
            for(unsigned int i = 0; i < ordered; i++)
            {
                if(ordering[i] == index)
                {
                    seen = true;
                    break;
                }
            }
            if(!seen)
            {
                ordering[ordered++] = index;
                processInput(inputs[index]);
                errorSum = 0.0;
                errorAvgPerInput[index] = 0.0;
                for(unsigned int a = 0; a < averaged; a++)
                {
                    sampleError = fabs(output[a][0] - expected[index][0]);
                    errorAvgPerInput[index] += sampleError;
                    errorSum += sampleError;
                    backprop(a, expected[index]);
                }
                error += (errorSum / double(averaged));
                errorAvgPerInput[index] /= double(averaged);
            }
        }
        withinError = true;
        for(unsigned int index = 0; index < inputs.size(); index++)
        {
            withinError &= (errorAvgPerInput[index] < stop);
        }
        if(withinError)
        {
            emit epochMilestone(id, epoch, error/double(inputs.size()));
            running = false;
            successful = true;
        }
        else if(epoch % 1000 == 0)
        {
            emit epochMilestone(id, epoch, error/double(inputs.size()));
        }
        mutex.unlock();
    }
}

bool FFNetwork::isSuccessful() const
{
    return successful;
}

void FFNetwork::restart()
{
    mutex.lock();
    running = false;
    successful = false;
    epoch = 0;
    error = 0.0;
    fillRandomWeights();
    mutex.unlock();
}

void FFNetwork::pause()
{
    mutex.lock();
    running = false;
    mutex.unlock();
}

void FFNetwork::resume()
{
    mutex.lock();
    if(!successful)
    {
        running = true;
        runningCond.wakeAll();
    }
    mutex.unlock();
}

void FFNetwork::quit()
{
    mutex.lock();
    quitNow = true;
    running = true;
    runningCond.wakeAll();
    mutex.unlock();
}

void FFNetwork::fillRandomWeights()
{
    for(unsigned int a = 0; a < averaged; a++)
    {
        for(unsigned int i = 1; i < layers.size(); i++)
        {
            for(unsigned int j = 0; j < (layers[i]*layers[i-1] + layers[i]); j++)
            {
                // random floating-point number between -1 and 1
                weights[a][i-1][j] = (rand() - RAND_MAX/2)/static_cast<double>(RAND_MAX/2);

                // set previous weight update to 0.0
                prevWeightUpdates[a][i-1][j] = 0.0;
            }
        }
    }
}

void FFNetwork::processInput(vector<double> input)
{
    assert(input.size() == layers[0]);

    for(unsigned int a = 0; a < averaged; a++)
    {
        // fill neuronVals[0] with input values
        for(unsigned i = 0; i < layers[0]; i++)
        {
            neuronVals[a][0][i] = input[i];
        }

        double sum;
        for(unsigned int i = 1; i < layers.size(); i++)
        {
            // for each neuron in layer
            for(unsigned int j = 0; j < layers[i]; j++)
            {
                sum = 0.0;

                // find sum with weights (ignore bias when counting with w)
                for(unsigned int w = 0; w < layers[i-1]; w++)
                {
                    sum += neuronVals[a][i-1][w] * weights[a][i-1][j*(layers[i-1]+1)+w];
                }

                // add bias
                sum += weights[a][i-1][j*(layers[i-1]+1) + layers[i-1]];

                // set neuron's final value
                neuronVals[a][i][j] = sigmoid(sum);
            }
        }

        // determine final output
        output[a] = vector<double>(layers[layers.size()-1]);
        for(unsigned int j = 0; j < layers[layers.size()-1]; j++)
        {
            output[a][j] = neuronVals[a][layers.size()-1][j];
        }
    }
}

void FFNetwork::backprop(unsigned int a, vector<double> expected)
{
    double sum;
    double weightUpdate;
    int weightIndexA, weightIndexB;

    // adjust weights on output layer

    // for each output neuron
    for(unsigned int j = 0; j < layers[layers.size()-1]; j++)
    {
        delta[a][layers.size()-2][j] = output[a][j] * (1 - output[a][j]) *
                                        (expected[j] - output[a][j]);

        // for each weight going to output layer
        // (ignore bias when counting with w)
        for(unsigned int w = 0; w < layers[layers.size()-2]; w++)
        {
            weightIndexA = layers.size()-2;
            weightIndexB = j*(layers[layers.size()-2]+1) + w;

            weightUpdate = eta * delta[a][layers.size()-2][j] * neuronVals[a][layers.size()-2][w] +
                           momentum * prevWeightUpdates[a][weightIndexA][weightIndexB];
            weights[a][weightIndexA][weightIndexB] += weightUpdate;
            prevWeightUpdates[a][weightIndexA][weightIndexB] = weightUpdate;
        }
        // update bias
        weightIndexA = layers.size()-2;
        weightIndexB = j*(layers[layers.size()-2]+1) + layers[layers.size()-2];
        weightUpdate = eta * delta[a][layers.size()-2][j]
                       + momentum * prevWeightUpdates[a][weightIndexA][weightIndexB];
        weights[a][weightIndexA][weightIndexB] += weightUpdate;
        prevWeightUpdates[a][weightIndexA][weightIndexB] = weightUpdate;
    }

    // for each hidden layer (backwards)
    for(unsigned int i = layers.size()-2; i > 0; i--)
    {
        // for each neuron on this layer
        for(unsigned int j = 0; j < layers[i]; j++)
        {
            sum = 0.0;
            // for every neuron that j connects to (forward)
            for(unsigned int k = 0; k < layers[i+1]; k++)
            {
                sum += weights[a][i][k*(layers[i]+1) + j] * delta[a][i][k];
            }
            delta[a][i-1][j] = neuronVals[a][i][j] * (1 - neuronVals[a][i][j]) * sum;

            // for each weight going to this layer
            // (ignore bias when counting with w)
            for(unsigned int w = 0; w < layers[i-1]; w++)
            {
                weightIndexA = i-1;
                weightIndexB = j*(layers[i]+1) + w;
                weightUpdate = eta * delta[a][i-1][j] * neuronVals[a][i-i][w]
                               + momentum * prevWeightUpdates[a][weightIndexA][weightIndexB];
                weights[a][weightIndexA][weightIndexB] += weightUpdate;
                prevWeightUpdates[a][weightIndexA][weightIndexB] = weightUpdate;
            }
            // update bias
            weightIndexA = i-1;
            weightIndexB = j*(layers[i]+1) + layers[i];
            weightUpdate = eta * delta[a][i-1][j]
                           + momentum * prevWeightUpdates[a][weightIndexA][weightIndexB];
            weights[a][weightIndexA][weightIndexB] += weightUpdate;
            prevWeightUpdates[a][weightIndexA][weightIndexB] = weightUpdate;
        }
    }
}

double FFNetwork::sigmoid(double x)
{
    return 1.0/(1.0+exp(-x));
}

