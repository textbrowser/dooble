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

#include <QCryptographicHash>

#include "dooble_aes256.h"
#include "dooble_cryptography.h"
#include "dooble_hmac.h"
#include "dooble_pbkdf2.h"
#include "dooble_random.h"
#include "dooble_threefish256.h"

dooble_cryptography::dooble_cryptography
(const QByteArray &authentication_key,
 const QByteArray &encryption_key,
 const QString &block_cipher_type):QObject()
{
  m_as_plaintext = false;
  m_authenticated = false;
  m_authentication_key = authentication_key;
  m_block_cipher_type = block_cipher_type.toLower().trimmed();
  m_encryption_key = encryption_key;

  if(m_authentication_key.isEmpty() || m_encryption_key.isEmpty())
    {
      m_as_plaintext = true;
      m_authenticated = true;
      m_authentication_key.clear();
      m_encryption_key.clear();
    }
}

dooble_cryptography::dooble_cryptography(const QString &block_cipher_type):
  QObject()
{
  m_as_plaintext = false;
  m_authenticated = false;
  m_authentication_key = dooble_random::random_bytes(64);
  m_block_cipher_type = block_cipher_type.toLower().trimmed();
  m_encryption_key = dooble_random::random_bytes(32);
}

dooble_cryptography::~dooble_cryptography()
{
  {
    QByteArray zeros(m_authentication_key.length(), 0);

    m_authentication_key.replace(0, m_authentication_key.length(), zeros);
  }

  {
    QByteArray zeros(m_encryption_key.length(), 0);

    m_encryption_key.replace(0, m_encryption_key.length(), zeros);
  }
}

QByteArray dooble_cryptography::encrypt_then_mac(const QByteArray &data) const
{
  if(m_as_plaintext)
    return data;

  QByteArray bytes;

  if(m_block_cipher_type == "aes-256")
    {
      dooble_aes256 aes(m_encryption_key);

      bytes = aes.encrypt(data);
    }
  else
    {
      dooble_threefish256 threefish(m_encryption_key);

      threefish.set_tweak("76543210fedcba98", 0);
      bytes = threefish.encrypt(data);
    }

  if(!bytes.isEmpty())
    bytes.prepend(hmac(bytes));

  return bytes;
}

QByteArray dooble_cryptography::hmac(const QByteArray &message) const
{
  if(m_as_plaintext)
    return message;
  else
    return dooble_hmac::sha3_512_hmac(m_authentication_key, message);
}

QByteArray dooble_cryptography::hmac(const QString &message) const
{
  if(m_as_plaintext)
    return message.toUtf8();
  else
    return dooble_hmac::sha3_512_hmac(m_authentication_key, message.toUtf8());
}

QByteArray dooble_cryptography::mac_then_decrypt(const QByteArray &data) const
{
  if(m_as_plaintext)
    return data;

  QByteArray computed_mac;
  QByteArray mac(data.mid(0, dooble_hmac::preferred_output_size_in_bytes()));

  computed_mac = hmac(data.mid(dooble_hmac::preferred_output_size_in_bytes()));

  if(!computed_mac.isEmpty() && !mac.isEmpty() && memcmp(computed_mac, mac))
    {
      if(m_block_cipher_type == "aes-256")
	{
	  dooble_aes256 aes(m_encryption_key);

	  return aes.decrypt
	    (data.mid(dooble_hmac::preferred_output_size_in_bytes()));
	}
      else
	{
	  dooble_threefish256 threefish(m_encryption_key);

	  threefish.set_tweak("76543210fedcba98", 0);
	  return threefish.decrypt
	    (data.mid(dooble_hmac::preferred_output_size_in_bytes()));
	}
    }

  return data;
}

QPair<QByteArray, QByteArray> dooble_cryptography::keys(void) const
{
  return QPair<QByteArray, QByteArray> (m_authentication_key, m_encryption_key);
}

bool dooble_cryptography::as_plaintext(void) const
{
  return m_as_plaintext;
}

bool dooble_cryptography::authenticated(void) const
{
  return m_authenticated;
}

bool dooble_cryptography::memcmp(const QByteArray &a, const QByteArray &b)
{
  QByteArray c1;
  QByteArray c2;
  int length = qMax(a.length(), b.length());
  int rc = 0;

  c1 = a.leftJustified(length, 0);
  c2 = b.leftJustified(length, 0);

  for(int i = 0; i < length; i++)
    rc |= c1[i] ^ c2[i];

  return rc == 0;
}

void dooble_cryptography::authenticate(const QByteArray &salt,
				       const QByteArray &salted_password,
				       const QString &password)
{
  QByteArray hash
    (QCryptographicHash::hash(password.toUtf8() + salt,
			      QCryptographicHash::Sha3_512));

  m_authenticated = memcmp(hash, salted_password);
}

void dooble_cryptography::prepare_keys(const QByteArray &password,
				       const QByteArray &salt,
				       int block_cipher_type_index,
				       int iteration_count)
{
  QList<QByteArray> list;
  dooble_pbkdf2 pbkdf2
    (password, salt, block_cipher_type_index, iteration_count, 1024);

  list = pbkdf2.pbkdf2(dooble_hmac::sha3_512_hmac);

  if(!list.isEmpty())
    {
      m_authentication_key = list.value(0).mid(0, 64);
      m_encryption_key = list.value(0).mid(64, 32);
    }
}

void dooble_cryptography::set_authenticated(bool state)
{
  m_authenticated = state;
}

void dooble_cryptography::set_block_cipher_type
(const QString &block_cipher_type_index)
{
  m_block_cipher_type = block_cipher_type_index.toLower().trimmed();
}

void dooble_cryptography::set_keys(const QByteArray &authentication_key,
				   const QByteArray &encryption_key)
{
  m_authentication_key = authentication_key;
  m_encryption_key = encryption_key;

  if(m_authentication_key.isEmpty() || m_encryption_key.isEmpty())
    {
      m_as_plaintext = true;
      m_authenticated = true;
      m_authentication_key.clear();
      m_encryption_key.clear();
    }
}
