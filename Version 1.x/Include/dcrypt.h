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

#ifndef _dcrypt_h_
#define _dcrypt_h_

extern "C"
{
  /*
  ** Older compilers (GCC 4.2.1) misbehave.
  */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gcrypt.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
}

#include <QMutex>

class dcrypt
{
 public:
  static QByteArray weakRandomBytes(const size_t size);
  static QStringList cipherTypes(void);
  static QStringList hashTypes(void);
  static void terminate(void);
  dcrypt(const QByteArray &salt,
	 const QString &cipherType,
	 const QString &hashType,
	 const QString &passphrase,
	 const int iterationCount);
  dcrypt(dcrypt *other);
  ~dcrypt();
  QByteArray daa(const QByteArray &byteArray, bool *ok);
  QByteArray decodedString(const QByteArray &byteArray, bool *ok);
  QByteArray encodedString(const QByteArray &byteArray, bool *ok);
  QByteArray etm(const QByteArray &byteArray, bool *ok);
  QByteArray keyedHash(const QByteArray &byteArray, bool *ok) const;
  QByteArray salt(void) const;
  QString cipherType(void) const;
  bool initialized(void) const;
  char *encryptionKey(void) const;
  int hashAlgorithm(void) const;
  size_t encryptionKeyLength(void) const;
  unsigned long iterationCount(void) const;

 private:
  QByteArray m_salt;
  QMutex m_cipherMutex;
  QString m_cipherType;
  QString m_hashType;
  char *m_encryptionKey; // Held in secure memory.
  char *m_hashKey; // Held in secure memory.
  gcry_cipher_hd_t m_cipherHandle;
  int m_cipherAlgorithm;
  int m_hashAlgorithm;
  size_t m_encryptionKeyLength;
  size_t m_hashKeyLength;
  unsigned long m_iterationCount;
  bool openCipherHandle(void);
  bool setCipherAlgorithm(void);
  bool setCipherPassphrase(const QString &passphrase);
  bool setHashAlgorithm(void);
  bool setInitializationVector(QByteArray &byteArray);
  void setIterationCount(const int iterationCount);
  void setSalt(const QByteArray &salt);
};

#endif
