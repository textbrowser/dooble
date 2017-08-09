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

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_settings.h"

QAtomicInteger<quint64> dooble_favicons::s_db_id;

QIcon dooble_favicons::icon(const QUrl &url)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return QIcon();

  QIcon icon;
  QString database_name(QString("dooble_favicons_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT favicon FROM dooble_favicons WHERE "
		      "url_digest IN (?, ?) OR url_host_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toString()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toString() + "/").toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.host()));

	if(query.exec() && query.next())
	  if(!query.isNull(0))
	    {
	      QBuffer buffer;
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));

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
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  return icon;
}

void dooble_favicons::purge_temporary(void)
{
  QString database_name(QString("dooble_favicons_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("DELETE FROM dooble_favicons WHERE temporary = 1");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_favicons::save_icon(const QIcon &icon, const QUrl &url)
{
  if(!dooble::s_cryptography)
    return;

  if(icon.isNull())
    return;

  QString database_name(QString("dooble_favicons_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_favicons ("
		   "favicon BLOB DEFAULT NULL, "
		   "temporary INTEGER NOT NULL DEFAULT 1, "
		   "url_digest TEXT PRIMARY KEY NOT NULL, "
		   "url_host_digest TEXT NOT NULL)");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_favicons "
	   "(favicon, temporary, url_digest, url_host_digest) "
	   "VALUES (?, ?, ?, ?)");

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

	query.addBindValue(dooble::s_cryptography->authenticated() ? 0 : 1);
	bytes = dooble::s_cryptography->hmac(url.toString().trimmed());
	ok &= !bytes.isEmpty();

	if(ok)
	  query.addBindValue(bytes.toBase64());

	bytes = dooble::s_cryptography->hmac(url.host().trimmed());
	ok &= !bytes.isEmpty();

	if(ok)
	  query.addBindValue(bytes.toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
