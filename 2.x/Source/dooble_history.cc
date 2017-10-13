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

#include <QBuffer>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_settings.h"

QAtomicInteger<quintptr> dooble_history::s_db_id;

dooble_history::dooble_history(void):QObject()
{
  connect(dooble::s_application,
	  SIGNAL(containers_cleared(void)),
	  this,
	  SLOT(slot_containers_cleared(void)));
  connect(&m_purge_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_timer_timeout(void)));
  m_purge_timer.start(15000);
}

dooble_history::~dooble_history()
{
  abort();
}

QHash<QUrl, QHash<int, QVariant> > dooble_history::history(void) const
{
  QReadLocker locker(&m_history_mutex);

  return m_history;
}

QList<QPair<QIcon, QString> > dooble_history::urls(void) const
{
  QList<QPair<QIcon, QString> > list;
  QReadLocker locker(&m_history_mutex);

  {
    QHashIterator<QUrl, QHash<int, QVariant> > it(m_history);

    while(it.hasNext())
      {
	it.next();
	list << QPair<QIcon, QString>
	  (it.value().value(FAVICON).value<QIcon> (), it.key().toString());
      }
  }

  return list;
}

bool dooble_history::is_favorite(const QUrl &url) const
{
  QReadLocker locker(&m_history_mutex);

  return m_history.value(url).value(FAVORITE, false).toBool();
}

void dooble_history::abort(void)
{
  m_interrupt.store(1);
  m_purge_future.cancel();
  m_purge_future.waitForFinished();
}

void dooble_history::purge(const QByteArray &authentication_key,
			   const QByteArray &encryption_key)
{
  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);
	dooble_cryptography cryptography
	  (authentication_key,
	   encryption_key,
	   dooble_settings::setting("block_cipher_type").toString());

	query.setForwardOnly(true);
	query.exec("SELECT last_visited, url, url_digest "
		   "FROM dooble_history WHERE favorite = ?");
	query.addBindValue(cryptography.hmac(QByteArray("false")).toBase64());

	if(query.exec())
	  {
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
			  {
			    QWriteLocker locker(&m_history_mutex);

			    m_history.remove(QUrl(bytes));
			  }
		      }
		  }
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::purge(void)
{
  {
    QWriteLocker locker(&m_history_mutex);

    m_history.clear();
  }

  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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

void dooble_history::remove_item(const QUrl &url)
{
  QWriteLocker locker(&m_history_mutex);

  m_history.remove(url);
}

void dooble_history::save_favicon(const QIcon &icon, const QUrl &url)
{
  {
    QWriteLocker locker(&m_history_mutex);

    if(m_history.contains(url))
      {
	QHash<int, QVariant> hash(m_history.value(url));

	if(icon.isNull())
	  hash[FAVICON] = dooble_favicons::icon(url);
	else
	  hash[FAVICON] = icon;

	m_history[url] = hash;
	emit icon_updated(hash[FAVICON].value<QIcon> (), url);
      }
  }

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(dooble_settings::setting("browsing_history_days").toInt() == 0)
    return;

  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare
	  ("UPDATE dooble_history SET favicon = ? WHERE url_digest = ?");

	QBuffer buffer;
	QByteArray bytes;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    if(icon.isNull())
	      out << dooble_favicons::icon(url);
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	query.addBindValue(bytes.toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::save_favorite(const QUrl &url, bool state)
{
  if(url.isEmpty() || !url.isValid())
    return;

  QHash<int, QVariant> hash;
  QWriteLocker locker(&m_history_mutex);

  if(m_history.contains(url))
    {
      hash = m_history.value(url);
      hash[FAVORITE] = state;
      m_history[url] = hash;
    }

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_history ("
		   "favicon BLOB DEFAULT NULL, "
		   "favorite TEXT NOT NULL, "
		   "last_visited TEXT NOT NULL, "
		   "number_of_visits TEXT NOT NULL, "
		   "title TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT PRIMARY KEY NOT NULL)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_history "
	   "(favicon, "
	   "favorite, "
	   "last_visited, "
	   "number_of_visits, "
	   "title, "
	   "url, "
	   "url_digest) "
	   "VALUES (?, ?, ?, ?, ?, ?, ?)");

	QBuffer buffer;
	QByteArray bytes;
	bool ok = true;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);
	    QIcon icon(hash.value(FAVICON).value<QIcon> ());

	    if(icon.isNull())
	      out << dooble_favicons::icon(QUrl());
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	buffer.close();
	bytes = dooble::s_cryptography->encrypt_then_mac(bytes);

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(state ? QByteArray("true") : QByteArray("false")).toBase64());
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (hash.
	   value(LAST_VISITED).toDateTime().toString(Qt::ISODate).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (QByteArray::number(hash.value(NUMBER_OF_VISITS, 0).toULongLong()));

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	if(hash.value(TITLE).toString().trimmed().isEmpty())
	  bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());
	else
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (hash.value(TITLE).toString().trimmed().toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::save_item(const QIcon &icon,
			       const QWebEngineHistoryItem &item)
{
  if(item.isValid())
    {
      QWriteLocker locker(&m_history_mutex);

      if(!m_history.contains(item.url()))
	{
	  QHash<int, QVariant> hash;

	  if(icon.isNull())
	    hash[FAVICON] = dooble_favicons::icon(item.url());
	  else
	    hash[FAVICON] = icon;

	  hash[FAVORITE] = false;
	  hash[LAST_VISITED] = item.lastVisited();
	  hash[NUMBER_OF_VISITS] = 1ULL;
	  hash[TITLE] = item.title();
	  hash[URL] = item.url();

	  if(dooble::s_cryptography)
	    hash[URL_DIGEST] = dooble::s_cryptography->hmac
	      (item.url().toEncoded());

	  m_history[item.url()] = hash;
	  emit new_item(hash[FAVICON].value<QIcon> (), item);
	}
      else
	{
	  QHash<int, QVariant> hash(m_history.value(item.url()));

	  hash[NUMBER_OF_VISITS] =
	    hash.value(NUMBER_OF_VISITS, 1).toULongLong() + 1;
	  m_history[item.url()] = hash;
	  emit item_updated(icon, item);
	}
    }

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(dooble_settings::setting("browsing_history_days").toInt() == 0)
    return;
  else if(!item.isValid())
    return;

  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_history ("
		   "favicon BLOB DEFAULT NULL, "
		   "favorite TEXT NOT NULL, "
		   "last_visited TEXT NOT NULL, "
		   "number_of_visits TEXT NOT NULL, "
		   "title TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT PRIMARY KEY NOT NULL)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_history "
	   "(favicon, "
	   "favorite, "
	   "last_visited, "
	   "number_of_visits, "
	   "title, "
	   "url, "
	   "url_digest) "
	   "VALUES (?, ?, ?, ?, ?, ?, ?)");

	QBuffer buffer;
	QByteArray bytes;
	bool ok = true;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    if(icon.isNull())
	      out << dooble_favicons::icon(QUrl());
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	buffer.close();
	bytes = dooble::s_cryptography->encrypt_then_mac(bytes);

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

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
	  ok = false;

	{
	  QReadLocker locker(&m_history_mutex);

	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (QByteArray::number(m_history.value(item.url()).
				value(NUMBER_OF_VISITS, 0).toULongLong()));
	}

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	if(item.title().trimmed().isEmpty())
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (item.url().toEncoded());
	else
	  bytes = dooble::s_cryptography->encrypt_then_mac
	    (item.title().trimmed().toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item.url().toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	query.addBindValue
	  (dooble::s_cryptography->hmac(item.url().toEncoded()).toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_history::slot_containers_cleared(void)
{
  QWriteLocker locker(&m_history_mutex);

  m_history.clear();
}

void dooble_history::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      emit populated();
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  {
    QWriteLocker locker(&m_history_mutex);

    m_history.clear();
  }

  QString database_name(QString("dooble_history_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);
	int days = dooble_settings::setting("browsing_history_days").toInt();

	query.setForwardOnly(true);

	if(query.exec("SELECT favicon, "   // 0
		      "favorite, "         // 1
		      "last_visited, "     // 2
		      "number_of_visits, " // 3
		      "title, "            // 4
		      "url, "              // 5
		      "url_digest "        // 6
		      "FROM dooble_history"))
	  while(query.next())
	    {
	      QBuffer buffer;
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));
	      QIcon icon;

	      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);
	      buffer.setBuffer(&bytes);

	      if(buffer.open(QIODevice::ReadOnly))
		{
		  QDataStream in(&buffer);

		  in >> icon;

		  if(in.status() != QDataStream::Ok)
		    icon = QIcon();

		  buffer.close();
		}

	      QByteArray last_visited
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      last_visited = dooble::s_cryptography->mac_then_decrypt
		(last_visited);

	      if(last_visited.isEmpty())
		continue;

	      bool is_favorite = dooble_cryptography::memcmp
		(dooble::s_cryptography->hmac(QByteArray("true")).toBase64(),
		 query.value(1).toByteArray());

	      if(!is_favorite)
		{
		  QDateTime dateTime
		    (QDateTime::
		     fromString(last_visited.constData(), Qt::ISODate));
		  QDateTime now(QDateTime::currentDateTime());

		  if(dateTime.daysTo(now) >= qAbs(days))
		    /*
		    ** Ignore expired entries, unless the entry is a favorite.
		    */

		    continue;
		}

	      QByteArray number_of_visits
		(QByteArray::fromBase64(query.value(3).toByteArray()));

	      number_of_visits = dooble::s_cryptography->mac_then_decrypt
		(number_of_visits);

	      if(number_of_visits.isEmpty())
		continue;

	      QByteArray title
		(QByteArray::fromBase64(query.value(4).toByteArray()));

	      title = dooble::s_cryptography->mac_then_decrypt(title);

	      if(title.isEmpty())
		continue;

	      QByteArray url
		(QByteArray::fromBase64(query.value(5).toByteArray()));

	      url = dooble::s_cryptography->mac_then_decrypt(url);

	      if(url.isEmpty())
		continue;

	      if(icon.isNull())
		icon = dooble_favicons::icon(QUrl::fromEncoded(url));

	      QHash<int, QVariant> hash;

	      hash[FAVICON] = icon;
	      hash[FAVORITE] = is_favorite;
	      hash[LAST_VISITED] = QDateTime::fromString
		(last_visited.constData(), Qt::ISODate);
	      hash[NUMBER_OF_VISITS] = qMax
		(1ULL, number_of_visits.toULongLong());
	      hash[TITLE] = title.constData();
	      hash[URL] = QUrl::fromEncoded(url);
	      hash[URL_DIGEST] = QByteArray::fromBase64
		(query.value(6).toByteArray());

	      QWriteLocker locker(&m_history_mutex);

	      m_history[hash[URL].toUrl()] = hash;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
  emit populated();
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
