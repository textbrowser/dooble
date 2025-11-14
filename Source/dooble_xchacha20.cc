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

#include <QtDebug>

#include "dooble_block_cipher.h"
#include "dooble_cryptography.h"
#include "dooble_random.h"
#include "dooble_xchacha20.h"

/*
** Read https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-xchacha and
** https://www.rfc-editor.org/rfc/rfc8439.
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
#ifdef DOOBLE_MMAN_PRESENT
  munlock(m_key.constData(), static_cast<size_t> (m_key.length()));
#endif
}

QByteArray dooble_xchacha20::chacha20_block
(const QByteArray &key, const QByteArray &nonce, const uint32_t counter)
{
  /*
  ** ChaCha20 Block
  */

  QVector<uint32_t> initial_state(16);
  QVector<uint32_t> state(16);

  state[0] = 0x61707865;
  state[1] = 0x3320646e;
  state[2] = 0x79622d32;
  state[3] = 0x6b206574;
  state[4] = extract_4_bytes(key, 0);
  state[5] = extract_4_bytes(key, 4);
  state[6] = extract_4_bytes(key, 8);
  state[7] = extract_4_bytes(key, 12);
  state[8] = extract_4_bytes(key, 16);
  state[9] = extract_4_bytes(key, 20);
  state[10] = extract_4_bytes(key, 24);
  state[11] = extract_4_bytes(key, 28);
  state[12] = counter;
  state[13] = extract_4_bytes(nonce, 0);
  state[14] = extract_4_bytes(nonce, 4);
  state[15] = extract_4_bytes(nonce, 8);

  for(int i = 0; i < initial_state.size(); i++)
    initial_state[i] = state[i];

  for(int i = 0; i < 10; i++)
    {
      quarter_round(state[0], state[4], state[8], state[12]);
      quarter_round(state[1], state[5], state[9], state[13]);
      quarter_round(state[2], state[6], state[10], state[14]);
      quarter_round(state[3], state[7], state[11], state[15]);
      quarter_round(state[0], state[5], state[10], state[15]);
      quarter_round(state[1], state[6], state[11], state[12]);
      quarter_round(state[2], state[7], state[8], state[13]);
      quarter_round(state[3], state[4], state[9], state[14]);
    }

  for(int i = 0; i < initial_state.size(); i++)
    state[i] += initial_state[i];

  dooble_cryptography::memzero(initial_state);

  QByteArray data(64, '0');

  for(int i = 0; i < state.size(); i++)
    infuse_4_bytes(data, i * 4, state[i]);

  dooble_cryptography::memzero(state);
  return data;
}

QByteArray dooble_xchacha20::chacha20_encrypt(const QByteArray &key,
					      const QByteArray &nonce,
					      const QByteArray &plaintext,
					      const uint32_t counter)
{
  QByteArray encrypted;

  for(int i = 0; i <= qFloor(plaintext.length() / 64.0) - 1; i++)
    {
      auto const block(plaintext.mid(i * 64, 64));
      auto const stream(chacha20_block(key, nonce, counter + i));

      encrypted.append(dooble_block_cipher::xor_arrays(block, stream));
    }

  if(plaintext.length() % 64 != 0)
    {
      QByteArray block;
      auto const i = qFloor(plaintext.length() / 64.0);
      auto const stream(chacha20_block(key, nonce, counter + i));

      block = plaintext.mid(i * 64, plaintext.length());
      encrypted.append
	(dooble_block_cipher::
	 xor_arrays(block, stream).mid(0, plaintext.length() % 64));
    }

  return encrypted;
}

QByteArray dooble_xchacha20::encrypt(const QByteArray &data)
{
  Q_UNUSED(data);

  QByteArray decrypted;

  return decrypted;
}

QByteArray dooble_xchacha20::decrypt(const QByteArray &data)
{
  Q_UNUSED(data);

  QByteArray decrypted;

  return decrypted;
}

QByteArray dooble_xchacha20::hchacha20_block
(const QByteArray &key, const QByteArray &nonce)
{
  /*
  ** HChaCha20 Block
  */

  QVector<uint32_t> state(16);

  state[0] = 0x61707865;
  state[1] = 0x3320646e;
  state[2] = 0x79622d32;
  state[3] = 0x6b206574;
  state[4] = extract_4_bytes(key, 0);
  state[5] = extract_4_bytes(key, 4);
  state[6] = extract_4_bytes(key, 8);
  state[7] = extract_4_bytes(key, 12);
  state[8] = extract_4_bytes(key, 16);
  state[9] = extract_4_bytes(key, 20);
  state[10] = extract_4_bytes(key, 24);
  state[11] = extract_4_bytes(key, 28);
  state[12] = extract_4_bytes(nonce, 0);
  state[13] = extract_4_bytes(nonce, 4);
  state[14] = extract_4_bytes(nonce, 8);
  state[15] = extract_4_bytes(nonce, 12);

  for(int i = 0; i < 10; i++)
    {
      quarter_round(state[0], state[4], state[8], state[12]);
      quarter_round(state[1], state[5], state[9], state[13]);
      quarter_round(state[2], state[6], state[10], state[14]);
      quarter_round(state[3], state[7], state[11], state[15]);
      quarter_round(state[0], state[5], state[10], state[15]);
      quarter_round(state[1], state[6], state[11], state[12]);
      quarter_round(state[2], state[7], state[8], state[13]);
      quarter_round(state[3], state[4], state[9], state[14]);
    }

  QByteArray data(32, '0');

  infuse_4_bytes(data, 0, state[0]);
  infuse_4_bytes(data, 4, state[1]);
  infuse_4_bytes(data, 8, state[2]);
  infuse_4_bytes(data, 12, state[3]);
  infuse_4_bytes(data, 16, state[12]);
  infuse_4_bytes(data, 20, state[13]);
  infuse_4_bytes(data, 24, state[14]);
  infuse_4_bytes(data, 28, state[15]);
  dooble_cryptography::memzero(state);
  return data;
}

QByteArray dooble_xchacha20::xchacha20_encrypt
(const QByteArray &key,
 const QByteArray &nonce,
 const QByteArray &plaintext,
 const uint32_t counter)
{
  /*
  ** XChaCha20 Encrypt
  */

  auto const chacha20_nonce(QByteArray::fromHex("00000000") + nonce.mid(16));
  auto const stream(hchacha20_block(key, nonce.mid(0, 16)));

  return chacha20_encrypt(stream, chacha20_nonce, plaintext, counter);
}

uint32_t dooble_xchacha20::extract_4_bytes
(const QByteArray &bytes, const int offset)
{
  if(bytes.length() > offset + 3 && offset >= 0)
    return static_cast<uint32_t> (bytes.at(offset)) |
      static_cast<uint32_t> (bytes.at(offset + 1) << 8) |
      static_cast<uint32_t> (bytes.at(offset + 2) << 16) |
      static_cast<uint32_t> (bytes.at(offset + 3) << 24);
  else
    return static_cast<uint32_t> (0);
}

void dooble_xchacha20::infuse_4_bytes
(QByteArray &bytes, const int offset, const uint32_t value)
{
  if(bytes.length() > offset + 3 && offset >= 0)
    {
      bytes[offset] = value & 0xff;
      bytes[offset + 1] = (value >> 8) & 0xff;
      bytes[offset + 2] = (value >> 16) & 0xff;
      bytes[offset + 3] = (value >> 24) & 0xff;
    }
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
