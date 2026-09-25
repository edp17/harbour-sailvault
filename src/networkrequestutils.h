#pragma once

#include "settingsstore.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

namespace SailVaultNetwork
{
static const int DefaultRequestTimeoutMs = 15000;
static const char TimedOutProperty[] = "sailvaultTimedOut";
static const char OfflineModeKey[] = "network/offlineMode";

inline bool offlineModeEnabled()
{
    return SailVaultSettings::value(
        QString::fromLatin1(OfflineModeKey), false).toBool();
}

inline void armTimeout(QNetworkReply *reply,
                       int timeoutMs = DefaultRequestTimeoutMs)
{
    if (!reply)
        return;

    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->setInterval(timeoutMs);

    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isFinished())
            return;
        reply->setProperty(TimedOutProperty, true);
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished,
                     timer, &QTimer::stop);
    timer->start();
}

inline bool timedOut(QNetworkReply *reply)
{
    return reply && reply->property(TimedOutProperty).toBool();
}

inline void cancelOutstanding(QNetworkAccessManager *manager)
{
    if (!manager)
        return;

    const QList<QNetworkReply *> replies =
        manager->findChildren<QNetworkReply *>();
    for (QNetworkReply *reply : replies) {
        if (reply && !reply->isFinished())
            reply->abort();
    }
}
} // namespace SailVaultNetwork
