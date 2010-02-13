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
FFNetwork::FFNetwork(vector<unsigned int> _layers,
                     vector<vector<double> > _inputs,
                     vector<vector<double> > _expected) :
    layers(_layers), inputs(_inputs), expected(_expected)
{
    assert(layers.size() > 1);

    isRunning = false;
    epoch = 0;
    error = 0.0;

    weights = new double*[layers.size() - 1];

    // need #layers neuron values because neuronVals[0] will hold input values
    neuronVals = new double*[layers.size()];
    neuronVals[0] = new double[layers[0]];

    delta = new double*[layers.size()-1];

    for(unsigned int i = 1; i < layers.size(); i++)
    {
        // each layer has n*p + n weights
        // where n = number of neurons on this layer (layers[i])
        // and p = number of neurons on previous layer (layers[i-1]);
        // the (+ n) is for n bias terms;

        // the weights for neuron j (counted from 0) on a layer i are in position
        // weights[i-1][j*(p+1)], weights[i-1][j*(p+1)+1], ..., weights[i-1][j*(p+1)+(p-1)]
        // with bias weights[i-1][j*(p+1)+p]
        weights[i-1] = new double[layers[i]*layers[i-1] + layers[i]];

        // since each layer has n neurons, we need n cells to hold the output
        // of each neuron (-1 or 1)
        neuronVals[i] = new double[layers[i]];

        delta[i-1] = new double[layers[i]];
    }

    ordering = new unsigned int[inputs.size()];

    qsrand(std::time(NULL));

    fillRandomWeights();
}

FFNetwork::~FFNetwork()
{
    delete[] ordering;
}

void FFNetwork::run()
{
    forever
    {
        mutex.lock();
        if(!isRunning)
        {
            running.wait(&mutex);
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
                output = processInput(inputs[index]);
                error += fabs(output[0] - expected[index][0]);
                backprop(output, expected[index]);
            }
        }
        if(epoch % 1000 == 0)
        {
            emit epochMilestone(epoch, error/double(inputs.size()));
        }
        mutex.unlock();
    }
}

void FFNetwork::restart()
{
    mutex.lock();
    isRunning = false;
    epoch = 0;
    error = 0.0;
    fillRandomWeights();
    mutex.unlock();
}

void FFNetwork::pause()
{
    mutex.lock();
    isRunning = false;
    mutex.unlock();
}

void FFNetwork::resume()
{
    mutex.lock();
    isRunning = true;
    running.wakeAll();
    mutex.unlock();
}

void FFNetwork::fillRandomWeights()
{
    for(unsigned int i = 1; i < layers.size(); i++)
    {
        for(unsigned int j = 0; j < (layers[i]*layers[i-1] + layers[i]); j++)
        {
            // random floating-point number between -1 and 1
            weights[i-1][j] = (rand() - RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
        }
    }
}

vector<double> FFNetwork::processInput(vector<double> input)
{
    assert(input.size() == layers[0]);

    // fill neuronVals[0] with input values
    for(unsigned i = 0; i < layers[0]; i++)
    {
        neuronVals[0][i] = input[i];
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
                sum += neuronVals[i-1][w] * weights[i-1][j*(layers[i-1]+1)+w];
            }

            // add bias
            sum += weights[i-1][j*(layers[i-1]+1) + layers[i-1]];

            // set neuron's final value
            neuronVals[i][j] = sigmoid(sum);
        }
    }

    // determine final output
    vector<double> output(layers[layers.size()-1]);
    for(unsigned int j = 0; j < layers[layers.size()-1]; j++)
    {
        output[j] = neuronVals[layers.size()-1][j];
    }

    return output;
}

void FFNetwork::backprop(vector<double> output, vector<double> expected)
{
    assert(output.size() == expected.size());

    double eta = 0.5;
    double sum;

    // adjust weights on output layer

    // for each output neuron
    for(unsigned int j = 0; j < layers[layers.size()-1]; j++)
    {
        delta[layers.size()-2][j] = output[j] * (1 - output[j]) * (expected[j] - output[j]);

        // for each weight going to output layer
        // (ignore bias when counting with w)
        for(unsigned int w = 0; w < layers[layers.size()-2]; w++)
        {
            weights[layers.size()-2][j*(layers[layers.size()-2]+1) + w] +=
                eta * delta[layers.size()-2][j] * neuronVals[layers.size()-2][w];
        }
        // update bias
        weights[layers.size()-2][j*(layers[layers.size()-2]+1) + layers[layers.size()-2]] +=
                eta * delta[layers.size()-2][j];
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
                sum += weights[i][k*(layers[i]+1) + j] * delta[i][k];
            }
            delta[i-1][j] = neuronVals[i][j] * (1 - neuronVals[i][j]) * sum;

            // for each weight going to this layer
            // (ignore bias when counting with w)
            for(unsigned int w = 0; w < layers[i-1]; w++)
            {
                weights[i-1][j*(layers[i]+1) + w] += eta * delta[i-1][j] * neuronVals[i-i][w];
            }
            // update bias
            weights[i-1][j*(layers[i]+1) + layers[i]] += eta * delta[i-1][j];
        }
    }
}

double FFNetwork::sigmoid(double x)
{
    return 1.0/(1.0+exp(-x));
}

