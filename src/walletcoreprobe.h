#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class WalletCoreProbe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY resultsChanged)
    Q_PROPERTY(QString buildInfo READ buildInfo NOTIFY resultsChanged)
    Q_PROPERTY(bool allPassed READ allPassed NOTIFY resultsChanged)
    Q_PROPERTY(int runCount READ runCount NOTIFY resultsChanged)

public:
    explicit WalletCoreProbe(QObject *parent = nullptr);

    QVariantList results() const;
    QString summary() const;
    QString buildInfo() const;
    bool allPassed() const;
    int runCount() const;

    Q_INVOKABLE void run();

signals:
    void resultsChanged();

private:
    void addResult(const QString &name,
                   bool passed,
                   const QString &detail,
                   const QString &expected = QString());

    QVariantList m_results;
    QString m_summary;
    QString m_buildInfo;
    bool m_allPassed = false;
    int m_runCount = 0;
};
