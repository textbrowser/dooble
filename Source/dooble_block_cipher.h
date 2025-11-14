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

#ifndef dooble_block_cipher_h
#define dooble_block_cipher_h

#include <QByteArray>

class dooble_block_cipher
{
 public:
  static QByteArray xor_arrays(const QByteArray &a, const QByteArray &b)
  {
    QByteArray bytes;
    auto const length = qMin(a.length(), b.length());

    for(int i = 0; i < length; i++)
      bytes.append(static_cast<char> (a[i] ^ b[i]));

    return bytes;
  }

  virtual ~dooble_block_cipher();
  virtual QByteArray decrypt(const QByteArray &data) = 0;
  virtual QByteArray encrypt(const QByteArray &data) = 0;
  virtual void set_key(const QByteArray &key) = 0;
  virtual void set_tweak(const QByteArray &tweak, bool *ok);

 protected:
  QByteArray m_key;
  int m_block_length;
  int m_key_length;
  dooble_block_cipher(const QByteArray &key);
  dooble_block_cipher(void);
};

#endif
