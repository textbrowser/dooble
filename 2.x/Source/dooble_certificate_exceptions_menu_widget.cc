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

#include "dooble.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_ui_utilities.h"

QAtomicInteger<qintptr> dooble_certificate_exceptions_menu_widget::s_db_id = 0;

dooble_certificate_exceptions_menu_widget::
dooble_certificate_exceptions_menu_widget(QWidget *parent):QWidget(parent)
{
  m_ui.setupUi(this);
  connect(m_ui.remove,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_exception(void)));
}

bool dooble_certificate_exceptions_menu_widget::has_exception(const QUrl &url)
{
  QString database_name(QString("dooble_certificate_exceptions_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));
  bool state = false;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT exception_accepted, OID FROM "
		      "dooble_certificate_exceptions WHERE url_digest "
		      "IN (?, ?)");
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded() + "/").toBase64());

	if(query.exec())
	  if(query.next())
	    {
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

	      if(!bytes.isEmpty())
		state = bytes == "true";
	      else
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_certificate_exceptions",
		     query.value(1).toLongLong());
		}
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  return state;
}

void dooble_certificate_exceptions_menu_widget::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_certificate_exceptions ("
	     "error TEXT NOT NULL, "
	     "exception_accepted TEXT NOT NULL, "
	     "temporary INTEGER NOT NULL DEFAULT 1, "
	     "url TEXT NOT NULL, "
	     "url_digest TEXT NOT NULL PRIMARY KEY)");
}

void dooble_certificate_exceptions_menu_widget::exception_accepted
(const QString &error, const QUrl &url)
{
  QString database_name(QString("dooble_certificate_exceptions_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT INTO dooble_certificate_exceptions "
	   "(error, exception_accepted, temporary, url, url_digest) "
	   "VALUES (?, ?, ?, ?, ?)");

	QByteArray bytes;
	bool ok = true;

	bytes = dooble::s_cryptography->encrypt_then_mac(error.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());

	bytes = dooble::s_cryptography->encrypt_then_mac("true");

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	query.addBindValue(dooble::s_cryptography->authenticated() ? 0 : 1);
	bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  ok = false;

	query.addBindValue
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());

	if(ok)
	  if(query.exec())
	    if(dooble::s_certificate_exceptions)
	      dooble::s_certificate_exceptions->exception_accepted(error, url);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_certificate_exceptions_menu_widget::purge(void)
{
  QString database_name(QString("dooble_certificate_exceptions_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_certificate_exceptions");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_certificate_exceptions_menu_widget::purge_temporary(void)
{
  QString database_name(QString("dooble_certificate_exceptions_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec
	  ("DELETE FROM dooble_certificate_exceptions WHERE temporary = 1");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_certificate_exceptions_menu_widget::set_url(const QUrl &url)
{
  m_url = QUrl(dooble_ui_utilities::simplified_url(url));

  if(m_url.scheme() == "https")
    {
      if(!m_url.isEmpty() && m_url.isValid())
	m_ui.label->setText
	  (tr("A security exception was accepted for %1.").
	   arg(m_url.toString()));
      else
	m_ui.label->setText
	  (tr("A security exception was accepted for this site."));
    }
  else
    m_ui.label->setText(tr("Expecting an HTTPS scheme."));
}

void dooble_certificate_exceptions_menu_widget::slot_remove_exception(void)
{
  QString database_name(QString("dooble_certificate_exceptions_%1").
			arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare
	  ("DELETE FROM dooble_certificate_exceptions WHERE url_digest "
	   "IN (?, ?)");
	query.addBindValue
	  (dooble::s_cryptography->hmac(m_url.toEncoded()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(m_url.toEncoded() + "/").toBase64());

	if(query.exec())
	  if(dooble::s_certificate_exceptions)
	    dooble::s_certificate_exceptions->remove_exception(m_url);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  emit triggered();
}
