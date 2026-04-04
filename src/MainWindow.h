#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include "PacketCapture.h"
#include "PacketData.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onStartCapture();
    void onStopCapture();
    void onPacketCaptured(PacketData packet);
    void onCaptureError(const QString &errorMsg);

private:
    void setupUi();

    QTableWidget *packetTable;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QComboBox *interfaceCombo;
    QLabel *statusLabel;

    PacketCapture *captureEngine;
};
