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
#include "dooble_favicons.h"

QAtomicInteger<quint64> dooble_favicons::s_db_id;

QIcon dooble_favicons::icon(const QUrl &url)
{
  Q_UNUSED(url);
  return QIcon();
}

void dooble_favicons::save_icon(const QIcon &icon, const QUrl &url)
{
  QString database_name(QString("dooble_favicons_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_favicons ("
		   "favicon BLOB DEFAULT NULL, "
		   "url_digest TEXT PRIMARY KEY NOT NULL)");
	query.exec("CREATE INDEX IF NOT EXISTS url_digest_index ON "
		   "dooble_favicons (url_digest)");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare("INSERT OR REPLACE INTO dooble_favicons "
		      "(favicon, url_digest) VALUES (?, ?)");

	QBuffer buffer;
	QByteArray bytes;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    if(url.scheme().toLower().trimmed() == "ftp" ||
	       url.scheme().toLower().trimmed() == "gopher")
	      out << icon.pixmap(icon.actualSize(QSize(16, 16)),
				 QIcon::Normal,
				 QIcon::On);
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	buffer.close();
	query.addBindValue(bytes);
	query.addBindValue(url.toString().trimmed());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
