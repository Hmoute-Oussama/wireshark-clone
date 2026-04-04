#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();

    captureEngine = new PacketCapture(this);

    connect(captureEngine, &PacketCapture::packetCaptured,
            this, &MainWindow::onPacketCaptured);
    connect(captureEngine, &PacketCapture::captureError,
            this, &MainWindow::onCaptureError);
}

void MainWindow::setupUi() {
    setWindowTitle("Wireshark Clone");
    resize(1000, 650);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *controlLayout = new QHBoxLayout();

    interfaceCombo = new QComboBox(this);
    interfaceCombo->setMinimumWidth(350);

    startBtn = new QPushButton("Start Capture", this);
    stopBtn = new QPushButton("Stop Capture", this);
    stopBtn->setEnabled(false);

    statusLabel = new QLabel("Ready", this);

    controlLayout->addWidget(interfaceCombo);
    controlLayout->addWidget(startBtn);
    controlLayout->addWidget(stopBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(statusLabel);

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

    PacketCapture tempCapture;
    QStringList devices = tempCapture.getDeviceList();
    if (devices.isEmpty()) {
        interfaceCombo->addItem("No interfaces found (run as Admin)");
        startBtn->setEnabled(false);
    } else {
        interfaceCombo->addItems(devices);
    }
}

void MainWindow::onStartCapture() {
    packetTable->setRowCount(0);

    QString selected = interfaceCombo->currentText();
    QString deviceName = selected.split(" (").first();

    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);
    interfaceCombo->setEnabled(false);
    statusLabel->setText("Capturing...");

    captureEngine->startCapture(deviceName);
}

void MainWindow::onStopCapture() {
    captureEngine->stopCapture();

    startBtn->setEnabled(true);
    stopBtn->setEnabled(false);
    interfaceCombo->setEnabled(true);
    statusLabel->setText(QString("Stopped. %1 packets captured.")
                         .arg(packetTable->rowCount()));
}

void MainWindow::onPacketCaptured(PacketData packet) {
    int row = packetTable->rowCount();
    packetTable->insertRow(row);

    packetTable->setItem(row, 0, new QTableWidgetItem(QString::number(packet.number)));
    packetTable->setItem(row, 1, new QTableWidgetItem(packet.timestamp));
    packetTable->setItem(row, 2, new QTableWidgetItem(packet.source));
    packetTable->setItem(row, 3, new QTableWidgetItem(packet.destination));
    packetTable->setItem(row, 4, new QTableWidgetItem(packet.protocol));
    packetTable->setItem(row, 5, new QTableWidgetItem(QString::number(packet.length)));

    packetTable->scrollToBottom();
}

void MainWindow::onCaptureError(const QString &errorMsg) {
    onStopCapture();
    QMessageBox::critical(this, "Capture Error", errorMsg);
}
