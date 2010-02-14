#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QMutex>

#include <qwt_plot.h>

#include "config.h"

class NetworkManager;

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
    NetworkManager *networkManager;
    QwtPlot *plot;
    Config *config;
    QMutex mutex;

private slots:
    void resume();
    void pause();
    void restart();
    void newConfig();
};

#endif // MAINWINDOW_H
