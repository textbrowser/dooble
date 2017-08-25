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

#ifndef dooble_history_h
#define dooble_history_h

#include <QAtomicInteger>
#include <QFuture>
#include <QTimer>
#include <QVariant>
#include <QWebEngineHistoryItem>

class dooble_history: public QObject
{
  Q_OBJECT

 public:
  enum HistoryItem
  {
    FAVICON = 0,
    LAST_VISITED,
    TITLE,
    URL,
    URL_DIGEST,
    VISIT_COUNT
  };

 dooble_history(void);
 ~dooble_history();
 QHash<QUrl, QHash<int, QVariant> > history(void) const;
 QList<QPair<QIcon, QString> > urls(void) const;
 static void purge(void);
 void save_favicon(const QIcon &icon, const QUrl &url);
 void save_item(const QIcon &icon, const QWebEngineHistoryItem &item);

 private:
  QAtomicInteger<short> m_interrupt;
  QFuture<void> m_purge_future;
  QHash<QUrl, QHash<int, QVariant> > m_history;
  QTimer m_purge_timer;
  static QAtomicInteger<quint64> s_db_id;
  void purge(const QByteArray &authentication_key,
	     const QByteArray &encryption_key);

 private slots:
  void slot_populate(void);
  void slot_purge_timer_timeout(void);

 signals:
  void icon_updated(const QIcon &icon, const QUrl &url);
  void new_item(const QIcon &icon, const QWebEngineHistoryItem &item);
  void populated(void);
};

#endif
