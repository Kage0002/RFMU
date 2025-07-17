#include "connectdialog.h"
#include <QLineEdit>
#include <QLabel>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QIntValidator>
#include <QDialogButtonBox>
#include <QGridLayout>

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to Hardware"));
    setModal(true);
    resize(350, 160);

    auto *grid = new QGridLayout(this);

    // IP field
    grid->addWidget(new QLabel(tr("IP address:")), 0, 0);
    m_ipEdit = new QLineEdit(QStringLiteral("192.168.137.11"), this);
    m_ipEdit->setInputMask("000.000.000.000;");   // simple sanity mask
    grid->addWidget(m_ipEdit, 0, 1);

    // Port field
    grid->addWidget(new QLabel(tr("Port:")), 1, 0);
    m_portEdit = new QLineEdit(QStringLiteral("7"), this);
    m_portEdit->setValidator(new QIntValidator(1, 65535, m_portEdit));
    grid->addWidget(m_portEdit, 1, 1);

    // OK / Cancel buttons
    m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                        QDialogButtonBox::Cancel, this);
    connect(m_btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    grid->addWidget(m_btnBox, 2, 0, 1, 2);
}

QString ConnectDialog::ip()   const { return m_ipEdit->text().trimmed(); }
quint16 ConnectDialog::port() const { return m_portEdit->text().toUShort(); }
