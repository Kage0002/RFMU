#include "frequencyspinbox.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QHBoxLayout>

FrequencySpinBox::FrequencySpinBox(QWidget* parent)
    : QWidget(parent), currentUnit(Gigahertz),
      minFrequencyHz(0), maxFrequencyHz(2e12)
{
    spinBox = new QDoubleSpinBox(this);
    unitComboBox = new QComboBox(this);

    unitComboBox->addItem("Hz");
    unitComboBox->addItem("kHz");
    unitComboBox->addItem("MHz");
    unitComboBox->addItem("GHz");

    unitComboBox->setCurrentIndex(Gigahertz); // Default to GHz

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(spinBox);
    layout->addWidget(unitComboBox);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    updateSpinBox();

    connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FrequencySpinBox::onValueChanged);
    connect(unitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FrequencySpinBox::onUnitChanged);
}

double FrequencySpinBox::frequency() const
{
    double value = spinBox->value();
    switch (currentUnit) {
    case Hertz:
        return value;
    case Kilohertz:
        return value * 1e3;
    case Megahertz:
        return value * 1e6;
    case Gigahertz:
        return value * 1e9;
    default:
        return value;
    }
}

void FrequencySpinBox::setFrequency(double frequencyHz)
{
    double value;
    switch (currentUnit) {
    case Hertz:
        value = frequencyHz;
        break;
    case Kilohertz:
        value = frequencyHz / 1e3;
        break;
    case Megahertz:
        value = frequencyHz / 1e6;
        break;
    case Gigahertz:
        value = frequencyHz / 1e9;
        break;
    default:
        value = frequencyHz;
        break;
    }
    spinBox->setValue(value);
}

void FrequencySpinBox::setRange(double minHz, double maxHz)
{
    minFrequencyHz = minHz;
    maxFrequencyHz = maxHz;
    updateSpinBox();
}

void FrequencySpinBox::onValueChanged(double /*value*/)
{
    emit frequencyChanged(frequency());
}

void FrequencySpinBox::onUnitChanged(int index)
{
    // Save the frequency in Hz
    double frequencyHz = frequency();

    // Update the current unit
    currentUnit = static_cast<Unit>(index);

    // Update the spin box settings
    updateSpinBox();

    // Restore the frequency value
    setFrequency(frequencyHz);
}

void FrequencySpinBox::updateSpinBox()
{
    double minVal = 0.0;
    double maxVal = 0.0;

    switch (currentUnit) {
    case Hertz:
        spinBox->setDecimals(0);
        minVal = minFrequencyHz;
        maxVal = maxFrequencyHz;
        break;
    case Kilohertz:
        spinBox->setDecimals(3);
        minVal = minFrequencyHz / 1e3;
        maxVal = maxFrequencyHz / 1e3;
        break;
    case Megahertz:
        spinBox->setDecimals(6);
        minVal = minFrequencyHz / 1e6;
        maxVal = maxFrequencyHz / 1e6;
        break;
    case Gigahertz:
        spinBox->setDecimals(9);
        minVal = minFrequencyHz / 1e9;
        maxVal = maxFrequencyHz / 1e9;
        break;
    }

    spinBox->setRange(minVal, maxVal);
}
