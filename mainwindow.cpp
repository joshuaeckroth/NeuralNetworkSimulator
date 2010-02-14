#include <vector>
using namespace std;

#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "networkmanager.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    config = new Config();
    connect(ui->configButton, SIGNAL(clicked()), config, SLOT(show()));
    connect(config, SIGNAL(accepted()), this, SLOT(newConfig()));

    plot = new QwtPlot(ui->centralWidget);
    ui->verticalLayout->addWidget(plot);

    networkManager = new NetworkManager(plot);
    networkManager->networksFromConfig(config);

    plot->setAxisTitle(QwtPlot::yLeft, QString("Error"));
    plot->setAxisTitle(QwtPlot::xBottom, QString("Epoch"));

    plot->replot();

    connect(ui->resumeButton, SIGNAL(clicked()), this, SLOT(resume()));
    connect(ui->resumeButton, SIGNAL(clicked()), networkManager, SLOT(resume()));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->pauseButton, SIGNAL(clicked()), networkManager, SLOT(pause()));
    connect(ui->restartButton, SIGNAL(clicked()), this, SLOT(restart()));
    connect(ui->restartButton, SIGNAL(clicked()), networkManager, SLOT(restart()));
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
    mutex.lock();
    ui->resumeButton->setText("Resume");
    ui->resumeButton->setDisabled(true);
    ui->pauseButton->setDisabled(false);
    ui->restartButton->setDisabled(false);
    mutex.unlock();
}

void MainWindow::pause()
{
    mutex.lock();
    ui->resumeButton->setText("Resume");
    ui->resumeButton->setDisabled(false);
    ui->pauseButton->setDisabled(true);
    ui->restartButton->setDisabled(false);
    mutex.unlock();
}

void MainWindow::restart()
{
    mutex.lock();
    ui->resumeButton->setText("Start");
    ui->resumeButton->setDisabled(false);
    ui->pauseButton->setDisabled(true);
    ui->restartButton->setDisabled(true);
    mutex.unlock();
}

void MainWindow::newConfig()
{
    networkManager->networksFromConfig(config);
    qDebug() << QString("Config: eta %1->%2 (by %3); momentum %4; %5 inputs; %6 outputs")
            .arg(config->getEtaStart()).arg(config->getEtaEnd()).arg(config->getEtaIncrement())
            .arg(config->getMomentum()).arg(config->getInputNodes()).arg(config->getOutputNodes());
}
