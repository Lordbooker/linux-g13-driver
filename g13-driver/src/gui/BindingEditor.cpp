#include "BindingEditor.h"
#include <QVBoxLayout>
#include <QGroupBox>

BindingEditor::BindingEditor(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    
    auto group = new QGroupBox("Key Configuration", this);
    auto groupLayout = new QVBoxLayout(group);

    lblCurrentKey = new QLabel("No key selected", this);
    QFont font = lblCurrentKey->font();
    font.setBold(true);
    font.setPointSize(12);
    lblCurrentKey->setFont(font);
    
    editBinding = new QLineEdit(this);
    editBinding->setPlaceholderText("Press a key or enter command...");
    
    chkRepeat = new QCheckBox("Enable Auto-Repeat", this);

    groupLayout->addWidget(lblCurrentKey);
    groupLayout->addWidget(new QLabel("Assigned Action:"));
    groupLayout->addWidget(editBinding);
    groupLayout->addWidget(chkRepeat);
    groupLayout->addStretch();

    layout->addWidget(group);
}

void BindingEditor::setKey(const QString &keyName) {
    lblCurrentKey->setText("Editing: " + keyName);
    // TODO: Load actual config for this key from file/pipe
}