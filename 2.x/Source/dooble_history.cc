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

#include <QDir>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_favicons.h"
#include "dooble_history.h"

dooble_history::dooble_history(void):QObject()
{
  connect(&m_purge_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_timer_timeout(void)));
  connect(this,
	  SIGNAL(populated_favorites(const QListVectorByteArray &)),
	  this,
	  SLOT(slot_populated_favorites(const QListVectorByteArray &)));
  connect(this,
	  SIGNAL(remove_items(const QListUrl &)),
	  this,
	  SLOT(slot_remove_items(const QListUrl &)));
  m_favorites_model = new QStandardItemModel(this);
  m_favorites_model->setHorizontalHeaderLabels
    (QStringList() << tr("Title")
                   << tr("Site")
                   << tr("Last Visited")
                   << tr("Number of Visits"));
  m_purge_timer.start(15000);
}

dooble_history::~dooble_history()
{
  abort();
}

QHash<QUrl, QHash<dooble_history::HistoryItem, QVariant> > dooble_history::
history(void) const
{
  QReadLocker locker(&m_history_mutex);

  return m_history;
}

QStandardItemModel *dooble_history::favorites_model(void) const
{
  return m_favorites_model;
}

bool dooble_history::is_favorite(const QUrl &url) const
{
  QReadLocker locker(&m_history_mutex);

  return m_history.value(url).value(FAVORITE, false).toBool();
}

void dooble_history::abort(void)
{
  m_interrupt.store(1);
  m_populate_future.cancel();
  m_populate_future.waitForFinished();
  m_purge_future.cancel();
  m_purge_future.waitForFinished();
}

void dooble_history::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_history ("
	     "favorite_digest TEXT NOT NULL, "
	     "last_visited TEXT NOT NULL, "
	     "number_of_visits TEXT NOT NULL, "
	     "title TEXT NOT NULL, "
	     "url TEXT NOT NULL, "
	     "url_digest TEXT PRIMARY KEY NOT NULL)");
}

void dooble_history::populate(const QByteArray &authentication_key,
			      const QByteArray &encryption_key)
{
  QListVectorByteArray favorites;
  QMultiMap<QDateTime, QPair<QIcon, QString> > map;
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);
	dooble_cryptography cryptography
	  (authentication_key,
	   encryption_key,
	   dooble_settings::setting("block_cipher_type").toString());
	int days = dooble_settings::setting("browsing_history_days").toInt();

	query.setForwardOnly(true);

	if(query.exec("SELECT "
		      "favorite_digest, "  // 0
		      "last_visited, "     // 1
		      "number_of_visits, " // 2
		      "title, "            // 3
		      "url, "              // 4
		      "url_digest "        // 5
		      "OID "               // 6
		      "FROM dooble_history"))
	  while(query.next())
	    {
	      if(m_populate_future.isCanceled())
		break;

	      QByteArray last_visited
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      last_visited = cryptography.mac_then_decrypt
		(last_visited);

	      if(last_visited.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_history",
		     query.value(6).toLongLong());
		  continue;
		}

	      bool is_favorite = dooble_cryptography::memcmp
		(cryptography.hmac(QByteArray("true")).toBase64(),
		 query.value(0).toByteArray());

	      if(!is_favorite)
		{
		  QDateTime dateTime
		    (QDateTime::
		     fromString(last_visited.constData(), Qt::ISODate));
		  QDateTime now(QDateTime::currentDateTime());

		  if(dateTime.daysTo(now) >= qAbs(days))
		    /*
		    ** Ignore an expired entry, unless the entry is a favorite.
		    */

		    continue;
		}

	      QByteArray number_of_visits
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      number_of_visits = cryptography.mac_then_decrypt
		(number_of_visits);

	      if(number_of_visits.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_history",
		     query.value(6).toLongLong());
		  continue;
		}

	      QByteArray title
		(QByteArray::fromBase64(query.value(3).toByteArray()));

	      title = cryptography.mac_then_decrypt(title);

	      if(title.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_history",
		     query.value(6).toLongLong());
		  continue;
		}

	      QByteArray url
		(QByteArray::fromBase64(query.value(4).toByteArray()));

	      url = cryptography.mac_then_decrypt(url);

	      if(url.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_history",
		     query.value(6).toLongLong());
		  continue;
		}

	      QHash<HistoryItem, QVariant> hash;

	      hash[FAVORITE] = is_favorite;
	      hash[LAST_VISITED] = QDateTime::fromString
		(last_visited.constData(), Qt::ISODate);
	      hash[NUMBER_OF_VISITS] = qMax
		(1ULL, number_of_visits.toULongLong());
	      hash[TITLE] = title.constData();
	      hash[URL] = QUrl::fromEncoded(url);
	      hash[URL_DIGEST] = QByteArray::fromBase64
		(query.value(5).toByteArray());

	      if(is_favorite)
		{
		  QVector<QByteArray> vector;

		  vector << title << url << last_visited << number_of_visits;
		  favorites << vector;
		}

	      map.insert(hash[LAST_VISITED].toDateTime(),
			 QPair<QIcon, QString> (QIcon(), url));

	      QWriteLocker locker(&m_history_mutex);

	      m_history[hash[URL].toUrl()] = hash;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);

  if(!m_populate_future.isCanceled())
    {
      emit populated();
      emit populated(map.values());
      emit populated_favorites(favorites);
    }
}

void dooble_history::purge(const QByteArray &authentication_key,
			   const QByteArray &encryption_key)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);
	dooble_cryptography cryptography
	  (authentication_key,
	   encryption_key,
	   dooble_settings::setting("block_cipher_type").toString());

	query.setForwardOnly(true);
	query.exec("SELECT last_visited, url, url_digest "
		   "FROM dooble_history WHERE favorite_digest = ?");
	query.addBindValue(cryptography.hmac(QByteArray("false")).toBase64());

	if(query.exec())
	  {
	    QListUrl urls;
	    int days = dooble_settings::setting
	      ("browsing_history_days").toInt();

	    while(query.next())
	      {
		if(m_interrupt.loadAcquire())
		  break;

		QByteArray bytes
		  (QByteArray::fromBase64(query.value(0).toByteArray()));

		bytes = cryptography.mac_then_decrypt(bytes);

		QDateTime dateTime
		  (QDateTime::fromString(bytes.constData(), Qt::ISODate));
		QDateTime now(QDateTime::currentDateTime());

		if(dateTime.daysTo(now) >= qAbs(days))
		  {
		    QSqlQuery delete_query(db);

		    delete_query.prepare
		      ("DELETE FROM dooble_history WHERE url_digest = ?");
		    delete_query.addBindValue(query.value(2));

		    if(delete_query.exec())
		      {
			bytes = QByteArray::fromBase64
			  (query.value(1).toByteArray());
			bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

			if(!bytes.isEmpty())
			  urls << QUrl(bytes);
		      }
		  }
	      }

	    if(!urls.isEmpty())
	      emit remove_items(urls);
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::purge_favorites(void)
{
  for(int i = 0; i < m_favorites_model->rowCount(); i++)
    {
      QStandardItem *item = m_favorites_model->item(i, 1);

      if(!item)
	continue;

      QWriteLocker locker(&m_history_mutex);

      if(m_history.contains(item->text()))
	{
	  QHash<HistoryItem, QVariant> hash(m_history.value(item->text()));

	  hash[FAVORITE] = false;
	  m_history[item->text()] = hash;
	}
    }

  m_favorites_model->removeRows(0, m_favorites_model->rowCount());

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QByteArray f
	(dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
      QByteArray t(dooble::s_cryptography->hmac(QByteArray("true")).toBase64());
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");
	    query.prepare
	      ("UPDATE dooble_history SET favorite_digest = ? WHERE "
	       "favorite_digest = ?");
	    query.addBindValue(f);
	    query.addBindValue(t);
	    query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::purge_history(void)
{
  m_populate_future.cancel();
  m_populate_future.waitForFinished();

  {
    QHash<QUrl, QHash<HistoryItem, QVariant> > hash;
    QWriteLocker locker(&m_history_mutex);

    for(int i = 0; i < m_favorites_model->rowCount(); i++)
      {
	QStandardItem *item = m_favorites_model->item(i, 1);

	if(!item)
	  continue;

	if(m_history.contains(item->text()))
	  hash[item->text()] = m_history.value(item->text());
      }

    m_history.clear();
    m_history = hash;
  }

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QByteArray f
	(dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");
	    query.prepare
	      ("DELETE FROM dooble_history WHERE favorite_digest = ?");
	    query.addBindValue(f);
	    query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::remove_favorite(const QUrl &url)
{
  QList<QStandardItem *> list
    (m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

  if(!list.isEmpty() && list.at(0))
    m_favorites_model->removeRow(list.at(0)->row());

  QHash<HistoryItem, QVariant> hash;
  QWriteLocker locker(&m_history_mutex);

  hash = m_history.value(url);

  if(!hash.isEmpty())
    {
      hash[FAVORITE] = false;
      m_history[url] = hash;
    }

  locker.unlock();

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.prepare
	      ("UPDATE dooble_history SET favorite_digest = ? "
	       "WHERE url_digest = ?");
	    query.addBindValue
	      (dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
	    query.addBindValue
	      (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	    query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::remove_items_list(const QList<QUrl> &urls)
{
  if(urls.isEmpty())
    return;

  QList<QByteArray> url_digests;

  for(int i = 0; i < urls.size(); i++)
    {
      QList<QStandardItem *> list
	(m_favorites_model->
	 findItems(urls.at(i).toString(), Qt::MatchExactly, 1));

      if(!list.isEmpty() && list.at(0))
	m_favorites_model->removeRow(list.at(0)->row());

      QWriteLocker locker(&m_history_mutex);

      m_history.remove(urls.at(i));
      locker.unlock();
      url_digests << dooble::s_cryptography->hmac
	(urls.at(i).toEncoded()).toBase64();
    }

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");

	    for(int i = 0; i < url_digests.size(); i++)
	      {
		query.prepare
		  ("DELETE FROM dooble_history WHERE url_digest = ?");
		query.addBindValue(url_digests.at(i));
		query.exec();
	      }
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::save_favicon(const QIcon &icon, const QUrl &url)
{
  {
    QWriteLocker locker(&m_history_mutex);

    if(m_history.contains(url))
      {
	QHash<HistoryItem, QVariant> hash(m_history.value(url));

	if(icon.isNull())
	  hash[FAVICON] = dooble_favicons::icon(url);
	else
	  hash[FAVICON] = icon;

	m_history[url] = hash;
	locker.unlock();
	update_favorite(hash);
	emit icon_updated(hash[FAVICON].value<QIcon> (), url);
      }
  }
}

void dooble_history::save_favorite(const QUrl &url, bool state)
{
  if(url.isEmpty() || !url.isValid())
    return;

  QHash<HistoryItem, QVariant> hash;
  QWriteLocker locker(&m_history_mutex);

  if(m_history.contains(url))
    {
      hash = m_history.value(url);

      if(hash.value(FAVICON).isNull())
	hash[FAVICON] = dooble_favicons::icon(url);

      hash[FAVORITE] = state;
      m_history[url] = hash;
    }
  else
    {
      /*
      ** The item may have been removed via the History window.
      */

      hash[FAVICON] = dooble_favicons::icon(url);
      hash[FAVORITE] = state;
      hash[LAST_VISITED] = QDateTime::currentDateTime();
      hash[NUMBER_OF_VISITS] = 1ULL;
      hash[TITLE] = url;
      hash[URL] = url;

      if(dooble::s_cryptography)
	hash[URL_DIGEST] = dooble::s_cryptography->hmac(url.toEncoded());

      m_history[url] = hash;
    }

  locker.unlock();

  if(state)
    update_favorite(hash);
  else
    {
      QList<QStandardItem *> list
	(m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

      if(!list.isEmpty() && list.at(0))
	m_favorites_model->removeRow(list.at(0)->row());
    }

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_history "
	   "(favorite_digest, "
	   "last_visited, "
	   "number_of_visits, "
	   "title, "
	   "url, "
	   "url_digest) "
	   "VALUES (?, ?, ?, ?, ?, ?)");

	QByteArray bytes;

	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(state ? QByteArray("true") : QByteArray("false")).toBase64());
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (hash.
	   value(LAST_VISITED).toDateTime().toString(Qt::ISODate).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (QByteArray::number(hash.value(NUMBER_OF_VISITS, 1).toULongLong()));

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	if(hash.value(TITLE).toString().trimmed().isEmpty())
	  bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());
	else
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (hash.value(TITLE).toString().trimmed().toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::save_item(const QIcon &icon,
			       const QWebEngineHistoryItem &item,
			       bool force)
{
  QHash<HistoryItem, QVariant> hash;

  if(item.isValid())
    {
      QWriteLocker locker(&m_history_mutex);
      bool contains = m_history.contains(item.url());

      if(icon.isNull())
	hash[FAVICON] = dooble_favicons::icon(item.url());
      else
	hash[FAVICON] = icon;

      hash[FAVORITE] = m_history.value(item.url()).value(FAVORITE, false);
      hash[LAST_VISITED] = item.lastVisited();
      hash[NUMBER_OF_VISITS] =
	m_history.value(item.url()).value(NUMBER_OF_VISITS, 1).
	toULongLong() + 1;

      if(hash.value(FAVORITE).toBool())
	hash[TITLE] = m_history.value(item.url()).value
	  (TITLE, item.title().trimmed().mid(0, dooble::MAXIMUM_TITLE_LENGTH));
      else
	hash[TITLE] = item.title().trimmed().mid
	  (0, dooble::MAXIMUM_TITLE_LENGTH);

      hash[URL] = item.url();

      if(dooble::s_cryptography)
	hash[URL_DIGEST] = dooble::s_cryptography->hmac(item.url().toEncoded());

      m_history[item.url()] = hash;
      locker.unlock();
      update_favorite(hash);

      if(!contains)
	emit new_item(hash[FAVICON].value<QIcon> (), item);
      else
	emit item_updated(icon, item);
    }
  else
    return;

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(dooble_settings::setting("browsing_history_days").toInt() == 0 &&
	  !force)
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_history "
	   "(favorite_digest, "
	   "last_visited, "
	   "number_of_visits, "
	   "title, "
	   "url, "
	   "url_digest) "
	   "VALUES (?, ?, ?, ?, ?, ?)");

	QByteArray bytes;

	{
	  QReadLocker locker(&m_history_mutex);

	  query.addBindValue
	    (dooble::s_cryptography->
	     hmac(m_history.value(item.url()).value(FAVORITE, false).toBool() ?
		  QByteArray("true") : QByteArray("false")).toBase64());
	}

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item.lastVisited().toString(Qt::ISODate).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	{
	  QReadLocker locker(&m_history_mutex);

	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (QByteArray::number(m_history.value(item.url()).
				value(NUMBER_OF_VISITS, 1).toULongLong()));
	}

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	QString title(hash.value(TITLE).toString().trimmed());

	if(title.isEmpty())
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (item.url().toEncoded());
	else
	  bytes = dooble::s_cryptography->encrypt_then_mac(title.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item.url().toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(item.url().toEncoded()).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      emit populated();
      return;
    }
  else if(m_populate_future.isRunning())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_favorites_model->removeRows(0, m_favorites_model->rowCount());

  {
    QWriteLocker locker(&m_history_mutex);

    m_history.clear();
  }

  QApplication::restoreOverrideCursor();
  m_populate_future = QtConcurrent::run
    (this,
     &dooble_history::populate,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
}

void dooble_history::slot_populated_favorites
(const QListVectorByteArray &favorites)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < favorites.size(); i++)
    {
      QByteArray last_visited(favorites.at(i).value(2));
      QByteArray number_of_visits(favorites.at(i).value(3));
      QByteArray title(favorites.at(i).value(0));
      QByteArray url(favorites.at(i).value(1));
      QList<QStandardItem *> list;

      for(int j = 0; j < m_favorites_model->columnCount(); j++)
	{
	  QStandardItem *item = new QStandardItem();

	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	  switch(j)
	    {
	    case 0:
	      {
		item->setData(QUrl::fromEncoded(url));
		item->setText(title);
		item->setToolTip(item->text());
		break;
	      }
	    case 1:
	      {
		item->setText(QUrl::fromEncoded(url).toString());
		item->setToolTip(item->text());
		break;
	      }
	    case 2:
	      {
		item->setText(last_visited);
		break;
	      }
	    case 3:
	      {
		item->setText
		  (number_of_visits.rightJustified(16, '0'));
		break;
	      }
	    }

	  list << item;
	}

      if(!list.isEmpty())
	m_favorites_model->appendRow(list);
    }

  QApplication::restoreOverrideCursor();
}

void dooble_history::slot_purge_timer_timeout(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_purge_future.isRunning())
    return;

  m_purge_future = QtConcurrent::run
    (this,
     &dooble_history::purge,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
}

void dooble_history::slot_remove_items(const QListUrl &urls)
{
  if(urls.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  {
    QWriteLocker locker(&m_history_mutex);

    for(int i = 0; i < urls.size(); i++)
      m_history.remove(urls.at(i));
  }

  QApplication::restoreOverrideCursor();
}

void dooble_history::update_favorite(const QHash<HistoryItem, QVariant> &hash)
{
  if(!hash.value(FAVORITE, false).toBool())
    return;

  QUrl url(hash.value(URL).toUrl());

  if(url.isEmpty() || !url.isValid())
    return;

  QList<QStandardItem *> list
    (m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

  if(list.isEmpty())
    {
      for(int i = 0; i < m_favorites_model->columnCount(); i++)
	{
	  QStandardItem *item = new QStandardItem();

	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	  switch(i)
	    {
	    case 0:
	      {
		item->setData(url);
		item->setIcon(hash.value(FAVICON).value<QIcon> ());

		if(hash.value(TITLE).toString().trimmed().isEmpty())
		  item->setText(hash.value(URL).toString());
		else
		  item->setText(hash.value(TITLE).toString());

		item->setToolTip(item->text());
		break;
	      }
	    case 1:
	      {
		item->setText(url.toString());
		item->setToolTip(item->text());
		break;
	      }
	    case 2:
	      {
		item->setText
		  (hash.value(LAST_VISITED).toDateTime().toString(Qt::ISODate));
		break;
	      }
	    case 3:
	      {
		item->setText
		  (QString::number(hash.value(NUMBER_OF_VISITS).toULongLong()).
		   rightJustified(16, '0'));
		break;
	      }
	    }

	  list << item;
	}

      if(!list.isEmpty())
	m_favorites_model->appendRow(list);
    }
  else
    {
      QStandardItem *item = list.at(0);

      if(!item)
	return;

      int row = item->row();

      for(int i = 0; i < m_favorites_model->columnCount(); i++)
	{
	  item = m_favorites_model->item(row, i);

	  if(!item)
	    continue;

	  switch(i)
	    {
	    case 0:
	      {
		item->setData(url);
		item->setIcon(hash.value(FAVICON).value<QIcon> ());

		if(hash.value(TITLE).toString().trimmed().isEmpty())
		  item->setText(hash.value(URL).toString());
		else
		  item->setText(hash.value(TITLE).toString());

		item->setToolTip(item->text());
		break;
	      }
	    case 1:
	      {
		item->setText(url.toString());
		item->setToolTip(item->text());
		break;
	      }
	    case 2:
	      {
		item->setText
		  (hash.value(LAST_VISITED).toDateTime().toString(Qt::ISODate));
		break;
	      }
	    case 3:
	      {
		item->setText
		  (QString::number(hash.value(NUMBER_OF_VISITS).toULongLong()).
		   rightJustified(16, '0'));
		break;
	      }
	    }
	}
    }
}
