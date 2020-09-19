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
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_cookies.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"

dooble_cookies::dooble_cookies(bool is_private, QObject *parent):QObject(parent)
{
  m_is_private = is_private;
}

QByteArray dooble_cookies::identifier(const QNetworkCookie &cookie)
{
  QByteArray bytes;

  bytes.append(cookie.domain().toUtf8());
  bytes.append(cookie.name());
  bytes.append(cookie.path().toUtf8());
  return bytes;
}

void dooble_cookies::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_cookies_domains ("
	     "domain TEXT NOT NULL, "
	     "domain_digest TEXT NOT NULL PRIMARY KEY, "
	     "favorite_digest TEXT NOT NULL)");
  query.exec("CREATE TABLE IF NOT EXISTS dooble_cookies ("
	     "domain_digest TEXT NOT NULL, "
	     "identifier_digest TEXT NOT NULL, "
	     "raw_form BLOB NOT NULL, "
	     "PRIMARY KEY (domain_digest, identifier_digest), "
	     "FOREIGN KEY (domain_digest) REFERENCES "
	     "dooble_cookies_domains (domain_digest) ON DELETE CASCADE "
	     ")");
}

void dooble_cookies::purge(void)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA foreign_keys = ON");
	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_cookies");
	query.exec("DELETE FROM dooble_cookies_domains");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_connect_cookie_added_signal(void)
{
  connect(QWebEngineProfile::defaultProfile()->cookieStore(),
	  SIGNAL(cookieAdded(const QNetworkCookie &)),
	  dooble::s_cookies,
	  SLOT(slot_cookie_added(const QNetworkCookie &)),
	  Qt::UniqueConnection);
  emit populated();
}

void dooble_cookies::slot_cookie_added(const QNetworkCookie &cookie)
{
  emit cookies_added
    (QList<QNetworkCookie> () << cookie, QList<bool> () << false);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_is_private)
    return;

  QDateTime now;

  if(cookie.isSessionCookie())
    {
      if(dooble_settings::cookie_policy_string(dooble_settings::
					       setting("cookie_policy_index").
					       toInt()) == "save_all")
	/*
	** Allow a session cookie to be saved.
	*/

	goto save_label;
      else
	return;
    }
  else if(dooble_settings::
	  cookie_policy_string(dooble::s_settings->
			       setting("cookie_policy_index").
			       toInt()) == "do_not_save")
    return;

  now = QDateTime::currentDateTime();

  if(cookie.expirationDate().toLocalTime() <= now)
    return;

 save_label:

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT INTO dooble_cookies_domains "
	   "(domain, domain_digest, favorite_digest) VALUES (?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (cookie.domain().toUtf8());

	if(!bytes.isEmpty())
	  {
	    query.addBindValue(bytes.toBase64());
	    query.addBindValue
	      (dooble::s_cryptography->hmac(cookie.domain()).toBase64());
	    query.addBindValue
	      (dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
	    query.exec();
	  }

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_cookies "
	   "(domain_digest, identifier_digest, raw_form) VALUES (?, ?, ?)");
	query.addBindValue
	  (dooble::s_cryptography->hmac(cookie.domain()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(identifier(cookie)).toBase64());
	bytes = dooble::s_cryptography->encrypt_then_mac(cookie.toRawForm());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_cookie_removed(const QNetworkCookie &cookie)
{
  emit cookie_removed(cookie);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_is_private)
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare("DELETE FROM dooble_cookies WHERE identifier_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(identifier(cookie)).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_delete_cookie(const QNetworkCookie &cookie)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_is_private)
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare("DELETE FROM dooble_cookies WHERE identifier_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(identifier(cookie)).toBase64());
	query.exec();
	query.prepare("DELETE FROM dooble_cookies_domains WHERE "
		      "domain_digest NOT IN (SELECT domain_digest FROM "
		      "dooble_cookies) AND favorite_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_delete_domain(const QString &domain)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_is_private)
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA foreign_keys = ON");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare("DELETE FROM dooble_cookies_domains WHERE "
		      "domain_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(domain.toUtf8()).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_delete_items(const QList<QNetworkCookie> &cookies,
				       const QStringList &domains)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(m_is_private)
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	for(const auto &cookie : cookies)
	  {
	    query.prepare
	      ("DELETE FROM dooble_cookies WHERE identifier_digest = ?");
	    query.addBindValue
	      (dooble::s_cryptography->
	       hmac(identifier(cookie)).toBase64());
	    query.exec();
	  }

	query.prepare("DELETE FROM dooble_cookies_domains WHERE "
		      "domain_digest NOT IN (SELECT domain_digest FROM "
		      "dooble_cookies) AND favorite_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(QByteArray("false")).toBase64());
	query.exec();
	query.exec("PRAGMA foreign_keys = ON");

	for(int i = 0; i < domains.size(); i++)
	  {
	    query.prepare("DELETE FROM dooble_cookies_domains WHERE "
			  "domain_digest = ?");
	    query.addBindValue
	      (dooble::s_cryptography->hmac(domains.at(i).toUtf8()).toBase64());
	    query.exec();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      emit populated();
      return;
    }
  else if(m_is_private)
    {
      emit populated();
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name(dooble_database_utilities::database_name());
  auto *profile = QWebEngineProfile::defaultProfile();
  int count = 0;

  disconnect(profile->cookieStore(),
	     SIGNAL(cookieAdded(const QNetworkCookie &)),
	     dooble::s_cookies,
	     SLOT(slot_cookie_added(const QNetworkCookie &)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	create_tables(db);

	QList<QNetworkCookie> cookies;
	QList<bool> is_favorites;
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT domain, favorite_digest FROM "
		      "dooble_cookies_domains"))
	  while(query.next())
	    {
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

	      if(bytes.isEmpty())
		{
		  QSqlQuery delete_query(db);

		  delete_query.exec("PRAGMA foreign_keys = ON");
		  delete_query.prepare
		    ("DELETE FROM dooble_cookies_domains WHERE domain = ?");
		  delete_query.addBindValue(query.value(0));
		  delete_query.exec();
		  continue;
		}

	      QNetworkCookie cookie;
	      bool is_favorite = dooble_cryptography::memcmp
		(dooble::s_cryptography->hmac(QByteArray("true")).toBase64(),
		 query.value(1).toByteArray());

	      cookie.setDomain(bytes);
	      cookies << cookie;
	      is_favorites << is_favorite;
	    }

	if(!cookies.isEmpty() && !is_favorites.isEmpty())
	  emit cookies_added(cookies, is_favorites);

	cookies.clear();
	is_favorites.clear();

	if(query.exec("SELECT "
		      "(SELECT favorite_digest FROM dooble_cookies_domains a "
		      "WHERE a.domain_digest = b.domain_digest) "
		      "AS favorite_digest, "
		      "raw_form FROM dooble_cookies b"))
	  while(query.next())
	    {
	      QByteArray bytes
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

	      if(bytes.isEmpty())
		{
		  QSqlQuery delete_query(db);

		  delete_query.prepare
		    ("DELETE FROM dooble_cookies WHERE raw_form = ?");
		  delete_query.addBindValue(query.value(1));
		  delete_query.exec();
		  delete_query.prepare
		    ("DELETE FROM dooble_cookies_domains WHERE "
		     "domain_digest NOT IN (SELECT domain_digest FROM "
		     "dooble_cookies) AND favorite_digest = ?");
		  delete_query.addBindValue
		    (dooble::s_cryptography->hmac(QByteArray("false")).
		     toBase64());
		  delete_query.exec();
		  continue;
		}

	      QList<QNetworkCookie> cookie = QNetworkCookie::parseCookies
		(bytes);

	      if(cookie.isEmpty())
		{
		  QSqlQuery delete_query(db);

		  delete_query.prepare
		    ("DELETE FROM dooble_cookies WHERE raw_form = ?");
		  delete_query.addBindValue(query.value(1));
		  delete_query.exec();
		  delete_query.prepare
		    ("DELETE FROM dooble_cookies_domains WHERE "
		     "domain_digest NOT IN (SELECT domain_digest FROM "
		     "dooble_cookies) AND favorite_digest = ?");
		  delete_query.addBindValue
		    (dooble::s_cryptography->hmac(QByteArray("false")).
		     toBase64());
		  delete_query.exec();
		  continue;
		}

	      QDateTime now(QDateTime::currentDateTime());
	      bool allow_expired = false;

	      if(cookie.at(0).isSessionCookie())
		{
		  if(dooble_settings::
		     cookie_policy_string(dooble_settings::
					  setting("cookie_policy_index").
					  toInt()) == "save_all")
		    /*
		    ** Ignore the session cookie's expiration date.
		    */

		    allow_expired = true;
		}
	      else
		allow_expired = false;

	      if(!allow_expired)
		if(cookie.at(0).expirationDate().toLocalTime() <= now)
		  {
		    QSqlQuery delete_query(db);

		    delete_query.prepare
		      ("DELETE FROM dooble_cookies WHERE raw_form = ?");
		    delete_query.addBindValue(query.value(1));
		    delete_query.exec();
		    delete_query.prepare
		      ("DELETE FROM dooble_cookies_domains WHERE "
		       "domain_digest NOT IN (SELECT domain_digest FROM "
		       "dooble_cookies) AND favorite_digest = ?");
		    delete_query.addBindValue
		      (dooble::s_cryptography->hmac(QByteArray("false")).
		       toBase64());
		    delete_query.exec();
		    continue;
		  }

	      QNetworkCookie c(cookie.at(0));
	      bool is_favorite = dooble_cryptography::memcmp
		(dooble::s_cryptography->hmac(QByteArray("true")).toBase64(),
		 query.value(0).toByteArray());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	      c.setName(c.name().replace('-', '_'));
#endif
	      cookies << c;
	      count += 1;
	      is_favorites << is_favorite;
	      profile->cookieStore()->setCookie(c);
	    }

	if(!cookies.isEmpty() && !is_favorites.isEmpty())
	  emit cookies_added(cookies, is_favorites);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();

  /*
  ** Re-connect the cookieAdded() signal.
  */

  QTimer::singleShot(count, this, SLOT(slot_connect_cookie_added_signal(void)));
  emit populated();
}
