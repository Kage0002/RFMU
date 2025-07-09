#include "ColorPickerWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>

ColorPickerWidget::ColorPickerWidget(QWidget* parent)
    : QWidget(parent), currentColor(Qt::darkBlue) // Default color
{
}

void ColorPickerWidget::setColor(const QColor& color)
{
    if (color != currentColor) {
        currentColor = color;
        update(); // Repaint the widget
        emit colorChanged(currentColor);
    }
}

QColor ColorPickerWidget::color() const
{
    return currentColor;
}

void ColorPickerWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // Draw the color block
    painter.setBrush(QBrush(currentColor));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
}

void ColorPickerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Open the color dialog
        QColor selectedColor = QColorDialog::getColor(currentColor, this, "Select Color");
        if (selectedColor.isValid()) {
            setColor(selectedColor);
        }
    }
    QWidget::mousePressEvent(event);
}
