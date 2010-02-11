#ifndef FFNETWORK_H
#define FFNETWORK_H

#include <vector>

#include <QObject>

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

    double sigmoid(double x);
};

#endif // FFNETWORK_H
