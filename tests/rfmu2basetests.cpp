#include <QtTest>
#include "rfmu2/rfmu2base.h"

class Rfmu2BaseTests: public QObject
{
    Q_OBJECT
private slots:
    void splitDoubleAtDecimal_data();
    void splitDoubleAtDecimal();
};

void Rfmu2BaseTests::splitDoubleAtDecimal_data()
{
    QTest::addColumn<double>("input");
    QTest::addColumn<qint8>("whole");
    QTest::addColumn<qint8>("tenths");

    QTest::newRow("zero") << 0.0 << qint8(0) << qint8(0);
    QTest::newRow("positive") << 3.45 << qint8(3) << qint8(5);
    QTest::newRow("neg") << -2.3 << qint8(-2) << qint8(-3);
    QTest::newRow("round-up") << 3.96 << qint8(4) << qint8(0);
    QTest::newRow("round-down-neg") << -2.96 << qint8(-3) << qint8(0);
    QTest::newRow("small-pos") << 0.05 << qint8(0) << qint8(1);
    QTest::newRow("small-neg") << -0.05 << qint8(0) << qint8(-1);
}

void Rfmu2BaseTests::splitDoubleAtDecimal()
{
    QFETCH(double, input);
    QFETCH(qint8, whole);
    QFETCH(qint8, tenths);

    auto result = Rfmu2Base::splitDoubleAtDecimal(input);
    QCOMPARE(result.first, whole);
    QCOMPARE(result.second, tenths);
}

QTEST_APPLESS_MAIN(Rfmu2BaseTests)
#include "rfmu2basetests.moc"
