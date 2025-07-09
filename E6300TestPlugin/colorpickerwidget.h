#ifndef COLORPICKERWIDGET_H
#define COLORPICKERWIDGET_H

#include <QWidget>
#include <QColor>

class ColorPickerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerWidget(QWidget* parent = nullptr);

    // Set and get the current color
    void setColor(const QColor& color);
    QColor color() const;

signals:
    void colorChanged(const QColor& newColor);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QColor currentColor;
};

#endif // COLORPICKERWIDGET_H
