#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancelConfig()));
    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveConfig()));

    saveConfig();
}

void Config::saveConfig()
{
    etaStart = ui->etaStartSpinBox->value();
    etaEnd = ui->etaEndSpinBox->value();
    etaIncrement = ui->etaIncrementSpinBox->value();
    momentum = ui->momentumSpinBox->value();
    averaged = ui->avgSpinBox->value();
    inputNodes = ui->inputSpinBox->value();
    outputNodes = ui->outputSpinBox->value();
    stop = ui->stopSpinBox->value();

    emit accept();
}

void Config::cancelConfig()
{
    ui->etaStartSpinBox->setValue(etaStart);
    ui->etaEndSpinBox->setValue(etaEnd);
    ui->etaIncrementSpinBox->setValue(etaIncrement);
    ui->momentumSpinBox->setValue(momentum);
    ui->avgSpinBox->setValue(averaged);
    ui->inputSpinBox->setValue(inputNodes);
    ui->outputSpinBox->setValue(outputNodes);
    ui->stopSpinBox->setValue(stop);

    emit reject();
}

double Config::getEtaStart() const
{
    return etaStart;
}

double Config::getEtaEnd() const
{
    return etaEnd;
}

double Config::getEtaIncrement() const
{
    return etaIncrement;
}

double Config::getMomentum() const
{
    return momentum;
}

unsigned int Config::getAveraged() const
{
    return averaged;
}

unsigned int Config::getInputNodes() const
{
    return inputNodes;
}

unsigned int Config::getOutputNodes() const
{
    return outputNodes;
}

double Config::getStop() const
{
    return stop;
}
