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

#ifndef dooble_cookies_h
#define dooble_cookies_h

#include <QAtomicInteger>
#include <QObject>
#include <QNetworkCookie>

class dooble_cookies: public QObject
{
  Q_OBJECT

 public:
  dooble_cookies(bool is_private, QObject *parent);
  static void purge(void);

 private:
  bool m_is_private;
  static QAtomicInteger<quintptr> s_db_id;

 private slots:
  void slot_cookie_added(const QNetworkCookie &cookie);
  void slot_cookie_removed(const QNetworkCookie &cookie);
  void slot_delete_cookie(const QNetworkCookie &cookie);
  void slot_delete_domain(const QString &domain);
  void slot_populate(void);

 signals:
  void cookie_added(const QNetworkCookie &cookie, bool is_favorite);
  void cookie_removed(const QNetworkCookie &cookie);
};

#endif
