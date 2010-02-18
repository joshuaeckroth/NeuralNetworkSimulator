#include <vector>
using namespace std;

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
    connect(config, SIGNAL(accepted()), this, SLOT(restart()));

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
    connect(networkManager, SIGNAL(stopped()), this, SLOT(stopped()));
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

void MainWindow::stopped()
{
    mutex.lock();
    ui->resumeButton->setText("Start");
    ui->resumeButton->setDisabled(true);
    ui->pauseButton->setDisabled(true);
    ui->restartButton->setDisabled(false);
    mutex.unlock();
}

void MainWindow::newConfig()
{
    mutex.lock();
    networkManager->networksFromConfig(config);
    mutex.unlock();
}
