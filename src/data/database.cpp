#include "database.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

Database::Database(QObject* parent) : QObject(parent) {}

bool Database::open() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(dataDir + "/timetracker.db");

    if (!db_.open()) {
        qCritical() << "Cannot open database:" << db_.lastError().text();
        return false;
    }

    QSqlQuery q(db_);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");
    runMigrations();
    return true;
}

void Database::runMigrations() {
    QSqlQuery q(db_);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS projects (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL,
            color       TEXT NOT NULL DEFAULT '#4DABF7',
            description TEXT,
            created_at  TEXT NOT NULL
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS time_entries (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            project_id  INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
            start_time  TEXT NOT NULL,
            end_time    TEXT
        )
    )");
}

Project Database::rowToProject(QSqlQuery& q) {
    Project p;
    p.id          = q.value(0).toLongLong();
    p.name        = q.value(1).toString();
    p.color       = q.value(2).toString();
    p.description = q.value(3).toString();
    p.createdAt   = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
    return p;
}

TimeEntry Database::rowToEntry(QSqlQuery& q) {
    TimeEntry e;
    e.id        = q.value(0).toLongLong();
    e.projectId = q.value(1).toLongLong();
    e.startTime = QDateTime::fromString(q.value(2).toString(), Qt::ISODate);
    QString end = q.value(3).toString();
    if (!end.isEmpty())
        e.endTime = QDateTime::fromString(end, Qt::ISODate);
    return e;
}

QList<Project> Database::allProjects() {
    QList<Project> list;
    QSqlQuery      q(db_);
    q.exec("SELECT id, name, color, description, created_at FROM projects ORDER BY created_at ASC");
    while (q.next())
        list.append(rowToProject(q));
    return list;
}

Project Database::addProject(const QString& name, const QString& color, const QString& description) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO projects (name, color, description, created_at) VALUES (?, ?, ?, ?)");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    q.addBindValue(name);
    q.addBindValue(color);
    q.addBindValue(description);
    q.addBindValue(now);
    q.exec();

    Project p;
    p.id          = q.lastInsertId().toLongLong();
    p.name        = name;
    p.color       = color;
    p.description = description;
    p.createdAt   = QDateTime::fromString(now, Qt::ISODate);
    return p;
}

bool Database::deleteProject(qint64 id) {
    QSqlQuery q(db_);
    q.prepare("DELETE FROM projects WHERE id = ?");
    q.addBindValue(id);
    return q.exec();
}

bool Database::updateProject(const Project& p) {
    QSqlQuery q(db_);
    q.prepare("UPDATE projects SET name=?, color=?, description=? WHERE id=?");
    q.addBindValue(p.name);
    q.addBindValue(p.color);
    q.addBindValue(p.description);
    q.addBindValue(p.id);
    return q.exec();
}

qint64 Database::totalSecsToday(qint64 projectId) {
    // Compute "today" in the app timezone by shifting the current local time
    int     sysOffsetSecs = QDateTime::currentDateTime().offsetFromUtc();
    int     adjSecs       = tzOffsetSecs_ - sysOffsetSecs;
    QString today         = QDateTime::currentDateTime().addSecs(adjSecs).date().toString(Qt::ISODate);

    // Shift stored local times by the same amount before comparing dates
    QString adjStr = (adjSecs >= 0) ? QString("+%1 seconds").arg(adjSecs)
                                    : QString("%1 seconds").arg(adjSecs);

    QSqlQuery q(db_);
    q.prepare(
        "SELECT COALESCE(SUM("
        "  strftime('%s', COALESCE(end_time, datetime('now', 'localtime'))) -"
        "  strftime('%s', start_time)"
        "), 0)"
        " FROM time_entries"
        " WHERE project_id = ? AND date(start_time, ?) = ?");
    q.addBindValue(projectId);
    q.addBindValue(adjStr);
    q.addBindValue(today);
    q.exec();
    return q.next() ? q.value(0).toLongLong() : 0;
}

TimeEntry Database::startSession(qint64 projectId) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO time_entries (project_id, start_time) VALUES (?, ?)");
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    q.addBindValue(projectId);
    q.addBindValue(now);
    q.exec();

    TimeEntry e;
    e.id        = q.lastInsertId().toLongLong();
    e.projectId = projectId;
    e.startTime = QDateTime::fromString(now, Qt::ISODate);
    return e;
}

bool Database::stopSession(qint64 entryId, const QDateTime& endTime) {
    QSqlQuery q(db_);
    q.prepare("UPDATE time_entries SET end_time=? WHERE id=?");
    q.addBindValue(endTime.toString(Qt::ISODate));
    q.addBindValue(entryId);
    return q.exec();
}

TimeEntry Database::activeSession() {
    QSqlQuery q(db_);
    q.exec("SELECT id, project_id, start_time, end_time FROM time_entries WHERE end_time IS NULL LIMIT 1");
    if (q.next())
        return rowToEntry(q);
    return {};
}

QList<TimeEntry> Database::entriesForProject(qint64 projectId, int limit) {
    QList<TimeEntry> list;
    QSqlQuery        q(db_);
    q.prepare("SELECT id, project_id, start_time, end_time FROM time_entries WHERE project_id=? ORDER BY start_time DESC LIMIT ?");
    q.addBindValue(projectId);
    q.addBindValue(limit);
    q.exec();
    while (q.next())
        list.append(rowToEntry(q));
    return list;
}

QList<TimeEntry> Database::recentEntries(int limit) {
    QList<TimeEntry> list;
    QSqlQuery        q(db_);
    q.prepare("SELECT id, project_id, start_time, end_time FROM time_entries ORDER BY start_time DESC LIMIT ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next())
        list.append(rowToEntry(q));
    return list;
}
