#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QSet>
#include <QVector>
#include "PacketData.h"
#include "controller/packet_controller.hpp"

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
    void onSaveSelectedPacket();
    void onOpenDatabaseTestDialog();
    void onTestDatabaseConnection();

private:
    void setupUi();
    void createDatabaseTestDialog();
    void populateDetailTree(const PacketData &pkt);
    void populateHexView(const QByteArray &data);
    static QString formatHexDump(const QByteArray &data);
    static QString tcpFlagsStr(uint8_t flags);

    // ── Toolbar / controls ────────────────────────────────
    QComboBox   *interfaceCombo;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *dbTestBtn;
    QPushButton *saveToDbBtn;
    QLabel      *statusLabel;

    // ── Three-pane layout ─────────────────────────────────
    QSplitter    *mainSplitter;
    QTableWidget *packetTable;
    QTreeWidget  *detailTree;
    QPlainTextEdit *hexView;

    // ── Capture engine + packet store ─────────────────────
    PacketController     *packetController;
    QVector<PacketData>   m_packets;
    QSet<uint32_t>        m_savedPacketNumbers;
    QDialog              *dbDialog;
    QLineEdit            *dbHostEdit;
    QLineEdit            *dbPortEdit;
    QLineEdit            *dbNameEdit;
    QLineEdit            *dbUserEdit;
    QLineEdit            *dbPasswordEdit;
    QLabel               *dbDriversLabel;
    QLabel               *dbResultLabel;
};
