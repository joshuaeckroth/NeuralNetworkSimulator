#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

double Config::getEtaStart() const
{
    return ui->etaStartSpinBox->value();
}

double Config::getEtaEnd() const
{
    return ui->etaEndSpinBox->value();
}

double Config::getEtaIncrement() const
{
    return ui->etaIncrementSpinBox->value();
}

double Config::getMomentum() const
{
    return ui->momentumSpinBox->value();
}

int Config::getInputNodes() const
{
    return ui->inputSpinBox->value();
}

int Config::getOutputNodes() const
{
    return ui->outputSpinBox->value();
}
