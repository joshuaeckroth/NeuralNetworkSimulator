#include <vector>
using namespace std;

#include <qwt_series_data.h>

#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ffnetwork.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    plot = new QwtPlot(ui->centralWidget);
    ui->verticalLayout->addWidget(plot);
    curve1 = new QwtPlotCurve("Curve 1");
    curve1->setSamples(epochMilestones, errors);
    curve1->attach(plot);

    logScaleEngine = new QwtLog10ScaleEngine;
    plot->setAxisScaleEngine(QwtPlot::yLeft, logScaleEngine);
    plot->setAxisFont(QwtPlot::yLeft, QFont("Calibri", 10));
    plot->setAxisFont(QwtPlot::xBottom, QFont("Calibri", 10));
    plot->setAxisTitle(QwtPlot::yLeft, QString("Error (log10 scale)"));
    plot->setAxisTitle(QwtPlot::xBottom, QString("Epoch"));

    plot->replot();

    isRunning = false;

    vector<vector<double> > inputs;
    vector<vector<double> > expected;

    // create inputs & expected values
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++)
            for(int k = 0; k < 2; k++)
                for(int l = 0; l < 2; l++)
                {
                    double a[4] = {i,j,k,l};
                    vector<double> inputVec(a, a+4);
                    double e[1] = {((i+j+k+l)%2 == 0) ? 0.0 : 1.0};
                    vector<double> expectedVec(e, e+1);
                    inputs.push_back(inputVec);
                    expected.push_back(expectedVec);
                }

    unsigned int l[3] = {4,4,1};
    vector<unsigned int> layers(l, l+3);
    network = new FFNetwork(layers, inputs, expected);

    connect(ui->resumeButton, SIGNAL(clicked()), network, SLOT(resume()));
    connect(ui->resumeButton, SIGNAL(clicked()), this, SLOT(resume()));
    connect(ui->pauseButton, SIGNAL(clicked()), network, SLOT(pause()));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->restartButton, SIGNAL(clicked()), network, SLOT(restart()));
    connect(ui->restartButton, SIGNAL(clicked()), this, SLOT(restart()));

    connect(network, SIGNAL(epochMilestone(int,double)), this, SLOT(epochMilestone(int, double)));

    network->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::resume()
{
    isRunning = !isRunning;
    if(isRunning)
    {
        ui->resumeButton->setText("Resume");
        ui->resumeButton->setDisabled(true);
        ui->pauseButton->setDisabled(false);
    }
    else
    {
        ui->resumeButton->setText("Resume");
        ui->resumeButton->setDisabled(false);
        ui->pauseButton->setDisabled(true);
    }
    ui->restartButton->setDisabled(false);
}

void MainWindow::pause()
{
    isRunning = !isRunning;
    if(isRunning)
    {
        ui->resumeButton->setText("Resume");
        ui->resumeButton->setDisabled(true);
        ui->pauseButton->setDisabled(false);
    }
    else
    {
        ui->resumeButton->setText("Resume");
        ui->resumeButton->setDisabled(false);
        ui->pauseButton->setDisabled(true);
    }
    ui->restartButton->setDisabled(false);
}

void MainWindow::restart()
{
    if(isRunning)
    {
        isRunning = false;
    }
    ui->resumeButton->setText("Start");
    ui->resumeButton->setDisabled(false);
    ui->pauseButton->setDisabled(true);
    ui->restartButton->setDisabled(true);

    epochMilestones.clear();
    errors.clear();
    curve1->setSamples(epochMilestones, errors);
    plot->replot();
}

void MainWindow::epochMilestone(int epoch, double error)
{
    epochMilestones << double(epoch);
    errors << error;
    curve1->setSamples(epochMilestones, errors);
    plot->replot();
}
