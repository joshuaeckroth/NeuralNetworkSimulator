#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <map>

#include <QObject>
#include <QVector>
#include <QMutex>

class Config;
class FFNetwork;
class QwtPlot;
class QwtLegend;
class QwtPlotCurve;
class QwtPlotItem;
class QwtPlotMarker;

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

signals:
    void stopped();

private slots:
    void epochMilestone(int, int, double);
    void legendChecked(QwtPlotItem*, bool);

private:
    int numNetworks;
    FFNetwork **networks;
    QwtPlot *plot;
    QwtLegend *legend;
    QwtPlotCurve **curves;
    QVector<double> **epochMilestones;
    QVector<double> **errors;
    QMutex mutex;
    double minEpochMilestone;
    bool isRunning;
    std::map<QwtPlotCurve*, bool> highlightedCurves;
    std::map<QwtPlotCurve*, QwtPlotMarker*> markers;

    void updateMarker(QwtPlotMarker *curve, double epoch, double error);
};

#endif // NETWORKMANAGER_H
