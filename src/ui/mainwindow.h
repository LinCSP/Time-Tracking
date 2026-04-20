#pragma once
#include "data/models.h"
#include <QMainWindow>
#include <QList>

class Database;
class TimerController;
class TimerDisplay;
class ProjectCardWidget;
class SessionHistoryWidget;
class QStackedWidget;
class QScrollArea;
class QWidget;
class QPushButton;
class QLabel;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onProjectClicked(qint64 projectId);
    void onDeleteProject(qint64 projectId);
    void onStopClicked();
    void onAddProjectClicked();
    void onSessionStarted(qint64 projectId, qint64 entryId);
    void onSessionStopped(qint64 projectId, qint64 entryId, qint64 durationSecs);
    void onTimerTick(qint64 elapsedSecs);
    void switchToProjectsPage();
    void switchToHistoryPage();

private:
    void buildUi();
    void loadProjects();
    void refreshCardDurations();
    ProjectCardWidget* cardForProject(qint64 projectId);

    Database*         db_;
    TimerController*  timer_;

    QStackedWidget*      stack_;
    QWidget*             cardsContainer_;
    TimerDisplay*        timerDisplay_;
    QPushButton*         stopBtn_;
    QPushButton*         navProjects_;
    QPushButton*         navHistory_;
    SessionHistoryWidget* historyWidget_;

    QList<ProjectCardWidget*> cards_;
};
