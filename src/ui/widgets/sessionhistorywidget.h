#pragma once
#include <QWidget>

class Database;
class QTableWidget;
class QComboBox;

class SessionHistoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit SessionHistoryWidget(Database* db, QWidget* parent = nullptr);
    void refresh();
    void setProjects(const QList<struct Project>& projects);

private:
    void buildUi();

    Database*     db_;
    QTableWidget* table_;
    QComboBox*    filterCombo_;
};
