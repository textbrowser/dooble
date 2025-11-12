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

#ifdef DOOBLE_MMAN_PRESENT
extern "C"
{
#include <sys/mman.h>
}
#endif

#include "dooble_cryptography.h"
#include "dooble_random.h"
#include "dooble_ui_utilities.h"
#include "dooble_xchacha20.h"

dooble_xchacha20::dooble_xchacha20(const QByteArray &key)
{
  m_key = key;
  m_key_length = 32; // Or, 256 bits.

  if(m_key.length() < m_key_length)
    m_key.append(m_key_length - m_key.length(), 0);
  else
    m_key.resize(m_key_length);

#ifdef DOOBLE_MMAN_PRESENT
  mlock(m_key.constData(), static_cast<size_t> (m_key.length()));
#endif
}

dooble_xchacha20::~dooble_xchacha20()
{
  dooble_cryptography::memzero(m_key);
#ifdef DOOBLE_MMAN_PRESENT
  munlock(m_key.constData(), static_cast<size_t> (m_key.length()));
#endif
}

QByteArray dooble_xchacha20::decrypt(const QByteArray &data)
{
  Q_UNUSED(data);

  QByteArray decrypted;

  return decrypted;
}

QByteArray dooble_xchacha20::encrypt(const QByteArray &data)
{
  Q_UNUSED(data);

  QByteArray encrypted;

  return encrypted;
}

void dooble_xchacha20::set_key(const QByteArray &key)
{
#ifdef DOOBLE_MMAN_PRESENT
  munlock(m_key.constData(), static_cast<size_t> (m_key.length()));
#endif
  dooble_cryptography::memzero(m_key);
  m_key = key;

  if(m_key.length() < m_key_length)
    m_key.append(m_key_length - m_key.length(), 0);
  else
    m_key.resize(m_key_length);

#ifdef DOOBLE_MMAN_PRESENT
  mlock(m_key.constData(), static_cast<size_t> (m_key.length()));
#endif
}
