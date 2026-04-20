#include "addprojectdialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

static const QStringList kColors = {
    "#4DABF7", "#69DB7C", "#FA5252", "#FF922B",
    "#CC5DE8", "#F06595", "#20C997", "#FFD43B"
};

AddProjectDialog::AddProjectDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("New Project"));
    setModal(true);
    setFixedWidth(380);
    buildUi(false);
}

AddProjectDialog::AddProjectDialog(const Project& project, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Edit Project"));
    setModal(true);
    setFixedWidth(380);
    selectedColor_ = project.color;
    buildUi(true);
    nameEdit_->setText(project.name);
    descEdit_->setText(project.description);
}

void AddProjectDialog::buildUi(bool editMode) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* titleLabel = new QLabel(editMode ? tr("Edit Project") : tr("New Project"), this);
    titleLabel->setObjectName("dialogTitle");
    root->addWidget(titleLabel);

    auto* nameLabel = new QLabel(tr("PROJECT NAME"), this);
    nameLabel->setObjectName("fieldLabel");
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("e.g. Website Redesign"));
    nameEdit_->setMinimumHeight(36);
    root->addWidget(nameLabel);
    root->addWidget(nameEdit_);

    auto* descLabel = new QLabel(tr("DESCRIPTION (OPTIONAL)"), this);
    descLabel->setObjectName("fieldLabel");
    descEdit_ = new QLineEdit(this);
    descEdit_->setPlaceholderText(tr("Short description..."));
    descEdit_->setMinimumHeight(36);
    root->addWidget(descLabel);
    root->addWidget(descEdit_);

    auto* colorLabel = new QLabel(tr("ACCENT COLOR"), this);
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
        if (color == selectedColor_) { btn->setChecked(true); checkedSwatch_ = btn; }
    }
    root->addLayout(swatchRow);
    root->addSpacing(4);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    cancelBtn->setObjectName("dialogCancel");
    cancelBtn->setMinimumHeight(36);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* acceptBtn = new QPushButton(editMode ? tr("Save") : tr("Create"), this);
    acceptBtn->setObjectName("dialogAccept");
    acceptBtn->setMinimumHeight(36);
    connect(acceptBtn, &QPushButton::clicked, this, [this]() {
        if (nameEdit_->text().trimmed().isEmpty()) { nameEdit_->setFocus(); return; }
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

QString AddProjectDialog::projectName()        const { return nameEdit_->text().trimmed(); }
QString AddProjectDialog::projectColor()       const { return selectedColor_; }
QString AddProjectDialog::projectDescription() const { return descEdit_->text().trimmed(); }
