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

    unsigned int getAveraged() const;

    unsigned int getInputNodes() const;
    unsigned int getOutputNodes() const;

    double getStop() const;

private slots:
    void saveConfig();
    void cancelConfig();

private:
    Ui::ConfigDialog *ui;
    double etaStart;
    double etaEnd;
    double etaIncrement;
    double momentum;
    unsigned int averaged;
    unsigned int inputNodes;
    unsigned int outputNodes;
    double stop;
};

#endif // CONFIG_H
