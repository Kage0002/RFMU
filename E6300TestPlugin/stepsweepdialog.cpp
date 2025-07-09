#include "stepsweepdialog.h"
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

StepSweepDialog::StepSweepDialog(QWidget *parent)
    : QDialog(parent)
{
    /* helper lambdas ---------------------------------------------------- */
    auto makeFreqBox = [this] {
        auto *sb = new QDoubleSpinBox(this);
        sb->setRange(0.1, 6.0);   // GHz
        sb->setDecimals(6);       // 1 kHz resolution
        sb->setSingleStep(0.001);
        return sb;
    };
    auto makeLevelBox = [this] {
        auto *sb = new QDoubleSpinBox(this);
        sb->setRange(-100.0, 10.0); // dBm
        sb->setDecimals(1);
        sb->setSingleStep(0.1);
        return sb;
    };

    /* widgets ----------------------------------------------------------- */
    m_startFreq = makeFreqBox();
    m_stopFreq  = makeFreqBox();

    m_startLvl  = makeLevelBox();
    m_stopLvl   = makeLevelBox();

    m_points   = new QSpinBox(this);
    m_points->setRange(2, 20001);               // arbitrary upper limit

    m_interval = new QSpinBox(this);
    m_interval->setRange(1, 10000);             // ms
    m_interval->setValue(1000);

    m_rfPath   = new QComboBox(this);
    QStringList paths;
    for (int i = 1; i <= 4; ++i) {           // 01..04
        for (char ch = 'A'; ch <= 'D'; ++ch) { // A..D
            paths << QString("RF-%1%2")
                         .arg(i, 2, 10, QLatin1Char('0')) // 01,02,03,04
                         .arg(ch);                       // A,B,C,D
        }
    }
    m_rfPath->addItems(paths);

    auto *lay  = new QFormLayout(this);
    lay->addRow(tr("Start Freq [GHz]:"),  m_startFreq);
    lay->addRow(tr("Stop Freq [GHz]:"),   m_stopFreq);
    lay->addRow(tr("Start Level [dBm]:"), m_startLvl);
    lay->addRow(tr("Stop Level [dBm]:"),  m_stopLvl);
    lay->addRow(tr("Points:"),                      m_points);
    lay->addRow(tr("Send Interval [ms]:"),m_interval);
    lay->addRow(tr("RF Path:"),                m_rfPath);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel, this);
    lay->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted,
            this, &StepSweepDialog::validateInputs);
    connect(m_buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

/* ---------- live validation & final acceptance ------------------------ */
void StepSweepDialog::validateInputs()
{
    const bool freqDiff = qFuzzyCompare(startFreqGHz(), stopFreqGHz()) == false;
    const bool lvlDiff  = qFuzzyCompare(startLvlDbm(),  stopLvlDbm())  == false;

    constexpr double kLvlRes   = 0.1;    // dB
    constexpr double kFreqRes  = 1e3;    // Hz  (1 kHz)

    if (freqDiff && lvlDiff) {
        QMessageBox::warning(this, tr("Not supported"),
                             tr("Frequency *or* level sweep is supported - not both."));
        return;
    }

    /* ensure step >= resolution */
    const int nPts  = points();
    if (nPts < 2) {
        QMessageBox::warning(this, tr("Invalid points"),
                             tr("Points must be at least 2."));
        return;
    }

    if (freqDiff) {
        const double stepHz = qAbs(stopFreqGHz() - startFreqGHz()) * 1e9 / double(nPts - 1);
        const double cells  = stepHz / kFreqRes;          // in 1 kHz units
        if (qAbs(cells - qRound64(cells)) > 1e-6) {
            QMessageBox::warning(this, tr("Resolution exceeded"),
                                 tr("Chosen frequency span / points does not result in >= 1 kHz cells."));
            return;
        }
    }

    if (lvlDiff) {
        const double step = qAbs(stopLvlDbm() - startLvlDbm()) / double(nPts - 1);

        // how many 0.1-dB cells does that step contain?
        const double cells = step / kLvlRes;

        // round to nearest integer to kill floating error
        if (qAbs(cells - qRound64(cells)) > 1e-6) {
            QMessageBox::warning(this, tr("Resolution exceeded"),
                                 tr("Chosen level span / points does not result in >= 0.1 dB cells."));
            return;
        }
    }
    accept();                                         // all good
}

/* ---------- trivial getters ------------------------------------------ */
double StepSweepDialog::startFreqGHz() const { return m_startFreq->value(); }
double StepSweepDialog::stopFreqGHz()  const { return m_stopFreq->value();  }
double StepSweepDialog::startLvlDbm()  const { return m_startLvl->value();  }
double StepSweepDialog::stopLvlDbm()   const { return m_stopLvl->value();   }
int    StepSweepDialog::points()       const { return m_points->value();    }
int    StepSweepDialog::intervalMs()   const { return m_interval->value();  }
QString StepSweepDialog::rfPath()      const { return m_rfPath->currentText();     }
