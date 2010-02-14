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
    FFNetwork(int _id,
              std::vector<unsigned int> _layers,
              double _eta,
              double _momentum,
              std::vector<std::vector<double> > _inputs,
              std::vector<std::vector<double> > _expected);
    ~FFNetwork();
    void restart();
    void pause();
    void resume();
    void run();

signals:
    void epochMilestone(int id, int epoch, double error);

private:
    int id;
    std::vector<unsigned int> layers;
    std::vector<std::vector<double> > inputs;
    std::vector<std::vector<double> > expected;
    double **weights;
    double **neuronVals;
    double **delta;
    double eta;
    double momentum;
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
