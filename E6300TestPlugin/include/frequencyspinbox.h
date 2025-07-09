#ifndef FREQUENCYSPINBOX_H
#define FREQUENCYSPINBOX_H

#include <QWidget>

class QDoubleSpinBox;
class QComboBox;

class FrequencySpinBox : public QWidget
{
    Q_OBJECT

public:
    enum Unit {
        Hertz,
        Kilohertz,
        Megahertz,
        Gigahertz
    };

    explicit FrequencySpinBox(QWidget* parent = nullptr);

    double frequency() const; // Returns the frequency in Hz
    void setFrequency(double frequencyHz);
    void setRange(double minHz, double maxHz);

signals:
    void frequencyChanged(double frequencyHz);

private slots:
    void onValueChanged(double value);
    void onUnitChanged(int index);

private:
    QDoubleSpinBox* spinBox;
    QComboBox* unitComboBox;
    Unit currentUnit;

    // Store the min and max range in Hz
    double minFrequencyHz;
    double maxFrequencyHz;

    void updateSpinBox();
};

#endif // FREQUENCYSPINBOX_H
