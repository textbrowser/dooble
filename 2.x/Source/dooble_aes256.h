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

#ifndef dooble_aes256_h
#define dooble_aes256_h

#include <QByteArray>
#include <stdint.h>

class dooble_aes256
{
 public:
  dooble_aes256(const QByteArray &key);
  ~dooble_aes256();
  static void test1_decrypt_block(void);
  static void test1_encrypt_block(void);
  static void test1_key_expansion(void);

 private:
  QByteArray m_key;
  int m_key_length;
  size_t m_Nb;
  size_t m_Nk;
  size_t m_Nr;
  uint8_t m_round_key[60][4];
  uint8_t m_state[4][4]; // 4 rows, Nb columns.
  QByteArray decrypt_block(const QByteArray &block);
  QByteArray encrypt_block(const QByteArray &block);
  uint8_t xtime(uint8_t x) const;
  uint8_t xtime_special(uint8_t x, uint8_t y) const;
  void add_round_key(size_t c);
  void inv_mix_columns(void);
  void inv_shift_rows(void);
  void inv_sub_bytes(void);
  void key_expansion(void);
  void mix_columns(void);
  void shift_rows(void);
  void sub_bytes();
};

#endif
