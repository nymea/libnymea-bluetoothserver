#include "bluetoothclient.h"
#include "loggingcategories.h"

BluetoothClient::BluetoothClient(const QBluetoothDeviceInfo &deviceInfo, QObject *parent) :
    QObject(parent),
    m_deviceInfo(deviceInfo)
{
    m_controller = QLowEnergyController::createCentral(deviceInfo, this);
    m_controller->setRemoteAddressType(QLowEnergyController::PublicAddress);

    connect(m_controller, &QLowEnergyController::connected, this, &BluetoothClient::onConnected);
    connect(m_controller, &QLowEnergyController::disconnected, this, &BluetoothClient::onDisconnected);
    connect(m_controller, &QLowEnergyController::stateChanged, this, &BluetoothClient::onDeviceStateChanged);
    connect(m_controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onDeviceError(QLowEnergyController::Error)));
    connect(m_controller, SIGNAL(discoveryFinished()), this, SIGNAL(serviceDiscoveryFinished()));
}

QString BluetoothClient::statusText() const
{
    return m_statusText;
}

void BluetoothClient::connectDevice()
{
    if (m_controller->state() != QLowEnergyController::UnconnectedState) {
        qCDebug(dcNymeaBluetoothClient()) << "Controller in state:" << m_controller->state() << "Not connecting...";
        return;
    }

    qCDebug(dcNymeaBluetoothClient()) << "Start connecting...";
    m_controller->connectToDevice();
}

void BluetoothClient::disconnectDevice()
{
    qCDebug(dcNymeaBluetoothClient()) << "Disconnecting from device";
    m_controller->disconnectFromDevice();
}

void BluetoothClient::setState(BluetoothClient::State state)
{
    if (m_state == state)
        return;

    qCDebug(dcNymeaBluetoothClient()) << "State changed" << state;
    m_state = state;
    emit stateChanged(m_state);
}

QLowEnergyController *BluetoothClient::controller()
{
    return m_controller;
}

void BluetoothClient::setStatusText(const QString &statusText)
{
    m_statusText = statusText;
    emit statusTextChanged(m_statusText);
}

void BluetoothClient::onConnected()
{
    qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Connected to" << m_deviceInfo.name() << m_deviceInfo.address().toString();
    m_controller->discoverServices();
}

void BluetoothClient::onDisconnected()
{
    qCDebug(dcNymeaBluetoothClient()) << "Disconnected from" << m_deviceInfo.name() << m_deviceInfo.address().toString();
    //setConnected(false);
    setStatusText("Disconnected from " +  m_deviceInfo.name());
}

void BluetoothClient::onDeviceError(const QLowEnergyController::Error &error)
{
    qCWarning(dcNymeaBluetoothClient()) << "Device error occured" << m_deviceInfo.name() << m_deviceInfo.address().toString() << ": " << error << m_controller->errorString();
    //setConnected(false);
}

void BluetoothClient::onDeviceStateChanged(const QLowEnergyController::ControllerState &state)
{
    switch (state) {
    case QLowEnergyController::ConnectingState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Connecting...";
        setStatusText(QString(tr("Connecting to %1...").arg(m_deviceInfo.name())));
        break;
    case QLowEnergyController::ConnectedState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Connected!";
        setStatusText(QString(tr("Connected to %1").arg(m_deviceInfo.name())));
        break;
    case QLowEnergyController::ClosingState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Connection: Closing...";
        setStatusText(QString(tr("Disconnecting from %1...").arg(m_deviceInfo.name())));
        break;
    case QLowEnergyController::DiscoveringState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Discovering...";
        setStatusText(QString(tr("Discovering services of %1...").arg(m_deviceInfo.name())));
        break;
    case QLowEnergyController::DiscoveredState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Discovered!";
        setStatusText(QString(tr("%1 connected and discovered.").arg(m_deviceInfo.name())));
        //setConnected(true);
        break;
    case QLowEnergyController::UnconnectedState:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Not connected.";
        setStatusText(QString(tr("%1 disconnected.").arg(m_deviceInfo.name())));
        break;
    default:
        qCDebug(dcNymeaBluetoothClient()) << "BluetoothDevice: Unhandled state entered:" << state;
        break;
    }
}
