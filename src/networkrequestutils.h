#pragma once

#include "settingsstore.h"

#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariant>

namespace SailVaultNetwork
{
static const int DefaultRequestTimeoutMs = 15000;
static const qint64 DefaultMaxResponseBytes = 16LL * 1024LL * 1024LL;
static const char TimedOutProperty[] = "sailvaultTimedOut";
static const char RedirectBlockedProperty[] = "sailvaultRedirectBlocked";
static const char ResponseTooLargeProperty[] = "sailvaultResponseTooLarge";
static const char UnsafeEndpointProperty[] = "sailvaultUnsafeEndpoint";
static const char OfflineModeKey[] = "network/offlineMode";

inline bool offlineModeEnabled()
{
    return SailVaultSettings::value(
        QString::fromLatin1(OfflineModeKey), false).toBool();
}

inline bool isValidHttpsEndpoint(const QUrl &url)
{
    return url.isValid()
        && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !url.host().isEmpty()
        && url.userInfo().isEmpty();
}

inline bool isValidHttpsEndpoint(const QString &text)
{
    return isValidHttpsEndpoint(QUrl(text.trimmed()));
}

inline QString safeHttpsEndpoint(const QString &candidate,
                                 const QString &fallback)
{
    const QString normalizedCandidate = candidate.trimmed();
    if (isValidHttpsEndpoint(normalizedCandidate))
        return normalizedCandidate;

    const QString normalizedFallback = fallback.trimmed();
    return isValidHttpsEndpoint(normalizedFallback)
        ? normalizedFallback : QString();
}

inline void hardenRequest(QNetworkRequest &request)
{
    // Provider calls are deliberately stateless. SailVault does not need
    // cookies, HTTP authentication state, referrer/origin context or a local
    // HTTP cache for its public read-only APIs. Keeping those channels closed
    // reduces avoidable request linkability and prevents stale cached provider
    // data from being mistaken for a fresh refresh.
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute,
                         QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute,
                         QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute,
                         QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

    // Remove any state-bearing/context headers that a future call site might
    // accidentally add before applying the shared policy. A null QByteArray
    // removes an existing raw header in Qt 5.6.
    request.setRawHeader("Cookie", QByteArray());
    request.setRawHeader("Authorization", QByteArray());
    request.setRawHeader("Referer", QByteArray());
    request.setRawHeader("Origin", QByteArray());
    request.setRawHeader("Cache-Control", "no-store, no-cache");

    // Sailfish Qt 5.6 exposes both FollowRedirectsAttribute and an explicit
    // redirect budget. Set both before the request leaves QNetworkAccessManager;
    // the reply guard below independently rejects redirect metadata as well.
    request.setMaximumRedirectsAllowed(0);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
}

inline QString responseLimitText(qint64 maxBytes = DefaultMaxResponseBytes)
{
    if (maxBytes > 0 && maxBytes % (1024LL * 1024LL) == 0) {
        return QStringLiteral("%1 MiB")
            .arg(maxBytes / (1024LL * 1024LL));
    }
    return QStringLiteral("%1 bytes").arg(maxBytes);
}

inline bool timedOut(QNetworkReply *reply)
{
    return reply && reply->property(TimedOutProperty).toBool();
}

inline bool redirectBlocked(QNetworkReply *reply)
{
    if (!reply)
        return false;

    if (reply->property(RedirectBlockedProperty).toBool())
        return true;

    const QVariant target =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    return target.isValid() && !target.toUrl().isEmpty();
}

inline bool responseTooLarge(QNetworkReply *reply)
{
    return reply && reply->property(ResponseTooLargeProperty).toBool();
}

inline bool unsafeEndpoint(QNetworkReply *reply)
{
    return reply && reply->property(UnsafeEndpointProperty).toBool();
}

inline void armTimeout(QNetworkReply *reply,
                       int timeoutMs = DefaultRequestTimeoutMs,
                       qint64 maxResponseBytes = DefaultMaxResponseBytes)
{
    if (!reply)
        return;

    // All SailVault providers are expected to be HTTPS. Settings are already
    // sanitized at startup; this is a second boundary check for every actual
    // request, including requests built from direct page/service parameters.
    if (!isValidHttpsEndpoint(reply->request().url())) {
        reply->setProperty(UnsafeEndpointProperty, true);
        QTimer::singleShot(0, reply, [reply]() {
            if (!reply->isFinished())
                reply->abort();
        });
    }

    // Do not depend on the network stack's redirect default: observe redirect
    // metadata and reject it before provider data is accepted.
    // This avoids silently moving an address-bearing request to another host.
    const auto inspectMetadata = [reply, maxResponseBytes]() {
        const QVariant redirect =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid() && !redirect.toUrl().isEmpty()) {
            reply->setProperty(RedirectBlockedProperty, true);
            if (!reply->isFinished())
                reply->abort();
            return;
        }

        if (maxResponseBytes <= 0)
            return;

        bool lengthOk = false;
        const qint64 contentLength =
            reply->header(QNetworkRequest::ContentLengthHeader)
                .toLongLong(&lengthOk);
        if (lengthOk && contentLength > maxResponseBytes) {
            reply->setProperty(ResponseTooLargeProperty, true);
            if (!reply->isFinished())
                reply->abort();
        }
    };

    QObject::connect(reply, &QNetworkReply::metaDataChanged,
                     reply, inspectMetadata);
    inspectMetadata();

    if (maxResponseBytes > 0) {
        // QNetworkReply otherwise buffers the complete body until each service
        // reads it on finished(). Keep at most one byte beyond the policy
        // ceiling so readyRead can observe the breach and abort promptly.
        reply->setReadBufferSize(maxResponseBytes + 1);

        QObject::connect(reply, &QNetworkReply::downloadProgress,
                         reply,
                         [reply, maxResponseBytes](qint64 received,
                                                   qint64 total) {
            if (received > maxResponseBytes
                    || (total >= 0 && total > maxResponseBytes)) {
                reply->setProperty(ResponseTooLargeProperty, true);
                if (!reply->isFinished())
                    reply->abort();
            }
        });

        // downloadProgress can describe compressed transfer size rather than
        // the final QIODevice buffer. Also cap what has actually accumulated.
        QObject::connect(reply, &QNetworkReply::readyRead,
                         reply, [reply, maxResponseBytes]() {
            if (reply->bytesAvailable() > maxResponseBytes) {
                reply->setProperty(ResponseTooLargeProperty, true);
                if (!reply->isFinished())
                    reply->abort();
            }
        });
    }

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

inline QString errorText(QNetworkReply *reply,
                         int timeoutMs = DefaultRequestTimeoutMs,
                         qint64 maxResponseBytes = DefaultMaxResponseBytes)
{
    if (!reply)
        return QStringLiteral("Network reply unavailable");

    if (unsafeEndpoint(reply))
        return QStringLiteral("Blocked non-HTTPS provider endpoint");

    if (redirectBlocked(reply))
        return QStringLiteral("Provider redirect blocked");

    if (responseTooLarge(reply)) {
        return QStringLiteral("Provider response exceeded %1 safety limit")
            .arg(responseLimitText(maxResponseBytes));
    }

    if (timedOut(reply)) {
        return QStringLiteral("Request timed out after %1 seconds")
            .arg(timeoutMs / 1000);
    }

    const int status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        return status > 0
            ? QStringLiteral("HTTP %1 · %2")
                  .arg(status)
                  .arg(reply->errorString())
            : reply->errorString();
    }

    if (status < 200 || status >= 300)
        return QStringLiteral("HTTP %1").arg(status);

    return QString();
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
