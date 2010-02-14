#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QVector>
#include <QMutex>

class Config;
class FFNetwork;
class QwtPlot;
class QwtPlotCurve;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    NetworkManager(QwtPlot *_plot);
    void networksFromConfig(Config *c);

public slots:
    void resume();
    void pause();
    void restart();

private slots:
    void epochMilestone(int, int, double);

private:
    int numNetworks;
    FFNetwork **networks;
    QwtPlot *plot;
    QwtPlotCurve **curves;
    QVector<double> **epochMilestones;
    QVector<double> **errors;
    QMutex mutex;
    double minEpochMilestone;
    bool isRunning;
};

#endif // NETWORKMANAGER_H
