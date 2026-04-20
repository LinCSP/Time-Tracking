#include "sessionhistorywidget.h"
#include "data/database.h"
#include "data/models.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

static QString fmtDuration(qint64 secs) {
    if (secs <= 0) return "–";
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    qint64 s = secs % 60;
    if (h > 0) return QString("%1h %2m").arg(h).arg(m);
    if (m > 0) return QString("%1m %2s").arg(m).arg(s);
    return QString("%1s").arg(s);
}

SessionHistoryWidget::SessionHistoryWidget(Database* db, QWidget* parent)
    : QWidget(parent), db_(db) {
    buildUi();
}

void SessionHistoryWidget::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 16);
    root->setSpacing(12);

    // Toolbar
    auto* toolRow = new QHBoxLayout();
    toolRow->setSpacing(12);

    auto* titleLabel = new QLabel("Session History", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #C1C2C5;");

    filterCombo_ = new QComboBox(this);
    filterCombo_->addItem("All Projects", -1LL);
    filterCombo_->setMinimumHeight(32);
    connect(filterCombo_, &QComboBox::currentIndexChanged, this, &SessionHistoryWidget::refresh);

    toolRow->addWidget(titleLabel);
    toolRow->addStretch();
    toolRow->addWidget(filterCombo_);
    root->addLayout(toolRow);

    // Table
    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels({"Project", "Date", "Start", "End", "Duration"});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);
    root->addWidget(table_);
}

void SessionHistoryWidget::setProjects(const QList<Project>& projects) {
    qint64 currentId = filterCombo_->currentData().toLongLong();
    filterCombo_->blockSignals(true);
    while (filterCombo_->count() > 1)
        filterCombo_->removeItem(1);
    for (const auto& p : projects)
        filterCombo_->addItem(p.name, p.id);
    // Restore selection
    int idx = filterCombo_->findData(currentId);
    if (idx >= 0) filterCombo_->setCurrentIndex(idx);
    filterCombo_->blockSignals(false);
    refresh();
}

void SessionHistoryWidget::refresh() {
    qint64 filterId = filterCombo_->currentData().toLongLong();

    QList<TimeEntry> entries;
    QList<Project>   projects = db_->allProjects();

    if (filterId > 0)
        entries = db_->entriesForProject(filterId, 200);
    else
        entries = db_->recentEntries(200);

    // Build project name map
    QHash<qint64, QString> nameMap;
    QHash<qint64, QString> colorMap;
    for (const auto& p : projects) {
        nameMap[p.id]  = p.name;
        colorMap[p.id] = p.color;
    }

    table_->setRowCount(0);
    for (const auto& e : entries) {
        int row = table_->rowCount();
        table_->insertRow(row);

        // Color dot + project name
        QString pName = nameMap.value(e.projectId, "Unknown");
        QString color = colorMap.value(e.projectId, "#4DABF7");
        auto*   nameItem = new QTableWidgetItem(pName);
        nameItem->setForeground(QColor(color));
        table_->setItem(row, 0, nameItem);

        table_->setItem(row, 1, new QTableWidgetItem(e.startTime.date().toString("dd MMM yyyy")));
        table_->setItem(row, 2, new QTableWidgetItem(e.startTime.time().toString("HH:mm:ss")));

        if (e.endTime.isValid()) {
            table_->setItem(row, 3, new QTableWidgetItem(e.endTime.time().toString("HH:mm:ss")));
            table_->setItem(row, 4, new QTableWidgetItem(fmtDuration(e.durationSecs())));
        } else {
            auto* runItem = new QTableWidgetItem("Running…");
            runItem->setForeground(QColor("#69DB7C"));
            table_->setItem(row, 3, runItem);
            qint64 elapsed = e.startTime.secsTo(QDateTime::currentDateTime());
            table_->setItem(row, 4, new QTableWidgetItem(fmtDuration(elapsed)));
        }

        table_->setRowHeight(row, 36);
    }
}
