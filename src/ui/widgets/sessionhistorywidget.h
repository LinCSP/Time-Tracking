#pragma once
#include <QWidget>

class Database;
class QTableWidget;
class QComboBox;
class QLabel;

class SessionHistoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit SessionHistoryWidget(Database* db, QWidget* parent = nullptr);
    void refresh();
    void setProjects(const QList<struct Project>& projects);

protected:
    void changeEvent(QEvent* event) override;

private:
    void buildUi();
    void retranslateUi();

    Database*     db_;
    QTableWidget* table_;
    QComboBox*    filterCombo_;
    QLabel*       titleLabel_;
};
