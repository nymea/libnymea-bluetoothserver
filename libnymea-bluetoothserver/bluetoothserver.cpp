/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of libnymea-bluetoothserver.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "bluetoothserver.h"
#include "loggingcategories.h"

#include <networkmanager.h>
#include <networkmanagerutils.h>

#include <QFile>
#include <QSysInfo>


BluetoothServer::BluetoothServer(QObject *parent) :
    QObject(parent)
{
    m_encryptionHandler = new EncryptionHandler(this);

    // Debug
    m_encryptionHandler->generateKeyPair();
    qCDebug(dcNymeaBluetoothServer()) << "Nonce:" << m_encryptionHandler->generateNonce().toHex();

    m_encryptionService = new EncryptionService(m_encryptionHandler, this);
    registerService(m_encryptionService);
}

BluetoothServer::~BluetoothServer()
{
    qCDebug(dcNymeaBluetoothServer()) << "Destroy bluetooth server.";
    if (m_controller)
        m_controller->stopAdvertising();

    if (m_localDevice)
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);

}

QString BluetoothServer::advertiseName() const
{
    return m_advertiseName;
}

void BluetoothServer::setAdvertiseName(const QString &advertiseName)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "set advertise name while server running is not allowed.");
    m_advertiseName = advertiseName;
}

QString BluetoothServer::modelName() const
{
    return m_modelName;
}

void BluetoothServer::setModelName(const QString &modelName)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "set model name while server running is not allowed.");
    m_modelName = modelName;
}

QString BluetoothServer::softwareVersion() const
{
    return m_softwareVersion;
}

void BluetoothServer::setSoftwareVersion(const QString &softwareVersion)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "set software version while server running is not allowed.");
    m_softwareVersion = softwareVersion;
}

QString BluetoothServer::hardwareVersion() const
{
    return m_hardwareVersion;
}

void BluetoothServer::setHardwareVersion(const QString &hardwareVersion)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "set hardware version while server running is not allowed.");
    m_hardwareVersion = hardwareVersion;
}

QString BluetoothServer::serialNumber() const
{
    return m_serialNumber;
}

void BluetoothServer::setSerialNumber(const QString &serialNumber)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "set serial number while server running is not allowed.");
    m_serialNumber = serialNumber;
}

bool BluetoothServer::running() const
{
    return m_running;
}

bool BluetoothServer::connected() const
{
    return m_connected;
}

void BluetoothServer::registerService(BluetoothService *service)
{
    Q_ASSERT_X(!m_running, "BluetoothServer", "register service while server running is not allowed.");
    m_registeredServices.append(service);
}

void BluetoothServer::registerNetworkManagerService(NetworkManager *networkManager)
{
    // Since this is the internal functionality, we create the service native.
    // For any other service we should use register service

    // Add deprecated services for remaining backwards compatible
    m_networkManager = networkManager;
    registerService(new NetworkManagerService(m_networkManager, this));
}

void BluetoothServer::registerDeprecatedServices()
{
    if (m_networkManager) {
        m_networkService = new NetworkService(m_controller->addService(NetworkService::serviceData(m_networkManager), m_controller),
                                              m_networkManager, m_controller);
        m_serviceUuids.append(m_networkService->service()->serviceUuid());

        m_wirelessService = new WirelessService(m_controller->addService(WirelessService::serviceData(m_networkManager), m_controller),
                                                m_networkManager, m_controller);
        m_serviceUuids.append(m_wirelessService->service()->serviceUuid());
    }
}

QLowEnergyServiceData BluetoothServer::deviceInformationServiceData()
{
    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::DeviceInformation);

    // Model number string 0x2a24
    QLowEnergyCharacteristicData modelNumberCharData;
    modelNumberCharData.setUuid(QBluetoothUuid::ModelNumberString);
    if (m_modelName.isEmpty()) {
        modelNumberCharData.setValue(QString("N.A.").toUtf8());
    } else {
        modelNumberCharData.setValue(m_modelName.toUtf8());
    }

    modelNumberCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(modelNumberCharData);

    // Serial number string 0x2a25
    QLowEnergyCharacteristicData serialNumberCharData;
    serialNumberCharData.setUuid(QBluetoothUuid::SerialNumberString);
    if (m_serialNumber.isNull()) {
        // Note: if no serialnumber specified use the system uuid from /etc/machine-id
        qCDebug(dcNymeaBluetoothServer()) << "Serial number not specified. Using system uuid from /etc/machine-id as serialnumber.";
        m_serialNumber = readMachineId().toString();
    }
    serialNumberCharData.setValue(m_serialNumber.toUtf8());
    serialNumberCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(serialNumberCharData);

    // Firmware revision string 0x2a26
    QLowEnergyCharacteristicData firmwareRevisionCharData;
    firmwareRevisionCharData.setUuid(QBluetoothUuid::FirmwareRevisionString);
    firmwareRevisionCharData.setValue(QString(VERSION_STRING).toUtf8());
    firmwareRevisionCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(firmwareRevisionCharData);

    // Hardware revision string 0x2a27
    QLowEnergyCharacteristicData hardwareRevisionCharData;
    hardwareRevisionCharData.setUuid(QBluetoothUuid::HardwareRevisionString);
    hardwareRevisionCharData.setValue(m_hardwareVersion.toUtf8());
    hardwareRevisionCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(hardwareRevisionCharData);

    // Software revision string 0x2a28
    QLowEnergyCharacteristicData softwareRevisionCharData;
    softwareRevisionCharData.setUuid(QBluetoothUuid::SoftwareRevisionString);
    softwareRevisionCharData.setValue(m_softwareVersion.toUtf8());
    softwareRevisionCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(softwareRevisionCharData);

    // Manufacturer name string 0x2a29
    QLowEnergyCharacteristicData manufacturerNameCharData;
    manufacturerNameCharData.setUuid(QBluetoothUuid::ManufacturerNameString);
    manufacturerNameCharData.setValue(QString("nymea GmbH").toUtf8());
    manufacturerNameCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(manufacturerNameCharData);

    return serviceData;
}

QLowEnergyServiceData BluetoothServer::genericAccessServiceData()
{
    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::GenericAccess);

    // Device name 0x2a00
    QLowEnergyCharacteristicData nameCharData;
    nameCharData.setUuid(QBluetoothUuid::DeviceName);
    if (m_advertiseName.isNull()) {
        qCWarning(dcNymeaBluetoothServer()) << "Advertise name not specified. Using system host name as device name.";
        m_advertiseName = QSysInfo::machineHostName();
    }
    nameCharData.setValue(m_advertiseName.toUtf8());
    nameCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(nameCharData);

    // Appearance 0x2a01
    QLowEnergyCharacteristicData appearanceCharData;
    appearanceCharData.setUuid(QBluetoothUuid::Appearance);
    appearanceCharData.setValue(QByteArray(4, 0));
    appearanceCharData.setProperties(QLowEnergyCharacteristic::Read);
    serviceData.addCharacteristic(appearanceCharData);

    // Peripheral Privacy Flag 0x2a02
    QLowEnergyCharacteristicData privacyFlagCharData;
    privacyFlagCharData.setUuid(QBluetoothUuid::PeripheralPrivacyFlag);
    privacyFlagCharData.setValue(QByteArray(2, 0));
    privacyFlagCharData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Write);
    serviceData.addCharacteristic(privacyFlagCharData);

    // Reconnection Address 0x2a03
    QLowEnergyCharacteristicData reconnectionAddressCharData;
    reconnectionAddressCharData.setUuid(QBluetoothUuid::ReconnectionAddress);
    reconnectionAddressCharData.setValue(QByteArray());
    reconnectionAddressCharData.setProperties(QLowEnergyCharacteristic::Write);
    serviceData.addCharacteristic(reconnectionAddressCharData);

    return serviceData;
}

QLowEnergyServiceData BluetoothServer::genericAttributeServiceData()
{
    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::GenericAttribute);

    QLowEnergyCharacteristicData charData;
    charData.setUuid(QBluetoothUuid::ServiceChanged);
    charData.setProperties(QLowEnergyCharacteristic::Indicate);

    serviceData.addCharacteristic(charData);

    return serviceData;
}

void BluetoothServer::setRunning(bool running)
{
    if (m_running == running)
        return;

    m_running = running;
    emit runningChanged(m_running);
}

void BluetoothServer::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

QUuid BluetoothServer::readMachineId()
{
    QUuid systemUuid;
    QFile systemUuidFile("/etc/machine-id");
    if (systemUuidFile.open(QFile::ReadOnly)) {
        QString tmpId = QString::fromLatin1(systemUuidFile.readAll()).trimmed();
        tmpId.insert(8, "-");
        tmpId.insert(13, "-");
        tmpId.insert(18, "-");
        tmpId.insert(23, "-");
        systemUuid = QUuid(tmpId);
    } else {
        qCWarning(dcNymeaBluetoothServer()) << "Failed to open /etc/machine-id for reading the system uuid as device information serialnumber.";
    }
    systemUuidFile.close();

    return systemUuid;
}

void BluetoothServer::onHostModeStateChanged(const QBluetoothLocalDevice::HostMode mode)
{
    switch (mode) {
    case QBluetoothLocalDevice::HostConnectable:
        qCDebug(dcNymeaBluetoothServer()) << "Bluetooth host in connectable mode.";
        break;
    case QBluetoothLocalDevice::HostDiscoverable:
        qCDebug(dcNymeaBluetoothServer()) << "Bluetooth host in discoverable mode.";
        break;
    case QBluetoothLocalDevice::HostPoweredOff:
        qCDebug(dcNymeaBluetoothServer()) << "Bluetooth host in power off mode.";
        break;
    case QBluetoothLocalDevice::HostDiscoverableLimitedInquiry:
        qCDebug(dcNymeaBluetoothServer()) << "Bluetooth host in discoverable limited inquiry mode.";
        break;
    }
}

void BluetoothServer::onDeviceConnected(const QBluetoothAddress &address)
{
    qCDebug(dcNymeaBluetoothServer()) << "Device connected" << address.toString();
}

void BluetoothServer::onDeviceDisconnected(const QBluetoothAddress &address)
{
    qCDebug(dcNymeaBluetoothServer()) << "Device disconnected" << address.toString();
}

void BluetoothServer::onError(QLowEnergyController::Error error)
{
    qCWarning(dcNymeaBluetoothServer()) << "Bluetooth error occured:" << error << m_controller->errorString();
}

void BluetoothServer::onConnected()
{
    qCDebug(dcNymeaBluetoothServer()) << "Client connected" << m_controller->remoteName() << m_controller->remoteAddress();
    setConnected(true);
}

void BluetoothServer::onDisconnected()
{
    qCDebug(dcNymeaBluetoothServer()) << "Client disconnected";
    setConnected(false);
    stop();
}

void BluetoothServer::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    switch (state) {
    case QLowEnergyController::UnconnectedState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state disonnected.";
        setConnected(false);
        break;
    case QLowEnergyController::ConnectingState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state connecting...";
        setConnected(false);
        break;
    case QLowEnergyController::ConnectedState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state connected." << m_controller->remoteName() << m_controller->remoteAddress();
        setConnected(true);
        break;
    case QLowEnergyController::DiscoveringState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state discovering...";
        break;
    case QLowEnergyController::DiscoveredState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state discovered.";
        break;
    case QLowEnergyController::ClosingState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state closing...";
        break;
    case QLowEnergyController::AdvertisingState:
        qCDebug(dcNymeaBluetoothServer()) << "Controller state advertising...";
        setRunning(true);
        break;
    }
}

void BluetoothServer::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << "Service characteristic changed" << characteristic.uuid() << value;
}

void BluetoothServer::characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << "Service characteristic read" << characteristic.uuid() << value;
}

void BluetoothServer::characteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << "Service characteristic written" << characteristic.uuid() << value;
}

void BluetoothServer::descriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << "Descriptor read" << descriptor.uuid() << value;
}

void BluetoothServer::descriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << "Descriptor written" << descriptor.uuid() << value;
}

void BluetoothServer::serviceError(QLowEnergyService::ServiceError error)
{
    QString errorString;
    switch (error) {
    case QLowEnergyService::NoError:
        errorString = "No error";
        break;
    case QLowEnergyService::OperationError:
        errorString = "Operation error";
        break;
    case QLowEnergyService::CharacteristicReadError:
        errorString = "Characteristic read error";
        break;
    case QLowEnergyService::CharacteristicWriteError:
        errorString = "Characteristic write error";
        break;
    case QLowEnergyService::DescriptorReadError:
        errorString = "Descriptor read error";
        break;
    case QLowEnergyService::DescriptorWriteError:
        errorString = "Descriptor write error";
        break;
    case QLowEnergyService::UnknownError:
        errorString = "Unknown error";
        break;
    }

    qCWarning(dcNymeaBluetoothServer()) << "Service error:" << errorString;
}

void BluetoothServer::start()
{
    if (running()) {
        qCDebug(dcNymeaBluetoothServer()) << "Start Bluetooth server called but the server is already running.  Doing nothing.";
        return;
    }

    if (connected()) {
        qCDebug(dcNymeaBluetoothServer()) << "Start Bluetooth server called but the server is running and a client is connected. Doing nothing.";
        return;
    }

    qCDebug(dcNymeaBluetoothServer()) << "-------------------------------------";
    qCDebug(dcNymeaBluetoothServer()) << "Starting bluetooth server...";
    qCDebug(dcNymeaBluetoothServer()) << "-------------------------------------";

    // Local bluetooth device
    m_localDevice = new QBluetoothLocalDevice(this);
    if (!m_localDevice->isValid()) {
        qCWarning(dcNymeaBluetoothServer()) << "Local bluetooth device is not valid.";
        delete m_localDevice;
        m_localDevice = nullptr;
        return;
    }

    connect(m_localDevice, &QBluetoothLocalDevice::hostModeStateChanged, this, &BluetoothServer::onHostModeStateChanged);
    connect(m_localDevice, &QBluetoothLocalDevice::deviceConnected, this, &BluetoothServer::onDeviceConnected);
    connect(m_localDevice, &QBluetoothLocalDevice::deviceDisconnected, this, &BluetoothServer::onDeviceDisconnected);

    qCDebug(dcNymeaBluetoothServer()) << "Local device" << m_localDevice->name() << m_localDevice->address().toString();
    m_localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    m_localDevice->powerOn();

    // Bluetooth low energy periperal controller
    m_controller = QLowEnergyController::createPeripheral(this);
    connect(m_controller, &QLowEnergyController::stateChanged, this, &BluetoothServer::onControllerStateChanged);
    connect(m_controller, &QLowEnergyController::connected, this, &BluetoothServer::onConnected);
    connect(m_controller, &QLowEnergyController::disconnected, this, &BluetoothServer::onDisconnected);
    connect(m_controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onError(QLowEnergyController::Error)));

    // Add default services: https://www.bluetooth.com/specifications/gatt/services
    m_deviceInfoService = m_controller->addService(deviceInformationServiceData(), m_controller);
    m_serviceUuids.append(deviceInformationServiceData().uuid());
    m_genericAccessService = m_controller->addService(genericAccessServiceData(), m_controller);
    m_serviceUuids.append(genericAccessServiceData().uuid());
    m_genericAttributeService = m_controller->addService(genericAttributeServiceData(), m_controller);
    m_serviceUuids.append(genericAttributeServiceData().uuid());

    // Add all registered generic services
    foreach (BluetoothService *bluetoothService, m_registeredServices) {
        qCDebug(dcNymeaBluetoothServer()) << "Register service" << bluetoothService->name() << bluetoothService->serviceUuid().toString();
        QLowEnergyServiceData serviceData;
        serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
        serviceData.setUuid(bluetoothService->serviceUuid());

        QLowEnergyCharacteristicData receiverCharacteristicData;
        receiverCharacteristicData.setUuid(bluetoothService->receiverCharacteristicUuid());
        receiverCharacteristicData.setProperties(QLowEnergyCharacteristic::Write);
        receiverCharacteristicData.setValueLength(1, 20);
        serviceData.addCharacteristic(receiverCharacteristicData);

        // Sender characteristic
        QLowEnergyCharacteristicData senderCharacteristicData;
        senderCharacteristicData.setUuid(bluetoothService->senderCharacteristicUuid());
        senderCharacteristicData.setProperties(QLowEnergyCharacteristic::Notify);
        senderCharacteristicData.addDescriptor(QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0)));
        senderCharacteristicData.setValueLength(1, 20);
        serviceData.addCharacteristic(senderCharacteristicData);

        QLowEnergyService *service = m_controller->addService(serviceData, m_controller);
        // Create the generic service handler, taking care about the encryption, SLIP packaging for receiving and sending.
        // Will be deleted with the controller on stop
        new BluetoothServiceDataHandler(m_encryptionHandler, service, bluetoothService, m_controller);
    }

    // Add deprecated services for backwards compatibility
    registerDeprecatedServices();

    QLowEnergyAdvertisingData advertisingData;
    advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    advertisingData.setIncludePowerLevel(true);
    advertisingData.setLocalName(m_advertiseName);
    advertisingData.setServices({m_encryptionService->serviceUuid()});
    // FIXME: set nymea manufacturer SIG data once available

    // Note: advertise in 100 ms interval, this makes the device better discoverable on certain client devices
    QLowEnergyAdvertisingParameters advertisingParameters;
    advertisingParameters.setInterval(100, 100);

    qCDebug(dcNymeaBluetoothServer()) << "Start advertising" << m_advertiseName << m_localDevice->address().toString();
    m_controller->startAdvertising(advertisingParameters, advertisingData, advertisingData);

    // Note: setRunning(true) will be called when the service is really advertising, see onControllerStateChanged()
}

void BluetoothServer::stop()
{
    qCDebug(dcNymeaBluetoothServer()) << "-------------------------------------";
    qCDebug(dcNymeaBluetoothServer()) << "Stopping bluetooth server.";
    qCDebug(dcNymeaBluetoothServer()) << "-------------------------------------";

    if (m_localDevice) {
        qCDebug(dcNymeaBluetoothServer()) << "Set host mode to connectable.";
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
        delete m_localDevice;
        m_localDevice = nullptr;
    }

    if (m_controller) {
        qCDebug(dcNymeaBluetoothServer()) << "Stop advertising.";
        m_controller->stopAdvertising();
        delete m_controller;
        m_controller = nullptr;
    }

    setConnected(false);
    setRunning(false);
}
