#include "collapsiblegroupbox.h"
#include <QLabel>
#include <QPropertyAnimation>

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    // Create the toggle button (header)
    // The default state is: checked, downArrow, visible
    toggleButton = new QToolButton(this);
    toggleButton->setStyleSheet(
        "QToolButton { border: none; background-color: #4D6081; color: white;}"
        "QToolButton::hover { border: none; background-color: #435370; color: white;}"
        );
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setArrowType(Qt::ArrowType::DownArrow);
    toggleButton->setText(title);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(true);
    toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Create the content widget
    contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #EFEFF2;");
    contentWidget->setVisible(true);

    // Set up the main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(toggleButton);
    mainLayout->addWidget(contentWidget);

    // Connect the toggle button signal to the slot
    connect(toggleButton, &QToolButton::clicked, this, &CollapsibleGroupBox::toggleCollapsed);
}

void CollapsibleGroupBox::toggleCollapsed()
{
    bool isChecked = toggleButton->isChecked();
    contentWidget->setVisible(isChecked);
    toggleButton->setArrowType(isChecked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
}

void CollapsibleGroupBox::setContentLayout(QLayout* contentLayout)
{
    delete contentWidget->layout(); // Remove any existing layout
    contentWidget->setLayout(contentLayout);
}
