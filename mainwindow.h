#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "ffnetwork.h"
#include "config.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    FFNetwork *network;
    bool isRunning;
    QwtPlot *plot;
    QwtPlotCurve *curve1;
    QVector<double> epochMilestones;
    QVector<double> errors;
    Config *config;

private slots:
    void resume();
    void pause();
    void restart();
    void epochMilestone(int, double);
    void newConfig();
};

#endif // MAINWINDOW_H
