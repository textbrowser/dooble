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

#include <QtGlobal>

#ifdef Q_OS_WIN
#pragma comment(lib, "crypt32.lib")
#include <windows.h>
#include <Wincrypt.h>
#else
#include <QFile>
#endif

#include "dooble_random.h"

QByteArray dooble_random::random_bytes(int length)
{
  /*
  ** Returns an empty byte array if an error occurs.
  */

  if(length <= 0)
    return QByteArray();

  QByteArray bytes;

#if defined(Q_OS_BSD4) || \
  defined(Q_OS_BSDI) ||	  \
  defined(Q_OS_LINUX) ||  \
  defined(Q_OS_MACOS)
#ifndef Q_OS_LINUX
  QFile file("/dev/random");
#else
  QFile file("/dev/urandom");
#endif
  if(file.open(QIODevice::ReadOnly))
    {
      bytes.resize(length);

      if(bytes.length() !=
	 static_cast<int> (file.read(bytes.data(),
				     static_cast<qintptr> (bytes.length()))))
	bytes.clear();
    }
#elif defined(Q_OS_WIN)
  /*
  ** msdn.microsoft.com/en-us/library/windows/desktop/aa379942(v=vs.85).aspx
  */

  HCRYPTPROV h_crypt_prov = 0;

  if(CryptAcquireContext(&h_crypt_prov, NULL, NULL, PROV_RSA_FULL, NULL))
    {
      auto *data = new BYTE[static_cast<size_t> (length)];

      if(CryptGenRandom(h_crypt_prov, static_cast<size_t> (length), data))
	/*
	** msdn.microsoft.com/en-us/library/windows/desktop/aa383751(v=vs.85).
	** aspx
	*/

	bytes = QByteArray(reinterpret_cast<char *> (data), length);

      delete []data;
    }

  if(h_crypt_prov)
    CryptReleaseContext(h_crypt_prov, 0);
#endif

  return bytes;
}
