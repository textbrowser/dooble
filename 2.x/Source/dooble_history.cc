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
#include "dooble_cryptography.h"
#include "dooble_history.h"
#include "dooble_settings.h"

QAtomicInteger<quint64> dooble_history::s_db_id;

dooble_history::dooble_history(void):QObject()
{
  connect(&m_purge_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_timer_timeout(void)));
  m_purge_timer.start(15000);
}

dooble_history::~dooble_history()
{
  m_interrupt.fetchAndStoreOrdered(1);
  m_purge_future.cancel();
  m_purge_future.waitForFinished();
}

QHash<QUrl, QHash<int, QVariant> > dooble_history::history(void) const
{
  return m_history;
}

QList<QPair<QIcon, QString> > dooble_history::urls(void) const
{
  QHashIterator<QUrl, QHash<int, QVariant> > it(m_history);
  QList<QPair<QIcon, QString> > list;

  while(it.hasNext())
    {
      it.next();
      list << QPair<QIcon, QString>
	(it.value().value(FAVICON).value<QIcon> (), it.key().toString());
    }

  return list;
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

	query.setForwardOnly(true);

	if(query.exec("SELECT last_visited, url_digest FROM dooble_history"))
	  {
	    dooble_cryptography cryptography
	      (authentication_key, encryption_key);
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
		    delete_query.addBindValue(query.value(1));
		    delete_query.exec();
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

void dooble_history::save_favicon(const QIcon &icon, const QUrl &url)
{
  if(!icon.isNull())
    if(m_history.contains(url))
      {
	QHash<int, QVariant> hash(m_history.value(url));

	hash[FAVICON] = icon;
	m_history[url] = hash;
	emit icon_updated(icon, url);
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

	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("UPDATE dooble_history SET favicon = ? WHERE url_digest = ?");

	QBuffer buffer;
	QByteArray bytes;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
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
      if(!m_history.contains(item.url()))
	{
	  QHash<int, QVariant> hash;

	  hash[FAVICON] = icon;
	  hash[LAST_VISITED] = item.lastVisited();
	  hash[TITLE] = item.title();
	  hash[URL] = item.url();

	  if(dooble::s_cryptography)
	    hash[URL_DIGEST] = dooble::s_cryptography->hmac
	      (item.url().toEncoded());

	  m_history[item.url()] = hash;
	  emit new_item(icon, item);
	}
      else
	emit item_updated(icon, item);
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
		   "last_visited TEXT NOT NULL, "
		   "title TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT PRIMARY KEY NOT NULL)");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare("INSERT OR REPLACE INTO dooble_history "
		      "(favicon, last_visited, title, url, url_digest) "
		      "VALUES (?, ?, ?, ?, ?)");

	QBuffer buffer;
	QByteArray bytes;
	bool ok = true;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

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

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item.lastVisited().toString(Qt::ISODate).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	bytes = dooble::s_cryptography->encrypt_then_mac(item.title().toUtf8());

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

void dooble_history::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_history.clear();

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

	query.setForwardOnly(true);

	if(query.exec("SELECT last_visited, title, url, url_digest "
		      "FROM dooble_history"))
	  while(query.next())
	    {
	      QByteArray data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		continue;

	      QByteArray data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		continue;

	      QByteArray data3
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      data3 = dooble::s_cryptography->mac_then_decrypt(data3);

	      if(data3.isEmpty())
		continue;

	      QHash<int, QVariant> hash;

	      hash[LAST_VISITED] = QDateTime::fromString
		(data1.constData(), Qt::ISODate);
	      hash[TITLE] = data2.constData();
	      hash[URL] = QUrl::fromEncoded(data3);
	      hash[URL_DIGEST] = QByteArray::fromBase64
		(query.value(3).toByteArray());
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
