#include <vector>
#include <cstdlib>
#include <ctime>

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
    int total = 0;
    int correct = 0;
    unsigned int *ordering = new unsigned int[inputs.size()];
    unsigned int ordered = 0;
    unsigned int index;
    bool seen;
    qsrand(std::time(NULL));
    for(unsigned int epoch = 0; epoch < 100000; epoch++)
    {
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
                if(activationFunc(output[0]) == (int)(expected[index][0])) correct++;
                network.backprop(output, expected[index]);
                total++;
            }
        }
        ordered = 0;
    }
    delete[] ordering;

    qDebug() << QString("%1/%2 = %3").arg(correct).arg(total).arg(double(correct)/double(total), 5, 'f', 3);

    //QApplication app(argc, argv);
    //MainWindow w;
    //w.show();
    //return app.exec();
    return 0;
}
