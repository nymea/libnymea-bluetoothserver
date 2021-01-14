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

#include "networkmanagerservice.h"
#include "loggingcategories.h"

NetworkManagerService::NetworkManagerService(NetworkManager *networkManager, QObject *parent) :
    BluetoothService(parent),
    m_networkManager(networkManager)
{

}

NetworkManagerService::~NetworkManagerService()
{

}

QString NetworkManagerService::name() const
{
    return "NetworkManager";
}

QBluetoothUuid NetworkManagerService::serviceUuid() const
{
    return QBluetoothUuid(QUuid("d918edd0-bdb8-4b4b-b7e1-b15d50d361a2"));
}

QBluetoothUuid NetworkManagerService::receiverCharacteristicUuid() const
{
    return QBluetoothUuid(QUuid("d918edd1-bdb8-4b4b-b7e1-b15d50d361a2"));
}

QBluetoothUuid NetworkManagerService::senderCharacteristicUuid() const
{
    return QBluetoothUuid(QUuid("d918edd2-bdb8-4b4b-b7e1-b15d50d361a2"));
}

bool NetworkManagerService::useEncryption() const
{
    return true;
}

void NetworkManagerService::receiveData(const QByteArray &data)
{
    qCDebug(dcNymeaBluetoothServer()) << name() << "message received" << qUtf8Printable(data);
}
