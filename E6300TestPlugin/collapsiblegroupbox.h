#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QWidget>
#include <QToolButton>
#include <QVBoxLayout>

class CollapsibleGroupBox : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString& title = "", QWidget* parent = nullptr);

    // Set the layout for the content area
    void setContentLayout(QLayout* contentLayout);

private slots:
    void toggleCollapsed();

private:
    QToolButton* toggleButton;
    QWidget* contentWidget;
    QVBoxLayout* mainLayout;
};

#endif // COLLAPSIBLEGROUPBOX_H
