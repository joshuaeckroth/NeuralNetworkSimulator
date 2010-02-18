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
              int _avgId,
              std::vector<unsigned int> _layers,
              double _eta,
              double _momentum,
              double _stop,
              std::vector<std::vector<double> > _inputs,
              std::vector<std::vector<double> > _expected);
    ~FFNetwork();
    bool isSuccessful() const;
    void restart();
    void pause();
    void resume();
    void cancel();
    void run();
    void quit();
    QString toString();

signals:
    void epochMilestone(int id, int avgId, int epoch, double error);
    void epochFinal(int id, int avgId, int epoch);

private:
    int id;
    int avgId;
    std::vector<unsigned int> layers;
    std::vector<std::vector<double> > inputs;
    std::vector<std::vector<double> > expected;
    double **weights;
    double **prevWeightUpdates;
    double **neuronVals;
    double **delta;
    double eta;
    double momentum;
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
    bool quitNow;
    bool successful;

    void fillRandomWeights();
    std::vector<double> processInput(std::vector<double> input);
    void backprop(std::vector<double> output, std::vector<double> expected);
    double sigmoid(double x);
};

#endif // FFNETWORK_H
