#pragma once
#include "data/models.h"
#include <QFrame>
#include <QPropertyAnimation>

class QLabel;
class QPushButton;

class ProjectCardWidget : public QFrame {
    Q_OBJECT
public:
    explicit ProjectCardWidget(const Project& project, QWidget* parent = nullptr);

    void   setActive(bool active);
    void   setTodayDuration(qint64 secs);
    qint64 projectId() const { return project_.id; }
    bool   isActive() const { return active_; }

signals:
    void clicked(qint64 projectId);
    void deleteRequested(qint64 projectId);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void buildUi();
    void updateStartBtn();

    Project     project_;
    bool        active_{false};
    QLabel*     nameLabel_;
    QLabel*     durationLabel_;
    QLabel*     dotLabel_;
    QPushButton* startBtn_;
    QPushButton* deleteBtn_;
    QPropertyAnimation* dotAnim_;
};
