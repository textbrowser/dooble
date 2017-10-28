/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef dooble_cryptography_h
#define dooble_cryptography_h

#include <QByteArray>
#include <QObject>
#include <QPair>

class dooble_cryptography: public QObject
{
  Q_OBJECT

 public:
  dooble_cryptography(const QByteArray &authentication_key,
		      const QByteArray &encryption_key,
		      const QString &block_cipher_type);
  dooble_cryptography(const QString &block_cipher_type);
  ~dooble_cryptography();
  QByteArray encrypt_then_mac(const QByteArray &data) const;
  QByteArray hmac(const QByteArray &message) const;
  QByteArray hmac(const QString &message) const;
  QByteArray mac_then_decrypt(const QByteArray &data) const;
  QPair<QByteArray, QByteArray> keys(void) const;
  bool as_plaintext(void) const;
  bool authenticated(void) const;
  static bool memcmp(const QByteArray &a, const QByteArray &b);
  void authenticate(const QByteArray &salt,
		    const QByteArray &salted_password,
		    const QString &password);
  void set_authenticated(bool state);
  void set_block_cipher_type(const QString &block_cipher_type);
  void set_keys(const QByteArray &authentication_key,
		const QByteArray &encryption_key);

 private:
  QByteArray m_authentication_key;
  QByteArray m_encryption_key;
  QString m_block_cipher_type;
  bool m_as_plaintext;
  bool m_authenticated;
};

#endif
