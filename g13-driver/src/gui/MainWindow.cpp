#include "MainWindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Fenster Titel & Größe
    setWindowTitle("G13 Configuration - Qt6 Edition");
    resize(800, 600);

    // Layout erstellen
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Ein Label
    statusLabel = new QLabel("G13 Driver Status: Checking...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    QFont font = statusLabel->font();
    font.setPointSize(16);
    statusLabel->setFont(font);

    // Ein Button
    saveButton = new QPushButton("Save Profile", this);
    
    // Interaktion (Signal & Slot - wie EventListener)
    connect(saveButton, &QPushButton::clicked, this, [=]() {
        QMessageBox::information(this, "Saved", "Profile saved successfully!");
        statusLabel->setText("Status: Profile Saved.");
    });

    // Elemente zum Layout hinzufügen
    layout->addWidget(statusLabel);
    layout->addWidget(saveButton);
}

MainWindow::~MainWindow()
{
}