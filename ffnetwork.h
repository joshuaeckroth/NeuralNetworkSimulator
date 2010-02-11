#ifndef FFNETWORK_H
#define FFNETWORK_H

#include <vector>

#include <QObject>
#include <QFile>

class FFNetwork : public QObject
{
Q_OBJECT
public:
    explicit FFNetwork(QObject *parent, std::vector<unsigned int> _layers);
    void fillRandomWeights();
    std::vector<double> processInput(std::vector<double> input);
    void backprop(std::vector<double> output, std::vector<double> expected);

signals:

public slots:

private:
    std::vector<unsigned int> layers;
    double **weights;
    double **neuronVals;
    double **delta;
    QFile *file;
    int row;

    double sigmoid(double x);
    int activationFunc(double x);
};

#endif // FFNETWORK_H
