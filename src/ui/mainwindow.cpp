#include "mainwindow.h"
#include "core/timercontroller.h"
#include "data/database.h"
#include "dialogs/addprojectdialog.h"
#include "widgets/projectcardwidget.h"
#include "widgets/sessionhistorywidget.h"
#include "widgets/timerdisplay.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

// ── Simple flow layout (no external dep) ────────────────────────────────────
#include <QLayout>
#include <QStyle>
#include <QWidgetItem>

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent, int hSpacing = 12, int vSpacing = 12)
        : QLayout(parent), hSpace_(hSpacing), vSpace_(vSpacing) {}
    ~FlowLayout() override { QLayoutItem* item; while ((item = takeAt(0))) delete item; }

    void addItem(QLayoutItem* item) override { items_.append(item); }
    int  count() const override { return items_.size(); }
    QLayoutItem* itemAt(int idx) const override { return items_.value(idx); }
    QLayoutItem* takeAt(int idx) override {
        return (idx >= 0 && idx < items_.size()) ? items_.takeAt(idx) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize sz;
        for (auto* item : items_) sz = sz.expandedTo(item->minimumSize());
        const auto margins = contentsMargins();
        return sz + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }
    void setGeometry(const QRect& rect) override {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

private:
    int doLayout(const QRect& rect, bool testOnly) const {
        auto margins = contentsMargins();
        QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
        int x = effective.x(), y = effective.y(), lineH = 0;

        for (auto* item : items_) {
            int nextX = x + item->sizeHint().width() + hSpace_;
            if (nextX - hSpace_ > effective.right() && lineH > 0) {
                x = effective.x();
                y += lineH + vSpace_;
                nextX = x + item->sizeHint().width() + hSpace_;
                lineH = 0;
            }
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            x = nextX;
            lineH = qMax(lineH, item->sizeHint().height());
        }
        return y + lineH - rect.y() + margins.bottom();
    }
    QList<QLayoutItem*> items_;
    int hSpace_, vSpace_;
};

// ── MainWindow ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    db_    = new Database(this);
    db_->open();
    timer_ = new TimerController(db_, this);

    connect(timer_, &TimerController::sessionStarted, this, &MainWindow::onSessionStarted);
    connect(timer_, &TimerController::sessionStopped, this, &MainWindow::onSessionStopped);
    connect(timer_, &TimerController::tick,           this, &MainWindow::onTimerTick);

    buildUi();
    loadProjects();

    // Restore active session display
    if (timer_->isRunning()) {
        qint64 pid = timer_->activeProjectId();
        auto*  card = cardForProject(pid);
        if (card) card->setActive(true);

        auto projects = db_->allProjects();
        for (const auto& p : projects) {
            if (p.id == pid) {
                timerDisplay_->setProjectName(p.name);
                break;
            }
        }
        timerDisplay_->setElapsed(timer_->elapsedSecs());
        stopBtn_->setEnabled(true);
    }
}

MainWindow::~MainWindow() {
    timer_->stopCurrent();
}

void MainWindow::buildUi() {
    setWindowTitle("Time Tracker");

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Header bar ────────────────────────────────────────────────────────
    auto* header = new QFrame(this);
    header->setObjectName("headerBar");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    headerLayout->setSpacing(0);

    auto* appTitle = new QLabel("⏱ Time Tracker", header);
    appTitle->setObjectName("appTitle");
    headerLayout->addWidget(appTitle);

    headerLayout->addSpacing(32);

    // Nav buttons
    navProjects_ = new QPushButton("Projects", header);
    navProjects_->setObjectName("navBtn");
    navProjects_->setCheckable(true);
    navProjects_->setChecked(true);
    connect(navProjects_, &QPushButton::clicked, this, &MainWindow::switchToProjectsPage);

    navHistory_ = new QPushButton("History", header);
    navHistory_->setObjectName("navBtn");
    navHistory_->setCheckable(true);
    connect(navHistory_, &QPushButton::clicked, this, &MainWindow::switchToHistoryPage);

    headerLayout->addWidget(navProjects_);
    headerLayout->addWidget(navHistory_);
    headerLayout->addStretch();

    // Timer display
    timerDisplay_ = new TimerDisplay(header);
    timerDisplay_->setMinimumWidth(200);
    headerLayout->addWidget(timerDisplay_);

    headerLayout->addSpacing(12);

    // Stop button
    stopBtn_ = new QPushButton("■  Stop", header);
    stopBtn_->setObjectName("stopBtn");
    stopBtn_->setFixedHeight(34);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    headerLayout->addWidget(stopBtn_);

    rootLayout->addWidget(header);

    // ── Content stack ─────────────────────────────────────────────────────
    stack_ = new QStackedWidget(this);
    rootLayout->addWidget(stack_, 1);

    // Page 0 — Projects
    auto* projectsPage = new QWidget();
    auto* projLayout   = new QVBoxLayout(projectsPage);
    projLayout->setContentsMargins(0, 0, 0, 0);
    projLayout->setSpacing(0);

    // Toolbar row
    auto* toolbar = new QWidget(projectsPage);
    toolbar->setObjectName("toolbarRow");
    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(16, 8, 16, 8);
    tbLayout->setSpacing(12);

    auto* addBtn = new QPushButton("+ New Project", toolbar);
    addBtn->setObjectName("addBtn");
    addBtn->setFixedHeight(34);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddProjectClicked);
    tbLayout->addWidget(addBtn);
    tbLayout->addStretch();

    projLayout->addWidget(toolbar);

    // Scroll area with cards
    auto* scrollArea = new QScrollArea(projectsPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    cardsContainer_ = new QWidget();
    cardsContainer_->setObjectName("cardsContainer");
    auto* flowLayout = new FlowLayout(cardsContainer_, 12, 12);
    flowLayout->setContentsMargins(16, 16, 16, 16);
    cardsContainer_->setLayout(flowLayout);

    scrollArea->setWidget(cardsContainer_);
    projLayout->addWidget(scrollArea, 1);

    stack_->addWidget(projectsPage);

    // Page 1 — History
    historyWidget_ = new SessionHistoryWidget(db_, this);
    stack_->addWidget(historyWidget_);
}

void MainWindow::loadProjects() {
    // Clear old cards
    auto* layout = cardsContainer_->layout();
    for (auto* c : cards_) {
        layout->removeWidget(c);
        c->deleteLater();
    }
    cards_.clear();

    auto projects = db_->allProjects();
    for (const auto& p : projects) {
        auto* card = new ProjectCardWidget(p, cardsContainer_);
        card->setFixedSize(240, 110);
        card->setTodayDuration(db_->totalSecsToday(p.id));

        connect(card, &ProjectCardWidget::clicked,         this, &MainWindow::onProjectClicked);
        connect(card, &ProjectCardWidget::deleteRequested, this, &MainWindow::onDeleteProject);

        layout->addWidget(card);
        cards_.append(card);
    }

    historyWidget_->setProjects(projects);

    // Show empty state if no projects
    if (projects.isEmpty()) {
        auto* empty = new QWidget(cardsContainer_);
        auto* vl    = new QVBoxLayout(empty);
        vl->setAlignment(Qt::AlignCenter);
        auto* lbl1 = new QLabel("No projects yet", empty);
        lbl1->setObjectName("emptyStateLabel");
        lbl1->setAlignment(Qt::AlignCenter);
        auto* lbl2 = new QLabel("Click \"+ New Project\" to get started", empty);
        lbl2->setObjectName("emptyStateHint");
        lbl2->setAlignment(Qt::AlignCenter);
        vl->addWidget(lbl1);
        vl->addWidget(lbl2);
        layout->addWidget(empty);
    }
}

ProjectCardWidget* MainWindow::cardForProject(qint64 projectId) {
    for (auto* c : cards_)
        if (c->projectId() == projectId) return c;
    return nullptr;
}

void MainWindow::refreshCardDurations() {
    for (auto* c : cards_)
        c->setTodayDuration(db_->totalSecsToday(c->projectId()));
}

void MainWindow::onProjectClicked(qint64 projectId) {
    timer_->startProject(projectId);
}

void MainWindow::onStopClicked() {
    timer_->stopCurrent();
}

void MainWindow::onAddProjectClicked() {
    AddProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    db_->addProject(dlg.projectName(), dlg.projectColor(), dlg.projectDescription());
    loadProjects();
}

void MainWindow::onDeleteProject(qint64 projectId) {
    if (timer_->isRunning() && timer_->activeProjectId() == projectId)
        timer_->stopCurrent();

    db_->deleteProject(projectId);
    loadProjects();
}

void MainWindow::onSessionStarted(qint64 projectId, qint64 /*entryId*/) {
    for (auto* c : cards_)
        c->setActive(c->projectId() == projectId);

    auto projects = db_->allProjects();
    for (const auto& p : projects) {
        if (p.id == projectId) {
            timerDisplay_->setProjectName(p.name);
            break;
        }
    }
    timerDisplay_->setElapsed(0);
    stopBtn_->setEnabled(true);
}

void MainWindow::onSessionStopped(qint64 /*projectId*/, qint64 /*entryId*/, qint64 /*dur*/) {
    for (auto* c : cards_) c->setActive(false);
    timerDisplay_->reset();
    stopBtn_->setEnabled(false);
    refreshCardDurations();
    if (stack_->currentIndex() == 1) historyWidget_->refresh();
}

void MainWindow::onTimerTick(qint64 elapsedSecs) {
    timerDisplay_->setElapsed(elapsedSecs);
}

void MainWindow::switchToProjectsPage() {
    stack_->setCurrentIndex(0);
    navProjects_->setChecked(true);
    navHistory_->setChecked(false);
    refreshCardDurations();
}

void MainWindow::switchToHistoryPage() {
    stack_->setCurrentIndex(1);
    navProjects_->setChecked(false);
    navHistory_->setChecked(true);
    historyWidget_->refresh();
}
