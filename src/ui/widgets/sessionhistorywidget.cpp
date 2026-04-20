#include "sessionhistorywidget.h"
#include "data/database.h"
#include "data/models.h"
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

static QString fmtDuration(qint64 secs) {
    if (secs <= 0) return "–";
    qint64 h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
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

    auto* toolRow = new QHBoxLayout();
    toolRow->setSpacing(12);

    titleLabel_ = new QLabel(tr("Session History"), this);
    titleLabel_->setStyleSheet("font-size: 15px; font-weight: bold; color: #C1C2C5;");

    filterCombo_ = new QComboBox(this);
    filterCombo_->addItem(tr("All Projects"), -1LL);
    filterCombo_->setMinimumHeight(32);
    connect(filterCombo_, &QComboBox::currentIndexChanged, this, &SessionHistoryWidget::refresh);

    toolRow->addWidget(titleLabel_);
    toolRow->addStretch();
    toolRow->addWidget(filterCombo_);
    root->addLayout(toolRow);

    table_ = new QTableWidget(0, 5, this);
    retranslateUi();
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

void SessionHistoryWidget::retranslateUi() {
    titleLabel_->setText(tr("Session History"));
    table_->setHorizontalHeaderLabels({
        tr("Project"), tr("Date"), tr("Start"), tr("End"), tr("Duration")
    });
    // Update combo first item text
    if (filterCombo_->count() > 0)
        filterCombo_->setItemText(0, tr("All Projects"));
}

void SessionHistoryWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

void SessionHistoryWidget::setProjects(const QList<Project>& projects) {
    qint64 currentId = filterCombo_->currentData().toLongLong();
    filterCombo_->blockSignals(true);
    while (filterCombo_->count() > 1)
        filterCombo_->removeItem(1);
    for (const auto& p : projects)
        filterCombo_->addItem(p.name, p.id);
    int idx = filterCombo_->findData(currentId);
    if (idx >= 0) filterCombo_->setCurrentIndex(idx);
    filterCombo_->blockSignals(false);
    refresh();
}

void SessionHistoryWidget::setTimezoneOffsetSecs(int secs) {
    tzOffsetSecs_ = secs;
}

void SessionHistoryWidget::refresh() {
    qint64 filterId = filterCombo_->currentData().toLongLong();
    QList<TimeEntry> entries = (filterId > 0)
        ? db_->entriesForProject(filterId, 200)
        : db_->recentEntries(200);

    QHash<qint64, QString> nameMap, colorMap;
    for (const auto& p : db_->allProjects()) {
        nameMap[p.id]  = p.name;
        colorMap[p.id] = p.color;
    }

    // Shift stored local times into the selected timezone
    int sysOffsetSecs = QDateTime::currentDateTime().offsetFromUtc();
    int adjSecs       = tzOffsetSecs_ - sysOffsetSecs;

    table_->setRowCount(0);
    for (const auto& e : entries) {
        int row = table_->rowCount();
        table_->insertRow(row);

        auto* nameItem = new QTableWidgetItem(nameMap.value(e.projectId, "?"));
        nameItem->setForeground(QColor(colorMap.value(e.projectId, "#4DABF7")));
        table_->setItem(row, 0, nameItem);

        QDateTime startAdj = e.startTime.addSecs(adjSecs);
        table_->setItem(row, 1, new QTableWidgetItem(startAdj.date().toString("dd MMM yyyy")));
        table_->setItem(row, 2, new QTableWidgetItem(startAdj.time().toString("HH:mm:ss")));

        if (e.endTime.isValid()) {
            QDateTime endAdj = e.endTime.addSecs(adjSecs);
            table_->setItem(row, 3, new QTableWidgetItem(endAdj.time().toString("HH:mm:ss")));
            table_->setItem(row, 4, new QTableWidgetItem(fmtDuration(e.durationSecs())));
        } else {
            auto* ri = new QTableWidgetItem(tr("Running…"));
            ri->setForeground(QColor("#69DB7C"));
            table_->setItem(row, 3, ri);
            table_->setItem(row, 4, new QTableWidgetItem(
                fmtDuration(e.startTime.secsTo(QDateTime::currentDateTime()))));
        }
        table_->setRowHeight(row, 36);
    }
}
