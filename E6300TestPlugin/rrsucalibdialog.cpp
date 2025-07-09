#include "rrsucalibdialog.h"
#include "include/rfmu2/rfmu2tool.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

bool RRSUCalibDialog::readFile(const QString &path, QByteArray &outData)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Could show a message box or log an error if you like
        return false;
    }

    outData.clear();  // Make sure we're starting fresh

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;  // Skip empty lines

        // Split by comma. We expect exactly 2 columns: freq, attenuation
        QStringList parts = line.split(',');
        if (parts.size() < 2) {
            // If there's a header or mismatch, decide how strict you want to be
            // Here we return false immediately:
            file.close();
            return false;
        }

        // Parse the frequency (unused, but we verify we can parse it)
        bool freqOk = false;
        double freq = parts.at(0).toDouble(&freqOk);

        // Parse the attenuation
        bool attOk = false;
        double attenuation = parts.at(1).toDouble(&attOk);

        // If any parse failed, return false (or handle differently)
        if (!freqOk || !attOk) {
            file.close();
            return false;
        }

        // Use your helper to split the attenuation into two qint8 values
        QPair<qint8, qint8> splitted = Rfmu2Base::splitDoubleAtDecimal(attenuation);

        // Convert qint8 -> char and append to the output buffer
        outData.append(static_cast<char>(splitted.first));
        outData.append(static_cast<char>(splitted.second));
    }

    file.close();
    return true;
}
/*-------------------------------------------------------*/

RRSUCalibDialog::RRSUCalibDialog(Rfmu2SystemControl *sys, QWidget *parent)
    : QDialog(parent), m_sys(sys)
{
    setWindowTitle(tr("RRSU TX Calibration Upload"));
    setModal(true);
    resize(500, 160);

    /* --- channel selector --- */
    m_chanBox = new QComboBox(this);
    const QStringList chans = {
                               "01A","01B","01C","01D",
                               "02A","02B","02C","02D",
                               "03A","03B","03C","03D",
                               "04A","04B","04C","04D"};
    m_chanBox->addItems(chans);

    /* --- file picker --- */
    QPushButton *fileBtn = new QPushButton(tr("Select"), this);
    m_fileEdit = new QLineEdit(this);
    m_fileEdit->setReadOnly(true);

    connect(fileBtn, &QPushButton::clicked, this, &RRSUCalibDialog::selectFile);

    /* --- layout --- */
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel(tr("Channel:"), this), 0, 0);
    grid->addWidget(m_chanBox, 0, 1, 1, 2);
    grid->addWidget(fileBtn, 1, 0);
    grid->addWidget(m_fileEdit, 1, 1, 1, 2);

    /* --- button box --- */
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(bb, &QDialogButtonBox::accepted, this, &RRSUCalibDialog::doConfirm);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(grid);
    v->addWidget(bb);
}

void RRSUCalibDialog::selectFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose calibration file"),
        QString(), tr("CSV files (*.csv);;All files (*)"));
    if (!path.isEmpty()) {
        m_filePath = path;
        m_fileEdit->setText(path);
    }
}

void RRSUCalibDialog::doConfirm()
{
    if (!m_sys) {
        QMessageBox::critical(this, {}, tr("SystemControl instance is null."));
        return;
    }
    if (m_filePath.isEmpty()) {
        QMessageBox::warning(this, {}, tr("Please choose a valid file first."));
        return;
    }

    QByteArray blob;
    bool ok = readFile(m_filePath, blob);
    if (!ok) {
        QMessageBox::critical(this, {}, tr("File parsing failed."));
        return;
    }

    const QString chan = m_chanBox->currentText();
    const bool sent = m_sys->sendRRSUCalibration(blob, chan);

    QMessageBox::information(this, {},
                             sent ? tr("Calibration data sent successfully.")
                                  : tr("Calibration upload failed."));

    if (sent)
        accept(); // close dialog
}
