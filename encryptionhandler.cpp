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

#include "encryptionhandler.h"
#include "loggingcategories.h"

#include <sodium.h>

EncryptionHandler::EncryptionHandler(QObject *parent) : QObject(parent)
{
    if (sodium_init() < 0) {
        qCWarning(dcNymeaBluetoothServerEncryption()) << "Could not initialize encryption library sodium";
        return;
    }

    m_initialized = true;
    qCDebug(dcNymeaBluetoothServer()) << "Encryption library initialized successfully: libsodium" << sodium_version_string();
}

bool EncryptionHandler::initialized() const
{
    return m_initialized;
}

bool EncryptionHandler::ready() const
{
    return m_ready;
}

void EncryptionHandler::reset()
{
    qCDebug(dcNymeaBluetoothServerEncryption()) << "Resetting encryption handler data";
    m_privateKey.clear();
    m_publicKey.clear();
    m_sharedKey.clear();
    m_clientPublicKey.clear();
    setReady(false);
}

bool EncryptionHandler::generateKeyPair()
{
    if (!m_initialized)
        return false;

    reset();

    qCDebug(dcNymeaBluetoothServerEncryption()) << "Generate new key pair...";
    unsigned char publicKey[crypto_box_PUBLICKEYBYTES];
    unsigned char secretKey[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(publicKey, secretKey);
    m_publicKey = QByteArray(reinterpret_cast<const char *>(publicKey), crypto_box_PUBLICKEYBYTES);
    m_privateKey = QByteArray(reinterpret_cast<const char *>(secretKey), crypto_box_SECRETKEYBYTES);
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Private key :" << m_privateKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Public key  :" << m_publicKey.toHex();
    return true;
}

bool EncryptionHandler::calculateSharedKey(const QByteArray &clientPublicKey)
{
    if (!m_initialized || m_publicKey.isEmpty() || m_privateKey.isEmpty())
        return false;

    m_clientPublicKey = clientPublicKey;

    qCDebug(dcNymeaBluetoothServerEncryption()) << "Calculate shared key for client public key" << clientPublicKey.toHex();
    unsigned char sharedKey[crypto_box_BEFORENMBYTES];
    int result = crypto_box_beforenm(sharedKey, reinterpret_cast<const unsigned char *>(m_clientPublicKey.data()), reinterpret_cast<const unsigned char *>(m_privateKey.data()));
    if (result != 0) {
        qCWarning(dcNymeaBluetoothServerEncryption()) << "Failed to calculate shared key for client.";
        return false;
    }

    m_sharedKey = QByteArray(reinterpret_cast<const char*>(sharedKey), crypto_box_BEFORENMBYTES);
    Q_ASSERT_X(m_sharedKey.length() == 32, "data length", "The shared key does not have the correct length.");
    qCDebug(dcNymeaBluetoothServerEncryption()) << "Shared key:" << m_sharedKey.toHex();

    qCDebug(dcNymeaBluetoothServerEncryption()) << "Encryption is ready for this client.";
    setReady(true);

    return true;
}

QByteArray EncryptionHandler::encryptData(const QByteArray &data, const QByteArray &nonce)
{
    qCDebug(dcNymeaBluetoothServerEncryption()) << "Encrypting data...";
    Q_ASSERT_X(nonce.length() == crypto_box_NONCEBYTES, "data length", "The nonce does not have the correct length.");

    /* Note: https://download.libsodium.org/doc/public-key_cryptography/authenticated_encryption.html
     *      unsigned char *c         The encrypted message (length of the data + crypto_box_MACBYTES)
     *      const unsigned char *m   The message to encrypt
     *      unsigned long long mlen  The length of the message to encrypt
     *      const unsigned char *n   The nonce (send in the unencrypted DATA)
     *      const unsigned char *pk  The public key of the client
     *      const unsigned char *sk  The private key
     */

    unsigned char encrypted[crypto_box_MACBYTES + data.length()];
    int result = crypto_box_easy(encrypted,
                                 reinterpret_cast<const unsigned char *>(data.data()),
                                 static_cast<unsigned long long>(data.length()),
                                 reinterpret_cast<const unsigned char *>(nonce.data()),
                                 reinterpret_cast<const unsigned char *>(m_clientPublicKey.data()),
                                 reinterpret_cast<const unsigned char *>(m_privateKey.data()));

    if (result != 0) {
        qCWarning(dcNymeaBluetoothServerEncryption()) << "Failed to encrypt data. Something went wrong" << result;
        return QByteArray();
    }

    QByteArray encryptedData = QByteArray(reinterpret_cast<const char*>(encrypted), crypto_box_MACBYTES + data.length());

    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Private key       :" << m_privateKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Public key        :" << m_publicKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Client public key :" << m_clientPublicKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Unencrypted data  :" << data.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Encrypted data    :" << encryptedData.toHex();

    return encryptedData;
}

QByteArray EncryptionHandler::decryptData(const QByteArray &data, const QByteArray &nonce)
{
    qCDebug(dcNymeaBluetoothServerEncryption()) << "Decrypting data...";
    Q_ASSERT_X(nonce.length() == crypto_box_NONCEBYTES, "data length", "The nonce does not have the correct length.");

    /* Note: https://download.libsodium.org/doc/public-key_cryptography/authenticated_encryption.html
     *      unsigned char *m         The decrypted message result
     *      const unsigned char *c   The message to decrypt / cyphertext (length of the encrypted data + crypto_box_MACBYTES)
     *      unsigned long long clen  The length of the message to decrypt
     *      const unsigned char *n   The nonce used while encryption (received in the unencrypted DATA)
     *      const unsigned char *pk  The public key of the client
     *      const unsigned char *sk  The private key
     */

    unsigned char decrypted[data.length() - crypto_box_MACBYTES];
    int result = crypto_box_open_easy(decrypted,
                                      reinterpret_cast<const unsigned char *>(data.data()),
                                      static_cast<unsigned long long>(data.length()),
                                      reinterpret_cast<const unsigned char *>(nonce.data()),
                                      reinterpret_cast<const unsigned char *>(m_clientPublicKey.data()),
                                      reinterpret_cast<const unsigned char *>(m_privateKey.data()));

    if (result != 0) {
        qCWarning(dcNymeaBluetoothServerEncryption()) << "Failed to decrypt data. Something went wrong" << result;
        return QByteArray();
    }

    QByteArray decryptedData = QByteArray(reinterpret_cast<const char*>(decrypted), data.length() - crypto_box_MACBYTES);

    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Private key       :" << m_privateKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Public key        :" << m_publicKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Client public key :" << m_clientPublicKey.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Encrypted data    :" << data.toHex();
    qCDebug(dcNymeaBluetoothServerEncryption()) << "    Decrypted data    :" << decryptedData.toHex();

    return decryptedData;
}

QByteArray EncryptionHandler::generateNonce(int length)
{
    unsigned char nounce[length];
    randombytes_buf(nounce, length);
    return QByteArray(reinterpret_cast<const char *>(nounce), length);
}

void EncryptionHandler::setReady(bool ready)
{
    if (m_ready == ready)
        return;

    m_ready = ready;
    emit readyChanged(m_ready);
}
