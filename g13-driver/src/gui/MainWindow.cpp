#include "MainWindow.h"
#include "ui_MainWindow.h" // Generated from .ui file
#include "G13Visualizer.h"
#include "BindingEditor.h"
#include "ColorSelector.h"
#include "MacroRecorder.h"

#include <QHBoxLayout>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Setup basic UI from Designer file (menubar, statusbar etc.)
    ui->setupUi(this);
    setWindowTitle("G13 Configuration Tool");

    // Create the central layout structure
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    // Left Panel: Visualizer (G13 Image)
    G13Visualizer *visualizer = new G13Visualizer(this);

    // Right Panel: Functional Tabs
    QTabWidget *tabs = new QTabWidget(this);
    
    BindingEditor *editor = new BindingEditor(this);
    ColorSelector *colors = new ColorSelector(this);
    MacroRecorder *macros = new MacroRecorder(this);

    tabs->addTab(editor, "Key Bindings");
    tabs->addTab(colors, "Backlight Color");
    tabs->addTab(macros, "Macro Recorder");

    // Add widgets to layout
    mainLayout->addWidget(visualizer);
    mainLayout->addWidget(tabs);
    
    // Adjust stretch factors (Visualizer fixed, Tabs expanding)
    mainLayout->setStretch(0, 0); 
    mainLayout->setStretch(1, 1);

    // Logic: Connect Visualizer click to Editor Tab
    connect(visualizer, &G13Visualizer::keySelected, [=](QString keyName){
        tabs->setCurrentIndex(0); // Switch to Editor
        editor->setKey(keyName);  // Update Editor Label
    });
}

MainWindow::~MainWindow() {
    delete ui;
}