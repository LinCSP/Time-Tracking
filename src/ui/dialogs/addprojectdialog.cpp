#include "addprojectdialog.h"
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

static const QStringList kColors = {
    "#4DABF7", "#69DB7C", "#FA5252", "#FF922B",
    "#CC5DE8", "#F06595", "#20C997", "#FFD43B"
};

AddProjectDialog::AddProjectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("New Project");
    setModal(true);
    setFixedWidth(380);
    buildUi();
}

void AddProjectDialog::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(20);

    // Title
    auto* titleLabel = new QLabel("New Project", this);
    titleLabel->setObjectName("dialogTitle");
    root->addWidget(titleLabel);

    // Name
    auto* nameLabel = new QLabel("PROJECT NAME", this);
    nameLabel->setObjectName("fieldLabel");
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText("e.g. Website Redesign");
    nameEdit_->setMinimumHeight(36);
    root->addWidget(nameLabel);
    root->addWidget(nameEdit_);

    // Description
    auto* descLabel = new QLabel("DESCRIPTION (OPTIONAL)", this);
    descLabel->setObjectName("fieldLabel");
    descEdit_ = new QLineEdit(this);
    descEdit_->setPlaceholderText("Short description...");
    descEdit_->setMinimumHeight(36);
    root->addWidget(descLabel);
    root->addWidget(descEdit_);

    // Color
    auto* colorLabel = new QLabel("ACCENT COLOR", this);
    colorLabel->setObjectName("fieldLabel");
    root->addWidget(colorLabel);

    auto* swatchRow = new QHBoxLayout();
    swatchRow->setSpacing(8);
    swatchRow->setAlignment(Qt::AlignLeft);

    for (const QString& color : kColors) {
        auto* btn = new QPushButton(this);
        btn->setObjectName("colorSwatch");
        btn->setCheckable(true);
        btn->setStyleSheet(QString("QPushButton#colorSwatch { background-color: %1; }").arg(color));

        connect(btn, &QPushButton::clicked, this, [this, color, btn]() {
            selectColor(color, btn);
        });

        swatchRow->addWidget(btn);

        if (color == selectedColor_) {
            btn->setChecked(true);
            checkedSwatch_ = btn;
        }
    }
    root->addLayout(swatchRow);

    // Spacer
    root->addSpacing(8);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("dialogCancel");
    cancelBtn->setMinimumHeight(36);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* acceptBtn = new QPushButton("Create", this);
    acceptBtn->setObjectName("dialogAccept");
    acceptBtn->setMinimumHeight(36);
    connect(acceptBtn, &QPushButton::clicked, this, [this]() {
        if (nameEdit_->text().trimmed().isEmpty()) {
            nameEdit_->setFocus();
            return;
        }
        accept();
    });

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(acceptBtn);
    root->addLayout(btnRow);

    nameEdit_->setFocus();
}

void AddProjectDialog::selectColor(const QString& color, QPushButton* btn) {
    if (checkedSwatch_) checkedSwatch_->setChecked(false);
    checkedSwatch_ = btn;
    btn->setChecked(true);
    selectedColor_ = color;
}

QString AddProjectDialog::projectName() const {
    return nameEdit_->text().trimmed();
}

QString AddProjectDialog::projectColor() const {
    return selectedColor_;
}

QString AddProjectDialog::projectDescription() const {
    return descEdit_->text().trimmed();
}
