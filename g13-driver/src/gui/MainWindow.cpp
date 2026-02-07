#include "MainWindow.h"
#include "ui_MainWindow.h" 

// Include headers for the promoted widgets so the compiler knows the types
#include "G13Visualizer.h"
#include "BindingEditor.h"
#include "ColorSelector.h"
#include "MacroRecorder.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Setup the User Interface defined in the .ui file
    ui->setupUi(this);
    
    // Connect the Visualizer signal to the Logic
    // When a key is clicked on the G13 image, switch to the Editor tab and set the key
    connect(ui->visualizer, &G13Visualizer::keySelected, [=](QString keyName){
        // Switch to the first tab (Binding Editor)
        ui->tabWidget->setCurrentIndex(0); 
        
        // Update the label in the Binding Editor
        ui->editor->setKey(keyName);
    });
}

MainWindow::~MainWindow() {
    delete ui;
}