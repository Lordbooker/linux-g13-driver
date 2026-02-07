#include "MacroRecorder.h"

MacroRecorder::MacroRecorder(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel("Macro Sequence:"));
    
    stepList = new QListWidget(this);
    layout->addWidget(stepList);
    
    auto btnLayout = new QHBoxLayout();
    btnRecord = new QPushButton("Start Recording");
    btnDelete = new QPushButton("Delete Step");
    
    btnLayout->addWidget(btnRecord);
    btnLayout->addWidget(btnDelete);
    
    layout->addLayout(btnLayout);
}