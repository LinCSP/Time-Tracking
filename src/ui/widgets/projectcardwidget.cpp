#include "projectcardwidget.h"
#include <QHBoxLayout>
#include <QStyle>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

static QString formatDuration(qint64 secs) {
    if (secs <= 0) return "No time today";
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    if (h > 0) return QString("%1h %2m today").arg(h).arg(m);
    if (m > 0) return QString("%1m today").arg(m);
    return QString("%1s today").arg(secs % 60);
}

ProjectCardWidget::ProjectCardWidget(const Project& project, QWidget* parent)
    : QFrame(parent), project_(project) {
    setFixedHeight(110);
    setMinimumWidth(200);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    buildUi();
}

void ProjectCardWidget::buildUi() {
    // Apply left accent color
    setStyleSheet(QString("ProjectCardWidget { border-left-color: %1; }").arg(project_.color));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 12, 12);
    root->setSpacing(4);

    // Top row: dot + name
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    dotLabel_ = new QLabel("●", this);
    dotLabel_->setStyleSheet("color: transparent; font-size: 10px;");
    dotLabel_->setFixedSize(14, 14);
    dotLabel_->setAlignment(Qt::AlignCenter);

    nameLabel_ = new QLabel(project_.name, this);
    nameLabel_->setStyleSheet("font-size: 14px; font-weight: bold; color: #C1C2C5;");

    topRow->addWidget(dotLabel_);
    topRow->addWidget(nameLabel_, 1);
    root->addLayout(topRow);

    // Duration
    durationLabel_ = new QLabel("No time today", this);
    durationLabel_->setStyleSheet("font-size: 11px; color: #909296; padding-left: 22px;");
    root->addWidget(durationLabel_);

    root->addStretch();

    // Bottom row: start + delete
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(6);

    startBtn_ = new QPushButton("▶  Start", this);
    startBtn_->setObjectName("cardStartBtn");
    startBtn_->setFixedHeight(28);
    startBtn_->setCursor(Qt::PointingHandCursor);
    connect(startBtn_, &QPushButton::clicked, this, [this]() {
        emit clicked(project_.id);
    });

    deleteBtn_ = new QPushButton("✕", this);
    deleteBtn_->setObjectName("cardDeleteBtn");
    deleteBtn_->setFixedSize(28, 28);
    deleteBtn_->setCursor(Qt::PointingHandCursor);
    connect(deleteBtn_, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(project_.id);
    });

    bottomRow->addWidget(startBtn_);
    bottomRow->addStretch();
    bottomRow->addWidget(deleteBtn_);
    root->addLayout(bottomRow);

    // Dot pulse animation
    dotAnim_ = new QPropertyAnimation(this);
}

void ProjectCardWidget::setActive(bool active) {
    active_ = active;
    setProperty("active", active);
    style()->unpolish(this);
    style()->polish(this);

    if (active) {
        dotLabel_->setStyleSheet("color: #69DB7C; font-size: 10px;");
    } else {
        dotLabel_->setStyleSheet("color: transparent; font-size: 10px;");
    }
    updateStartBtn();
}

void ProjectCardWidget::setTodayDuration(qint64 secs) {
    durationLabel_->setText(formatDuration(secs));
}

void ProjectCardWidget::updateStartBtn() {
    startBtn_->setProperty("active", active_);
    startBtn_->style()->unpolish(startBtn_);
    startBtn_->style()->polish(startBtn_);
    startBtn_->setText(active_ ? "■  Running" : "▶  Start");
}

void ProjectCardWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        emit clicked(project_.id);
    QFrame::mousePressEvent(event);
}
