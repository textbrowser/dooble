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
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_favicons.h"
#include "dooble_search_engines_popup.h"

QIcon dooble_favicons::icon(const QUrl &url)
{
  if(!dooble::s_cryptography)
    return QIcon(":/Miscellaneous/blank_page.png");
  else if(url == QUrl("about:blank") || url.isEmpty() || !url.isValid())
    return QIcon(":/Logo/dooble.png");

  QIcon icon;
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT favicon, OID FROM dooble_favicons WHERE "
		      "url_digest IN (?, ?)");
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded() + "/").toBase64());

	if(query.exec() && query.next())
	  if(!query.isNull(0))
	    {
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

	      if(!bytes.isEmpty())
		{
		  QBuffer buffer;

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
	      else
		dooble_database_utilities::remove_entry
		  (db,
		   "dooble_favicons",
		   query.value(1).toLongLong());
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);

  if(icon.isNull())
    icon = QIcon(":/Miscellaneous/blank_page.png");

  return icon;
}

void dooble_favicons::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_favicons ("
	     "favicon BLOB DEFAULT NULL, "
	     "temporary INTEGER NOT NULL DEFAULT 1, "
	     "url_digest TEXT PRIMARY KEY NOT NULL, "
	     "url_host_digest NOT NULL)");
  query.exec
    ("CREATE INDEX IF NOT EXISTS dooble_favicons_index_url_digest ON "
     "dooble_favicons (url_digest)");
  query.exec
    ("CREATE INDEX IF NOT EXISTS dooble_favicons_index_url_host ON "
     "dooble_favicons (url_host_digest)");
}

void dooble_favicons::purge(void)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_favicons");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_favicons::purge_temporary(void)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_favicons WHERE temporary = 1");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_favicons::save_favicon(const QIcon &icon, const QUrl &url)
{
  if(dooble::s_search_engines_window)
    dooble::s_search_engines_window->set_icon(icon, url);

  if(!dooble::s_cryptography || icon.isNull())
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_favicons "
	   "(favicon, temporary, url_digest, url_host_digest) "
	   "VALUES (?, ?, ?, ?)");

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

	buffer.close();
	bytes = dooble::s_cryptography->encrypt_then_mac(bytes);

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue(dooble::s_cryptography->authenticated() ? 0 : 1);
	bytes = dooble::s_cryptography->hmac(url.toEncoded());

	if(bytes.isEmpty())
	  goto done_label;

	query.addBindValue(bytes.toBase64());
	bytes = dooble::s_cryptography->hmac(url.host());

	if(bytes.isEmpty())
	  goto done_label;

	query.addBindValue(bytes.toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
