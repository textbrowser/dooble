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
#include "dooble_ui_utilities.h"

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

QList<QAction *> dooble_history::last_n_actions(int n) const
{
  QHash<QUrl, char> hash;
  QList<QAction *> list;
  QReadLocker locker(&m_history_mutex);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QMapIterator<QDateTime, QUrl> it(m_history_date_time);
#else
  QMultiMapIterator<QDateTime, QUrl> it(m_history_date_time);
#endif

  it.toBack();

  while(it.hasPrevious())
    {
      it.previous();

      if(hash.contains(it.value()))
	continue;

      auto title
	(m_history.value(it.value()).
	 value(dooble_history::HistoryItem::TITLE).toString());

      title = title.trimmed();
      title.replace("&", "");

      if(title.isEmpty())
	continue;

      auto action = new QAction(title);

      action->setData
	(m_history.value(it.value()).
	 value(dooble_history::HistoryItem::URL).toUrl());
      action->setIcon(dooble_favicons::icon(it.value()));
      hash[it.value()] = 0;
      list << action;

      if(list.size() >= n)
	break;
    }

  return list;
}

QList<QUrl> dooble_history::previous_session_tabs(void) const
{
  QList<QUrl> list;

  if(!dooble::s_cryptography)
    return list;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	if(query.exec("SELECT url FROM dooble_previous_session_tabs"))
	  while(query.next())
	    {
	      auto const bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));
	      auto const url
		(QUrl::fromEncoded(dooble::
				   s_cryptography->mac_then_decrypt(bytes)));

	      if(!url.isEmpty() && url.isValid())
		list << url;
	    }

	query.exec("DELETE FROM dooble_previous_session_tabs");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  return list;
}

QStandardItemModel *dooble_history::favorites_model(void) const
{
  return m_favorites_model;
}

bool dooble_history::is_favorite(const QUrl &url) const
{
  QReadLocker locker(&m_history_mutex);

  return m_history.value(url).value
    (dooble_history::HistoryItem::FAVORITE, false).toBool();
}

void dooble_history::abort(void)
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  m_interrupt.store(1);
#else
  m_interrupt.storeRelaxed(1);
#endif
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
  query.exec("CREATE TABLE IF NOT EXISTS dooble_previous_session_tabs ("
	     "url TEXT NOT NULL, "
	     "url_digest TEXT PRIMARY KEY NOT NULL)");
}

void dooble_history::populate(const QByteArray &authentication_key,
			      const QByteArray &encryption_key)
{
  QListVectorByteArray favorites;
  QMultiMap<QDateTime, QPair<QIcon, QString> > map;
  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);
	auto const days = dooble_settings::setting
	  ("browsing_history_days").toInt();
	dooble_cryptography cryptography
	  (authentication_key,
	   encryption_key,
	   dooble_settings::setting("block_cipher_type").toString(),
	   dooble_settings::setting("hash_type").toString());

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

	      auto last_visited
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

	      auto const is_favorite = dooble_cryptography::memcmp
		(cryptography.hmac(QByteArray("true")).toBase64(),
		 query.value(0).toByteArray());

	      if(!is_favorite)
		{
		  auto const date_time
		    (QDateTime::
		     fromString(last_visited.constData(), Qt::ISODate));
		  auto const now(QDateTime::currentDateTime());

		  if(date_time.daysTo(now) >= qAbs(days))
		    /*
		    ** Ignore an expired entry, unless the entry is a favorite.
		    */

		    continue;
		}

	      auto number_of_visits
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

	      auto title
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

	      auto url
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

	      QHash<dooble_history::HistoryItem, QVariant> hash;

	      hash[dooble_history::HistoryItem::FAVORITE] = is_favorite;
	      hash[dooble_history::HistoryItem::LAST_VISITED] =
		QDateTime::fromString
		(last_visited.constData(), Qt::ISODate);
	      hash[dooble_history::HistoryItem::NUMBER_OF_VISITS] = qMax
		(1ULL, number_of_visits.toULongLong());
	      hash[dooble_history::HistoryItem::TITLE] = title.constData();
	      hash[dooble_history::HistoryItem::URL] = QUrl::fromEncoded(url);
	      hash[dooble_history::HistoryItem::URL_DIGEST] =
		QByteArray::fromBase64(query.value(5).toByteArray());

	      if(is_favorite)
		{
		  QVector<QByteArray> vector;

		  vector << title << url << last_visited << number_of_visits;
		  favorites << vector;
		}

	      map.insert
		(hash.value(dooble_history::HistoryItem::LAST_VISITED).
		 toDateTime(), QPair<QIcon, QString> (QIcon(), url));

	      QWriteLocker locker(&m_history_mutex);

	      m_history[hash.value(dooble_history::HistoryItem::URL).toUrl()] =
		hash;
	      m_history_date_time.insert
		(hash.value(dooble_history::HistoryItem::LAST_VISITED).
		 toDateTime(),
		 hash.value(dooble_history::HistoryItem::URL).toUrl());
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
  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
	   dooble_settings::setting("block_cipher_type").toString(),
	   dooble_settings::setting("hash_type").toString());

	query.setForwardOnly(true);
	query.exec("SELECT last_visited, url, url_digest "
		   "FROM dooble_history WHERE favorite_digest = ?");
	query.addBindValue(cryptography.hmac(QByteArray("false")).toBase64());

	if(query.exec())
	  {
	    QListUrl urls;
	    auto const days = dooble_settings::setting
	      ("browsing_history_days").toInt();

	    while(query.next())
	      {
		if(m_interrupt.loadAcquire())
		  break;

		auto bytes
		  (QByteArray::fromBase64(query.value(0).toByteArray()));

		bytes = cryptography.mac_then_decrypt(bytes);

		auto const date_time
		  (QDateTime::fromString(bytes.constData(), Qt::ISODate));
		auto const now(QDateTime::currentDateTime());

		if(date_time.daysTo(now) >= qAbs(days))
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
			  urls << QUrl::fromUserInput(bytes);
		      }
		  }
	      }

	    if(!urls.isEmpty())
	      {
		emit remove_items(urls);
		query.exec("VACUUM");
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::purge_all(void)
{
  m_favorites_model->removeRows(0, m_favorites_model->rowCount());

  QWriteLocker locker(&m_history_mutex);

  m_history.clear();
  m_history_date_time.clear();
  m_history_mutex.unlock();

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_history");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::purge_favorites(void)
{
  for(int i = 0; i < m_favorites_model->rowCount(); i++)
    {
      auto item = m_favorites_model->item(i, 1);

      if(!item)
	continue;

      QWriteLocker locker(&m_history_mutex);

      if(m_history.contains(item->text()))
	{
	  auto hash(m_history.value(item->text()));

	  hash[dooble_history::HistoryItem::FAVORITE] = false;
	  m_history[item->text()] = hash;
	}
    }

  m_favorites_model->removeRows(0, m_favorites_model->rowCount());

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());
      auto const f
	(dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
      auto const t
	(dooble::s_cryptography->hmac(QByteArray("true")).toBase64());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
    QHash<QUrl, QHash<dooble_history::HistoryItem, QVariant> > hash;
    QWriteLocker locker(&m_history_mutex);

    for(int i = 0; i < m_favorites_model->rowCount(); i++)
      {
	auto item = m_favorites_model->item(i, 1);

	if(!item)
	  continue;

	if(m_history.contains(item->text()))
	  hash[item->text()] = m_history.value(item->text());
      }

    m_history.clear();
    m_history = hash;
    m_history_date_time.clear();
  }

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());
      auto const f
	(dooble::s_cryptography->hmac(QByteArray("false")).toBase64());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::remove_favorite(const QUrl &url)
{
  auto const list
    (m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

  if(!list.isEmpty() && list.at(0))
    m_favorites_model->removeRow(list.at(0)->row());

  QHash<dooble_history::HistoryItem, QVariant> hash;
  QWriteLocker locker(&m_history_mutex);

  hash = m_history.value(url);

  if(!hash.isEmpty())
    {
      hash[dooble_history::HistoryItem::FAVORITE] = false;
      m_history[url] = hash;
    }

  locker.unlock();

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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

  foreach(auto const &url, urls)
    {
      auto const list
	(m_favorites_model->
	 findItems(url.toString(), Qt::MatchExactly, 1));

      if(!list.isEmpty() && list.at(0))
	m_favorites_model->removeRow(list.at(0)->row());

      QWriteLocker locker(&m_history_mutex);

      m_history_date_time.remove
	(m_history.value(url).
	 value(dooble_history::HistoryItem::LAST_VISITED).toDateTime(),
	 url);
      m_history.remove(url);
      locker.unlock();
      url_digests << dooble::s_cryptography->hmac
	(url.toEncoded()).toBase64();
    }

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");

	    foreach(auto const &url_digest, url_digests)
	      {
		query.prepare
		  ("DELETE FROM dooble_history WHERE url_digest = ?");
		query.addBindValue(url_digest);
		query.exec();
	      }

	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
}

void dooble_history::save_favicon(const QIcon &icon, const QUrl &url)
{
  QWriteLocker locker(&m_history_mutex);

  if(m_history.contains(url))
    {
      auto hash(m_history.value(url));

      if(icon.isNull())
	hash[dooble_history::HistoryItem::FAVICON] =
	  dooble_favicons::icon(url);
      else
	hash[dooble_history::HistoryItem::FAVICON] = icon;

      m_history[url] = hash;
      locker.unlock();
      update_favorite(hash);
      emit icon_updated
	(hash.value(dooble_history::HistoryItem::FAVICON).value<QIcon> (),
	 url);
    }
}

void dooble_history::save_favorite(const QUrl &url, bool state)
{
  if(url.isEmpty() || !url.isValid())
    return;

  QHash<dooble_history::HistoryItem, QVariant> hash;
  QWriteLocker locker(&m_history_mutex);

  if(m_history.contains(url))
    {
      hash = m_history.value(url);

      if(hash.value(dooble_history::HistoryItem::FAVICON).isNull())
	hash[dooble_history::HistoryItem::FAVICON] = dooble_favicons::icon(url);

      hash[dooble_history::HistoryItem::FAVORITE] = state;
      m_history[url] = hash;
    }
  else
    {
      /*
      ** The item may have been removed via the History window.
      */

      hash[dooble_history::HistoryItem::FAVICON] = dooble_favicons::icon(url);
      hash[dooble_history::HistoryItem::FAVORITE] = state;
      hash[dooble_history::HistoryItem::LAST_VISITED] =
	QDateTime::currentDateTime();
      hash[dooble_history::HistoryItem::NUMBER_OF_VISITS] = 1ULL;
      hash[dooble_history::HistoryItem::TITLE] = url;
      hash[dooble_history::HistoryItem::URL] = url;

      if(dooble::s_cryptography)
	hash[dooble_history::HistoryItem::URL_DIGEST] =
	  dooble::s_cryptography->hmac(url.toEncoded());

      m_history[url] = hash;
    }

  locker.unlock();

  if(state)
    update_favorite(hash);
  else
    {
      auto const list
	(m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

      if(!list.isEmpty() && list.at(0))
	m_favorites_model->removeRow(list.at(0)->row());
    }

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
	   value(dooble_history::HistoryItem::LAST_VISITED).
	   toDateTime().toString(Qt::ISODate).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (QByteArray::
	   number(hash.value(dooble_history::HistoryItem::NUMBER_OF_VISITS, 1).
		  toULongLong()));

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	if(hash.value(dooble_history::HistoryItem::TITLE).
	   toString().trimmed().isEmpty())
	  bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());
	else
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (hash.value(dooble_history::HistoryItem::TITLE).
	     toString().trimmed().toUtf8());

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
  QHash<dooble_history::HistoryItem, QVariant> hash;

  if(item.isValid())
    {
      QWriteLocker locker(&m_history_mutex);
      auto const contains = m_history.contains(item.url());

      if(icon.isNull())
	hash[dooble_history::HistoryItem::FAVICON] =
	  dooble_favicons::icon(item.url());
      else
	hash[dooble_history::HistoryItem::FAVICON] = icon;

      hash[dooble_history::HistoryItem::FAVORITE] =
	m_history.value(item.url()).
	value(dooble_history::HistoryItem::FAVORITE, false);
      hash[dooble_history::HistoryItem::LAST_VISITED] = item.lastVisited();
      hash[dooble_history::HistoryItem::NUMBER_OF_VISITS] =
	m_history.value(item.url()).
	value(dooble_history::HistoryItem::NUMBER_OF_VISITS, 1).
	toULongLong() + 1;

      if(hash.value(dooble_history::HistoryItem::FAVORITE).toBool())
	hash[dooble_history::HistoryItem::TITLE] =
	  m_history.value(item.url()).value
	  (dooble_history::HistoryItem::TITLE,
	   item.title().trimmed().mid
	   (0, static_cast<int> (dooble::Limits::MAXIMUM_TITLE_LENGTH)));
      else
	hash[dooble_history::HistoryItem::TITLE] = item.title().trimmed().mid
	  (0,
	   static_cast<int> (dooble::Limits::MAXIMUM_TITLE_LENGTH));

      hash[dooble_history::HistoryItem::URL] = item.url();

      if(dooble::s_cryptography)
	hash[dooble_history::HistoryItem::URL_DIGEST] =
	  dooble::s_cryptography->hmac(item.url().toEncoded());

      m_history[item.url()] = hash;

      if(!m_history_date_time.contains(item.lastVisited(), item.url()))
	m_history_date_time.insert(item.lastVisited(), item.url());

      locker.unlock();
      update_favorite(hash);

      if(!contains)
	emit new_item
	  (hash.value(dooble_history::HistoryItem::FAVICON).value<QIcon> (),
	   item);
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

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
	     hmac(m_history.
		  value(item.url()).value(dooble_history::HistoryItem::FAVORITE,
					  false).toBool() ?
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
				value(dooble_history::HistoryItem::
				      NUMBER_OF_VISITS, 1).toULongLong()));
	}

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	auto const title
	  (hash.value(dooble_history::HistoryItem::TITLE).toString().trimmed());

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

void dooble_history::save_session_tabs(const QList<QUrl> &urls)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_previous_session_tabs");

	if(dooble::s_cryptography &&
	   dooble_settings::setting("retain_session_tabs").toBool())
	  {
	    foreach(auto const &url, urls)
	      if(!url.isEmpty() && url.isValid())
		{
		  query.prepare
		    ("INSERT INTO dooble_previous_session_tabs "
		     "(url, url_digest) VALUES (?, ?)");
		  query.addBindValue
		    (dooble::s_cryptography->encrypt_then_mac(url.toEncoded()).
		     toBase64());
		  query.addBindValue
		    (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
		  query.exec();
		}
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
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
    m_history_date_time.clear();
  }

  QApplication::restoreOverrideCursor();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_populate_future = QtConcurrent::run
    (this,
     &dooble_history::populate,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
#else
  m_populate_future = QtConcurrent::run
    (&dooble_history::populate,
     this,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
#endif
}

void dooble_history::slot_populated_favorites
(const QListVectorByteArray &favorites)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(auto const &favorite, favorites)
    {
      QList<QStandardItem *> list;
      auto const last_visited(favorite.value(2));
      auto const number_of_visits(favorite.value(3));
      auto const title(favorite.value(0));
      auto const url(favorite.value(1));

      for(int j = 0; j < m_favorites_model->columnCount(); j++)
	{
	  auto item = new QStandardItem();

	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	  switch(j)
	    {
	    case 0:
	      {
		item->setData(QUrl::fromEncoded(url));
		item->setText(title);
		item->setToolTip
		  (dooble_ui_utilities::pretty_tool_tip(item->text()));
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

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_purge_future = QtConcurrent::run
    (this,
     &dooble_history::purge,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
#else
  m_purge_future = QtConcurrent::run
    (&dooble_history::purge,
     this,
     dooble::s_cryptography->keys().first,
     dooble::s_cryptography->keys().second);
#endif
}

void dooble_history::slot_remove_items(const QListUrl &urls)
{
  if(urls.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  {
    QWriteLocker locker(&m_history_mutex);

    foreach(auto const &url, urls)
      {
	m_history_date_time.remove
	  (m_history.value(url).value
	   (dooble_history::HistoryItem::LAST_VISITED).toDateTime(),
	   url);
	m_history.remove(url);
      }
  }

  QApplication::restoreOverrideCursor();
}

void dooble_history::update_favorite
(const QHash<dooble_history::HistoryItem, QVariant> &hash)
{
  if(!hash.value(dooble_history::HistoryItem::FAVORITE, false).toBool())
    return;

  auto const url(hash.value(dooble_history::HistoryItem::URL).toUrl());

  if(url.isEmpty() || !url.isValid())
    return;

  auto list(m_favorites_model->findItems(url.toString(), Qt::MatchExactly, 1));

  if(list.isEmpty())
    {
      for(int i = 0; i < m_favorites_model->columnCount(); i++)
	{
	  auto item = new QStandardItem();

	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	  switch(i)
	    {
	    case 0:
	      {
		item->setData(url);
		item->setIcon
		  (hash.value(dooble_history::HistoryItem::FAVICON).
		   value<QIcon> ());

		if(hash.value(dooble_history::HistoryItem::TITLE).
		   toString().trimmed().isEmpty())
		  item->setText
		    (hash.value(dooble_history::HistoryItem::URL).toString());
		else
		  item->setText
		    (hash.value(dooble_history::HistoryItem::TITLE).toString());

		item->setToolTip
		  (dooble_ui_utilities::pretty_tool_tip(item->text()));
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
		  (hash.value(dooble_history::HistoryItem::LAST_VISITED).
		   toDateTime().toString(Qt::ISODate));
		break;
	      }
	    case 3:
	      {
		item->setText
		  (QString::
		   number(hash.
			  value(dooble_history::HistoryItem::NUMBER_OF_VISITS).
			  toULongLong()).rightJustified(16, '0'));
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
      auto item = list.at(0);

      if(!item)
	return;

      auto const row = item->row();

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
		item->setIcon
		  (hash.value(dooble_history::HistoryItem::FAVICON).
		   value<QIcon> ());

		if(hash.value(dooble_history::HistoryItem::TITLE).
		   toString().trimmed().isEmpty())
		  item->setText
		    (hash.value(dooble_history::HistoryItem::URL).toString());
		else
		  item->setText
		    (hash.value(dooble_history::HistoryItem::TITLE).toString());

		item->setToolTip
		  (dooble_ui_utilities::pretty_tool_tip(item->text()));
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
		  (hash.value(dooble_history::HistoryItem::LAST_VISITED).
		   toDateTime().toString(Qt::ISODate));
		break;
	      }
	    case 3:
	      {
		item->setText
		  (QString::
		   number(hash.
			  value(dooble_history::HistoryItem::NUMBER_OF_VISITS).
			  toULongLong()).rightJustified(16, '0'));
		break;
	      }
	    }
	}
    }
}
