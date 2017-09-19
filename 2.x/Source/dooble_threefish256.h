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

#ifndef dooble_threefish256_h
#define dooble_threefish256_h

#include <QByteArray>
#include <QReadWriteLock>

#include "dooble_block_cipher.h"

class dooble_threefish256: public dooble_block_cipher
{
 public:
  dooble_threefish256(const QByteArray &key);
  ~dooble_threefish256();
  QByteArray decrypt(const QByteArray &bytes);
  QByteArray encrypt(const QByteArray &bytes);
  static void test1(void);
  static void test2(void);
  static void test3(void);
  void set_key(const QByteArray &key);
  void set_tweak(const QByteArray &tweak, bool *ok);

 private:
  QByteArray m_tweak;
  mutable QReadWriteLock m_locker;
  size_t m_tweak_length;
  void set_initialization_vector(QByteArray &bytes, bool *ok) const;
};

#endif
