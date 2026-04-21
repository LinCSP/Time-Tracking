#pragma once
#include "models.h"
#include <QDate>
#include <QList>
#include <QObject>
#include <QSqlDatabase>

class Database : public QObject {
    Q_OBJECT
public:
    explicit Database(QObject* parent = nullptr);

    bool open();

    QList<Project> allProjects();
    Project        addProject(const QString& name, const QString& color, const QString& description = {});
    bool           deleteProject(qint64 id);
    bool           updateProject(const Project& p);
    qint64         totalSecsToday(qint64 projectId);
    void           setTimezoneOffsetSecs(int secs) { tzOffsetSecs_ = secs; }
    int            timezoneOffsetSecs() const      { return tzOffsetSecs_; }

    QList<DailyEntry> dailyBreakdown(const QDate& from, const QDate& to);
    QList<TimeEntry> entriesForProject(qint64 projectId, int limit = 100);
    QList<TimeEntry> recentEntries(int limit = 200);
    TimeEntry        startSession(qint64 projectId);
    bool             stopSession(qint64 entryId, const QDateTime& endTime);
    TimeEntry        activeSession();

private:
    void        runMigrations();
    TimeEntry   rowToEntry(QSqlQuery& q);
    Project     rowToProject(QSqlQuery& q);
    QSqlDatabase db_;
    int          tzOffsetSecs_{3 * 3600};
};
