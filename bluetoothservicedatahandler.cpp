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

#include "bluetoothservicedatahandler.h"
#include "loggingcategories.h"

#include <QDataStream>

BluetoothServiceDataHandler::BluetoothServiceDataHandler(EncryptionHandler *enryptionHandler, QLowEnergyService *service, BluetoothService *bluetoothService, QObject *parent) :
    QObject(parent),
    m_enryptionHandler(enryptionHandler),
    m_service(service),
    m_bluetoothService(bluetoothService)
{
    // Create characteristic connections
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic, QByteArray)), this, SLOT(characteristicChanged(QLowEnergyCharacteristic, QByteArray)));
    connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic, QByteArray)), this, SLOT(characteristicChanged(QLowEnergyCharacteristic, QByteArray)));
    connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic, QByteArray)), this, SLOT(characteristicWritten(QLowEnergyCharacteristic, QByteArray)));
    connect(m_service, SIGNAL(descriptorWritten(QLowEnergyDescriptor, QByteArray)), this, SLOT(descriptorWritten(QLowEnergyDescriptor, QByteArray)));
    connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(serviceError(QLowEnergyService::ServiceError)));

    connect(m_bluetoothService, &BluetoothService::requestSendData, this, &BluetoothServiceDataHandler::sendData);
}

QByteArray BluetoothServiceDataHandler::unescapeData(const QByteArray &data)
{
    QByteArray deserializedData;
    // Parse serial data and built InterfaceMessage
    bool escaped = false;
    for (int i = 0; i < data.length(); i++) {
        quint8 byte = static_cast<quint8>(data.at(i));

        if (escaped) {
            if (byte == ProtocolByteTransposedEnd) {
                deserializedData.append(static_cast<char>(ProtocolByteEnd));
            } else if (byte == ProtocolByteTransposedEsc) {
                deserializedData.append(static_cast<char>(ProtocolByteEsc));
            } else {
                qCWarning(dcNymeaBluetoothServerTraffic()) << m_bluetoothService->name() << "error while deserialing data. Escape character received but the escaped character was not recognized.";
                return QByteArray();
            }

            escaped = false;
            continue;
        }

        // If escape byte, the next byte has to be a modified byte
        if (byte == ProtocolByteEsc) {
            escaped = true;
        } else {
            deserializedData.append(static_cast<char>(byte));
        }
    }

    return deserializedData;
}

QByteArray BluetoothServiceDataHandler::escapeData(const QByteArray &data)
{
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);

    for (int i = 0; i < data.length(); i++) {
        quint8 byte = static_cast<quint8>(data.at(i));
        switch (byte) {
        case ProtocolByteEnd:
            stream << static_cast<quint8>(ProtocolByteEsc);
            stream << static_cast<quint8>(ProtocolByteTransposedEnd);
            break;
        case ProtocolByteEsc:
            stream << static_cast<quint8>(ProtocolByteEsc);
            stream << static_cast<quint8>(ProtocolByteTransposedEsc);
            break;
        default:
            stream << byte;
            break;
        }
    }

    return serializedData;
}

void BluetoothServiceDataHandler::processPackage(const QByteArray &package)
{
    qCDebug(dcNymeaBluetoothServerTraffic()) << m_bluetoothService->name() << "processing package" << package.toHex();

    // Note: package has already been unescaped

    // Decrypt data
    QByteArray data;
    if (m_enryptionHandler->ready() && m_bluetoothService->useEncryption()) {
        QByteArray nonce = package.left(32);
        QByteArray data = package.right(package.length() - 33);
        data = m_enryptionHandler->decryptData(data, nonce);
    } else {
        data = package;
    }

    // Process
    m_bluetoothService->receiveData(package);
}

void BluetoothServiceDataHandler::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_bluetoothService->receiverCharacteristicUuid()) {
        // Add data to the buffer and check if the package is complete. If so, process the data
        for (int i = 0; i < value.length(); i++) {
            quint8 byte = static_cast<quint8>(value.at(i));
            if (byte == ProtocolByteEnd) {
                // If there is no data...continue since it might be a starting END byte
                if (m_dataBuffer.isEmpty())
                    continue;

                qCDebug(dcNymeaBluetoothServerTraffic()) << m_bluetoothService->name() << "<--" << m_dataBuffer.toHex();
                QByteArray package = unescapeData(m_dataBuffer);
                if (package.isNull()) {
                    qCWarning(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "received inconsistant package. Ignoring data" << m_dataBuffer.toHex();
                } else {
                    processPackage(package);
                }
                m_dataBuffer.clear();
            } else {
                m_dataBuffer.append(value.at(i));
            }
        }
    } else {
        qCWarning(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "received service data on unhandled characteristic" << characteristic.uuid().toString() << value.toHex();
    }
}

void BluetoothServiceDataHandler::characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "characteristic read" << characteristic.uuid().toString() << value;
}

void BluetoothServiceDataHandler::characteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "characteristic written" << characteristic.uuid().toString() << value;
}

void BluetoothServiceDataHandler::descriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "descriptor read" << descriptor.uuid().toString() << value;
}

void BluetoothServiceDataHandler::descriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &value)
{
    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "descriptor written" << descriptor.uuid().toString() << value;
}

void BluetoothServiceDataHandler::serviceError(const QLowEnergyService::ServiceError &error)
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

    qCWarning(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "Error:" << errorString;
}

void BluetoothServiceDataHandler::sendData(const QByteArray &data)
{
    QByteArray finalData;
    // Encrypt
    if (m_enryptionHandler->ready() && m_bluetoothService->useEncryption()) {
        QByteArray nonce = m_enryptionHandler->generateNonce();
        QByteArray encryptedData = m_enryptionHandler->encryptData(data, nonce);
        finalData = nonce + encryptedData;
    } else {
        finalData = data;
    }

    // Escape
    QByteArray frame = escapeData(finalData);

    // Write
    QLowEnergyCharacteristic characteristic = m_service->characteristic(m_bluetoothService->senderCharacteristicUuid());
    if (!characteristic.isValid()) {
        qCWarning(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "sender characteristic not valid" << m_bluetoothService->senderCharacteristicUuid().toString();
        return;
    }

    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "start streaming escaped data:" << frame.count() << "bytes";
    int sentDataLength = 0;
    QByteArray remainingData = frame;
    while (!remainingData.isEmpty()) {
        QByteArray package = remainingData.left(20);
        sentDataLength += package.count();
        m_service->writeCharacteristic(characteristic, package);
        remainingData = remainingData.remove(0, package.count());
    }

    qCDebug(dcNymeaBluetoothServer()) << m_bluetoothService->name() << "finished streaming response data";
}
