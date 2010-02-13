#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_engine.h>

#include "ffnetwork.h"

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
    QwtLog10ScaleEngine *logScaleEngine;

private slots:
    void resume();
    void pause();
    void restart();
    void epochMilestone(int, double);
};

#endif // MAINWINDOW_H
