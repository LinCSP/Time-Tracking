#pragma once
#include <QDateTime>
#include <QString>

struct Project {
    qint64    id = -1;
    QString   name;
    QString   color;
    QString   description;
    QDateTime createdAt;
};

struct TimeEntry {
    qint64    id        = -1;
    qint64    projectId = -1;
    QDateTime startTime;
    QDateTime endTime;

    bool isRunning() const { return !endTime.isValid(); }

    qint64 durationSecs() const {
        if (!endTime.isValid()) return 0;
        return startTime.secsTo(endTime);
    }
};
