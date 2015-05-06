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

#include <QMutexLocker>
#include <QObject>
#include <QtCore/qmath.h>

#include "dcrypt.h"
#include "dmisc.h"
#include "dooble.h"

dcrypt::dcrypt(const QByteArray &salt,
	       const QString &cipherType,
	       const QString &hashType,
	       const QString &passphrase,
	       const int iterationCount)
{
  m_cipherAlgorithm = 0;
  m_cipherHandle = 0;
  m_cipherType = cipherType;
  m_encryptionKey = 0;
  m_encryptionKeyLength = 0;
  m_hashAlgorithm = 0;
  m_hashKey = 0;
  m_hashKeyLength = 0;
  m_hashType = hashType;
  m_iterationCount = 0;
  setCipherAlgorithm();
  setHashAlgorithm();
  setIterationCount(iterationCount);
  setSalt(salt);
  openCipherHandle();
  setCipherPassphrase(passphrase);
}

dcrypt::dcrypt(dcrypt *other)
{
  bool ok = true;

  if(other)
    {
      m_cipherAlgorithm = other->m_cipherAlgorithm;
      m_cipherHandle = 0;
      m_cipherType = other->m_cipherType;

      if(other->m_encryptionKeyLength > 0)
	m_encryptionKey = static_cast<char *>
	  (gcry_calloc_secure(other->m_encryptionKeyLength, sizeof(char)));
      else
	{
	  m_encryptionKey = 0;
	  ok = false;
	}

      m_encryptionKeyLength = 0;
      m_hashAlgorithm = other->m_hashAlgorithm;

      if(other->m_hashKeyLength > 0)
	m_hashKey = static_cast<char *>
	  (gcry_calloc_secure(other->m_hashKeyLength, sizeof(char)));
      else
	{
	  m_hashKey = 0;
	  ok = false;
	}

      m_hashKeyLength = 0;
      m_hashType = other->m_hashType;
      m_iterationCount = other->m_iterationCount;
      setCipherAlgorithm();
      setHashAlgorithm();

      if(m_encryptionKey)
	{
	  m_encryptionKeyLength = other->m_encryptionKeyLength;
	  memcpy(m_encryptionKey,
		 other->m_encryptionKey,
		 m_encryptionKeyLength);
	}
      else
	{
	  ok = false;
	  dmisc::logError
	    (QObject::tr("dcrypt::dcrypt(): "
			 "gcry_calloc_secure() failure."));
	}

      if(m_hashKey)
	{
	  m_hashKeyLength = other->m_hashKeyLength;
	  memcpy(m_hashKey,
		 other->m_hashKey,
		 m_hashKeyLength);
	}
      else
	{
	  ok = false;
	  dmisc::logError
	    (QObject::tr("dcrypt::dcrypt(): "
			 "gcry_calloc_secure() failure."));
	}

      setSalt(other->m_salt);

      if(ok)
	{
	  if((ok = openCipherHandle()))
	    if(gcry_cipher_setkey(m_cipherHandle,
				  m_encryptionKey,
				  m_encryptionKeyLength) != 0)
	      {
		ok = false;
		dmisc::logError
		  (QObject::tr("dcrypt::dcrypt(): "
			       "gcry_cipher_setkey() failure."));
	      }
	}

      if(!ok)
	{
	  gcry_cipher_close(m_cipherHandle);
	  gcry_free(m_encryptionKey);
	  gcry_free(m_hashKey);
	}
    }

  if(!ok || !other)
    {
      m_cipherAlgorithm = 0;
      m_cipherHandle = 0;
      m_cipherType.clear();
      m_encryptionKey = 0;
      m_encryptionKeyLength = 0;
      m_hashAlgorithm = 0;
      m_hashKey = 0;
      m_hashKeyLength = 0;
      m_hashType.clear();
      m_iterationCount = 0;
      m_salt.clear();
    }
}

dcrypt::~dcrypt()
{
  gcry_cipher_close(m_cipherHandle);
  gcry_free(m_encryptionKey);
  gcry_free(m_hashKey);
}

void dcrypt::terminate(void)
{
  gcry_control(GCRYCTL_TERM_SECMEM);
}

QByteArray dcrypt::decodedString(const QByteArray &byteArray,
				 bool *ok)
{
  /*
  ** Returns the byteArray parameter if an error occurs.
  */

  if(!initialized())
    {
      if(ok)
	*ok = false;

      return byteArray;
    }

  QByteArray encodedArray(byteArray);
  QMutexLocker locker(&m_cipherMutex);

  if(!setInitializationVector(encodedArray))
    {
      if(ok)
	*ok = false;

      dmisc::logError(QObject::tr("dcrypt::decodedString(): "
				  "setInitializationVector() failure."));
      return byteArray;
    }
  else
    {
      QByteArray decodedArray(encodedArray);
      gcry_error_t err = 0;

      if((err = gcry_cipher_decrypt(m_cipherHandle,
				    decodedArray.data(),
				    decodedArray.length(),
				    0,
				    0)) == 0)
	{
	  QByteArray originalLength;

	  if(decodedArray.length() > 4)
	    originalLength = decodedArray.mid(decodedArray.length() - 4, 4);

	  if(!originalLength.isEmpty())
	    {
	      QDataStream in(&originalLength, QIODevice::ReadOnly);
	      int s = 0;

	      in >> s;

	      if(in.status() == QDataStream::Ok &&
		 s >= 0 && s <= decodedArray.length())
		{
		  if(ok)
		    *ok = true;

		  return decodedArray.mid(0, s);
		}
	      else
		{
		  if(ok)
		    *ok = false;

		  return byteArray;
		}
	    }
	  else
	    {
	      if(ok)
		*ok = false;

	      return byteArray;
	    }
	}
      else
	{
	  if(ok)
	    *ok = false;

	  QByteArray buffer(64, '0');

	  gpg_strerror_r(err, buffer.data(), buffer.length());
	  dmisc::logError(QObject::tr("dcrypt::decodedString(): "
				      "gcry_cipher_decrypt() failure (%1).").
			  arg(buffer.constData()));
	  return byteArray;
	}
    }

  if(ok)
    *ok = false;

  return byteArray;
}

QByteArray dcrypt::encodedString(const QByteArray &byteArray,
				 bool *ok)
{
  /*
  ** Returns the byteArray parameter if an error occurs.
  */

  if(!initialized())
    {
      if(ok)
	*ok = false;

      return byteArray;
    }

  QByteArray iv;
  QMutexLocker locker(&m_cipherMutex);

  if(!setInitializationVector(iv))
    {
      if(ok)
	*ok = false;

      dmisc::logError(QObject::tr("dcrypt::encodedString(): "
				  "setInitializationVector() failure."));
      return byteArray;
    }
  else
    {
      size_t blockLength = gcry_cipher_get_algo_blklen(m_cipherAlgorithm);

      if(blockLength <= 0)
	{
	  if(ok)
	    *ok = false;

	  dmisc::logError(QObject::tr("dcrypt::encodedString(): "
				      "gcry_cipher_get_algo_blklen() "
				      "failed."));
	  return byteArray;
	}

      QByteArray encodedArray(byteArray);

      if(encodedArray.isEmpty())
	encodedArray = encodedArray.leftJustified
	  (static_cast<int> (blockLength), 0);
      else if(static_cast<size_t> (encodedArray.length()) < blockLength)
	encodedArray = encodedArray.leftJustified
	  (static_cast<int> (blockLength *
			     qCeil((qreal) encodedArray.length() /
				   (qreal) blockLength)), 0);

      QByteArray originalLength;
      QDataStream out(&originalLength, QIODevice::WriteOnly);

      out << byteArray.length();

      if(out.status() != QDataStream::Ok)
	{
	  if(ok)
	    *ok = false;

	  dmisc::logError
	    (QObject::tr("dcrypt::encodedString(): "
			 "QDataStream error."));
	  return byteArray;
	}

      encodedArray.append(originalLength);

      gcry_error_t err = 0;

      if((err = gcry_cipher_encrypt(m_cipherHandle,
				    encodedArray.data(),
				    encodedArray.length(),
				    0,
				    0)) == 0)
	{
	  if(ok)
	    *ok = true;

	  return iv + encodedArray;
	}
      else
	{
	  if(ok)
	    *ok = false;

	  QByteArray buffer(64, '0');

	  gpg_strerror_r(err, buffer.data(), buffer.length());
	  dmisc::logError(QObject::tr("dcrypt::encodedString(): "
				      "gcry_cipher_encrypt() failure (%1).").
			  arg(buffer.constData()));
	  return byteArray;
	}
    }

  if(ok)
    *ok = false;

  return byteArray;
}

QByteArray dcrypt::salt(void) const
{
  return m_salt;
}

QStringList dcrypt::cipherTypes(void)
{
  QStringList types;

  /*
  ** Block ciphers only!
  */

  types << "aes256"
	<< "camellia256"
	<< "serpent256"
	<< "twofish";

  for(int i = types.size() - 1; i >= 0; i--)
    {
      int algorithm = gcry_cipher_map_name(types.at(i).toLatin1().
					   constData());

      if(!(algorithm != 0 && gcry_cipher_test_algo(algorithm) == 0))
	types.removeAt(i);
    }

  return types;
}

QStringList dcrypt::hashTypes(void)
{
  QStringList types;

  types << "sha512"
	<< "stribog512"
	<< "whirlpool";

  for(int i = types.size() - 1; i >= 0; i--)
    {
      int algorithm = gcry_md_map_name(types.at(i).toLatin1().constData());

      if(!(algorithm != 0 && gcry_md_test_algo(algorithm) == 0))
	types.removeAt(i);
    }

  return types;
}

bool dcrypt::initialized(void) const
{
  return
    gcry_cipher_test_algo(m_cipherAlgorithm) == 0 &&
    gcry_md_test_algo(m_hashAlgorithm) == 0 &&
    m_cipherHandle != 0;
}

bool dcrypt::openCipherHandle(void)
{
  if(m_cipherAlgorithm == 0)
    {
      dmisc::logError("dcrypt::openCipherHandle(): "
		      "m_cipherAlgorithm is 0.");
      return false;
    }

  gcry_cipher_close(m_cipherHandle);
  m_cipherHandle = 0;

  gcry_error_t err = 0;

  if((err = gcry_cipher_open(&m_cipherHandle, m_cipherAlgorithm,
			     GCRY_CIPHER_MODE_CBC,
			     GCRY_CIPHER_SECURE |
			     GCRY_CIPHER_CBC_CTS)) != 0 || !m_cipherHandle)
    {
      if(err != 0)
	dmisc::logError(QObject::tr("dcrypt::openCipherHandle(): "
				    "gcry_cipher_open() failure (%1).").
			arg(gcry_strerror(err)));
      else
	dmisc::logError(QObject::tr("dcrypt::openCipherHandle(): "
				    "gcry_cipher_open() failure."));

      return false;
    }

  return true;
}

bool dcrypt::setCipherPassphrase(const QString &passphrase)
{
  if(!initialized())
    return false;

  QByteArray l_passphrase;

  if(passphrase.isEmpty())
    {
      l_passphrase.resize(256);
      gcry_fast_random_poll();
      gcry_randomize(l_passphrase.data(),
		     l_passphrase.length(),
		     GCRY_STRONG_RANDOM);
    }
  else
    {
      l_passphrase.resize(passphrase.toUtf8().length());
      memcpy(l_passphrase.data(),
	     passphrase.toUtf8().constData(),
	     passphrase.toUtf8().length());
    }

  gcry_error_t err = 0;

#if DOOBLE_MINIMUM_GCRYPT_VERSION >= 0x010500
  if(m_encryptionKey)
    {
      gcry_free(m_encryptionKey);
      m_encryptionKey = 0;
    }

  if(m_hashKey)
    {
      gcry_free(m_hashKey);
      m_hashKey = 0;
    }

  m_encryptionKeyLength = gcry_cipher_get_algo_keylen(m_cipherAlgorithm);

  if(m_encryptionKeyLength <= 0)
    {
      err = GPG_ERR_INV_KEYLEN;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_cipher_get_algo_keylen() "
		     "failed."));
      goto error_label;
    }

  m_hashKeyLength = gcry_md_get_algo_dlen(m_hashAlgorithm);

  if(m_hashKeyLength <= 0)
    {
      err = GPG_ERR_INV_KEYLEN;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_md_get_algo_dlen() "
		     "failed."));
      goto error_label;
    }

  if((m_encryptionKey = static_cast<char *>
      (gcry_calloc_secure(m_encryptionKeyLength, sizeof(char)))) &&
     (m_hashKey = static_cast<char *>
      (gcry_calloc_secure(m_hashKeyLength, sizeof(char)))))
    {
      QByteArray temporary1;
      QByteArray temporary2;

      temporary1.resize(static_cast<int> (m_encryptionKeyLength +
					  m_hashKeyLength));
      temporary2.resize(temporary1.length());
      gcry_fast_random_poll();
      err = gcry_kdf_derive(l_passphrase.constData(),
			    l_passphrase.length(),
			    GCRY_KDF_PBKDF2,
			    m_hashAlgorithm,
			    m_salt.constData(),
			    m_salt.length(),
			    m_iterationCount,
			    temporary1.length(),
			    temporary1.data());

      if(err == 0)
	{
	  gcry_fast_random_poll();
	  err = gcry_kdf_derive(temporary1.constData(),
				temporary1.length(),
				GCRY_KDF_PBKDF2,
				m_hashAlgorithm,
				m_salt.constData(),
				m_salt.length(),
				m_iterationCount,
				temporary2.length(),
				temporary2.data());
	}

      if(err == 0)
	{
	  unsigned int length = gcry_md_get_algo_dlen(m_hashAlgorithm);

	  if(length > 0)
	    {
	      memcpy
		(m_encryptionKey,
		 temporary2.mid(0, static_cast<int> (m_encryptionKeyLength)).
		 constData(),
		 m_encryptionKeyLength);
	      memcpy
		(m_hashKey,
		 temporary2.mid(static_cast<int> (m_encryptionKeyLength)).
		 constData(),
		 m_hashKeyLength);
	    }
	  else
	    {
	      err = GPG_ERR_INV_LENGTH;
	      dmisc::logError
		(QObject::tr("dcrypt::setCipherPassphrase(): "
			     "gcry_md_get_algo_dlen() "
			     "failed."));
	      goto error_label;
	    }

	  if(m_cipherHandle)
	    {
	      if((err = gcry_cipher_setkey(m_cipherHandle,
					   m_encryptionKey,
					   m_encryptionKeyLength)) != 0)
		{
		  dmisc::logError
		    (QObject::tr("dcrypt::setCipherPassphrase(): "
				 "gcry_cipher_setkey() failure (%1).").
		     arg(gcry_strerror(err)));
		  goto error_label;
		}
	    }
	  else
	    {
	      dmisc::logError
		(QObject::tr("dcrypt::setCipherPassphrase(): "
			     "m_cipherHandle is 0."));
	      goto error_label;
	    }
	}
      else
	{
	  dmisc::logError
	    (QObject::tr("dcrypt::setCipherPassphrase(): "
			 "gcry_kdf_derive() failure (%1).").
	     arg(gcry_strerror(err)));
	  goto error_label;
	}
    }
  else
    {
      err = GPG_ERR_ENOMEM;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_calloc_secure() "
		     "failed."));
      goto error_label;
    }
#else
  dmisc::logError
    (QObject::tr("dcrypt::setCipherPassphrase(): "
		 "gcry_kdf_derive() is not defined. "
		 "Using the provided passphrase's hash as the keys."));

  if(m_encryptionKey)
    {
      gcry_free(m_encryptionKey);
      m_encryptionKey = 0;
    }

  if(m_hashKey)
    {
      gcry_free(m_hashKey);
      m_hashKey = 0;
    }

  m_encryptionKeyLength = gcry_cipher_get_algo_keylen(m_cipherAlgorithm);

  if(m_encryptionKeyLength <= 0)
    {
      err = GPG_ERR_INV_KEYLEN;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_cipher_get_algo_keylen() "
		     "failed."));
      goto error_label;
    }

  m_hashKeyLength = gcry_md_get_algo_dlen(m_hashAlgorithm);

  if(m_hashKeyLength <= 0)
    {
      err = GPG_ERR_INV_KEYLEN;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_md_get_algo_dlen() "
		     "failed."));
      goto error_label;
    }

  if((m_encryptionKey = static_cast<char *>
      (gcry_calloc_secure(m_encryptionKeyLength, sizeof(char)))) &&
     (m_hashKey = static_cast<char *>
      (gcry_calloc_secure(m_hashKeyLength, sizeof(char)))))
    {
      /*
      ** Retain the passphrase's hash. We'll use the hash as a key.
      */

      unsigned int length = gcry_md_get_algo_dlen(m_hashAlgorithm);

      if(length > 0)
	{
	  QByteArray byteArray(length, 0);

	  gcry_md_hash_buffer
	    (m_hashAlgorithm,
	     byteArray.data(),
	     l_passphrase.constData(),
	     l_passphrase.length());
	  memcpy
	    (m_encryptionKey,
	     byteArray.mid(0, static_cast<int> (m_encryptionKeyLength)).
	     constData(),
	     m_encryptionKeyLength);

	  if(byteArray.length() >=
	     static_cast<int> (m_encryptionKeyLength) + 16)
	    memcpy
	      (m_hashKey,
	       byteArray.mid(static_cast<int> (m_encryptionKeyLength)).
	       constData(),
	       m_hashKeyLength);
	  else
	    memcpy
	      (m_hashKey,
	       byteArray.mid(0, static_cast<int> (m_hashKeyLength)).
	       constData(),
	       m_hashKeyLength);

	  if(m_cipherHandle)
	    {
	      if((err =
		  gcry_cipher_setkey(m_cipherHandle,
				     m_encryptionKey,
				     m_encryptionKeyLength)) != 0)
		{
		  dmisc::logError
		    (QObject::tr("dcrypt::setCipherPassphrase(): "
				 "gcry_cipher_setkey() failure (%1).").
		     arg(gcry_strerror(err)));
		  goto error_label;
		}
	    }
	  else
	    {
	      dmisc::logError
		(QObject::tr("dcrypt::setCipherPassphrase(): "
			     "m_cipherHandle is 0."));
	      goto error_label;
	    }
	}
      else
	{
	  err = GPG_ERR_INV_LENGTH;
	  dmisc::logError
	    (QObject::tr("dcrypt::setCipherPassphrase(): "
			 "gcry_md_get_algo_dlen() "
			 "failed."));
	  goto error_label;
	}
    }
  else
    {
      err = GPG_ERR_ENOMEM;
      dmisc::logError
	(QObject::tr("dcrypt::setCipherPassphrase(): "
		     "gcry_calloc_secure() "
		     "failed."));
      goto error_label;
    }
#endif

  return err == 0;

 error_label:

  if(err != 0)
    {
      gcry_free(m_encryptionKey);
      gcry_free(m_hashKey);
      m_encryptionKey = 0;
      m_encryptionKeyLength = 0;
      m_hashKey = 0;
      m_hashKeyLength = 0;
    }

  return err == 0;
}

bool dcrypt::setCipherAlgorithm(void)
{
  m_cipherAlgorithm = gcry_cipher_map_name(m_cipherType.toLatin1().
					   constData());

  if(m_cipherAlgorithm == 0)
    m_cipherAlgorithm = gcry_cipher_map_name("aes256");

  bool rc = gcry_cipher_test_algo(m_cipherAlgorithm) == 0;

  if(!rc)
    m_cipherAlgorithm = 0;

  return rc;
}

bool dcrypt::setHashAlgorithm(void)
{
  m_hashAlgorithm = gcry_md_map_name(m_hashType.toLatin1().constData());

  if(m_hashAlgorithm == 0)
    m_hashAlgorithm = gcry_md_map_name("sha512");

  bool rc = gcry_md_test_algo(m_hashAlgorithm) == 0;

  if(!rc)
    m_hashAlgorithm = 0;

  return rc;
}

bool dcrypt::setInitializationVector(QByteArray &byteArray)
{
  if(!initialized())
    return false;

  size_t ivLength = 0;
  gcry_error_t err = 0;

  if((ivLength = gcry_cipher_get_algo_blklen(m_cipherAlgorithm)) <= 0)
    {
      err = GPG_ERR_INV_LENGTH;
      dmisc::logError
	(QObject::tr("dcrypt::setInitializationVector(): "
		     "gcry_cipher_get_algo_blklen() "
		     "failed."));
    }
  else
    {
      char *iv = static_cast<char *> (gcry_calloc(ivLength, sizeof(char)));

      if(iv)
	{
	  if(byteArray.isEmpty())
	    {
	      gcry_fast_random_poll();
	      gcry_create_nonce(iv, ivLength);
	      byteArray.append(iv, static_cast<int> (ivLength));
	    }
	  else
	    {
	      memcpy
		(iv,
		 byteArray.constData(),
		 qMin(ivLength, static_cast<size_t> (byteArray.length())));
	      byteArray.remove(0, static_cast<int> (ivLength));
	    }

	  gcry_cipher_reset(m_cipherHandle);

	  if((err = gcry_cipher_setiv(m_cipherHandle,
				      iv,
				      ivLength)) != 0)
	    {
	      QByteArray buffer(64, '0');

	      gpg_strerror_r(err, buffer.data(), buffer.length());
	      dmisc::logError
		(QObject::tr("dcrypt::setInitializationVector(): "
			     "gcry_cipher_setiv() failure (%1).").
		 arg(buffer.constData()));
	    }

	  gcry_free(iv);
	}
      else
	{
	  err = GPG_ERR_ENOMEM;
	  dmisc::logError
	    (QObject::tr("dcrypt::setInitializationVector(): "
			 "gcry_calloc() "
			 "failed."));
	}
    }

  return err == 0;
}

char *dcrypt::encryptionKey(void) const
{
  return m_encryptionKey;
}

int dcrypt::hashAlgorithm(void) const
{
  return m_hashAlgorithm;
}

unsigned long dcrypt::iterationCount(void) const
{
  return m_iterationCount;
}

size_t dcrypt::encryptionKeyLength(void) const
{
  return m_encryptionKeyLength;
}

void dcrypt::setIterationCount(const int iterationCount)
{
  m_iterationCount = qMax(1000, iterationCount);
}

void dcrypt::setSalt(const QByteArray &salt)
{
  bool ok = true;
  int saltLength = qAbs
    (qMax(256, dooble::s_settings.value("settingsWindow/saltLength",
					256).toInt(&ok)));

  if(!ok)
    saltLength = 256;

  if(salt.trimmed().isEmpty() || salt.trimmed().length() < saltLength)
    {
      QByteArray l_salt;

      if(ok)
	l_salt.resize(saltLength);
      else
	l_salt.resize(256);

      gcry_fast_random_poll();
      gcry_randomize(l_salt.data(),
		     l_salt.length(),
		     GCRY_STRONG_RANDOM);
      m_salt = l_salt;
    }
  else
    m_salt = salt.trimmed();
}

QString dcrypt::cipherType(void) const
{
  return m_cipherType;
}

QByteArray dcrypt::weakRandomBytes(const size_t size)
{
  if(size <= 0)
    return QByteArray();

  QByteArray bytes(static_cast<int> (size), 0);

  gcry_fast_random_poll();
  gcry_randomize(bytes.data(), bytes.length(), GCRY_WEAK_RANDOM);
  return bytes;
}

QByteArray dcrypt::keyedHash(const QByteArray &byteArray, bool *ok) const
{
  /*
  ** Returns the byteArray parameter if an error occurs.
  */

  gcry_error_t err = 0;
  gcry_md_hd_t hd = 0;
  QByteArray hashedArray(byteArray);

  if(m_hashKey == 0 || m_hashKeyLength <= 0)
    {
      if(ok)
	*ok = false;

      dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
				  "m_hashKey or m_hashKeyLength "
				  "is peculiar."));
      goto done_label;
    }

  if((err = gcry_md_open(&hd, m_hashAlgorithm,
			 GCRY_MD_FLAG_HMAC |
			 GCRY_MD_FLAG_SECURE)) != 0 || !hd)
    {
      if(ok)
	*ok = false;

      if(err != 0)
	dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
				    "gcry_md_open() failure (%1).").
			arg(gcry_strerror(err)));
      else
	dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
				    "gcry_md_open() failure."));
    }
  else
    {
      if((err = gcry_md_setkey(hd,
			       m_hashKey,
			       m_hashKeyLength)) != 0)
	{
	  if(ok)
	    *ok = false;

	  dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
				      "gcry_md_setkey() failure (%1).").
			  arg(gcry_strerror(err)));
	}
      else
	{
	  gcry_md_write
	    (hd,
	     byteArray.constData(),
	     byteArray.length());

	  unsigned char *buffer = gcry_md_read(hd, m_hashAlgorithm);

	  if(buffer)
	    {
	      unsigned int length = gcry_md_get_algo_dlen(m_hashAlgorithm);

	      if(length > 0)
		{
		  if(ok)
		    *ok = true;

		  hashedArray.clear();
		  hashedArray.resize(length);
		  memcpy(hashedArray.data(),
			 buffer,
			 hashedArray.length());
		}
	      else
		{
		  if(ok)
		    *ok = false;

		  dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
					      "gcry_md_get_algo_dlen() "
					      "failed."));
		}
	    }
	  else
	    {
	      if(ok)
		*ok = false;

	      dmisc::logError(QObject::tr("dcrypt::keyedHash(): "
					  "gcry_md_read() failed."));
	    }
	}
    }

 done_label:
  gcry_md_close(hd);
  return hashedArray;
}

QByteArray dcrypt::etm(const QByteArray &byteArray, bool *ok)
{
  /*
  ** Returns the byteArray parameter if an error occurs.
  */

  QByteArray bytes1;
  QByteArray bytes2;

  {
    bool ok = true;

    bytes1 = encodedString(byteArray, &ok);

    if(!ok)
      bytes1.clear();
  }

  if(!bytes1.isEmpty())
    {
      bool ok = true;

      bytes2 = keyedHash(bytes1, &ok);

      if(!ok)
	bytes2.clear();
    }

  if(bytes1.isEmpty() || bytes2.isEmpty())
    {
      if(ok)
	*ok = false;

      return byteArray;
    }
  else
    return bytes2 + bytes1;
}

QByteArray dcrypt::daa(const QByteArray &byteArray, bool *ok)
{
  /*
  ** Returns the byteArray parameter, or a portion of it, if an error occurs.
  */

  unsigned int length = gcry_md_get_algo_dlen(m_hashAlgorithm);

  if(length == 0)
    {
      if(ok)
	*ok = false;

      return byteArray;
    }

  QByteArray computedHash;
  QByteArray hash(byteArray.mid(0, length));

  {
    bool ok = true;

    computedHash = keyedHash(byteArray.mid(length), &ok);

    if(!ok)
      computedHash.clear();
  }

  if(!computedHash.isEmpty() &&
     !hash.isEmpty() && dmisc::compareByteArrays(computedHash, hash))
    return decodedString(byteArray.mid(length), ok);
  else
    {
      if(ok)
	*ok = false;

      return byteArray;
    }
}
