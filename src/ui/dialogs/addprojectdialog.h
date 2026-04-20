#pragma once
#include <QDialog>

class QLineEdit;
class QPushButton;

class AddProjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddProjectDialog(QWidget* parent = nullptr);

    QString projectName() const;
    QString projectColor() const;
    QString projectDescription() const;

private:
    void buildUi();
    void selectColor(const QString& color, QPushButton* btn);

    QLineEdit*  nameEdit_;
    QLineEdit*  descEdit_;
    QString     selectedColor_{"#4DABF7"};
    QPushButton* checkedSwatch_{nullptr};
};
