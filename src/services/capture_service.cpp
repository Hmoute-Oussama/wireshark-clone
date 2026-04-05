#include "capture_service.hpp"

#include <algorithm>

CaptureService::CaptureService(QObject *parent) : QObject(parent) {
    connect(&captureEngine_, &PacketCapture::packetCaptured,
            this, &CaptureService::onPacketCaptured);
    connect(&captureEngine_, &PacketCapture::captureError,
            this, &CaptureService::onCaptureError);
}

CaptureService::~CaptureService() {
    stop();
}

QStringList CaptureService::availableDevices() const {
    return captureEngine_.getDeviceList();
}

bool CaptureService::start(const QString &deviceName) {
    if (capturing_ || deviceName.trimmed().isEmpty()) {
        return false;
    }

    currentDevice_ = deviceName.trimmed();
    capturedPacketCount_ = 0;
    packets_.clear();
    capturing_ = true;

    emit captureStarted(currentDevice_);
    captureEngine_.startCapture(currentDevice_);
    return true;
}

void CaptureService::stop() {
    if (!capturing_) {
        return;
    }

    captureEngine_.stopCapture();
    captureEngine_.wait();
    capturing_ = false;

    emit captureStopped(capturedPacketCount_);
}

void CaptureService::clearHistory() {
    packets_.clear();
    capturedPacketCount_ = 0;
}

void CaptureService::setHistoryLimit(qsizetype limit) {
    historyLimit_ = std::max<qsizetype>(1, limit);
    if (packets_.size() > historyLimit_) {
        packets_.erase(packets_.begin(), packets_.end() - historyLimit_);
    }
}

bool CaptureService::isCapturing() const {
    return capturing_;
}

QString CaptureService::currentDevice() const {
    return currentDevice_;
}

quint64 CaptureService::capturedPacketCount() const {
    return capturedPacketCount_;
}

const QVector<PacketData> &CaptureService::packets() const {
    return packets_;
}

void CaptureService::onPacketCaptured(PacketData packet) {
    ++capturedPacketCount_;
    packets_.append(packet);

    if (packets_.size() > historyLimit_) {
        packets_.remove(0, packets_.size() - historyLimit_);
    }

    emit packetCaptured(packet);
}

void CaptureService::onCaptureError(const QString &message) {
    if (capturing_) {
        captureEngine_.stopCapture();
        captureEngine_.wait();
        capturing_ = false;
        emit captureStopped(capturedPacketCount_);
    }

    emit errorOccurred(message);
}
