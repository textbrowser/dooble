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
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QTimer>
#include <QWebEngineHistoryItem>

class QStandardItemModel;
typedef QList<QPair<QIcon, QString> > QListPairIconString;
typedef QList<QUrl> QListUrl;
typedef QList<QVector<QByteArray> > QListVectorByteArray;

class dooble_history: public QObject
{
  Q_OBJECT

 public:
  enum HistoryItem
  {
    FAVICON = 0,
    FAVORITE,
    LAST_VISITED,
    NUMBER_OF_VISITS,
    TITLE,
    URL,
    URL_DIGEST
  };

  dooble_history(void);
  ~dooble_history();
  QHash<QUrl, QHash<dooble_history::HistoryItem, QVariant> >
    history(void) const;
  QStandardItemModel *favorites_model(void) const;
  bool is_favorite(const QUrl &url) const;
  void abort(void);
  void purge_favorites(void);
  void purge_history(void);
  void remove_favorite(const QUrl &url);
  void remove_items_list(const QList<QUrl> &url);
  void save_favicon(const QIcon &icon, const QUrl &url);
  void save_favorite(const QUrl &url, bool state);
  void save_item(const QIcon &icon,
		 const QWebEngineHistoryItem &item,
		 bool force);

 private:
  QAtomicInteger<short> m_interrupt;
  QFuture<void> m_populate_future;
  QFuture<void> m_purge_future;
  QHash<QUrl, QHash<HistoryItem, QVariant> > m_history;
  QList<QFuture<void> > m_futures;
  QStandardItemModel *m_favorites_model;
  QTimer m_purge_timer;
  mutable QReadWriteLock m_history_mutex;
  static QAtomicInteger<quintptr> s_db_id;
  void create_tables(QSqlDatabase &db);
  void populate(const QByteArray &authentication_key,
		const QByteArray &encryption_key);
  void purge(const QByteArray &authentication_key,
	     const QByteArray &encryption_key);
  void purge_favorites_concurrent(const QByteArray &f, const QByteArray &t);
  void purge_history_concurrent(const QByteArray &f);
  void remove_items_list_concurrent(const QList<QByteArray> &url_digest);
  void save_item_concurrent(const QByteArray &authentication_key,
			    const QByteArray &encryption_key,
			    const QString &title,
			    const QWebEngineHistoryItem &item);
  void update_favorite(const QHash<HistoryItem, QVariant> &hash);

 private slots:
  void slot_populate(void);
  void slot_populated_favorites(const QListVectorByteArray &favorites);
  void slot_remove_items(const QListUrl &urls);
  void slot_purge_timer_timeout(void);

 signals:
  void icon_updated(const QIcon &icon, const QUrl &url);
  void item_updated(const QIcon &icon, const QWebEngineHistoryItem &item);
  void new_item(const QIcon &icon, const QWebEngineHistoryItem &item);
  void populated(const QListPairIconString &list);
  void populated(void);
  void populated_favorites(const QListVectorByteArray &favorites);
  void remove_items(const QListUrl &urls);
};

#endif
