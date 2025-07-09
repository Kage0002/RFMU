#pragma once
#include <QDialog>

class QComboBox;
class QLineEdit;
class QPushButton;
class Rfmu2SystemControl;

class RRSUCalibDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RRSUCalibDialog(Rfmu2SystemControl *sys,
                             QWidget *parent = nullptr);

private slots:
    void selectFile();
    void doConfirm();

private:
    static bool readFile(const QString &path, QByteArray &outData); // stub

    Rfmu2SystemControl *m_sys {};            // not owned
    QComboBox          *m_chanBox {};
    QLineEdit          *m_fileEdit {};
    QString             m_filePath;
};
