#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QMenuBar>
#include <QFileDialog>
#include <QSplitter>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QVector>
#include <QLineEdit>
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
    void onPacketSelectionChanged();
    void onApplyFilter();
    void onClearFilter();
    void onFilterTextChanged(const QString &text);
    void onOpenCapture();
    void onSaveCapture();

private:
    void setupUi();
    void populateDetailTree(const PacketData &pkt);
    void populateHexView(const QByteArray &data);
    static QString formatHexDump(const QByteArray &data);
    static QString tcpFlagsStr(uint8_t flags);

    QMenuBar    *menuBar;
    QMenu       *fileMenu;
    QAction     *openAction;
    QAction     *saveAction;

    QComboBox   *interfaceCombo;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QLabel      *statusLabel;

    QLineEdit   *filterEdit;
    QPushButton *applyFilterBtn;
    QPushButton *clearFilterBtn;

    QSplitter    *mainSplitter;
    QTableWidget *packetTable;
    QTreeWidget  *detailTree;
    QPlainTextEdit *hexView;

    PacketCapture        *captureEngine;
    QVector<PacketData>   m_packets;
};
