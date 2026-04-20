#include "mainwindow.h"
#include "core/timercontroller.h"
#include "data/database.h"
#include "dialogs/addprojectdialog.h"
#include "widgets/projectcardwidget.h"
#include "widgets/sessionhistorywidget.h"
#include "widgets/timerdisplay.h"
#include <QApplication>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QSettings>
#include <QTranslator>
#include <QVBoxLayout>
#include <QWindow>

// ── FlowLayout ───────────────────────────────────────────────────────────────
#include <QLayout>
#include <QWidgetItem>

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent, int hSpacing = 12, int vSpacing = 12)
        : QLayout(parent), hSpace_(hSpacing), vSpace_(vSpacing) {}
    ~FlowLayout() override { QLayoutItem* item; while ((item = takeAt(0))) delete item; }

    void         addItem(QLayoutItem* item) override { items_.append(item); }
    int          count() const override { return items_.size(); }
    QLayoutItem* itemAt(int idx) const override { return items_.value(idx); }
    QLayoutItem* takeAt(int idx) override {
        return (idx >= 0 && idx < items_.size()) ? items_.takeAt(idx) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool             hasHeightForWidth() const override { return true; }
    int              heightForWidth(int w) const override { return doLayout(QRect(0, 0, w, 0), true); }
    QSize            sizeHint() const override { return minimumSize(); }
    QSize            minimumSize() const override {
        QSize sz;
        for (auto* item : items_) sz = sz.expandedTo(item->minimumSize());
        auto m = contentsMargins();
        return sz + QSize(m.left() + m.right(), m.top() + m.bottom());
    }
    void setGeometry(const QRect& rect) override { QLayout::setGeometry(rect); doLayout(rect, false); }

private:
    int doLayout(const QRect& rect, bool testOnly) const {
        auto  m = contentsMargins();
        QRect eff = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
        int   x = eff.x(), y = eff.y(), lineH = 0;
        for (auto* item : items_) {
            int nextX = x + item->sizeHint().width() + hSpace_;
            if (nextX - hSpace_ > eff.right() && lineH > 0) {
                x = eff.x(); y += lineH + vSpace_; nextX = x + item->sizeHint().width() + hSpace_; lineH = 0;
            }
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            x = nextX;
            lineH = qMax(lineH, item->sizeHint().height());
        }
        return y + lineH - rect.y() + m.bottom();
    }
    QList<QLayoutItem*> items_;
    int hSpace_, vSpace_;
};

// ── MainWindow ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    db_    = new Database(this);
    db_->open();
    timer_ = new TimerController(db_, this);

    connect(timer_, &TimerController::sessionStarted, this, &MainWindow::onSessionStarted);
    connect(timer_, &TimerController::sessionStopped, this, &MainWindow::onSessionStopped);
    connect(timer_, &TimerController::tick,           this, &MainWindow::onTimerTick);

    buildUi();
    loadProjects();

    // Restore saved language (blockSignals to avoid double-applying)
    QSettings cfg;
    QString savedLang = cfg.value("language", "en").toString();
    int idx = langCombo_->findData(savedLang);
    if (idx > 0) {
        langCombo_->blockSignals(true);
        langCombo_->setCurrentIndex(idx);
        langCombo_->blockSignals(false);
        onLanguageChanged(idx);
    }

    if (timer_->isRunning()) {
        qint64 pid = timer_->activeProjectId();
        if (auto* card = cardForProject(pid)) card->setActive(true);
        for (const auto& p : db_->allProjects()) {
            if (p.id == pid) { timerDisplay_->setProjectName(p.name); break; }
        }
        timerDisplay_->setElapsed(timer_->elapsedSecs());
        stopBtn_->setEnabled(true);
    }
}

MainWindow::~MainWindow() {
    timer_->stopCurrent();
}

// ── Language ──────────────────────────────────────────────────────────────────

void MainWindow::onLanguageChanged(int index) {
    QString lang = langCombo_->itemData(index).toString();

    if (translator_) {
        qApp->removeTranslator(translator_);
        delete translator_;
        translator_ = nullptr;
    }

    if (lang != "en") {
        translator_ = new QTranslator(qApp);
        QString path = QString(":/translations/timetracker_%1.qm").arg(lang);
        if (translator_->load(path)) {
            qApp->installTranslator(translator_);
        } else {
            delete translator_;
            translator_ = nullptr;
        }
    }

    QSettings cfg;
    cfg.setValue("language", lang);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi() {
    navProjects_->setText(tr("Projects"));
    navHistory_->setText(tr("History"));
    stopBtn_->setText(tr("■  Stop"));
    addBtn_->setText(tr("+ New Project"));
    if (emptyLabel1_) emptyLabel1_->setText(tr("No projects"));
    if (emptyLabel2_) emptyLabel2_->setText(tr("Click '+ New Project' to get started"));
}

// ── Drag to move ──────────────────────────────────────────────────────────────

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == headerBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget* child = headerBar_->childAt(me->pos());
                if (!child || !qobject_cast<QPushButton*>(child)) {
                    windowHandle()->startSystemMove();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget* child = headerBar_->childAt(me->pos());
                if (!child || !qobject_cast<QPushButton*>(child)) {
                    isMaximized() ? showNormal() : showMaximized();
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ── UI Build ──────────────────────────────────────────────────────────────────

void MainWindow::buildUi() {
    auto* outer = new QFrame(this);
    outer->setObjectName("outerFrame");
    setCentralWidget(outer);

    auto* rootLayout = new QVBoxLayout(outer);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────
    headerBar_ = new QFrame(outer);
    headerBar_->setObjectName("headerBar");
    headerBar_->setCursor(Qt::SizeAllCursor);
    headerBar_->installEventFilter(this);

    auto* hl = new QHBoxLayout(headerBar_);
    hl->setContentsMargins(16, 0, 12, 0);
    hl->setSpacing(0);

    auto* appTitle = new QLabel("⏱  Time Tracker", headerBar_);
    appTitle->setObjectName("appTitle");
    appTitle->setCursor(Qt::SizeAllCursor);
    hl->addWidget(appTitle);
    hl->addSpacing(20);

    navProjects_ = new QPushButton(tr("Projects"), headerBar_);
    navProjects_->setObjectName("navBtn");
    navProjects_->setCheckable(true);
    navProjects_->setChecked(true);
    navProjects_->setCursor(Qt::PointingHandCursor);
    connect(navProjects_, &QPushButton::clicked, this, &MainWindow::switchToProjectsPage);

    navHistory_ = new QPushButton(tr("History"), headerBar_);
    navHistory_->setObjectName("navBtn");
    navHistory_->setCheckable(true);
    navHistory_->setCursor(Qt::PointingHandCursor);
    connect(navHistory_, &QPushButton::clicked, this, &MainWindow::switchToHistoryPage);

    hl->addWidget(navProjects_);
    hl->addWidget(navHistory_);
    hl->addStretch();

    timerDisplay_ = new TimerDisplay(headerBar_);
    timerDisplay_->setMinimumWidth(180);
    hl->addWidget(timerDisplay_);
    hl->addSpacing(10);

    stopBtn_ = new QPushButton(tr("■  Stop"), headerBar_);
    stopBtn_->setObjectName("stopBtn");
    stopBtn_->setFixedHeight(32);
    stopBtn_->setCursor(Qt::PointingHandCursor);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    hl->addWidget(stopBtn_);
    hl->addSpacing(12);

    // Language selector
    langCombo_ = new QComboBox(headerBar_);
    langCombo_->setObjectName("langCombo");
    langCombo_->setCursor(Qt::PointingHandCursor);
    langCombo_->addItem("English", "en");
    langCombo_->addItem("Русский", "ru");
    langCombo_->setFixedHeight(28);
    langCombo_->setMinimumWidth(90);
    connect(langCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::onLanguageChanged);
    hl->addWidget(langCombo_);
    hl->addSpacing(12);

    // Window controls
    auto makeWinBtn = [&](const char* name, const QString& color) {
        auto* btn = new QPushButton("", headerBar_);
        btn->setObjectName(name);
        btn->setFixedSize(14, 14);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton { background-color:%1; border-radius:7px; border:none; padding:0; }"
            "QPushButton:hover { background-color:%1; opacity:0.8; }"
        ).arg(color));
        return btn;
    };

    auto* winClose = makeWinBtn("winClose", "#FF5F57");
    connect(winClose, &QPushButton::clicked, this, &QMainWindow::close);

    auto* winMin = makeWinBtn("winMinimize", "#FEBC2E");
    connect(winMin, &QPushButton::clicked, this, &QMainWindow::showMinimized);

    auto* winMax = makeWinBtn("winMaximize", "#28C840");
    connect(winMax, &QPushButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });

    auto* wbl = new QHBoxLayout();
    wbl->setSpacing(8);
    wbl->setContentsMargins(0, 0, 0, 0);
    wbl->addWidget(winMin);
    wbl->addWidget(winMax);
    wbl->addWidget(winClose);
    hl->addLayout(wbl);

    rootLayout->addWidget(headerBar_);

    auto* sep = new QFrame(outer);
    sep->setObjectName("headerSep");
    sep->setFixedHeight(1);
    rootLayout->addWidget(sep);

    // ── Stack ─────────────────────────────────────────────────────────────
    stack_ = new QStackedWidget(outer);
    rootLayout->addWidget(stack_, 1);

    // Page 0 — Projects
    auto* projectsPage = new QWidget();
    auto* projLayout   = new QVBoxLayout(projectsPage);
    projLayout->setContentsMargins(0, 0, 0, 0);
    projLayout->setSpacing(0);

    auto* toolbar  = new QWidget(projectsPage);
    toolbar->setObjectName("toolbarRow");
    auto* tbl = new QHBoxLayout(toolbar);
    tbl->setContentsMargins(16, 8, 16, 8);
    tbl->setSpacing(12);

    addBtn_ = new QPushButton(tr("+ New Project"), toolbar);
    addBtn_->setObjectName("addBtn");
    addBtn_->setFixedHeight(32);
    addBtn_->setCursor(Qt::PointingHandCursor);
    connect(addBtn_, &QPushButton::clicked, this, &MainWindow::onAddProjectClicked);
    tbl->addWidget(addBtn_);
    tbl->addStretch();

    projLayout->addWidget(toolbar);

    auto* scrollArea = new QScrollArea(projectsPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    cardsContainer_ = new QWidget();
    cardsContainer_->setObjectName("cardsContainer");
    auto* flowLayout = new FlowLayout(cardsContainer_, 14, 14);
    flowLayout->setContentsMargins(16, 16, 16, 16);
    cardsContainer_->setLayout(flowLayout);

    scrollArea->setWidget(cardsContainer_);
    projLayout->addWidget(scrollArea, 1);
    stack_->addWidget(projectsPage);

    // Page 1 — History
    historyWidget_ = new SessionHistoryWidget(db_, this);
    stack_->addWidget(historyWidget_);
}

// ── Projects ──────────────────────────────────────────────────────────────────

void MainWindow::loadProjects() {
    emptyLabel1_ = nullptr;
    emptyLabel2_ = nullptr;

    auto* layout = cardsContainer_->layout();
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
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

    if (projects.isEmpty()) {
        auto* empty = new QWidget(cardsContainer_);
        empty->setFixedSize(360, 120);
        auto* vl = new QVBoxLayout(empty);
        vl->setAlignment(Qt::AlignCenter);

        emptyLabel1_ = new QLabel(tr("No projects"), empty);
        emptyLabel1_->setObjectName("emptyStateLabel");
        emptyLabel1_->setAlignment(Qt::AlignCenter);

        emptyLabel2_ = new QLabel(tr("Click '+ New Project' to get started"), empty);
        emptyLabel2_->setObjectName("emptyStateHint");
        emptyLabel2_->setAlignment(Qt::AlignCenter);

        vl->addWidget(emptyLabel1_);
        vl->addWidget(emptyLabel2_);
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

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::onProjectClicked(qint64 projectId) { timer_->startProject(projectId); }
void MainWindow::onStopClicked()                     { timer_->stopCurrent(); }

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

void MainWindow::onSessionStarted(qint64 projectId, qint64) {
    for (auto* c : cards_) c->setActive(c->projectId() == projectId);
    for (const auto& p : db_->allProjects()) {
        if (p.id == projectId) { timerDisplay_->setProjectName(p.name); break; }
    }
    timerDisplay_->setElapsed(0);
    stopBtn_->setEnabled(true);
}

void MainWindow::onSessionStopped(qint64, qint64, qint64) {
    for (auto* c : cards_) c->setActive(false);
    timerDisplay_->reset();
    stopBtn_->setEnabled(false);
    refreshCardDurations();
    if (stack_->currentIndex() == 1) historyWidget_->refresh();
}

void MainWindow::onTimerTick(qint64 secs) { timerDisplay_->setElapsed(secs); }

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
