#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>

namespace Ui {
    class ConfigDialog;
}

class Config : public QDialog
{
    Q_OBJECT
public:
    Config(QWidget *parent = 0);

    double getEtaStart() const;
    double getEtaEnd() const;
    double getEtaIncrement() const;

    double getMomentum() const;

    int getInputNodes() const;
    int getOutputNodes() const;

    double getStop() const;

private:
    Ui::ConfigDialog *ui;
};

#endif // CONFIG_H
