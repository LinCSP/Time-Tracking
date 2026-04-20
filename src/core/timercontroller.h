#pragma once
#include "data/models.h"
#include <QObject>
#include <QTimer>

class Database;

class TimerController : public QObject {
    Q_OBJECT
public:
    explicit TimerController(Database* db, QObject* parent = nullptr);

    bool   isRunning() const;
    qint64 activeProjectId() const;
    qint64 elapsedSecs() const;

public slots:
    void startProject(qint64 projectId);
    void stopCurrent();

signals:
    void sessionStarted(qint64 projectId, qint64 entryId);
    void sessionStopped(qint64 projectId, qint64 entryId, qint64 durationSecs);
    void tick(qint64 elapsedSecs);

private slots:
    void onTimerTick();

private:
    Database*  db_;
    QTimer*    ticker_;
    TimeEntry  activeEntry_;
};
