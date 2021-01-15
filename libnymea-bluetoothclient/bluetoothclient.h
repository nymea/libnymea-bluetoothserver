#ifndef BLUETOOTHCLIENT_H
#define BLUETOOTHCLIENT_H

#include <QObject>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyCharacteristic>

class BluetoothClient : public QObject
{
    Q_OBJECT
public:
    enum State {
        StateUnconnected,
        StateConnecting,
        StateConnected,
        StateServiceDiscovery,
        StateEncrypting,
        StateConnectedEncrypted
    };
    Q_ENUM(State)

    explicit BluetoothClient(const QBluetoothDeviceInfo &deviceInfo, QObject *parent = nullptr);

    State state() const;
    QString statusText() const;

    void connectDevice();
    void disconnectDevice();

private:
    QBluetoothDeviceInfo m_deviceInfo;
    QLowEnergyController *m_controller = nullptr;
    State m_state = StateUnconnected;
    QString m_statusText;

    void setState(State state);

protected:
    QLowEnergyController *controller();
    void setStatusText(const QString &statusText);

signals:
    void stateChanged(State state);
    void serviceDiscoveryFinished();
    void statusTextChanged(const QString &statusText);

private slots:
    void onConnected();
    void onDisconnected();

    void onDeviceError(const QLowEnergyController::Error &error);
    void onDeviceStateChanged(const QLowEnergyController::ControllerState &state);

};

#endif // BLUETOOTHCLIENT_H
