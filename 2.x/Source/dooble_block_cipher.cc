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

#include <climits>

#include "dooble_block_cipher.h"

dooble_block_cipher::dooble_block_cipher(const QByteArray &key)
{
  m_block_length = key.length(); // True implementation must correct.
  m_key = key;
  m_key_length = key.length(); // True implementation must correct.
}

dooble_block_cipher::dooble_block_cipher(void)
{
  m_block_length = m_key_length = 0;
}

dooble_block_cipher::~dooble_block_cipher()
{
}

QByteArray dooble_block_cipher::xor_arrays(const QByteArray &a,
					   const QByteArray &b)
{
  QByteArray bytes;
  int length = qMin(a.length(), b.length());

  for(int i = 0; i < length; i++)
    bytes.append(a[i] ^ b[i]);

  return bytes;
}

void dooble_block_cipher::set_tweak(const QByteArray &tweak, bool *ok)
{
  Q_UNUSED(ok);
  Q_UNUSED(tweak);
}
