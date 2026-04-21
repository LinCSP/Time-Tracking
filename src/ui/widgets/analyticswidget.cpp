#include "analyticswidget.h"
#include "data/database.h"
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSet>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString fmtDuration(qint64 secs) {
    if (secs <= 0) return {};
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    if (h > 0) return QString("%1ч %2м").arg(h).arg(m, 2, 10, QChar('0'));
    return QString("%1м").arg(m);
}

static QString fmtDurationOrDash(qint64 secs) {
    QString s = fmtDuration(secs);
    return s.isEmpty() ? "—" : s;
}

// ── BarChartWidget ────────────────────────────────────────────────────────────

BarChartWidget::BarChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize BarChartWidget::sizeHint()        const { return {600, 220}; }
QSize BarChartWidget::minimumSizeHint() const { return {200, 180}; }

void BarChartWidget::setData(const QList<DailyEntry>& data,
                              const QList<Project>&    projects,
                              const QDate&             from,
                              const QDate&             to) {
    data_     = data;
    projects_ = projects;
    from_     = from;
    to_       = to;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent*) {
    const int PAD_L = 52;
    const int PAD_R = 16;
    const int PAD_T = 16;
    const int PAD_B = 44;   // x-axis labels + legend

    int W = width()  - PAD_L - PAD_R;
    int H = height() - PAD_T - PAD_B;
    if (W <= 0 || H <= 0 || !from_.isValid() || !to_.isValid()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#1E1E2E"));

    // Build day map: date -> {projectId -> secs}
    QMap<QDate, QMap<qint64, qint64>> dayMap;
    for (const auto& e : data_)
        dayMap[e.date][e.projectId] += e.secs;

    // Max daily total → round up to nice hour boundary
    qint64 maxDaySecs = 0;
    for (auto it = dayMap.cbegin(); it != dayMap.cend(); ++it) {
        qint64 tot = 0;
        for (auto s : it.value()) tot += s;
        maxDaySecs = qMax(maxDaySecs, tot);
    }
    qint64 gridMaxHours = qMax(qint64(1), (maxDaySecs + 3599) / 3600);
    if      (gridMaxHours > 12) gridMaxHours = ((gridMaxHours + 3) / 4) * 4;
    else if (gridMaxHours > 6)  gridMaxHours = ((gridMaxHours + 1) / 2) * 2;
    qint64 gridMaxSecs = gridMaxHours * 3600;

    // ── Grid lines & Y labels ─────────────────────────────────────────────────
    QFont labelFont;
    labelFont.setPixelSize(10);
    p.setFont(labelFont);

    const int NUM_GRID = 4;
    for (int i = 0; i <= NUM_GRID; ++i) {
        int y = PAD_T + H - H * i / NUM_GRID;
        p.setPen(QPen(QColor("#272740"), 1));
        p.drawLine(PAD_L, y, PAD_L + W, y);

        qint64 labelSecs = gridMaxSecs * i / NUM_GRID;
        QString txt;
        if (labelSecs == 0) {
            txt = "0";
        } else {
            qint64 lh = labelSecs / 3600;
            qint64 lm = (labelSecs % 3600) / 60;
            txt = (lm == 0) ? QString("%1ч").arg(lh)
                            : QString("%1ч%2м").arg(lh).arg(lm, 2, 10, QChar('0'));
        }
        p.setPen(QColor("#505070"));
        p.drawText(0, y - 7, PAD_L - 8, 14, Qt::AlignRight | Qt::AlignVCenter, txt);
    }

    // ── Bars ──────────────────────────────────────────────────────────────────
    int    numDays = from_.daysTo(to_) + 1;
    double barSlot = (double)W / numDays;
    double gap     = qMax(1.0, barSlot * 0.18);
    double bw      = qMax(1.0, barSlot - gap);

    // Precompute topmost project per day (last project in list with secs > 0)
    QMap<QDate, qint64> topProjForDay;
    for (auto it = dayMap.cbegin(); it != dayMap.cend(); ++it) {
        for (int i = projects_.size() - 1; i >= 0; --i) {
            if (it.value().value(projects_[i].id, 0) > 0) {
                topProjForDay[it.key()] = projects_[i].id;
                break;
            }
        }
    }

    // X label interval
    int labelInterval = 1;
    if      (numDays > 60) labelInterval = 14;
    else if (numDays > 28) labelInterval = 7;
    else if (numDays > 14) labelInterval = 3;
    else if (numDays > 7)  labelInterval = 2;

    for (int di = 0; di < numDays; ++di) {
        QDate  day   = from_.addDays(di);
        double xLeft = PAD_L + di * barSlot + gap / 2.0;

        const auto& projMap = dayMap.value(day);
        qint64 cumul = 0;

        for (const auto& proj : projects_) {
            qint64 secs = projMap.value(proj.id, 0);
            if (secs <= 0) continue;

            double yBottom = PAD_T + H - (double)(cumul + secs) * H / gridMaxSecs;
            double yTop    = PAD_T + H - (double)cumul * H / gridMaxSecs;
            double bh      = qMax(1.0, yTop - yBottom);

            QColor col(proj.color);
            bool   isTop = (topProjForDay.value(day, -1) == proj.id);

            if (bw >= 4.0 && isTop && bh >= 4.0) {
                double r = qMin(2.5, bw / 4.0);
                QPainterPath path;
                path.moveTo(xLeft,      yBottom + bh);
                path.lineTo(xLeft + bw, yBottom + bh);
                path.lineTo(xLeft + bw, yBottom + r);
                path.quadTo(xLeft + bw, yBottom, xLeft + bw - r, yBottom);
                path.lineTo(xLeft + r,  yBottom);
                path.quadTo(xLeft,      yBottom, xLeft, yBottom + r);
                path.closeSubpath();
                p.fillPath(path, col);
            } else {
                p.fillRect(QRectF(xLeft, yBottom, bw, bh), col);
            }

            cumul += secs;
        }

        // X label
        if (di % labelInterval == 0) {
            int xc = (int)(xLeft + bw / 2.0);
            p.setPen(QColor("#505070"));
            p.setFont(labelFont);
            p.drawText(xc - 28, PAD_T + H + 6, 56, 16,
                       Qt::AlignCenter,
                       day.toString("dd MMM"));
        }
    }

    // ── Axes ──────────────────────────────────────────────────────────────────
    p.setPen(QPen(QColor("#3A3A55"), 1));
    p.drawLine(PAD_L, PAD_T,     PAD_L,     PAD_T + H);
    p.drawLine(PAD_L, PAD_T + H, PAD_L + W, PAD_T + H);

    // ── Legend (bottom strip) ─────────────────────────────────────────────────
    // Only show projects that have any data
    QSet<qint64> usedIds;
    for (const auto& e : data_) usedIds.insert(e.projectId);

    QFont legendFont;
    legendFont.setPixelSize(10);
    p.setFont(legendFont);
    QFontMetrics fm(legendFont);

    int legendY  = PAD_T + H + 24;
    int legendX  = PAD_L;
    for (const auto& proj : projects_) {
        if (!usedIds.contains(proj.id)) continue;
        // Colored square
        p.fillRect(QRectF(legendX, legendY + 2, 8, 8), QColor(proj.color));
        // Name
        p.setPen(QColor("#808095"));
        p.drawText(legendX + 11, legendY, fm.horizontalAdvance(proj.name) + 2, 14,
                   Qt::AlignLeft | Qt::AlignVCenter, proj.name);
        legendX += 12 + fm.horizontalAdvance(proj.name) + 16;
        if (legendX > PAD_L + W - 20) break; // don't overflow
    }

    // "No data" placeholder
    if (data_.isEmpty()) {
        p.setPen(QColor("#383858"));
        QFont noDataFont;
        noDataFont.setPixelSize(13);
        p.setFont(noDataFont);
        p.drawText(PAD_L, PAD_T, W, H, Qt::AlignCenter, "Нет данных за выбранный период");
    }
}

// ── AnalyticsWidget ───────────────────────────────────────────────────────────

AnalyticsWidget::AnalyticsWidget(Database* db, QWidget* parent)
    : QWidget(parent), db_(db) {
    buildUi();
}

void AnalyticsWidget::setTimezoneOffsetSecs(int secs) {
    tzOffsetSecs_ = secs;
    loadData();
}

void AnalyticsWidget::refresh() { loadData(); }

// Helper: create a stat card with a big value label and a description label.
static QFrame* makeStatCard(QLabel*& valueOut, const QString& desc, QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName("statCard");
    auto* vl = new QVBoxLayout(card);
    vl->setContentsMargins(16, 14, 16, 14);
    vl->setSpacing(4);

    valueOut = new QLabel("—", card);
    valueOut->setObjectName("statValue");

    auto* descLbl = new QLabel(desc, card);
    descLbl->setObjectName("statDesc");

    vl->addWidget(valueOut);
    vl->addWidget(descLbl);
    return card;
}

void AnalyticsWidget::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("toolbarRow");
    auto* tbl = new QHBoxLayout(toolbar);
    tbl->setContentsMargins(16, 8, 16, 8);
    tbl->setSpacing(4);

    btn7d_  = new QPushButton("7 дней",  toolbar);
    btn30d_ = new QPushButton("30 дней", toolbar);
    btn90d_ = new QPushButton("90 дней", toolbar);

    for (auto* b : {btn7d_, btn30d_, btn90d_}) {
        b->setObjectName("rangeBtn");
        b->setCheckable(true);
        b->setFixedHeight(28);
        b->setCursor(Qt::PointingHandCursor);
    }
    btn30d_->setChecked(true);

    connect(btn7d_,  &QPushButton::clicked, this, [this]() { setRangeDays(7,  btn7d_);  });
    connect(btn30d_, &QPushButton::clicked, this, [this]() { setRangeDays(30, btn30d_); });
    connect(btn90d_, &QPushButton::clicked, this, [this]() { setRangeDays(90, btn90d_); });

    tbl->addWidget(btn7d_);
    tbl->addWidget(btn30d_);
    tbl->addWidget(btn90d_);
    tbl->addStretch();

    auto* exportBtn = new QPushButton("↓  Экспорт CSV", toolbar);
    exportBtn->setObjectName("exportBtn");
    exportBtn->setFixedHeight(28);
    exportBtn->setCursor(Qt::PointingHandCursor);
    connect(exportBtn, &QPushButton::clicked, this, &AnalyticsWidget::exportCsv);
    tbl->addWidget(exportBtn);

    root->addWidget(toolbar);

    // ── Stats cards ───────────────────────────────────────────────────────────
    auto* statsRow = new QWidget(this);
    auto* sl = new QHBoxLayout(statsRow);
    sl->setContentsMargins(16, 12, 16, 4);
    sl->setSpacing(12);

    sl->addWidget(makeStatCard(statTotal_, "Всего за период",   statsRow));
    sl->addWidget(makeStatCard(statDays_,  "Активных дней",     statsRow));
    sl->addWidget(makeStatCard(statAvg_,   "В среднем в день",  statsRow));
    sl->addWidget(makeStatCard(statTop_,   "Главный проект",    statsRow));

    root->addWidget(statsRow);

    // ── Chart ─────────────────────────────────────────────────────────────────
    auto* chartWrap = new QWidget(this);
    chartWrap->setObjectName("chartWrap");
    auto* cwl = new QVBoxLayout(chartWrap);
    cwl->setContentsMargins(16, 8, 16, 4);
    cwl->addWidget((chart_ = new BarChartWidget(chartWrap)));

    root->addWidget(chartWrap);

    // ── Separator ─────────────────────────────────────────────────────────────
    auto* sep = new QFrame(this);
    sep->setObjectName("headerSep");
    sep->setFixedHeight(1);
    root->addWidget(sep);

    // ── Table ─────────────────────────────────────────────────────────────────
    table_ = new QTableWidget(this);
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(true);
    table_->setFrameShape(QFrame::NoFrame);

    root->addWidget(table_, 1);
}

void AnalyticsWidget::setRangeDays(int days, QPushButton* activeBtn) {
    rangeDays_ = days;
    for (auto* b : {btn7d_, btn30d_, btn90d_})
        b->setChecked(b == activeBtn);
    loadData();
}

void AnalyticsWidget::loadData() {
    projects_ = db_->allProjects();

    int   sysOff = QDateTime::currentDateTime().offsetFromUtc();
    int   adj    = tzOffsetSecs_ - sysOff;
    QDate today  = QDateTime::currentDateTime().addSecs(adj).date();
    QDate from   = today.addDays(-(rangeDays_ - 1));

    auto data = db_->dailyBreakdown(from, today);
    updateSummary(data);
    updateChart(data);
    updateTable(data);
}

void AnalyticsWidget::updateSummary(const QList<DailyEntry>& data) {
    qint64          totalSecs = 0;
    QSet<QDate>     activeDays;
    QMap<qint64, qint64> projTotals;

    for (const auto& e : data) {
        totalSecs += e.secs;
        activeDays.insert(e.date);
        projTotals[e.projectId] += e.secs;
    }

    statTotal_->setText(fmtDurationOrDash(totalSecs));

    int days = activeDays.size();
    statDays_->setText(days > 0 ? QString::number(days) : "—");
    statAvg_->setText(days > 0 ? fmtDurationOrDash(totalSecs / days) : "—");

    qint64 maxSecs = 0; qint64 topId = -1;
    for (auto it = projTotals.cbegin(); it != projTotals.cend(); ++it)
        if (it.value() > maxSecs) { maxSecs = it.value(); topId = it.key(); }

    if (topId >= 0) {
        for (const auto& proj : projects_)
            if (proj.id == topId) { statTop_->setText(proj.name); break; }
    } else {
        statTop_->setText("—");
    }
}

void AnalyticsWidget::updateChart(const QList<DailyEntry>& data) {
    int   sysOff = QDateTime::currentDateTime().offsetFromUtc();
    int   adj    = tzOffsetSecs_ - sysOff;
    QDate today  = QDateTime::currentDateTime().addSecs(adj).date();
    QDate from   = today.addDays(-(rangeDays_ - 1));
    chart_->setData(data, projects_, from, today);
}

void AnalyticsWidget::updateTable(const QList<DailyEntry>& data) {
    // Collect which projects have data in range
    QSet<qint64> usedIds;
    for (const auto& e : data) usedIds.insert(e.projectId);

    QList<Project> usedProjects;
    for (const auto& p : projects_)
        if (usedIds.contains(p.id)) usedProjects.append(p);

    int   sysOff = QDateTime::currentDateTime().offsetFromUtc();
    int   adj    = tzOffsetSecs_ - sysOff;
    QDate today  = QDateTime::currentDateTime().addSecs(adj).date();

    // Build lookup
    QMap<QDate, QMap<qint64, qint64>> dayMap;
    for (const auto& e : data)
        dayMap[e.date][e.projectId] += e.secs;

    int cols = 1 + usedProjects.size() + 1; // Date | projects... | Total
    table_->setColumnCount(cols);
    table_->setRowCount(rangeDays_);

    // Headers
    QStringList headers;
    headers << "Дата";
    for (const auto& p : usedProjects) headers << p.name;
    headers << "Итого";
    table_->setHorizontalHeaderLabels(headers);

    // Color project header text by project color
    for (int c = 0; c < usedProjects.size(); ++c) {
        if (auto* hdr = table_->horizontalHeaderItem(c + 1))
            hdr->setForeground(QColor(usedProjects[c].color));
    }

    // Rows — newest date first
    for (int di = 0; di < rangeDays_; ++di) {
        QDate day = today.addDays(-(rangeDays_ - 1 - di));
        int   row = rangeDays_ - 1 - di;

        auto* dateItem = new QTableWidgetItem(day.toString("dd.MM.yyyy  ddd"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        if (day == today) dateItem->setForeground(QColor("#4DABF7"));
        table_->setItem(row, 0, dateItem);

        qint64 dayTotal = 0;
        const auto& projMap = dayMap.value(day);

        for (int c = 0; c < usedProjects.size(); ++c) {
            qint64 secs = projMap.value(usedProjects[c].id, 0);
            dayTotal += secs;
            auto* item = new QTableWidgetItem(secs > 0 ? fmtDuration(secs) : "");
            item->setTextAlignment(Qt::AlignCenter);
            if (secs > 0)
                item->setForeground(QColor(usedProjects[c].color).lighter(130));
            table_->setItem(row, c + 1, item);
        }

        auto* totItem = new QTableWidgetItem(dayTotal > 0 ? fmtDuration(dayTotal) : "");
        totItem->setTextAlignment(Qt::AlignCenter);
        if (dayTotal > 0) {
            QFont f = totItem->font();
            f.setBold(true);
            totItem->setFont(f);
            totItem->setForeground(QColor("#C1C2C5"));
        }
        table_->setItem(row, cols - 1, totItem);
    }

    // Summary row at top (totals per project)
    table_->insertRow(0);
    auto* sumLabel = new QTableWidgetItem("Итого");
    sumLabel->setTextAlignment(Qt::AlignCenter);
    QFont bf = sumLabel->font(); bf.setBold(true); sumLabel->setFont(bf);
    sumLabel->setForeground(QColor("#606080"));
    table_->setItem(0, 0, sumLabel);

    qint64 grandTotal = 0;
    for (int c = 0; c < usedProjects.size(); ++c) {
        qint64 sum = 0;
        for (const auto& e : data)
            if (e.projectId == usedProjects[c].id) sum += e.secs;
        grandTotal += sum;
        auto* item = new QTableWidgetItem(sum > 0 ? fmtDuration(sum) : "");
        item->setTextAlignment(Qt::AlignCenter);
        QFont f = item->font(); f.setBold(true); item->setFont(f);
        if (sum > 0) item->setForeground(QColor(usedProjects[c].color));
        table_->setItem(0, c + 1, item);
    }

    auto* grandItem = new QTableWidgetItem(grandTotal > 0 ? fmtDuration(grandTotal) : "");
    grandItem->setTextAlignment(Qt::AlignCenter);
    QFont f2 = grandItem->font(); f2.setBold(true); grandItem->setFont(f2);
    if (grandTotal > 0) grandItem->setForeground(QColor("#4DABF7"));
    table_->setItem(0, cols - 1, grandItem);
}

void AnalyticsWidget::exportCsv() {
    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить отчёт",
        QDir::homePath() + "/time_report.csv",
        "CSV (*.csv)"
    );
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось записать файл.");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    int cols = table_->columnCount();
    int rows = table_->rowCount();

    // Header
    QStringList hdr;
    for (int c = 0; c < cols; ++c)
        hdr << (table_->horizontalHeaderItem(c) ? table_->horizontalHeaderItem(c)->text() : "");
    out << hdr.join(";") << "\n";

    // Rows (skip summary row 0 — it's a derived value)
    for (int r = 0; r < rows; ++r) {
        QStringList row;
        for (int c = 0; c < cols; ++c) {
            auto* item = table_->item(r, c);
            row << (item ? item->text() : "");
        }
        out << row.join(";") << "\n";
    }

    file.close();
}
