#ifndef FFNETWORK_H
#define FFNETWORK_H

#include <vector>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class FFNetwork : public QThread
{
Q_OBJECT
public:
    FFNetwork(std::vector<unsigned int> _layers,
              std::vector<std::vector<double> > _inputs,
              std::vector<std::vector<double> > _expected);
    ~FFNetwork();
    void run();

signals:
    void epochMilestone(int epoch, double error);

public slots:
    void restart();
    void pause();
    void resume();

private:
    std::vector<unsigned int> layers;
    std::vector<std::vector<double> > inputs;
    std::vector<std::vector<double> > expected;
    double **weights;
    double **neuronVals;
    double **delta;
    QMutex mutex;
    QWaitCondition running;
    bool isRunning;
    unsigned int epoch;
    double error;
    unsigned int *ordering;
    unsigned int ordered;
    unsigned int index;
    bool seen;
    std::vector<double> output;

    void fillRandomWeights();
    std::vector<double> processInput(std::vector<double> input);
    void backprop(std::vector<double> output, std::vector<double> expected);
    double sigmoid(double x);
};

#endif // FFNETWORK_H
