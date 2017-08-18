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
}

QHash<QUrl, QHash<int, QVariant> > dooble_history::history(void) const
{
  return m_history;
}

void dooble_history::save_item(const QIcon &icon,
			       const QWebEngineHistoryItem &item)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
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
}
