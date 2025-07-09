#include "marker.h"

Marker::Marker(QCustomPlot *plot, const QString &name)
    : name(name),
    active(false),
    index(0),
    traceIndex(0),
    deltaMode(false),
    pkTracking(false)
{
    static QMap<QString, QColor> colorMap = {
        {"Marker 1", QColor(255, 0, 0)}, // Red
        {"Marker 2", QColor(0, 255, 0)}, // Green
        {"Marker 3", QColor(147, 112, 219)}, // MediumPurple
        {"Marker 4", QColor(189, 183, 107)}, // DarkKhaki
        {"Marker 5", QColor(0, 255, 255)}, // Aqua
        {"Marker 6", QColor(255, 0, 255)}, // Fuchsia
        {"Marker 7", QColor(255, 20, 147)}, // DeepPink
        {"Marker 8", QColor(128, 0, 0)}, // Maroon
        {"Marker 9", QColor(255, 165, 0)} // Orange
    };

    QColor color = colorMap.value(name, QColor(128, 128, 128)); // Default Gray if name is not found

    tracer = new QCPItemTracer(plot);
    tracer->setVisible(false);
    tracer->setStyle(QCPItemTracer::tsSquare);
    tracer->setPen(QPen(color));
    tracer->setSize(10);

    // Annotation
    annotation = new QCPItemText(plot);
    annotation->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    annotation->position->setType(QCPItemPosition::ptPlotCoords);
    annotation->setFont(QFont("Arial"));
    annotation->setText(name.split(' ').last());
    annotation->setPen(Qt::NoPen);
    annotation->setBrush(Qt::NoBrush);
    annotation->setVisible(false);

    // Delta tracer
    deltaTracer = new QCPItemTracer(plot);
    deltaTracer->setVisible(false);
    deltaTracer->setStyle(QCPItemTracer::tsCircle);
    deltaTracer->setPen(QPen(color));
    deltaTracer->setBrush(QBrush(Qt::NoBrush));
    deltaTracer->setSize(12);
}

Marker::~Marker()
{
    delete tracer;
    delete annotation;
    delete deltaTracer;
}

void Marker::setPosition(double frequency, double amplitude)
{
    tracer->setGraphKey(frequency);

    // Update annotation
    annotation->position->setCoords(frequency, amplitude + 1);
}

void Marker::setVisible(bool visible)
{
    tracer->setVisible(visible);
    annotation->setVisible(visible);
    if (!visible) {
        deltaTracer->setVisible(false);
    } else if (deltaMode) {
        deltaTracer->setVisible(true);
    }
}

void Marker::toggleDeltaMode()
{
    deltaMode = !deltaMode;
    if (deltaMode) {
        // Activate delta mode: set reference point
        deltaRefFrequency = tracer->graphKey();
        deltaRefAmplitude = tracer->position->value();

        // Update delta tracer position
        deltaTracer->setGraph(tracer->graph());
        deltaTracer->setGraphKey(deltaRefFrequency);

        deltaTracer->setVisible(true);
    } else {
        // Deactivate delta mode
        deltaRefFrequency = 0;
        deltaRefAmplitude = 0;
        deltaTracer->setVisible(false);
    }
}
