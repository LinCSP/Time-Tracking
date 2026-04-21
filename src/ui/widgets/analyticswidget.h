#pragma once
#include "data/models.h"
#include <QDate>
#include <QList>
#include <QWidget>

class Database;
class QLabel;
class QPushButton;
class QTableWidget;

// ── Stacked bar chart ─────────────────────────────────────────────────────────

class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget* parent = nullptr);
    void setData(const QList<DailyEntry>& data,
                 const QList<Project>&    projects,
                 const QDate&             from,
                 const QDate&             to);
    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<DailyEntry> data_;
    QList<Project>    projects_;
    QDate             from_, to_;
};

// ── Analytics page widget ─────────────────────────────────────────────────────

class AnalyticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsWidget(Database* db, QWidget* parent = nullptr);
    void refresh();
    void setTimezoneOffsetSecs(int secs);

private:
    void buildUi();
    void loadData();
    void updateSummary(const QList<DailyEntry>& data);
    void updateChart(const QList<DailyEntry>& data);
    void updateTable(const QList<DailyEntry>& data);
    void exportCsv();
    void setRangeDays(int days, QPushButton* activeBtn);

    Database*  db_;
    int        tzOffsetSecs_{3 * 3600};
    int        rangeDays_{30};

    BarChartWidget* chart_{nullptr};
    QTableWidget*   table_{nullptr};

    QLabel* statTotal_{nullptr};
    QLabel* statDays_{nullptr};
    QLabel* statAvg_{nullptr};
    QLabel* statTop_{nullptr};

    QPushButton* btn7d_{nullptr};
    QPushButton* btn30d_{nullptr};
    QPushButton* btn90d_{nullptr};

    QList<Project> projects_;
};
