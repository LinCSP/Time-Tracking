#include "projectcardwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

ProjectCardWidget::ProjectCardWidget(const Project& project, QWidget* parent)
    : QFrame(parent), project_(project) {
    setFixedHeight(110);
    setMinimumWidth(200);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    buildUi();
}

void ProjectCardWidget::buildUi() {
    // Full per-card stylesheet: per-widget QSS completely replaces global QSS for this widget
    setStyleSheet(QString(R"(
        ProjectCardWidget {
            background-color: #232336;
            border: 1px solid #2C2C48;
            border-left: 4px solid %1;
        }
        ProjectCardWidget:hover {
            background-color: #28283E;
            border-color: #3C3C5C;
        }
        ProjectCardWidget[active="true"] {
            background-color: #252540;
            border-color: #3A3A5A;
        }
    )").arg(project_.color));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 12, 12);
    root->setSpacing(4);

    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    dotLabel_ = new QLabel("●", this);
    dotLabel_->setStyleSheet("color: transparent; font-size: 10px; background: transparent;");
    dotLabel_->setFixedSize(14, 14);
    dotLabel_->setAlignment(Qt::AlignCenter);

    nameLabel_ = new QLabel(project_.name, this);
    nameLabel_->setStyleSheet("font-size: 14px; font-weight: bold; color: #C1C2C5; background: transparent;");

    topRow->addWidget(dotLabel_);
    topRow->addWidget(nameLabel_, 1);
    root->addLayout(topRow);

    durationLabel_ = new QLabel(tr("No time today"), this);
    durationLabel_->setStyleSheet("font-size: 11px; color: #606080; padding-left: 22px; background: transparent;");
    root->addWidget(durationLabel_);

    root->addStretch();

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(6);

    startBtn_ = new QPushButton(tr("▶  Start"), this);
    startBtn_->setObjectName("cardStartBtn");
    startBtn_->setFixedHeight(28);
    startBtn_->setCursor(Qt::PointingHandCursor);
    connect(startBtn_, &QPushButton::clicked, this, [this]() { emit clicked(project_.id); });

    auto* editBtn = new QPushButton("✎", this);
    editBtn->setObjectName("cardEditBtn");
    editBtn->setFixedSize(28, 28);
    editBtn->setCursor(Qt::PointingHandCursor);
    connect(editBtn, &QPushButton::clicked, this, [this]() { emit editRequested(project_.id); });

    deleteBtn_ = new QPushButton("✕", this);
    deleteBtn_->setObjectName("cardDeleteBtn");
    deleteBtn_->setFixedSize(28, 28);
    deleteBtn_->setCursor(Qt::PointingHandCursor);
    connect(deleteBtn_, &QPushButton::clicked, this, [this]() { emit deleteRequested(project_.id); });

    bottomRow->addWidget(startBtn_);
    bottomRow->addStretch();
    bottomRow->addWidget(editBtn);
    bottomRow->addWidget(deleteBtn_);
    root->addLayout(bottomRow);
}

void ProjectCardWidget::setActive(bool active) {
    active_ = active;
    setProperty("active", active);
    style()->unpolish(this);
    style()->polish(this);
    dotLabel_->setStyleSheet(active
        ? "color: #69DB7C; font-size: 10px; background: transparent;"
        : "color: transparent; font-size: 10px; background: transparent;");
    updateStartBtn();
}

void ProjectCardWidget::setTodayDuration(qint64 secs) {
    todaySecs_ = secs;
    retranslateUi();
}

void ProjectCardWidget::updateStartBtn() {
    startBtn_->setProperty("active", active_);
    startBtn_->style()->unpolish(startBtn_);
    startBtn_->style()->polish(startBtn_);
    startBtn_->setText(active_ ? tr("■  Running") : tr("▶  Start"));
}

void ProjectCardWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QFrame::changeEvent(event);
}

void ProjectCardWidget::retranslateUi() {
    if (todaySecs_ <= 0) {
        durationLabel_->setText(tr("No time today"));
    } else {
        qint64 h = todaySecs_ / 3600;
        qint64 m = (todaySecs_ % 3600) / 60;
        qint64 s = todaySecs_ % 60;
        if (h > 0)      durationLabel_->setText(tr("%1h %2m today").arg(h).arg(m));
        else if (m > 0) durationLabel_->setText(tr("%1m today").arg(m));
        else            durationLabel_->setText(tr("%1s today").arg(s));
    }
    updateStartBtn();
}

void ProjectCardWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        emit clicked(project_.id);
    QFrame::mousePressEvent(event);
}
