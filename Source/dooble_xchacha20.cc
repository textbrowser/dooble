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
#include "dooble_xchacha20.h"

/*
** Read https://datatracker.ietf.org/doc/html/draft-arciszewski-xchacha-02.
*/

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
  dooble_cryptography::memzero(m_state);
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

uint32_t dooble_xchacha20::extract_4_bytes
(const QByteArray &bytes, const int offset)
{
  if(bytes.length() > offset + 3)
    return static_cast<uint32_t> (bytes.at(offset)) |
      static_cast<uint32_t> (bytes.at(offset + 1) << 8) |
      static_cast<uint32_t> (bytes.at(offset + 2) << 16) |
      static_cast<uint32_t> (bytes.at(offset + 3) << 24);
  else
    return static_cast<uint32_t> (0);
}

void dooble_xchacha20::initialize
(const QByteArray &key, const QByteArray &nonce, const uint32_t counter)
{
  dooble_cryptography::memzero(m_state);
  m_state.resize(16);
  m_state[0] = 0x61707865;
  m_state[1] = 0x3320646e;
  m_state[2] = 0x79622d32;
  m_state[3] = 0x6b206574;
  m_state[4] = extract_4_bytes(key, 0);
  m_state[5] = extract_4_bytes(key, 4);
  m_state[6] = extract_4_bytes(key, 8);
  m_state[7] = extract_4_bytes(key, 12);
  m_state[8] = extract_4_bytes(key, 16);
  m_state[9] = extract_4_bytes(key, 20);
  m_state[10] = extract_4_bytes(key, 24);
  m_state[11] = extract_4_bytes(key, 28);
  m_state[12] = counter;
  m_state[13] = extract_4_bytes(nonce, 0);
  m_state[14] = extract_4_bytes(nonce, 4);
  m_state[15] = extract_4_bytes(nonce, 8);
}

void dooble_xchacha20::quarter_round
(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d)
{
  a += b;
  d ^= a;
  rotate(d, 16);
  c += d;
  b ^= c;
  rotate(b, 12);
  a += b;
  d ^= a;
  rotate(d, 8);
  c += d;
  b ^= c;
  rotate(b, 7);
}

void dooble_xchacha20::rotate(uint32_t &x, const uint32_t n)
{
  x = (x << n) | (x >> (32 - n));
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
