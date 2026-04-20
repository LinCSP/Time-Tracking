#include "timercontroller.h"
#include "data/database.h"
#include <QDateTime>

TimerController::TimerController(Database* db, QObject* parent) : QObject(parent), db_(db) {
    ticker_ = new QTimer(this);
    ticker_->setInterval(1000);
    connect(ticker_, &QTimer::timeout, this, &TimerController::onTimerTick);

    // Restore any session that was running when app was last closed
    TimeEntry active = db_->activeSession();
    if (active.id > 0) {
        activeEntry_ = active;
        ticker_->start();
    }
}

bool TimerController::isRunning() const {
    return activeEntry_.id > 0;
}

qint64 TimerController::activeProjectId() const {
    return activeEntry_.projectId;
}

qint64 TimerController::elapsedSecs() const {
    if (!isRunning()) return 0;
    return activeEntry_.startTime.secsTo(QDateTime::currentDateTime());
}

void TimerController::startProject(qint64 projectId) {
    if (isRunning()) {
        if (activeEntry_.projectId == projectId) return;
        stopCurrent();
    }

    activeEntry_ = db_->startSession(projectId);
    ticker_->start();
    emit sessionStarted(projectId, activeEntry_.id);
}

void TimerController::stopCurrent() {
    if (!isRunning()) return;

    ticker_->stop();
    QDateTime now = QDateTime::currentDateTime();
    db_->stopSession(activeEntry_.id, now);

    qint64 dur = activeEntry_.startTime.secsTo(now);
    qint64 pid = activeEntry_.projectId;
    qint64 eid = activeEntry_.id;

    activeEntry_ = {};
    emit sessionStopped(pid, eid, dur);
}

void TimerController::onTimerTick() {
    emit tick(elapsedSecs());
}
