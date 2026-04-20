#pragma once
#include "data/models.h"
#include <QMainWindow>
#include <QList>
#include <QSystemTrayIcon>

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
class QComboBox;
class QTranslator;
class QMenu;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onProjectClicked(qint64 projectId);
    void onDeleteProject(qint64 projectId);
    void onEditProject(qint64 projectId);
    void onStopClicked();
    void onAddProjectClicked();
    void onSessionStarted(qint64 projectId, qint64 entryId);
    void onSessionStopped(qint64 projectId, qint64 entryId, qint64 durationSecs);
    void onTimerTick(qint64 elapsedSecs);
    void switchToProjectsPage();
    void switchToHistoryPage();
    void switchToSettingsPage();
    void onLanguageChanged(int index);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildUi();
    void retranslateUi();
    void loadProjects();
    void refreshCardDurations();
    void setupTray();
    void updateTrayMenu();
    void restoreWindow();
    ProjectCardWidget* cardForProject(qint64 projectId);

    Database*         db_;
    TimerController*  timer_;
    QTranslator*      translator_{nullptr};

    QFrame*              headerBar_;
    QStackedWidget*      stack_;
    QWidget*             cardsContainer_;
    TimerDisplay*        timerDisplay_;
    QPushButton*         stopBtn_;
    QPushButton*         navProjects_;
    QPushButton*         navHistory_;
    QPushButton*         navSettings_;
    QComboBox*           langCombo_;
    QPushButton*         addBtn_;
    QLabel*              emptyLabel1_{nullptr};
    QLabel*              emptyLabel2_{nullptr};
    SessionHistoryWidget* historyWidget_;

    // Settings page
    QCheckBox* minimizeToTrayCheck_{nullptr};
    QComboBox* tzCombo_{nullptr};
    QLabel*    settingsTitleLabel_{nullptr};
    QLabel*    settingsBehaviorLabel_{nullptr};
    QLabel*    settingsTzLabel_{nullptr};
    bool       minimizeToTray_{false};

    // Tray
    QSystemTrayIcon* tray_{nullptr};
    QMenu*           trayMenu_{nullptr};
    bool             forceQuit_{false};

    QList<ProjectCardWidget*> cards_;
};
