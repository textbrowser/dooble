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

#ifndef Q_OS_WINDOWS
extern "C"
{
#include <sys/mman.h>
}
#endif

#include "dooble_aes256.h"
#include "dooble_cryptography.h"
#include "dooble_hmac.h"
#include "dooble_random.h"
#include "dooble_threefish256.h"
#include "dooble_xchacha20.h"

int dooble_cryptography::s_authentication_key_length = 64;
int dooble_cryptography::s_encryption_key_length = 32;

dooble_cryptography::dooble_cryptography
(const QByteArray &authentication_key,
 const QByteArray &encryption_key,
 const QString &cipher_type,
 const QString &hash_type):QObject()
{
  m_as_plaintext = false;
  m_authenticated = false;
  m_authentication_key = authentication_key;
  m_cipher_type = cipher_type.toLower().trimmed();
  m_encryption_key = encryption_key;

  if(hash_type.toLower().trimmed() == "keccak-512")
    m_hash_type = dooble_cryptography::HashTypes::KECCAK_512;
  else
    m_hash_type = dooble_cryptography::HashTypes::SHA3_512;

  if(m_authentication_key.isEmpty() || m_encryption_key.isEmpty())
    {
      m_as_plaintext = true;
      m_authenticated = true;
      m_authentication_key.clear();
      m_encryption_key.clear();
    }
#ifdef DOOBLE_MMAN_PRESENT
  else
    {
      mlock(m_authentication_key.constData(),
	    static_cast<size_t> (m_authentication_key.length()));
      mlock(m_encryption_key.constData(),
	    static_cast<size_t> (m_encryption_key.length()));
    }
#endif
}

dooble_cryptography::dooble_cryptography(const QString &cipher_type,
					 const QString &hash_type):QObject()
{
  m_as_plaintext = false;
  m_authenticated = false;
  m_authentication_key = dooble_random::random_bytes
    (s_authentication_key_length);
  m_cipher_type = cipher_type.toLower().trimmed();
  m_encryption_key = dooble_random::random_bytes(s_encryption_key_length);

  if(hash_type.toLower().trimmed() == "keccak-512")
    m_hash_type = dooble_cryptography::HashTypes::KECCAK_512;
  else
    m_hash_type = dooble_cryptography::HashTypes::SHA3_512;

#ifdef DOOBLE_MMAN_PRESENT
  mlock(m_authentication_key.constData(),
	static_cast<size_t> (m_authentication_key.length()));
  mlock(m_encryption_key.constData(),
	static_cast<size_t> (m_encryption_key.length()));
#endif
}

dooble_cryptography::~dooble_cryptography()
{
  memzero(m_authentication_key);
  memzero(m_encryption_key);
#ifdef DOOBLE_MMAN_PRESENT
  munlock(m_authentication_key.constData(),
	  static_cast<size_t> (m_authentication_key.length()));
  munlock(m_encryption_key.constData(),
	  static_cast<size_t> (m_encryption_key.length()));
#endif
}

QByteArray dooble_cryptography::encrypt_then_mac(const QByteArray &data) const
{
  if(m_as_plaintext)
    return data;

  QByteArray bytes;

  if(m_cipher_type == "aes-256")
    {
      dooble_aes256 aes(m_encryption_key);

      bytes = aes.encrypt(data);

      if(!bytes.isEmpty())
	bytes.prepend(hmac(bytes));
    }
  else if(m_cipher_type == "threefish-256")
    {
      dooble_threefish256 threefish(m_encryption_key);

      threefish.set_tweak("76543210fedcba98", nullptr);
      bytes = threefish.encrypt(data);

      if(!bytes.isEmpty())
	bytes.prepend(hmac(bytes));
    }
  else
    {
      dooble_xchacha20 xchacha20(m_encryption_key);

      bytes = xchacha20.encrypt(data);

      if(!bytes.isEmpty())
	bytes.prepend(hmac(bytes));
    }

  return bytes;
}

QByteArray dooble_cryptography::hmac(const QByteArray &message) const
{
  if(m_as_plaintext)
    return message;
  else
    {
      switch(m_hash_type)
	{
	case dooble_cryptography::HashTypes::KECCAK_512:
	  return dooble_hmac::keccak_512_hmac(m_authentication_key, message);
	default:
	  return dooble_hmac::sha3_512_hmac(m_authentication_key, message);
	}
    }
}

QByteArray dooble_cryptography::hmac(const QString &message) const
{
  if(m_as_plaintext)
    return message.toUtf8();
  else
    {
      switch(m_hash_type)
	{
	case dooble_cryptography::HashTypes::KECCAK_512:
	  return dooble_hmac::keccak_512_hmac
	    (m_authentication_key, message.toUtf8());
	default:
	  return dooble_hmac::sha3_512_hmac
	    (m_authentication_key, message.toUtf8());
	}
    }
}

QByteArray dooble_cryptography::mac_then_decrypt(const QByteArray &data) const
{
  if(m_as_plaintext)
    return data;

  if(m_cipher_type == "aes-256" || m_cipher_type == "threefish-256")
    {
      QByteArray computed_mac;
      auto const mac
	(data.mid(0, dooble_hmac::preferred_output_size_in_bytes()));

      computed_mac = hmac
	(data.mid(dooble_hmac::preferred_output_size_in_bytes()));

      if(!computed_mac.isEmpty() &&
	 !mac.isEmpty() &&
	 dooble_cryptography::memcmp(computed_mac, mac))
	{
	  if(m_cipher_type == "aes-256")
	    {
	      dooble_aes256 aes(m_encryption_key);

	      return aes.decrypt
		(data.mid(dooble_hmac::preferred_output_size_in_bytes()));
	    }
	  else
	    {
	      dooble_threefish256 threefish(m_encryption_key);

	      threefish.set_tweak("76543210fedcba98", nullptr);
	      return threefish.decrypt
		(data.mid(dooble_hmac::preferred_output_size_in_bytes()));
	    }
	}
    }
  else
    {
      dooble_xchacha20 xchacha20(m_encryption_key);

      return xchacha20.decrypt(data);
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
  auto const length = qMax(a.length(), b.length());
  quint64 rc = 0;

  c1 = a.leftJustified(length, 0);
  c2 = b.leftJustified(length, 0);

  for(int i = 0; i < length; i++)
    rc |= static_cast<quint64> (c1[i]) ^ static_cast<quint64> (c2[i]);

  return rc == 0;
}

void dooble_cryptography::authenticate(const QByteArray &salt,
				       const QByteArray &salted_password,
				       const QString &password)
{
  QByteArray hash;

  switch(m_hash_type)
    {
    case dooble_cryptography::HashTypes::KECCAK_512:
      {
	hash = QCryptographicHash::hash
	  (password.toUtf8() + salt, QCryptographicHash::Keccak_512);
	break;
      }
    default:
      {
	hash = QCryptographicHash::hash
	  (password.toUtf8() + salt, QCryptographicHash::Sha3_512);
	break;
      }
    }

  m_authenticated = dooble_cryptography::memcmp(hash, salted_password);
}

void dooble_cryptography::memzero(QByteArray &bytes)
{
  for(auto &&i : bytes)
    i = 0;
}

void dooble_cryptography::memzero(QString &text)
{
  for(auto &&i : text)
    i = QChar(0);
}

void dooble_cryptography::set_authenticated(bool state)
{
  m_authenticated = state;
}

void dooble_cryptography::set_cipher_type
(const QString &cipher_type_index)
{
  m_cipher_type = cipher_type_index.toLower().trimmed();
}

void dooble_cryptography::set_hash_type(const QString &hash_type)
{
  if(hash_type.toLower().trimmed() == "keccak-512")
    m_hash_type = dooble_cryptography::HashTypes::KECCAK_512;
  else
    m_hash_type = dooble_cryptography::HashTypes::SHA3_512;
}

void dooble_cryptography::set_keys(const QByteArray &authentication_key,
				   const QByteArray &encryption_key)
{
#ifdef DOOBLE_MMAN_PRESENT
  munlock(m_authentication_key.constData(),
	  static_cast<size_t> (m_authentication_key.length()));
  munlock(m_encryption_key.constData(),
	  static_cast<size_t> (m_encryption_key.length()));
#endif
  m_authentication_key = authentication_key;
  m_encryption_key = encryption_key;

  if(m_authentication_key.isEmpty() || m_encryption_key.isEmpty())
    {
      m_as_plaintext = true;
      m_authenticated = true;
      m_authentication_key.clear();
      m_encryption_key.clear();
    }
  else
    {
      m_as_plaintext = false;
#ifdef DOOBLE_MMAN_PRESENT
      mlock(m_authentication_key.constData(),
	    static_cast<size_t> (m_authentication_key.length()));
      mlock(m_encryption_key.constData(),
	    static_cast<size_t> (m_encryption_key.length()));
#endif
    }
}
