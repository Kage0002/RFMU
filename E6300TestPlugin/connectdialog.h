#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

class QLineEdit;
class QDialogButtonBox;

class ConnectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    QString ip()   const;   // e.g. "192.168.137.10"
    quint16 port() const;   // e.g. 7

private:
    QLineEdit* m_ipEdit;
    QLineEdit* m_portEdit;
    QDialogButtonBox* m_btnBox;
};

#endif // CONNECTDIALOG_H
