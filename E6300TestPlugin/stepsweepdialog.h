#pragma once
#include <QDialog>
#include <QComboBox>

class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QDialogButtonBox;

class StepSweepDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StepSweepDialog(QWidget *parent = nullptr);

    double startFreqGHz() const;     // GHz
    double stopFreqGHz()  const;
    double startLvlDbm()  const;
    double stopLvlDbm()   const;
    int    points()       const;
    int    intervalMs()   const;
    QString rfPath()      const;

private slots:
    void validateInputs();           // live validation

private:
    QDoubleSpinBox *m_startFreq, *m_stopFreq;
    QDoubleSpinBox *m_startLvl,  *m_stopLvl;
    QSpinBox       *m_points;
    QSpinBox       *m_interval;
    QComboBox      *m_rfPath;
    QDialogButtonBox *m_buttons;
};
