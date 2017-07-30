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

#include "dooble_pbkdf2.h"

dooble_pbkdf2::dooble_pbkdf2
(const QByteArray &password,
 const QByteArray &salt,
 int iterations_count,
 int output_size)
{
  m_iterations_count = qAbs(iterations_count);
  m_output_size = qAbs(output_size);
  m_password = password;
  m_salt = salt;
}

dooble_pbkdf2::~dooble_pbkdf2()
{
  if(!m_password.isEmpty())
    {
      QByteArray zeros(m_password.length(), 0);

      m_password.replace(0, m_password.length(), zeros);
    }

  m_password.clear();
}

QByteArray dooble_pbkdf2::pbkdf2(dooble_hmac_function function) const
{
  Q_UNUSED(function);
  return QByteArray();
}

void dooble_pbkdf2::interrupt(void)
{
}
