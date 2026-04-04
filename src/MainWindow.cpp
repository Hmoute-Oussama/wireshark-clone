#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

void MainWindow::setupUi() {
    setWindowTitle("Wireshark Clone");
    resize(800, 600);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

     
    QHBoxLayout *controlLayout = new QHBoxLayout();
    startBtn = new QPushButton("Start Capture", this);
    stopBtn = new QPushButton("Stop Capture", this);
    stopBtn->setEnabled(false); 

    controlLayout->addWidget(startBtn);
    controlLayout->addWidget(stopBtn);
    controlLayout->addStretch();  

     
    packetTable = new QTableWidget(0, 6, this);
    QStringList headers = {"No.", "Time", "Source", "Destination", "Protocol", "Length"};
    packetTable->setHorizontalHeaderLabels(headers);
    packetTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    packetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(packetTable);

    
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartCapture);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopCapture);
}

void MainWindow::onStartCapture() {
    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);
     
}

void MainWindow::onStopCapture() {
    startBtn->setEnabled(true);
    stopBtn->setEnabled(false);
    
}
