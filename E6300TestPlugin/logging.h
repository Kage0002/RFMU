#pragma once
#include <QTextBrowser>
#include <QTime>
#include <QStringView>

namespace logger
{
    inline void log(QTextBrowser *box, QStringView msg)
    {
        if (!box) return;
        const QString ts = QTime::currentTime().toString("HH:mm:ss.zzz");
        box->append(QStringLiteral(
                        "<span style='color:#666666;font-family:monospace'>[%1]</span> %2")
                        .arg(ts)
                        .arg(msg.toString()));

    }
}
