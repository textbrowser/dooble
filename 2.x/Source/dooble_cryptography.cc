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

#include <QString>

#include "dooble_cryptography.h"
#include "dooble_hmac.h"
#include "dooble_random.h"

dooble_cryptography::dooble_cryptography(void)
{
  m_authenticated = false;
  m_authentication_key = dooble_random::random_bytes(64);
  m_encryption_key = dooble_random::random_bytes(32);
}

QByteArray dooble_cryptography::hmac(const QByteArray &message) const
{
  return dooble_hmac::sha3_512_hmac(m_authentication_key, message);
}

QByteArray dooble_cryptography::hmac(const QString &message) const
{
  return dooble_hmac::sha3_512_hmac(m_authentication_key, message.toUtf8());
}

bool dooble_cryptography::authenticated(void) const
{
  return m_authenticated;
}

void dooble_cryptography::setKeys(const QByteArray &authentication_key,
				  const QByteArray &encryption_key)
{
  m_authentication_key = authentication_key;
  m_encryption_key = encryption_key;
}
