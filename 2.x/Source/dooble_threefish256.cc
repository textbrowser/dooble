/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
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
**    derived from Spot-On without specific prior written permission.
**
** SPOT-ON IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QDataStream>
#include <QtCore/qmath.h>
#include <QtDebug>

#include "dooble_random.h"
#include "dooble_threefish256.h"

static const uint8_t *Pi = 0;
static const uint8_t *RPi = 0;
static const uint8_t Pi_4[4] = {0, 3, 2, 1};
static const uint8_t RPi_4[4] = {0, 3, 2, 1};
static const uint8_t R_4[8][2] = {{14, 16},
				  {52, 57},
				  {23, 40},
				  {5, 37},
				  {25, 33},
				  {46, 12},
				  {58, 22},
				  {32, 32}};
static size_t Nr = 0;
static size_t Nw = 0;
static void bytes_to_words(uint64_t *W,
			   const char *bytes,
			   const size_t bytes_size);
static void threefish_decrypt(char *D,
			      const char *K,
			      const char *T,
			      const char *C,
			      const size_t C_size,
			      const size_t block_size,
			      bool *ok);
static void threefish_decrypt_implementation(char *D,
					     const char *K,
					     const char *T,
					     const char *C,
					     const size_t C_size,
					     const size_t block_size,
					     bool *ok);
static void threefish_encrypt(char *E,
			      const char *K,
			      const char *T,
			      const char *P,
			      const size_t P_size,
			      const size_t block_size,
			      bool *ok);
static void threefish_encrypt_implementation(char *E,
					     const char *K,
					     const char *T,
					     const char *P,
					     const size_t P_size,
					     const size_t block_size,
					     bool *ok);
static void words_to_bytes(char *B,
			   const uint64_t *words,
			   const size_t words_size);

static void bytes_to_words(uint64_t *W,
			   const char *bytes,
			   const size_t bytes_size)
{
  if(Q_UNLIKELY(!W || !bytes || bytes_size == 0))
    return;

  for(size_t i = 0; i < bytes_size / 8; i++)
    {
      char b[8];

      for(size_t j = 0; j < 8; j++)
	b[j] = bytes[i * 8 + j];

      W[i] = static_cast<uint64_t> (b[0] & 0xff) |
	(static_cast<uint64_t> (b[1] & 0xff) << 8) |
	(static_cast<uint64_t> (b[2] & 0xff) << 16) |
	(static_cast<uint64_t> (b[3] & 0xff) << 24) |
	(static_cast<uint64_t> (b[4] & 0xff) << 32) |
	(static_cast<uint64_t> (b[5] & 0xff) << 40) |
	(static_cast<uint64_t> (b[6] & 0xff) << 48) |
	(static_cast<uint64_t> (b[7] & 0xff) << 56);
    }
}

static void mix(const uint64_t x0,
		const uint64_t x1,
		const size_t d,
		const size_t i,
		uint64_t *y0,
		uint64_t *y1,
		const size_t block_size)
{
  Q_UNUSED(block_size);

  if(Q_UNLIKELY(!y0 || !y1))
    return;

  /*
  ** Section 3.3.1.
  */

  uint64_t r = R_4[d % 8][i];

  *y0 = x0 + x1;

  /*
  ** Please see https://en.wikipedia.org/wiki/Circular_shift.
  */

  *y1 = ((x1 << r) | (x1 >> (64 - r))) ^ *y0;
}

static void mix_inverse(const uint64_t y0,
			const uint64_t y1,
			const size_t d,
			const size_t i,
			uint64_t *x0,
			uint64_t *x1,
			const size_t block_size)
{
  Q_UNUSED(block_size);

  if(Q_UNLIKELY(!x0 || !x1))
    return;

  /*
  ** Section 3.3.1.
  */

  uint64_t r = R_4[d % 8][i];

  /*
  ** Please see https://en.wikipedia.org/wiki/Circular_shift.
  */

  *x1 = ((y1 ^ y0) >> r) | ((y1 ^ y0) << (64 - r));
  *x0 = y0 - *x1;
}

static void threefish_decrypt(char *D,
			      const char *K,
			      const char *T,
			      const char *C,
			      const size_t C_size,
			      const size_t block_size,
			      bool *ok)
{
  if(Q_UNLIKELY(!C || C_size == 0 || !D || !K || !T || block_size == 0))
    {
      if(ok)
	*ok = false;

      return;
    }

  Nr = 72;
  Nw = 4;
  RPi = RPi_4;
  threefish_decrypt_implementation(D, K, T, C, C_size, block_size, ok);
}

static void threefish_decrypt_implementation(char *D,
					     const char *K,
					     const char *T,
					     const char *C,
					     const size_t C_size,
					     const size_t block_size,
					     bool *ok)
{
  if(Q_UNLIKELY(!C || C_size == 0 || !D || !K || !T || block_size == 0))
    {
      if(ok)
	*ok = false;

      return;
    }

  /*
  ** The inverse of section 3.3.
  */

  bool error = false;
  uint64_t C240 = 0x1bd11bdaa9fc1a22;
  uint64_t *k = new (std::nothrow) uint64_t[Nw + 1];
  uint64_t kNw = C240; // Section 3.3.2.
  uint64_t **s = new (std::nothrow) uint64_t*[Nr / 4 + 1];
  uint64_t t[3];
  uint64_t *v = new (std::nothrow) uint64_t[Nw];

  if(Q_UNLIKELY(!k || !s || !v))
    {
      if(ok)
	*ok = false;

      goto done_label;
    }

  for(size_t i = 0; i < Nr / 4 + 1; i++)
    {
      s[i] = new (std::nothrow) uint64_t[Nw];

      if(Q_UNLIKELY(!s[i]))
	error = true; // Do not break.
    }

  if(Q_UNLIKELY(error))
    {
      if(ok)
	*ok = false;

      goto done_label;
    }

  bytes_to_words(k, K, C_size);
  bytes_to_words(t, T, 16);
  bytes_to_words(v, C, C_size);

  for(size_t i = 0; i < Nw; i++)
    kNw ^= k[i]; // Section 3.3.2.

  k[Nw] = kNw;
  t[2] = t[0] ^ t[1]; // Section 3.3.2.

  /*
  ** Prepare the key schedule, section 3.3.2.
  */

  for(size_t d = 0; d < Nr / 4 + 1; d++)
    for(size_t i = 0; i < Nw; i++)
      {
	s[d][i] = k[(d + i) % (Nw + 1)];

	if(i == Nw - 1)
	  s[d][i] += d;
	else if(i == Nw - 2)
	  s[d][i] += t[(d + 1) % 3];
	else if(i == Nw - 3)
	  s[d][i] += t[d % 3];
      }

  for(size_t i = 0; i < Nw; i++)
    v[i] -= s[Nr / 4][i];

  for(size_t d = Nr - 1;; d--)
    {
      uint64_t *f = new (std::nothrow) uint64_t[Nw];

      if(Q_UNLIKELY(!f))
	{
	  if(ok)
	    *ok = false;

	  goto done_label;
	}

      for(size_t i = 0; i < Nw; i++)
	f[i] = v[RPi[i]];

      for(size_t i = 0; i < Nw / 2; i++)
	{
	  uint64_t x0 = 0;
	  uint64_t x1 = 0;
	  uint64_t y0 = f[i * 2];
	  uint64_t y1 = f[i * 2 + 1];

	  mix_inverse(y0, y1, d, i, &x0, &x1, block_size);
	  v[i * 2] = x0;
	  v[i * 2 + 1] = x1;
	}

      memset(f, 0, sizeof(*f) * static_cast<size_t> (Nw));
      delete []f;

      if(d % 4 == 0)
	for(size_t i = 0; i < Nw; i++)
	  v[i] -= s[d / 4][i];

      if(d == 0)
	break;
    }

  words_to_bytes(D, v, Nw);

  if(ok)
    *ok = true;

 done_label:

  if(Q_LIKELY(k))
    memset(k, 0, sizeof(*k) * static_cast<size_t> (Nw + 1));

  memset(t, 0, sizeof(t));

  if(Q_LIKELY(v))
    memset(v, 0, sizeof(*v) * static_cast<size_t> (Nw));

  delete []k;

  for(size_t i = 0; i < Nr / 4 + 1; i++)
    {
      if(Q_LIKELY(s[i]))
	memset(s[i], 0, sizeof(*s[i]) * static_cast<size_t> (Nw));

      delete []s[i];
    }

  delete []s;
  delete []v;
}

static void threefish_encrypt(char *E,
			      const char *K,
			      const char *T,
			      const char *P,
			      const size_t P_size,
			      const size_t block_size,
			      bool *ok)
{
  if(Q_UNLIKELY(!E || !K || !P || P_size == 0 || !T || block_size == 0))
    {
      if(ok)
	*ok = false;

      return;
    }

  Nr = 72;
  Nw = 4;
  Pi = Pi_4;
  threefish_encrypt_implementation(E, K, T, P, P_size, block_size, ok);
}

static void threefish_encrypt_implementation(char *E,
					     const char *K,
					     const char *T,
					     const char *P,
					     const size_t P_size,
					     const size_t block_size,
					     bool *ok)
{
  if(Q_UNLIKELY(!E || !K || !T || !P || P_size == 0 || block_size == 0))
    {
      if(ok)
	*ok = false;

      return;
    }

  /*
  ** Section 3.3.
  */

  bool error = false;
  uint64_t C240 = 0x1bd11bdaa9fc1a22;
  uint64_t *k = new (std::nothrow) uint64_t[Nw + 1];
  uint64_t kNw = C240; // Section 3.3.2.
  uint64_t **s = new (std::nothrow) uint64_t*[Nr / 4 + 1];
  uint64_t t[3];
  uint64_t *v = new (std::nothrow) uint64_t[Nw];

  if(Q_UNLIKELY(!k || !s || !v))
    {
      if(ok)
	*ok = false;

      goto done_label;
    }

  for(size_t i = 0; i < Nr / 4 + 1; i++)
    {
      s[i] = new (std::nothrow) uint64_t[Nw];

      if(Q_UNLIKELY(!s[i]))
	error = true; // Do not break.
    }

  if(Q_UNLIKELY(error))
    {
      if(ok)
	*ok = false;

      goto done_label;
    }

  bytes_to_words(k, K, P_size);
  bytes_to_words(t, T, 16);
  bytes_to_words(v, P, P_size);

  for(size_t i = 0; i < Nw; i++)
    kNw ^= k[i]; // Section 3.3.2.

  k[Nw] = kNw;
  t[2] = t[0] ^ t[1]; // Section 3.3.2.

  /*
  ** Prepare the key schedule, section 3.3.2.
  */

  for(size_t d = 0; d < Nr / 4 + 1; d++)
    for(size_t i = 0; i < Nw; i++)
      {
	s[d][i] = k[(d + i) % (Nw + 1)];

	if(i == Nw - 1)
	  s[d][i] += d;
	else if(i == Nw - 2)
	  s[d][i] += t[(d + 1) % 3];
	else if(i == Nw - 3)
	  s[d][i] += t[d % 3];
      }

  for(size_t d = 0; d < Nr; d++)
    {
      if(d % 4 == 0)
	for(size_t i = 0; i < Nw; i++)
	  v[i] += s[d / 4][i];

      uint64_t *f = new (std::nothrow) uint64_t[Nw];

      if(Q_UNLIKELY(!f))
	{
	  if(ok)
	    *ok = false;

	  goto done_label;
	}

      for(size_t i = 0; i < Nw / 2; i++)
	{
	  uint64_t x0 = v[i * 2];
	  uint64_t x1 = v[i * 2 + 1];
	  uint64_t y0 = 0;
	  uint64_t y1 = 0;

	  mix(x0, x1, d, i, &y0, &y1, block_size);
	  f[i * 2] = y0;
	  f[i * 2 + 1] = y1;
	}

      for(size_t i = 0; i < Nw; i++)
	v[i] = f[Pi[i]];

      memset(f, 0, sizeof(*f) * static_cast<size_t> (Nw));
      delete []f;
    }

  for(size_t i = 0; i < Nw; i++)
    v[i] += s[Nr / 4][i];

  words_to_bytes(E, v, Nw);

  if(ok)
    *ok = true;

 done_label:

  if(Q_LIKELY(k))
    memset(k, 0, sizeof(*k) * static_cast<size_t> (Nw + 1));

  memset(t, 0, sizeof(t));

  if(Q_LIKELY(v))
    memset(v, 0, sizeof(*v) * static_cast<size_t> (Nw));

  delete []k;

  for(size_t i = 0; i < Nr / 4 + 1; i++)
    {
      if(Q_LIKELY(s[i]))
	memset(s[i], 0, sizeof(*s[i]) * static_cast<size_t> (Nw));

      delete []s[i];
    }

  delete []s;
  delete []v;
}

static void words_to_bytes(char *B,
			   const uint64_t *words,
			   const size_t words_size)
{
  if(Q_UNLIKELY(!B || !words || words_size == 0))
    return;

  for(size_t i = 0; i < words_size; i++)
    {
      B[i * 8 + 0] = static_cast<char> (words[i]);
      B[i * 8 + 1] = static_cast<char> ((words[i] >> 8) & 0xff);
      B[i * 8 + 2] = static_cast<char> ((words[i] >> 16) & 0xff);
      B[i * 8 + 3] = static_cast<char> ((words[i] >> 24) & 0xff);
      B[i * 8 + 4] = static_cast<char> ((words[i] >> 32) & 0xff);
      B[i * 8 + 5] = static_cast<char> ((words[i] >> 40) & 0xff);
      B[i * 8 + 6] = static_cast<char> ((words[i] >> 48) & 0xff);
      B[i * 8 + 7] = static_cast<char> ((words[i] >> 56) & 0xff);
    }
}

dooble_threefish256::dooble_threefish256(const QByteArray &key):
  dooble_block_cipher(key)
{
  m_block_length = key.length();
  m_key_length = key.length();
  m_tweak_length = 16;
}

dooble_threefish256::~dooble_threefish256()
{
}

QByteArray dooble_threefish256::decrypt(const QByteArray &bytes)
{
  QReadLocker locker(&m_locker);

  if(Q_UNLIKELY(m_key.isEmpty() || m_tweak.isEmpty()))
    return QByteArray();

  QByteArray iv(bytes.mid(0, m_key_length));

  if(Q_UNLIKELY(iv.length() != m_key_length))
    return QByteArray();

  QByteArray block(m_block_length, 0);
  QByteArray c;
  QByteArray ciphertext(bytes.mid(iv.length()));
  QByteArray decrypted;
  int iterations = ciphertext.length() / m_block_length;

  for(int i = 0; i < iterations; i++)
    {
      bool ok = true;
      int position = i * m_block_length;

      threefish_decrypt
	(block.data(),
	 m_key,
	 m_tweak,
	 ciphertext.mid(position, m_block_length).constData(),
	 static_cast<size_t> (m_block_length),
	 8 * static_cast<size_t> (m_block_length),
	 &ok);

      if(!ok)
	{
	  decrypted.clear();
	  break;
	}

      if(i == 0)
	block = dooble_block_cipher::xor_arrays(block, iv);
      else
	block = dooble_block_cipher::xor_arrays(block, c);

      c = ciphertext.mid(position, m_block_length);
      decrypted.append(block);
    }

  if(decrypted.isEmpty())
    return decrypted;

  QByteArray original_length;

  if(decrypted.length() > static_cast<int> (sizeof(int)))
    original_length = decrypted.mid
      (decrypted.length() - static_cast<int> (sizeof(int)),
       static_cast<int> (sizeof(int)));

  if(!original_length.isEmpty())
    {
      QDataStream in(&original_length, QIODevice::ReadOnly);
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

QByteArray dooble_threefish256::encrypt(const QByteArray &bytes)
{
  QReadLocker locker(&m_locker);

  if(Q_UNLIKELY(m_key.isEmpty() || m_tweak.isEmpty()))
    return QByteArray();

  locker.unlock();

  QByteArray iv;
  bool ok = true;

  set_initialization_vector(iv, &ok);

  if(Q_UNLIKELY(iv.isEmpty()))
    return QByteArray();

  locker.relock();

  /*
  ** Let's resize the container to the block size.
  */

  QByteArray block(iv.length(), 0);
  QByteArray encrypted;
  QByteArray plaintext(bytes);

  if(plaintext.isEmpty())
    plaintext = plaintext.leftJustified(m_block_length, 0);
  else
    plaintext = plaintext.leftJustified
      (m_block_length *
       static_cast<int> (qCeil(static_cast<qreal> (plaintext.length()) /
			       static_cast<qreal> (m_block_length)) + 1), 0);

  QByteArray original_length;
  QDataStream out(&original_length, QIODevice::WriteOnly);

  out << bytes.length();

  if(out.status() != QDataStream::Ok)
    return QByteArray();

  plaintext.replace
    (plaintext.length() - static_cast<int> (sizeof(int)),
     static_cast<int> (sizeof(int)), original_length);

  int iterations = plaintext.length() / m_block_length;

  for(int i = 0; i < iterations; i++)
    {
      QByteArray p;
      bool ok = true;
      int position = i * m_block_length;

      p = plaintext.mid(position, m_block_length);

      if(i == 0)
	block = dooble_block_cipher::xor_arrays(iv, p);
      else
	block = dooble_block_cipher::xor_arrays(block, p);

      threefish_encrypt(block.data(),
			m_key,
			m_tweak,
			block,
			static_cast<size_t> (m_block_length),
			8 * static_cast<size_t> (m_block_length),
			&ok);

      if(!ok)
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

void dooble_threefish256::set_initialization_vector
(QByteArray &bytes, bool *ok) const
{
  QReadLocker locker(&m_locker);
  int iv_length = m_key_length;

  locker.unlock();

  if(ok)
    *ok = false;

  if(Q_UNLIKELY(iv_length == 0))
    return;

  bytes = dooble_random::random_bytes(iv_length);

  if(ok)
    *ok = true;
}

void dooble_threefish256::set_key(const QByteArray &key)
{
  m_block_length = key.length();
  m_key = key;
  m_key_length = key.length();
}

void dooble_threefish256::set_tweak(const QByteArray &tweak, bool *ok)
{
  QWriteLocker locker(&m_locker);

  if(tweak.length() != 16)
    {
      if(ok)
	*ok = false;

      return;
    }

  m_tweak = tweak;

  if(*ok)
    *ok = true;
}

void dooble_threefish256::test1(void)
{
  dooble_threefish256 *s = new (std::nothrow) dooble_threefish256
    (dooble_random::random_bytes(32));

  if(!s)
    return;

  QByteArray c;
  QByteArray d;
  QByteArray p;

  s->set_tweak("76543210fedcba98", 0);
  p = "The pink duck visited the Soap Queen. A happy moment indeed.";
  c = s->encrypt(p);
  d = s->decrypt(c);
  qDebug() << "test1 " << (d == p ? "passed" : "failed");
  delete s;
}

void dooble_threefish256::test2(void)
{
  dooble_threefish256 *s = new (std::nothrow) dooble_threefish256
    (dooble_random::random_bytes(32));

  if(!s)
    return;

  QByteArray c;
  QByteArray d;
  QByteArray p;

  s->set_tweak("76543210fedcba98", 0);
  p = "If you wish to glimpse inside a human soul "
    "and get to know a man, don't bother analyzing "
    "his ways of being silent, of talking, of weeping, "
    "of seeing how much he is moved by noble ideas; you "
    "will get better results if you just watch him laugh. "
    "If he laughs well, he's a good man.";
  c = s->encrypt(p);
  d = s->decrypt(c);
  qDebug() << "test2 " << (d == p ? "passed" : "failed");
  delete s;
}

void dooble_threefish256::test3(void)
{
  dooble_threefish256 *s = new (std::nothrow) dooble_threefish256
    (dooble_random::random_bytes(32));

  if(!s)
    return;

  QByteArray c;
  QByteArray d;
  QByteArray p;

  s->set_tweak("76543210fedcba98", 0);
  p = "The truth is always an abyss. One must - as in a swimming pool "
    "- dare to dive from the quivering springboard of trivial "
    "everyday experience and sink into the depths, in order to "
    "later rise again - laughing and fighting for breath - to "
    "the now doubly illuminated surface of things.";
  c = s->encrypt(p);
  d = s->decrypt(c);
  qDebug() << "test3 " << (d == p ? "passed" : "failed");
  delete s;
}
