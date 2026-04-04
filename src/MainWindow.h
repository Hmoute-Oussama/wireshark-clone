#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onStartCapture();
    void onStopCapture();

private:
    void setupUi();

    QTableWidget *packetTable;
    QPushButton *startBtn;
    QPushButton *stopBtn;
};
