#pragma once
#include <QWidget>

class QLabel;

class TimerDisplay : public QWidget {
    Q_OBJECT
public:
    explicit TimerDisplay(QWidget* parent = nullptr);

public slots:
    void setElapsed(qint64 secs);
    void setProjectName(const QString& name);
    void reset();

private:
    QLabel* timeLabel_;
    QLabel* projectLabel_;
};
