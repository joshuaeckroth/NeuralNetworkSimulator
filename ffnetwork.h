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
              unsigned int _averaged,
              double _stop,
              std::vector<std::vector<double> > _inputs,
              std::vector<std::vector<double> > _expected);
    ~FFNetwork();
    bool isSuccessful() const;
    void restart();
    void pause();
    void resume();
    void run();
    void quit();

signals:
    void epochMilestone(int id, int epoch, double error);

private:
    int id;
    std::vector<unsigned int> layers;
    std::vector<std::vector<double> > inputs;
    std::vector<std::vector<double> > expected;
    double ***weights;
    double ***prevWeightUpdates;
    double ***neuronVals;
    double ***delta;
    double eta;
    double momentum;
    unsigned int averaged;
    double stop;
    QMutex mutex;
    QWaitCondition runningCond;
    bool running;
    unsigned int epoch;
    double error;
    unsigned int *ordering;
    unsigned int ordered;
    unsigned int index;
    bool seen;
    std::vector<double> *output;
    bool quitNow;
    bool successful;

    void fillRandomWeights();
    void processInput(std::vector<double> input);
    void backprop(unsigned int a, std::vector<double> expected);
    double sigmoid(double x);
};

#endif // FFNETWORK_H
