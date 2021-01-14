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

#include "encryptionservice.h"
#include "loggingcategories.h"

#include <QMetaEnum>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCryptographicHash>
#include <QLowEnergyDescriptorData>
#include <QLowEnergyCharacteristicData>

EncryptionService::EncryptionService(EncryptionHandler *encryptionHandler, QObject *parent) :
    BluetoothService(parent),
    m_encryptionHandler(encryptionHandler)
{

}

EncryptionService::~EncryptionService()
{

}

QString EncryptionService::name() const
{
    return "Encryption";
}

QBluetoothUuid EncryptionService::serviceUuid() const
{
    return QBluetoothUuid(QUuid("56c8ae10-def5-4d9c-8233-795a32d01cd2"));
}

QBluetoothUuid EncryptionService::receiverCharacteristicUuid() const
{
    return QBluetoothUuid(QUuid("56c8ae11-def5-4d9c-8233-795a32d01cd2"));
}

QBluetoothUuid EncryptionService::senderCharacteristicUuid() const
{
    return QBluetoothUuid(QUuid("56c8ae12-def5-4d9c-8233-795a32d01cd2"));
}

bool EncryptionService::useEncryption() const
{
    return false;
}

void EncryptionService::receiveData(const QByteArray &data)
{
    qCDebug(dcNymeaBluetoothServer()) << name() << "message received" << qUtf8Printable(data);

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qCWarning(dcNymeaBluetoothServerTraffic()) << name() << "received invalid json data" << jsonError.errorString() << qUtf8Printable(data);
        sendResponse(MethodUnknown, ResponseCodeInvalidProtocol);
        return;
    }

    QVariantMap requestData = jsonDoc.toVariant().toMap();

    if (!requestData.contains("c")) {
        qCWarning(dcNymeaBluetoothServerTraffic()) << name() << "received invalid request data. The method property \"c\" is not included" << requestData;
        sendResponse(MethodUnknown, ResponseCodeInvalidProtocol);
        return;
    }

    int methodInt = requestData.value("c").toInt();
    bool methodIntValid = false;
    QMetaEnum methodEnum = QMetaEnum::fromType<Method>();
    for (int i = 0; i < methodEnum.keyCount(); i++) {
        if (methodEnum.value(i) == methodInt) {
            methodIntValid = true;
            break;
        }
    }

    if (!methodIntValid) {
        qCWarning(dcNymeaBluetoothServerTraffic()) << "Invalid method received. There is no method/command with id" << methodInt;
        sendResponse(methodInt, ResponseCodeInvalidMethod);
        return;
    }

    QVariantMap params;
    if (requestData.contains("p")) {
        params = requestData.value("p").toMap();
    }

    processRequest(static_cast<Method>(methodInt), params);
}

void EncryptionService::processRequest(EncryptionService::Method method, const QVariantMap &params)
{
    switch (method) {
    case MethodUnknown:
        qCWarning(dcNymeaBluetoothServerTraffic()) << "The method unknown is forbidden. It will only be used if the protocol is violated and the method unknown fot the response.";
        break;
    case MethodInitiateEncryption: {
        if (params.isEmpty() || !params.contains("pk")) {
            qCWarning(dcNymeaBluetoothServerTraffic()) << "Invalid params for" << method << params << "The public key from the client is missing";
            sendResponse(method, ResponseCodeInvalidParams);
            return;
        }

        QString clientPublicKeyString = params.value("pk").toString();
        QByteArray clientPublicKey = QByteArray::fromHex(clientPublicKeyString.toUtf8());
        qCDebug(dcNymeaBluetoothServer()) << "Received client public key" << clientPublicKey.toHex();
        if (!m_encryptionHandler->calculateSharedKey(clientPublicKey)) {
            qCWarning(dcNymeaBluetoothServerTraffic()) << "Failed to create shared key for client public key" << clientPublicKey.toHex();
            sendResponse(method, ResponseCodeEncryptionFailed);
            return;
        }

        // Encrypt challenge
        QByteArray nonce = m_encryptionHandler->generateNonce();
        QByteArray encryptedChallenge = m_encryptionHandler->encryptData(m_encryptionHandler->generateChallenge(), nonce);

        // Create response parameters
        QVariantMap responseParams;
        responseParams.insert("pk", m_encryptionHandler->publicKey().toHex());
        responseParams.insert("n", nonce.toHex());
        responseParams.insert("c", encryptedChallenge.toHex());
        sendResponse(method, ResponseCodeSuccess, responseParams);
        break;
    }
    case MethodConfirmChallenge: {
        if (params.isEmpty() || !params.contains("n") || !params.contains("c")) {
            qCWarning(dcNymeaBluetoothServerTraffic()) << "Invalid params for" << method << params;
            sendResponse(method, ResponseCodeInvalidParams);
            return;
        }

        QByteArray nonce = QByteArray::fromHex(params.value("n").toString().toUtf8());
        QByteArray encryptedChallengeConfirmation = QByteArray::fromHex(params.value("c").toString().toUtf8());

        // Decrypt the message
        QByteArray challengeConfirmation = m_encryptionHandler->decryptData(encryptedChallengeConfirmation, nonce);
        if (!m_encryptionHandler->verifyChallenge(challengeConfirmation)) {
            qCWarning(dcNymeaBluetoothServerEncryption()) << "Challenge confirmation does not match the expected value.";
            sendResponse(method, ResponseCodeEncryptionFailed);
            return;
        }

        qCDebug(dcNymeaBluetoothServer()) << "Encryption established successfully";
        sendResponse(method, ResponseCodeSuccess);
        break;
    }
    }
}

void EncryptionService::sendResponse(int method, EncryptionService::ResponseCode responseCode, const QVariantMap &responseParams)
{
    QVariantMap response;
    response.insert("c", method);
    response.insert("r", static_cast<int>(responseCode));
    if (!responseParams.isEmpty()) {
        response.insert("p", responseParams);
    }

    QByteArray data = QJsonDocument::fromVariant(response).toJson(QJsonDocument::Compact);
    sendData(data);
}
