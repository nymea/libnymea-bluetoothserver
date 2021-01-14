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

#ifndef BLUETOOTHSERVICEDATAHANDLER_H
#define BLUETOOTHSERVICEDATAHANDLER_H

#include <QObject>
#include <QLowEnergyService>

#include "encryptionhandler.h"
#include "bluetoothservice.h"

class BluetoothServiceDataHandler : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothServiceDataHandler(EncryptionHandler *enryptionHandler, QLowEnergyService *service, BluetoothService *bluetoothService, QObject *parent = nullptr);

private:
    enum ProtocolByte {
        ProtocolByteEnd = 0xC0,
        ProtocolByteEsc = 0xDB,
        ProtocolByteTransposedEnd = 0xDC,
        ProtocolByteTransposedEsc = 0xDD
    };

    EncryptionHandler *m_enryptionHandler = nullptr;
    QLowEnergyService *m_service = nullptr;
    BluetoothService *m_bluetoothService = nullptr;
    QByteArray m_dataBuffer;

    QByteArray unescapeData(const QByteArray &data);
    QByteArray escapeData(const QByteArray &data);

    void processPackage(const QByteArray &package);

private slots:
    void characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void characteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void descriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void descriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void serviceError(const QLowEnergyService::ServiceError &error);

    void sendData(const QByteArray &data);

signals:

};

#endif // BLUETOOTHSERVICEDATAHANDLER_H
