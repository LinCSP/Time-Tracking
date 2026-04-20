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
class QWidget;
class QPushButton;
class QLabel;
class QFrame;
class QTranslator;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;

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
    void toggleLanguage();

private:
    void buildUi();
    void retranslateUi();
    void loadProjects();
    void refreshCardDurations();
    ProjectCardWidget* cardForProject(qint64 projectId);

    Database*         db_;
    TimerController*  timer_;
    QTranslator*      translator_{nullptr};
    bool              isRussian_{false};

    QFrame*              headerBar_;
    QStackedWidget*      stack_;
    QWidget*             cardsContainer_;
    TimerDisplay*        timerDisplay_;
    QPushButton*         stopBtn_;
    QPushButton*         navProjects_;
    QPushButton*         navHistory_;
    QPushButton*         langBtn_;
    QPushButton*         addBtn_;
    QLabel*              emptyLabel1_{nullptr};
    QLabel*              emptyLabel2_{nullptr};
    SessionHistoryWidget* historyWidget_;

    QList<ProjectCardWidget*> cards_;
};
