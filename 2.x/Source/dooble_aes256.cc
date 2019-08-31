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

/*
** Implementation of https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf.
*/

#include <QDataStream>
#include <QtMath>
#include <iostream>

#ifndef Q_OS_WIN
extern "C"
{
  #include <sys/mman.h>
}
#endif

#include "dooble_aes256.h"
#include "dooble_cryptography.h"
#include "dooble_random.h"

/*
** https://en.wikipedia.org/wiki/Rijndael_S-box
*/

static uint8_t s_rcon[][11] = {{0x00, 0x00, 0x00, 0x00},
			       {0x01, 0x00, 0x00, 0x00},
			       {0x02, 0x00, 0x00, 0x00},
			       {0x04, 0x00, 0x00, 0x00},
			       {0x08, 0x00, 0x00, 0x00},
			       {0x10, 0x00, 0x00, 0x00},
			       {0x20, 0x00, 0x00, 0x00},
			       {0x40, 0x00, 0x00, 0x00},
			       {0x80, 0x00, 0x00, 0x00},
			       {0x1b, 0x00, 0x00, 0x00},
			       {0x36, 0x00, 0x00, 0x00}};

static uint8_t s_inv_sbox[256] =
{
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
  0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44,
  0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b,
  0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
  0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc,
  0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57,
  0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
  0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03,
  0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce,
  0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
  0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e,
  0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe,
  0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
  0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f,
  0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c,
  0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
  0x55, 0x21, 0x0c, 0x7d
};

static uint8_t s_sbox[256] =
{
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
  0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf,
  0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1,
  0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
  0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3,
  0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39,
  0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
  0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21,
  0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d,
  0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
  0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62,
  0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea,
  0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
  0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9,
  0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9,
  0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
  0xb0, 0x54, 0xbb, 0x16
};

dooble_aes256::dooble_aes256(const QByteArray &key):dooble_block_cipher(key)
{
  m_Nb = 4;
  m_Nk = 8;
  m_Nr = 14;
  m_block_length = 16; // Or, 128 bits.
  m_key_length = 32; // Or, 256 bits.

  if(m_key.length() < m_key_length)
    m_key.append(m_key_length - m_key.length(), 0);
  else
    m_key.resize(m_key_length);

#ifndef Q_OS_WIN
  mlock(m_key.constData(), static_cast<size_t> (m_key.length()));
  mlock(m_round_key, 4 * 60 * sizeof(m_round_key[0][0]));
  mlock(m_state, 4 * 4 * sizeof(m_state[0][0]));
#endif
  m_state[0][0] = m_state[0][1] = m_state[0][2] = m_state[0][3] = 0;
  m_state[1][0] = m_state[1][1] = m_state[1][2] = m_state[1][3] = 0;
  m_state[2][0] = m_state[2][1] = m_state[2][2] = m_state[2][3] = 0;
  m_state[3][0] = m_state[3][1] = m_state[3][2] = m_state[3][3] = 0;
  memset(m_round_key, 0, 4 * 60 * sizeof(m_round_key[0][0]));
  key_expansion();
}

dooble_aes256::~dooble_aes256()
{
  dooble_cryptography::memzero(m_key);
  memset(m_round_key, 0, 4 * 60 * sizeof(m_round_key[0][0]));
  memset(m_state, 0, 4 * 4 * sizeof(m_state[0][0]));
#ifndef Q_OS_WIN
  munlock(m_key.constData(), static_cast<size_t> (m_key.length()));
  munlock(m_round_key, 4 * 60 * sizeof(m_round_key[0][0]));
  munlock(m_state, 4 * 4 * sizeof(m_state[0][0]));
#endif
}

QByteArray dooble_aes256::decrypt(const QByteArray &data)
{
  /*
  ** CBC
  */

  QByteArray iv(data.mid(0, m_block_length));

  if(Q_UNLIKELY(iv.length() != m_block_length))
    return QByteArray();

  QByteArray block(m_block_length, 0);
  QByteArray c;
  QByteArray ciphertext(data.mid(iv.length()));
  QByteArray decrypted;
  int iterations = ciphertext.length() / m_block_length;

  for(int i = 0; i < iterations; i++)
    {
      int position = i * m_block_length;

      block = decrypt_block(ciphertext.mid(position, m_block_length));

      if(Q_UNLIKELY(block.isEmpty()))
	{
	  decrypted.clear();
	  break;
	}

      if(i == 0)
	block = xor_arrays(block, iv);
      else
	block = xor_arrays(block, c);

      c = ciphertext.mid(position, m_block_length);
      decrypted.append(block);
    }

  if(decrypted.isEmpty())
    return decrypted;

  QByteArray originalLength;

  if(decrypted.length() > static_cast<int> (sizeof(int)))
    originalLength = decrypted.mid
      (decrypted.length() - static_cast<int> (sizeof(int)),
       static_cast<int> (sizeof(int)));

  if(!originalLength.isEmpty())
    {
      QDataStream in(&originalLength, QIODevice::ReadOnly);
      int s = 0;

      in >> s;

      if(in.status() != QDataStream::Ok)
	decrypted.clear();
      else
	{
	  if(s >= 0 && s <= decrypted.length())
	    decrypted = decrypted.mid(0, s);
	  else
	    decrypted.clear();
	}
    }
  else
    decrypted.clear();

  return decrypted;
}

QByteArray dooble_aes256::decrypt_block(const QByteArray &block)
{
  QByteArray b(block);

  if(b.length() < 16)
    b.append(16 - b.length(), static_cast<char> (16 - b.length()));
  else
    b.resize(16);

  m_state[0][0] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 0)));
  m_state[0][1] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 1)));
  m_state[0][2] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 2)));
  m_state[0][3] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 3)));
  m_state[1][0] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 0)));
  m_state[1][1] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 1)));
  m_state[1][2] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 2)));
  m_state[1][3] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 3)));
  m_state[2][0] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 0)));
  m_state[2][1] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 1)));
  m_state[2][2] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 2)));
  m_state[2][3] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 3)));
  m_state[3][0] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 0)));
  m_state[3][1] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 1)));
  m_state[3][2] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 2)));
  m_state[3][3] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 3)));
  add_round_key(m_Nr);

  for(size_t i = m_Nr - 1; i > 0; i--)
    {
      inv_shift_rows();
      inv_sub_bytes();
      add_round_key(i);
      inv_mix_columns();
    }

  inv_shift_rows();
  inv_sub_bytes();
  add_round_key(0);
  b[static_cast<int> (0 + 4 * 0)] = static_cast<char> (m_state[0][0]);
  b[static_cast<int> (0 + 4 * 1)] = static_cast<char> (m_state[0][1]);
  b[static_cast<int> (0 + 4 * 2)] = static_cast<char> (m_state[0][2]);
  b[static_cast<int> (0 + 4 * 3)] = static_cast<char> (m_state[0][3]);
  b[static_cast<int> (1 + 4 * 0)] = static_cast<char> (m_state[1][0]);
  b[static_cast<int> (1 + 4 * 1)] = static_cast<char> (m_state[1][1]);
  b[static_cast<int> (1 + 4 * 2)] = static_cast<char> (m_state[1][2]);
  b[static_cast<int> (1 + 4 * 3)] = static_cast<char> (m_state[1][3]);
  b[static_cast<int> (2 + 4 * 0)] = static_cast<char> (m_state[2][0]);
  b[static_cast<int> (2 + 4 * 1)] = static_cast<char> (m_state[2][1]);
  b[static_cast<int> (2 + 4 * 2)] = static_cast<char> (m_state[2][2]);
  b[static_cast<int> (2 + 4 * 3)] = static_cast<char> (m_state[2][3]);
  b[static_cast<int> (3 + 4 * 0)] = static_cast<char> (m_state[3][0]);
  b[static_cast<int> (3 + 4 * 1)] = static_cast<char> (m_state[3][1]);
  b[static_cast<int> (3 + 4 * 2)] = static_cast<char> (m_state[3][2]);
  b[static_cast<int> (3 + 4 * 3)] = static_cast<char> (m_state[3][3]);
  return b;
}

QByteArray dooble_aes256::encrypt(const QByteArray &data)
{
  /*
  ** CBC
  */

  QByteArray iv(dooble_random::random_bytes(m_block_length));

  if(Q_UNLIKELY(iv.isEmpty()))
    return QByteArray();

  /*
  ** Resize the container to the block size.
  */

  QByteArray block(iv.length(), 0);
  QByteArray encrypted;
  QByteArray plaintext(data);

  if(plaintext.isEmpty())
    plaintext = plaintext.leftJustified(m_block_length, 0);
  else
    plaintext = plaintext.leftJustified
      (m_block_length *
       static_cast<int> (qCeil(static_cast<qreal> (plaintext.length()) /
			       static_cast<qreal> (m_block_length)) + 1), 0);

  QByteArray originalLength;
  QDataStream out(&originalLength, QIODevice::WriteOnly);

  out << data.length();

  if(out.status() != QDataStream::Ok)
    return QByteArray();

  plaintext.replace
    (plaintext.length() - static_cast<int> (sizeof(int)),
     static_cast<int> (sizeof(int)), originalLength);

  int iterations = plaintext.length() / m_block_length;

  for(int i = 0; i < iterations; i++)
    {
      QByteArray p;
      int position = i * m_block_length;

      p = plaintext.mid(position, m_block_length);

      if(i == 0)
	block = xor_arrays(iv, p);
      else
	block = xor_arrays(block, p);

      block = encrypt_block(block);

      if(Q_UNLIKELY(block.isEmpty()))
	{
	  encrypted.clear();
	  break;
	}

      encrypted.append(block);
    }

  if(encrypted.isEmpty())
    return encrypted;

  return iv + encrypted;
}

QByteArray dooble_aes256::encrypt_block(const QByteArray &block)
{
  QByteArray b(block);

  if(b.length() < 16)
    b.append(16 - b.length(), static_cast<char> (16 - b.length()));
  else
    b.resize(16);

  m_state[0][0] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 0)));
  m_state[0][1] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 1)));
  m_state[0][2] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 2)));
  m_state[0][3] = static_cast<uint8_t> (b.at(static_cast<int> (0 + 4 * 3)));
  m_state[1][0] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 0)));
  m_state[1][1] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 1)));
  m_state[1][2] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 2)));
  m_state[1][3] = static_cast<uint8_t> (b.at(static_cast<int> (1 + 4 * 3)));
  m_state[2][0] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 0)));
  m_state[2][1] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 1)));
  m_state[2][2] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 2)));
  m_state[2][3] = static_cast<uint8_t> (b.at(static_cast<int> (2 + 4 * 3)));
  m_state[3][0] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 0)));
  m_state[3][1] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 1)));
  m_state[3][2] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 2)));
  m_state[3][3] = static_cast<uint8_t> (b.at(static_cast<int> (3 + 4 * 3)));
  add_round_key(0);

  for(size_t i = 1; i < m_Nr; i++)
    {
      sub_bytes();
      shift_rows();
      mix_columns();
      add_round_key(i);
    }

  sub_bytes();
  shift_rows();
  add_round_key(m_Nr);
  b[static_cast<int> (0 + 4 * 0)] = static_cast<char> (m_state[0][0]);
  b[static_cast<int> (0 + 4 * 1)] = static_cast<char> (m_state[0][1]);
  b[static_cast<int> (0 + 4 * 2)] = static_cast<char> (m_state[0][2]);
  b[static_cast<int> (0 + 4 * 3)] = static_cast<char> (m_state[0][3]);
  b[static_cast<int> (1 + 4 * 0)] = static_cast<char> (m_state[1][0]);
  b[static_cast<int> (1 + 4 * 1)] = static_cast<char> (m_state[1][1]);
  b[static_cast<int> (1 + 4 * 2)] = static_cast<char> (m_state[1][2]);
  b[static_cast<int> (1 + 4 * 3)] = static_cast<char> (m_state[1][3]);
  b[static_cast<int> (2 + 4 * 0)] = static_cast<char> (m_state[2][0]);
  b[static_cast<int> (2 + 4 * 1)] = static_cast<char> (m_state[2][1]);
  b[static_cast<int> (2 + 4 * 2)] = static_cast<char> (m_state[2][2]);
  b[static_cast<int> (2 + 4 * 3)] = static_cast<char> (m_state[2][3]);
  b[static_cast<int> (3 + 4 * 0)] = static_cast<char> (m_state[3][0]);
  b[static_cast<int> (3 + 4 * 1)] = static_cast<char> (m_state[3][1]);
  b[static_cast<int> (3 + 4 * 2)] = static_cast<char> (m_state[3][2]);
  b[static_cast<int> (3 + 4 * 3)] = static_cast<char> (m_state[3][3]);
  return b;
}

uint8_t dooble_aes256::xtime(uint8_t x)
{
  return static_cast<uint8_t> ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

uint8_t dooble_aes256::xtime_special(uint8_t x, uint8_t y)
{
  return ((x & 1) * y) ^
    (((x >> 1) & 1) * xtime(y)) ^
    (((x >> 2) & 1) * xtime(xtime(y))) ^
    (((x >> 3) & 1) * xtime(xtime(xtime(y)))) ^
    (((x >> 4) & 1) * xtime(xtime(xtime(xtime(y)))));
}

void dooble_aes256::add_round_key(size_t c)
{
  m_state[0][0] ^= m_round_key[c * m_Nb + 0][0];
  m_state[0][1] ^= m_round_key[c * m_Nb + 1][0];
  m_state[0][2] ^= m_round_key[c * m_Nb + 2][0];
  m_state[0][3] ^= m_round_key[c * m_Nb + 3][0];
  m_state[1][0] ^= m_round_key[c * m_Nb + 0][1];
  m_state[1][1] ^= m_round_key[c * m_Nb + 1][1];
  m_state[1][2] ^= m_round_key[c * m_Nb + 2][1];
  m_state[1][3] ^= m_round_key[c * m_Nb + 3][1];
  m_state[2][0] ^= m_round_key[c * m_Nb + 0][2];
  m_state[2][1] ^= m_round_key[c * m_Nb + 1][2];
  m_state[2][2] ^= m_round_key[c * m_Nb + 2][2];
  m_state[2][3] ^= m_round_key[c * m_Nb + 3][2];
  m_state[3][0] ^= m_round_key[c * m_Nb + 0][3];
  m_state[3][1] ^= m_round_key[c * m_Nb + 1][3];
  m_state[3][2] ^= m_round_key[c * m_Nb + 2][3];
  m_state[3][3] ^= m_round_key[c * m_Nb + 3][3];
}

void dooble_aes256::inv_mix_columns(void)
{
  uint8_t a[4];

  a[0] = m_state[0][0];
  a[1] = m_state[1][0];
  a[2] = m_state[2][0];
  a[3] = m_state[3][0];
  m_state[0][0] = xtime_special(0x0e, a[0]) ^ xtime_special(0x0b, a[1]) ^
    xtime_special(0x0d, a[2]) ^ xtime_special(0x09, a[3]);
  m_state[1][0] = xtime_special(0x09, a[0]) ^ xtime_special(0x0e, a[1]) ^
    xtime_special(0x0b, a[2]) ^ xtime_special(0x0d, a[3]);
  m_state[2][0] = xtime_special(0x0d, a[0]) ^ xtime_special(0x09, a[1]) ^
    xtime_special(0x0e, a[2]) ^ xtime_special(0x0b, a[3]);
  m_state[3][0] = xtime_special(0x0b, a[0]) ^ xtime_special(0x0d, a[1]) ^
    xtime_special(0x09, a[2]) ^ xtime_special(0x0e, a[3]);
  a[0] = m_state[0][1];
  a[1] = m_state[1][1];
  a[2] = m_state[2][1];
  a[3] = m_state[3][1];
  m_state[0][1] = xtime_special(0x0e, a[0]) ^ xtime_special(0x0b, a[1]) ^
    xtime_special(0x0d, a[2]) ^ xtime_special(0x09, a[3]);
  m_state[1][1] = xtime_special(0x09, a[0]) ^ xtime_special(0x0e, a[1]) ^
    xtime_special(0x0b, a[2]) ^ xtime_special(0x0d, a[3]);
  m_state[2][1] = xtime_special(0x0d, a[0]) ^ xtime_special(0x09, a[1]) ^
    xtime_special(0x0e, a[2]) ^ xtime_special(0x0b, a[3]);
  m_state[3][1] = xtime_special(0x0b, a[0]) ^ xtime_special(0x0d, a[1]) ^
    xtime_special(0x09, a[2]) ^ xtime_special(0x0e, a[3]);
  a[0] = m_state[0][2];
  a[1] = m_state[1][2];
  a[2] = m_state[2][2];
  a[3] = m_state[3][2];
  m_state[0][2] = xtime_special(0x0e, a[0]) ^ xtime_special(0x0b, a[1]) ^
    xtime_special(0x0d, a[2]) ^ xtime_special(0x09, a[3]);
  m_state[1][2] = xtime_special(0x09, a[0]) ^ xtime_special(0x0e, a[1]) ^
    xtime_special(0x0b, a[2]) ^ xtime_special(0x0d, a[3]);
  m_state[2][2] = xtime_special(0x0d, a[0]) ^ xtime_special(0x09, a[1]) ^
    xtime_special(0x0e, a[2]) ^ xtime_special(0x0b, a[3]);
  m_state[3][2] = xtime_special(0x0b, a[0]) ^ xtime_special(0x0d, a[1]) ^
    xtime_special(0x09, a[2]) ^ xtime_special(0x0e, a[3]);
  a[0] = m_state[0][3];
  a[1] = m_state[1][3];
  a[2] = m_state[2][3];
  a[3] = m_state[3][3];
  m_state[0][3] = xtime_special(0x0e, a[0]) ^ xtime_special(0x0b, a[1]) ^
    xtime_special(0x0d, a[2]) ^ xtime_special(0x09, a[3]);
  m_state[1][3] = xtime_special(0x09, a[0]) ^ xtime_special(0x0e, a[1]) ^
    xtime_special(0x0b, a[2]) ^ xtime_special(0x0d, a[3]);
  m_state[2][3] = xtime_special(0x0d, a[0]) ^ xtime_special(0x09, a[1]) ^
    xtime_special(0x0e, a[2]) ^ xtime_special(0x0b, a[3]);
  m_state[3][3] = xtime_special(0x0b, a[0]) ^ xtime_special(0x0d, a[1]) ^
    xtime_special(0x09, a[2]) ^ xtime_special(0x0e, a[3]);
  memset(a, 0, 4 * sizeof(a[0]));
}

void dooble_aes256::inv_shift_rows(void)
{
  uint8_t temp[4];

  temp[0] = m_state[1][0];
  temp[1] = m_state[1][1];
  temp[2] = m_state[1][2];
  temp[3] = m_state[1][3];
  m_state[1][(1 + 0) % m_Nb] = temp[0];
  m_state[1][(1 + 1) % m_Nb] = temp[1];
  m_state[1][(1 + 2) % m_Nb] = temp[2];
  m_state[1][(1 + 3) % m_Nb] = temp[3];
  temp[0] = m_state[2][0];
  temp[1] = m_state[2][1];
  temp[2] = m_state[2][2];
  temp[3] = m_state[2][3];
  m_state[2][(2 + 0) % m_Nb] = temp[0];
  m_state[2][(2 + 1) % m_Nb] = temp[1];
  m_state[2][(2 + 2) % m_Nb] = temp[2];
  m_state[2][(2 + 3) % m_Nb] = temp[3];
  temp[0] = m_state[3][0];
  temp[1] = m_state[3][1];
  temp[2] = m_state[3][2];
  temp[3] = m_state[3][3];
  m_state[3][(3 + 0) % m_Nb] = temp[0];
  m_state[3][(3 + 1) % m_Nb] = temp[1];
  m_state[3][(3 + 2) % m_Nb] = temp[2];
  m_state[3][(3 + 3) % m_Nb] = temp[3];
  memset(temp, 0, 4 * sizeof(temp[0]));
}

void dooble_aes256::inv_sub_bytes(void)
{
  m_state[0][0] = s_inv_sbox[static_cast<size_t> (m_state[0][0])];
  m_state[0][1] = s_inv_sbox[static_cast<size_t> (m_state[0][1])];
  m_state[0][2] = s_inv_sbox[static_cast<size_t> (m_state[0][2])];
  m_state[0][3] = s_inv_sbox[static_cast<size_t> (m_state[0][3])];
  m_state[1][0] = s_inv_sbox[static_cast<size_t> (m_state[1][0])];
  m_state[1][1] = s_inv_sbox[static_cast<size_t> (m_state[1][1])];
  m_state[1][2] = s_inv_sbox[static_cast<size_t> (m_state[1][2])];
  m_state[1][3] = s_inv_sbox[static_cast<size_t> (m_state[1][3])];
  m_state[2][0] = s_inv_sbox[static_cast<size_t> (m_state[2][0])];
  m_state[2][1] = s_inv_sbox[static_cast<size_t> (m_state[2][1])];
  m_state[2][2] = s_inv_sbox[static_cast<size_t> (m_state[2][2])];
  m_state[2][3] = s_inv_sbox[static_cast<size_t> (m_state[2][3])];
  m_state[3][0] = s_inv_sbox[static_cast<size_t> (m_state[3][0])];
  m_state[3][1] = s_inv_sbox[static_cast<size_t> (m_state[3][1])];
  m_state[3][2] = s_inv_sbox[static_cast<size_t> (m_state[3][2])];
  m_state[3][3] = s_inv_sbox[static_cast<size_t> (m_state[3][3])];
}

void dooble_aes256::key_expansion(void)
{
  size_t i = 0;

  while(i < m_Nk)
    {
      m_round_key[i][0] = static_cast<uint8_t>
	(m_key[static_cast<int> (4 * i + 0)]);
      m_round_key[i][1] = static_cast<uint8_t>
	(m_key[static_cast<int> (4 * i + 1)]);
      m_round_key[i][2] = static_cast<uint8_t>
	(m_key[static_cast<int> (4 * i + 2)]);
      m_round_key[i][3] = static_cast<uint8_t>
	(m_key[static_cast<int> (4 * i + 3)]);
      i += 1;
    }

  uint8_t temp[4];

  while(i < m_Nb * (m_Nr + 1))
    {
      temp[0] = m_round_key[i - 1][0];
      temp[1] = m_round_key[i - 1][1];
      temp[2] = m_round_key[i - 1][2];
      temp[3] = m_round_key[i - 1][3];

      if(m_Nk > 0 && i % m_Nk == 0)
	{
	  uint8_t t = temp[0];

	  temp[0] = temp[1];
	  temp[1] = temp[2];
	  temp[2] = temp[3];
	  temp[3] = t;
	  temp[0] = s_sbox[static_cast<size_t> (temp[0])] ^ s_rcon[i / m_Nk][0];
	  temp[1] = s_sbox[static_cast<size_t> (temp[1])] ^ s_rcon[i / m_Nk][1];
	  temp[2] = s_sbox[static_cast<size_t> (temp[2])] ^ s_rcon[i / m_Nk][2];
	  temp[3] = s_sbox[static_cast<size_t> (temp[3])] ^ s_rcon[i / m_Nk][3];
	  t = 0;
	}
      else if(m_Nk > 0 && i % m_Nk == 4)
	{
	  temp[0] = s_sbox[static_cast<size_t> (temp[0])];
	  temp[1] = s_sbox[static_cast<size_t> (temp[1])];
	  temp[2] = s_sbox[static_cast<size_t> (temp[2])];
	  temp[3] = s_sbox[static_cast<size_t> (temp[3])];
	}

      m_round_key[i][0] = m_round_key[i - m_Nk][0] ^ temp[0];
      m_round_key[i][1] = m_round_key[i - m_Nk][1] ^ temp[1];
      m_round_key[i][2] = m_round_key[i - m_Nk][2] ^ temp[2];
      m_round_key[i][3] = m_round_key[i - m_Nk][3] ^ temp[3];
      memset(temp, 0, 4 * sizeof(temp[0]));
      i += 1;
    }
}

void dooble_aes256::mix_columns(void)
{
  uint8_t a[4];
  uint8_t b[4];

  a[0] = m_state[0][0];
  a[1] = m_state[1][0];
  a[2] = m_state[2][0];
  a[3] = m_state[3][0];
  b[0] = xtime(m_state[0][0]);
  b[1] = xtime(m_state[1][0]);
  b[2] = xtime(m_state[2][0]);
  b[3] = xtime(m_state[3][0]);
  m_state[0][0] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
  m_state[1][0] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
  m_state[2][0] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
  m_state[3][0] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  a[0] = m_state[0][1];
  a[1] = m_state[1][1];
  a[2] = m_state[2][1];
  a[3] = m_state[3][1];
  b[0] = xtime(m_state[0][1]);
  b[1] = xtime(m_state[1][1]);
  b[2] = xtime(m_state[2][1]);
  b[3] = xtime(m_state[3][1]);
  m_state[0][1] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
  m_state[1][1] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
  m_state[2][1] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
  m_state[3][1] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  a[0] = m_state[0][2];
  a[1] = m_state[1][2];
  a[2] = m_state[2][2];
  a[3] = m_state[3][2];
  b[0] = xtime(m_state[0][2]);
  b[1] = xtime(m_state[1][2]);
  b[2] = xtime(m_state[2][2]);
  b[3] = xtime(m_state[3][2]);
  m_state[0][2] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
  m_state[1][2] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
  m_state[2][2] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
  m_state[3][2] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  a[0] = m_state[0][3];
  a[1] = m_state[1][3];
  a[2] = m_state[2][3];
  a[3] = m_state[3][3];
  b[0] = xtime(m_state[0][3]);
  b[1] = xtime(m_state[1][3]);
  b[2] = xtime(m_state[2][3]);
  b[3] = xtime(m_state[3][3]);
  m_state[0][3] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
  m_state[1][3] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
  m_state[2][3] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
  m_state[3][3] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  memset(a, 0, 4 * sizeof(a[0]));
  memset(b, 0, 4 * sizeof(b[0]));
}

void dooble_aes256::set_key(const QByteArray &key)
{
#ifndef Q_OS_WIN
  munlock(m_key.constData(), static_cast<size_t> (m_key.length()));
  munlock(m_round_key, 4 * 60 * sizeof(m_round_key[0][0]));
  munlock(m_state, 4 * 4 * sizeof(m_state[0][0]));
#endif
  m_key = key;

  if(m_key.length() < m_key_length)
    m_key.append(m_key_length - m_key.length(), 0);
  else
    m_key.resize(m_key_length);

#ifndef Q_OS_WIN
  mlock(m_key.constData(), static_cast<size_t> (m_key.length()));
  mlock(m_round_key, 4 * 60 * sizeof(m_round_key[0][0]));
  mlock(m_state, 4 * 4 * sizeof(m_state[0][0]));
#endif
  m_state[0][0] = m_state[0][1] = m_state[0][2] = m_state[0][3] = 0;
  m_state[1][0] = m_state[1][1] = m_state[1][2] = m_state[1][3] = 0;
  m_state[2][0] = m_state[2][1] = m_state[2][2] = m_state[2][3] = 0;
  m_state[3][0] = m_state[3][1] = m_state[3][2] = m_state[3][3] = 0;
  memset(m_round_key, 0, 4 * 60 * sizeof(m_round_key[0][0]));
  key_expansion();
}

void dooble_aes256::shift_rows(void)
{
  uint8_t temp[4];

  temp[0] = m_state[1][(1 + 0) % m_Nb];
  temp[1] = m_state[1][(1 + 1) % m_Nb];
  temp[2] = m_state[1][(1 + 2) % m_Nb];
  temp[3] = m_state[1][(1 + 3) % m_Nb];
  m_state[1][0] = temp[0];
  m_state[1][1] = temp[1];
  m_state[1][2] = temp[2];
  m_state[1][3] = temp[3];
  temp[0] = m_state[2][(2 + 0) % m_Nb];
  temp[1] = m_state[2][(2 + 1) % m_Nb];
  temp[2] = m_state[2][(2 + 2) % m_Nb];
  temp[3] = m_state[2][(2 + 3) % m_Nb];
  m_state[2][0] = temp[0];
  m_state[2][1] = temp[1];
  m_state[2][2] = temp[2];
  m_state[2][3] = temp[3];
  temp[0] = m_state[3][(3 + 0) % m_Nb];
  temp[1] = m_state[3][(3 + 1) % m_Nb];
  temp[2] = m_state[3][(3 + 2) % m_Nb];
  temp[3] = m_state[3][(3 + 3) % m_Nb];
  m_state[3][0] = temp[0];
  m_state[3][1] = temp[1];
  m_state[3][2] = temp[2];
  m_state[3][3] = temp[3];
  memset(temp, 0, 4 * sizeof(temp[0]));
}

void dooble_aes256::sub_bytes()
{
  m_state[0][0] = s_sbox[static_cast<size_t> (m_state[0][0])];
  m_state[0][1] = s_sbox[static_cast<size_t> (m_state[0][1])];
  m_state[0][2] = s_sbox[static_cast<size_t> (m_state[0][2])];
  m_state[0][3] = s_sbox[static_cast<size_t> (m_state[0][3])];
  m_state[1][0] = s_sbox[static_cast<size_t> (m_state[1][0])];
  m_state[1][1] = s_sbox[static_cast<size_t> (m_state[1][1])];
  m_state[1][2] = s_sbox[static_cast<size_t> (m_state[1][2])];
  m_state[1][3] = s_sbox[static_cast<size_t> (m_state[1][3])];
  m_state[2][0] = s_sbox[static_cast<size_t> (m_state[2][0])];
  m_state[2][1] = s_sbox[static_cast<size_t> (m_state[2][1])];
  m_state[2][2] = s_sbox[static_cast<size_t> (m_state[2][2])];
  m_state[2][3] = s_sbox[static_cast<size_t> (m_state[2][3])];
  m_state[3][0] = s_sbox[static_cast<size_t> (m_state[3][0])];
  m_state[3][1] = s_sbox[static_cast<size_t> (m_state[3][1])];
  m_state[3][2] = s_sbox[static_cast<size_t> (m_state[3][2])];
  m_state[3][3] = s_sbox[static_cast<size_t> (m_state[3][3])];
}

void dooble_aes256::test1(void)
{
  QByteArray text;
  dooble_aes256 aes256(dooble_random::random_bytes(32));

  text = "for(size_t i = 0; i < 4; i++) "
    "{"
    "m_state[i][0] = s_sbox[static_cast<size_t> (m_state[i][0])];"
    "m_state[i][1] = s_sbox[static_cast<size_t> (m_state[i][1])];"
    "m_state[i][2] = s_sbox[static_cast<size_t> (m_state[i][2])];"
    "m_state[i][3] = s_sbox[static_cast<size_t> (m_state[i][3])];"
    "}";
  std::cout << ((text == aes256.decrypt(aes256.encrypt(text))) ?
		"dooble_aes256::test1() passed!" :
		"dooble_aes256::test1() failed!")
	    << std::endl;
}

void dooble_aes256::test1_decrypt_block(void)
{
  QByteArray key
    (QByteArray::fromHex("000102030405060708090a0b0c0d0e0f"
			 "101112131415161718191a1b1c1d1e1f"));
  dooble_aes256 aes256(key);

  std::cout << aes256.decrypt_block
    (QByteArray::fromHex("8ea2b7ca516745bfeafc49904b496089")).toHex().
    toStdString() << std::endl;
}

void dooble_aes256::test1_encrypt_block(void)
{
  QByteArray key
    (QByteArray::fromHex("000102030405060708090a0b0c0d0e0f"
			 "101112131415161718191a1b1c1d1e1f"));
  dooble_aes256 aes256(key);

  std::cout << aes256.encrypt_block
    (QByteArray::fromHex("00112233445566778899aabbccddeeff")).toHex().
    toStdString() << std::endl;
}

void dooble_aes256::test1_key_expansion(void)
{
  /*
  ** Section A.3 of the standard.
  */

  QByteArray key
    (QByteArray::fromHex("603deb1015ca71be2b73aef0857d7781"
			 "1f352c073b6108d72d9810a30914dff4"));
  dooble_aes256 aes256(key);

  for(size_t i = 0; i < 60; i++)
    if((i >= 8 && i <= 15) ||
       (i >= 52 && i <= 59))
      {
	std::cout << "i = " << i << " ";

	for(size_t j = 0; j < 4; j++)
	  std::cout << QByteArray
	    (1, static_cast<char> (aes256.m_round_key[i][j])).toHex().
	    toStdString();

	std::cout << std::endl;
      }
}
