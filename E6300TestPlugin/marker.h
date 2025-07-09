#ifndef MARKER_H
#define MARKER_H

#include "include/qcustomplot.h"
#include <QLabel>

class Marker {
public:
    Marker(QCustomPlot *plot, const QString &name);
    ~Marker();

    void setPosition(double frequency, double amplitude);
    void setVisible(bool visible);
    void toggleDeltaMode();

    QString name;
    bool active;
    int index;
    int traceIndex;
    QCPItemTracer *tracer;
    QCPItemText *annotation;

    bool deltaMode;
    double deltaRefFrequency;
    double deltaRefAmplitude;
    QCPItemTracer *deltaTracer;

    bool pkTracking;
};

#endif // MARKER_H
