#include "timerdisplay.h"
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

TimerDisplay::TimerDisplay(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->setAlignment(Qt::AlignCenter);

    timeLabel_ = new QLabel("--:--:--", this);
    timeLabel_->setObjectName("timerTimeStopped");
    timeLabel_->setAlignment(Qt::AlignCenter);

    projectLabel_ = new QLabel("", this);
    projectLabel_->setObjectName("timerProject");
    projectLabel_->setAlignment(Qt::AlignCenter);

    layout->addWidget(timeLabel_);
    layout->addWidget(projectLabel_);
}

static QString formatTime(qint64 secs) {
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    qint64 s = secs % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void TimerDisplay::setElapsed(qint64 secs) {
    timeLabel_->setText(formatTime(secs));
    timeLabel_->setObjectName("timerTime");
    timeLabel_->style()->unpolish(timeLabel_);
    timeLabel_->style()->polish(timeLabel_);
}

void TimerDisplay::setProjectName(const QString& name) {
    projectLabel_->setText(name.isEmpty() ? "" : name);
}

void TimerDisplay::reset() {
    timeLabel_->setText("--:--:--");
    timeLabel_->setObjectName("timerTimeStopped");
    timeLabel_->style()->unpolish(timeLabel_);
    timeLabel_->style()->polish(timeLabel_);
    projectLabel_->setText("");
}
