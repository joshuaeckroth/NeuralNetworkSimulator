#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <QDebug>
#include <QtGui/QApplication>

#include "mainwindow.h"
#include "ffnetwork.h"


int activationFunc(double x)
{
    return (x >= 0.5 ? 1 : 0);
}

int main(int argc, char *argv[])
{
    unsigned int l[3] = {4,4,1};
    std::vector<unsigned int> layers(l, l+3);
    FFNetwork network(0, layers);
    network.fillRandomWeights();

    std::vector<std::vector<double> > inputs;
    std::vector<std::vector<double> > expected;

    for(int i = 0; i < 2; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            for(int k = 0; k < 2; k++)
            {
                for(int l = 0; l < 2; l++)
                {
                    double a[4] = {i,j,k,l};
                    std::vector<double> inputVec(a, a+4);
                    double e[1] = {((i+j+k+l)%2 == 0) ? 0.0 : 1.0};
                    std::vector<double> expectedVec(e, e+1);
                    inputs.push_back(inputVec);
                    expected.push_back(expectedVec);
                }
            }
        }
    }

    std::vector<double> output;

    unsigned int *ordering = new unsigned int[inputs.size()];
    unsigned int ordered;
    unsigned int index;
    unsigned int epoch = 0;
    double error;
    bool seen;
    qsrand(std::time(NULL));
    while(true)
    {
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
                output = network.processInput(inputs[index]);
                error += std::fabs(output[0] - expected[index][0]);
                network.backprop(output, expected[index]);
            }
        }
        if(error/double(inputs.size()) <= 0.05) break;
    }
    delete[] ordering;

    qDebug() << QString("# of epochs: %1").arg(epoch);

    //QApplication app(argc, argv);
    //MainWindow w;
    //w.show();
    //return app.exec();
    return 0;
}
