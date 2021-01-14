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

#ifndef ENCRYPTIONSERVICE_H
#define ENCRYPTIONSERVICE_H

#include <QObject>
#include <QLowEnergyService>

#include "bluetoothservice.h"
#include "encryptionhandler.h"

class EncryptionService : public BluetoothService
{
    Q_OBJECT
public:
    enum Method {
        MethodUnknown = -1,
        MethodInitiateEncryption = 0,
        MethodConfirmChallenge = 1
    };
    Q_ENUM(Method)

    enum ResponseCode {
        ResponseCodeSuccess = 0,
        ResponseCodeInvalidProtocol = 1,
        ResponseCodeInvalidMethod = 2,
        ResponseCodeInvalidParams = 3,
        ResponseCodeInvalidKeyFormat = 4,
        ResponseCodeAlreadyEncrypted = 5,
        ResponseCodeEncryptionFailed = 6
    };
    Q_ENUM(ResponseCode)

    explicit EncryptionService(EncryptionHandler *encryptionHandler, QObject *parent = nullptr);
    ~EncryptionService() override;

    QString name() const override;
    QBluetoothUuid serviceUuid() const override;
    QBluetoothUuid receiverCharacteristicUuid() const override;
    QBluetoothUuid senderCharacteristicUuid() const override;
    bool useEncryption() const override;

public slots:
    void receiveData(const QByteArray &data) override;

private:
    EncryptionHandler *m_encryptionHandler = nullptr;

    void processRequest(Method method, const QVariantMap &params = QVariantMap());
    void sendResponse(int method, ResponseCode responseCode = ResponseCodeSuccess, const QVariantMap &responseParams = QVariantMap());



};

#endif // ENCRYPTIONSERVICE_H
