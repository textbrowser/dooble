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
#include <QProgressBar>
#include <QNetworkCookie>
#include <QSqlDatabase>
#include <QSqlQuery>
#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#endif
#include <QtCore>

#include "dcookies.h"
#include "dmisc.h"
#include "dooble.h"

dcookies::dcookies(const bool arePrivate, QObject *parent):
  QNetworkCookieJar(parent)
{
  /*
  ** settingsWindow/cookiesShouldBe
  ** 0 - deleted upon exit
  ** 1 - kept forever (that's a long time)
  ** 2 - kept until they expire
  */

  m_arePrivate = arePrivate;
  m_stopWriteThread = false;
  m_writeTimer = 0;
  m_timer = new QTimer(this);
  connect(m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotRemoveDomains(void)));
  connect(this,
	  SIGNAL(cookieReceived(const QString &,
				const QUrl &,
				const QDateTime &)),
	  dooble::s_cookiesBlockWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));

  int interval = computedTimerInterval();

  if(interval > 0)
    m_timer->start(interval);

  createCookieDatabase();
}

dcookies::~dcookies()
{
  if(!m_arePrivate)
    m_future.waitForFinished();
}

QList<QNetworkCookie> dcookies::allCookies(void) const
{
  /*
  ** If cookies are disabled, should this method return an empty
  ** list or the current list? For now, it will return the
  ** current list of cookies minus the expired cookies.
  */

  QList<QNetworkCookie> list(QNetworkCookieJar::allCookies());

  if(dooble::s_settings.value("settingsWindow/cookiesShouldBe", 0).
     toInt() == 2)
    {
      QDateTime now(QDateTime::currentDateTime());

      /*
      ** Remove expired cookies.
      */

      for(int i = list.size() - 1; i >= 0; i--)
	if(list.at(i).isSessionCookie())
	  continue;
	else if(list.at(i).expirationDate().toLocalTime() < now)
	  list.removeAt(i);
    }

  return list;
}

QList<QNetworkCookie> dcookies::cookiesForUrl(const QUrl &url) const
{
  /*
  ** If cookies are disabled, should this method return an empty
  ** list or the current list? For now, it will return the
  ** current list of cookies minus the expired cookies.
  */

  QList<QNetworkCookie> list(QNetworkCookieJar::cookiesForUrl(url));

  if(url.scheme().toLower().trimmed() == "http")
    for(int i = list.size() - 1; i >= 0; i--)
      if(list.at(i).isSecure())
	{
	  dmisc::logError
	    (QString("Secure cookie %1 removed because "
		     "%2 is requesting it.").
	     arg(list.at(i).toRawForm().constData()).arg(url.toString()));
	  list.removeAt(i);
	}

  if(dooble::s_settings.value("settingsWindow/cookiesShouldBe", 0).
     toInt() == 2)
    {
      QDateTime now(QDateTime::currentDateTime());

      /*
      ** Remove expired cookies.
      */

      for(int i = list.size() - 1; i >= 0; i--)
	if(list.at(i).isSessionCookie())
	  continue;
	else if(list.at(i).expirationDate().toLocalTime() < now)
	  list.removeAt(i);
    }

  return list;
}

bool dcookies::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
				 const QUrl &url)
{
  if(!dooble::s_settings.value("settingsWindow/cookiesEnabled",
			       true).toBool())
    return false;
  else if(cookieList.isEmpty())
    return false;

  /*
  ** Other exceptions models are only populated if some
  ** action has been denied. With cookies, knowledge of the
  ** cookies may be beneficial.
  */

  QDateTime now(QDateTime::currentDateTime());
  bool httpOnly = dooble::s_settings.value
    ("settingsWindow/httpOnlyCookies", false).toBool();

  emit cookieReceived(url.host(), url, now);

  if(httpOnly)
    {
      emit exceptionRaised
	(dooble::s_httpOnlyExceptionsWindow, url.host());
      emit httpCookieReceived(url.host(), url, now);
    }

  if(dooble::s_cookiesBlockWindow->allowed(url.host()))
    if(dooble::s_cookiesBlockWindow->approach() == "exempt")
      {
	emit exceptionRaised(dooble::s_cookiesBlockWindow, url);
	return false;
      }

  /*
  ** Now proceed with adding new cookies to the jar.
  */

  bool added = false;
#if QT_VERSION < 0x050000
  int thirdPartyCookiesPolicy = dooble::s_settings.value
    ("settingsWindow/thirdPartyCookiesPolicy", 1).toInt();
#endif

  for(int i = 0; i < cookieList.size(); i++)
    {
#if QT_VERSION < 0x050000
      if(thirdPartyCookiesPolicy != 0)
	if(url.host() != cookieList.at(i).domain())
	  if(cookieList.at(i).domain() != "" &&
	     cookieList.at(i).domain() != ".")
	    /*
	    ** Sorry, but third-party cookies are not
	    ** allowed. Goodbye!
	    */

	    continue;
#endif

      QNetworkCookie test(cookieList.at(i));

#if QT_VERSION < 0x050000
      if(test.domain().isEmpty())
	{
	  if(url.host().isEmpty())
	    continue;

	  test.setDomain(url.host().toLower());
	}

      if(test.path().isEmpty())
	test.setPath("/");
#else
      test.normalize(url);
#endif

      if(httpOnly)
	{
	  QString host(test.domain());

	  if(host.startsWith("."))
	    host = host.mid(1);

	  if(host.isEmpty())
	    host = url.host();

	  emit httpCookieReceived
	    (host, url, QDateTime::currentDateTime());

	  if(!cookieList.at(i).isHttpOnly())
	    if(!dooble::s_httpOnlyExceptionsWindow->allowed(host))
	      continue;
	}

      if(!dooble::s_cookiesBlockWindow->allowed(test.domain()))
	if(dooble::s_cookiesBlockWindow->approach() == "accept")
	  continue;

      if(url.scheme().toLower().trimmed() == "http")
	if(cookieList.at(i).isSecure())
	  {
	    dmisc::logError
	      (QString("Secure cookie %1 declined because "
		       "%2 is providing it.").
	       arg(cookieList.at(i).toRawForm().constData()).
	       arg(url.toString()));
	    continue;
	  }

      QList<QNetworkCookie> cookie;

      cookie.append(cookieList.at(i));

      if(QNetworkCookieJar::setCookiesFromUrl(cookie, url))
	{
	  QList<QNetworkCookie> all(QNetworkCookieJar::allCookies());

	  for(int j = all.size() - 1; j >= 0; j--)
	    if(cookie.at(0).domain() == all.at(j).domain() &&
	       cookie.at(0).name() == all.at(j).name() &&
	       cookie.at(0).path() == all.at(j).path())
	      {
		/*
		** cookie.at(0) already exists. Let's remove it before
		** adding it.
		*/

		all.removeAt(j);
		break;
	      }

	  all.append(cookie.at(0));
	  setAllCookies(all);
	  added = true;
	}
      else
	added = true;
    }

  if(added)
    emit changed();

  if(dmisc::passphraseWasAuthenticated())
    if(added)
      makeTimeToWrite();

  return added;
}

void dcookies::removeDomains(const QStringList &domains)
{
  bool removed = false;
  QList<QNetworkCookie> all(QNetworkCookieJar::allCookies());

  for(int i = all.size() - 1; i >= 0; i--)
    if(domains.contains(all.at(i).domain()))
      {
	removed = true;
	all.removeAt(i);
      }

  setAllCookies(all);

  if(dmisc::passphraseWasAuthenticated())
    if(removed)
      if(!dooble::s_settings.
	 value("settingsWindow/"
	       "disableAllEncryptedDatabaseWrites", false).
	 toBool())
	deleteAndSaveCookies(allCookies());

  removed = false;

  for(int i = 0; i < domains.size(); i++)
    if(m_favorites.keys().contains(domains.at(i)))
      {
	removed = true;
	m_favorites.remove(domains.at(i));
      }

  if(removed)
    if(!dooble::s_settings.
       value("settingsWindow/"
	     "disableAllEncryptedDatabaseWrites", false).
       toBool())
      deleteAndSaveFavorites();
}

void dcookies::removeCookie(const QNetworkCookie &cookie)
{
  bool removed = false;
  QList<QNetworkCookie> all(QNetworkCookieJar::allCookies());

  for(int i = all.size() - 1; i >= 0; i--)
    if(all.at(i).domain() == cookie.domain() &&
       all.at(i).name() == cookie.name() &&
       all.at(i).path() == cookie.path())
      {
	/*
	** Remove exactly one cookie.
	*/

	all.removeAt(i);
	removed = true;
	break;
      }

  setAllCookies(all);

  if(dmisc::passphraseWasAuthenticated())
    if(removed)
      if(!dooble::s_settings.
	 value("settingsWindow/"
	       "disableAllEncryptedDatabaseWrites", false).
	 toBool())
	deleteAndSaveCookies(allCookies());
}

void dcookies::createCookieDatabase(void)
{
  if(!dooble::s_settings.value("settingsWindow/cookiesEnabled",
			       true).toBool())
    return;
  else if(m_arePrivate)
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cookies");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() + "cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS cookies ("
		   "raw_form BLOB NOT NULL)");
	query.exec("CREATE TABLE IF NOT EXISTS favorites ("
		   "domain TEXT PRIMARY KEY NOT NULL)");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("cookies");
}

void dcookies::allowDomain(const QString &domain, const bool allowed)
{
  if(allowed)
    m_favorites[domain] = true;
  else
    m_favorites.remove(domain);

  if(!dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    deleteAndSaveFavorites();
}

void dcookies::populate(void)
{
  m_favorites.clear();
  setAllCookies(QList<QNetworkCookie> ());

  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(m_arePrivate)
    return;

  if(!m_writeTimer)
    {
      m_writeTimer = new QTimer(this);
      m_writeTimer->setInterval(1500);
      m_writeTimer->setSingleShot(true);
      connect(m_writeTimer,
	      SIGNAL(timeout(void)),
	      this,
	      SLOT(slotWriteTimeout(void)));
    }

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cookies");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "cookies.db");

    if(db.open())
      {
	QDateTime now(QDateTime::currentDateTime());
	QSqlQuery query(db);
	QList<QNetworkCookie> all;

	query.setForwardOnly(true);

	if(query.exec("SELECT raw_form FROM cookies"))
	  while(query.next())
	    {
	      bool ok = true;
	      QByteArray bytes
		(dmisc::daa
		 (query.value(0).toByteArray(), &ok));
	      QList<QNetworkCookie> parsed;

	      if(ok)
		parsed = QNetworkCookie::parseCookies(bytes);

	      if(!ok || parsed.isEmpty())
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare("DELETE FROM cookies WHERE "
				      "raw_form = ?");
		  deleteQuery.bindValue(0, query.value(0));
		  deleteQuery.exec();
		  continue;
		}
	      else if(dooble::s_settings.value("settingsWindow/"
					       "cookiesShouldBe", 0).
		      toInt() == 2)
		{
		  /*
		  ** The parsed container should contain at most one
		  ** cookie. Why? Well, we write the raw form of a cookie
		  ** to the cookies.db file.
		  */

		  for(int i = parsed.size() - 1; i >= 0; i--)
		    if(parsed.at(i).expirationDate().toLocalTime() < now)
		      {
			parsed.removeAt(i);

			QSqlQuery deleteQuery(db);

			deleteQuery.exec("PRAGMA secure_delete = ON");
			deleteQuery.prepare("DELETE FROM cookies WHERE "
					    "raw_form = ?");
			deleteQuery.bindValue(0, query.value(0));
			deleteQuery.exec();
		      }
		}

	      if(!parsed.isEmpty())
		all.append(parsed);
	    }

	if(!all.isEmpty())
	  setAllCookies(all);

	if(query.exec("SELECT domain FROM favorites"))
	  while(query.next())
	    {
	      QNetworkCookie cookie;
	      bool ok = true;

	      cookie.setDomain
		(QString::fromUtf8
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok)));

	      if(!ok || !QUrl::fromEncoded(cookie.domain().toUtf8(),
					   QUrl::StrictMode).isValid())
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare("DELETE FROM favorites WHERE "
				      "domain = ?");
		  deleteQuery.bindValue(0, query.value(0));
		  deleteQuery.exec();
		  continue;
		}

	      m_favorites[cookie.domain()] = true;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("cookies");
}

int dcookies::computedTimerInterval(void) const
{
  int interval = 0;

  if(dooble::s_settings.value("settingsWindow/cookiesEnabled",
			      true).toBool() &&
     dooble::s_settings.value("settingsWindow/cookieTimerEnabled",
			      false).toBool())
    {
      int unit = dooble::s_settings.value
	("settingsWindow/cookieTimerUnit", 0).toInt();

      if(!(unit == 0 || unit == 1))
	unit = 0;

      interval = dooble::s_settings.value
	("settingsWindow/cookieTimerInterval", 1).toInt();

      if(unit == 0)
	{
	  if(interval < 1 || interval > 60)
	    interval = 1;

	  interval = 3600000 * interval;
	}
      else
	{
	  if(interval < 1 || interval > 3600)
	    interval = 1;

	  interval = 60000 * interval;
	}
    }

  return interval;
}

void dcookies::slotCookieTimerChanged(void)
{
  if(!m_timer)
    return;

  int interval = computedTimerInterval();

  if(interval > 0)
    {
      if(interval != m_timer->interval())
	m_timer->start(interval);
    }
  else
    m_timer->stop();
}

bool dcookies::isFavorite(const QString &domain) const
{
  if(m_favorites.contains(domain))
    return m_favorites.value(domain);
  else
    return false;
}

void dcookies::slotRemoveDomains(void)
{
  QStringList domains;
  QList<QNetworkCookie> all(QNetworkCookieJar::allCookies());

  /*
  ** Locate the domains that are not in the
  ** m_favorites container.
  */

  for(int i = 0; i < all.size(); i++)
    if(m_favorites.contains(all.at(i).domain()))
      {
	if(!m_favorites.value(all.at(i).domain()))
	  if(!domains.contains(all.at(i).domain()))
	    domains.append(all.at(i).domain());
      }
    else if(!domains.contains(all.at(i).domain()))
      domains.append(all.at(i).domain());

  removeDomains(domains);
  emit domainsRemoved(domains);
}

void dcookies::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(dooble::s_settings.
	  value("settingsWindow/"
		"disableAllEncryptedDatabaseWrites", false).
	  toBool())
    return;
  else if(m_arePrivate)
    return;

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  createCookieDatabase();
  deleteAndSaveCookies(allCookies());
  deleteAndSaveFavorites();

  if(progress)
    progress->setVisible(false);
}

QList<QNetworkCookie> dcookies::allCookiesAndFavorites(void) const
{
  QList<QNetworkCookie> favorites;

  for(int i = 0; i < m_favorites.keys().size(); i++)
    {
      QNetworkCookie cookie;

      cookie.setDomain(m_favorites.keys().at(i));
      favorites.append(cookie);
    }

  return QNetworkCookieJar::allCookies() + favorites;
}

void dcookies::deleteAndSaveCookies(const QList<QNetworkCookie> &p_all)
{
  if(m_arePrivate)
    return;

  QMutexLocker locker(&m_writeMutex);

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "cookies_write_thread");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() + "cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS cookies ("
		   "raw_form BLOB NOT NULL)");
	query.exec("PRAGMA secure_delete = ON");
	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM cookies");
	query.exec("VACUUM");
	query.prepare("INSERT OR REPLACE INTO cookies (raw_form) VALUES (?)");
	m_stopMutex.lockForWrite();
	m_stopWriteThread = false;
	m_stopMutex.unlock();

	QList<QNetworkCookie> all(p_all);

	while(!all.isEmpty())
	  {
	    m_stopMutex.lockForRead();

	    bool stop = m_stopWriteThread;

	    m_stopMutex.unlock();

	    if(stop)
	      break;

	    QNetworkCookie cookie(all.takeFirst());

	    if(cookie.isSessionCookie())
	      continue;

	    bool ok = true;

	    query.bindValue
	      (0,
	       dmisc::etm(cookie.toRawForm(), true, &ok));

	    if(ok)
	      query.exec();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("cookies_write_thread");
}

void dcookies::deleteAndSaveFavorites(void)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(m_arePrivate)
    return;

  createCookieDatabase();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cookies");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() + "cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");
	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM favorites");
	query.exec("VACUUM");
	query.prepare("INSERT OR REPLACE INTO favorites (domain) VALUES (?)");

	for(int i = 0; i < m_favorites.keys().size(); i++)
	  {
	    bool ok = true;

	    query.bindValue(0, dmisc::etm(m_favorites.keys().at(i).
					  toUtf8(),
					  true, &ok).toBase64());

	    if(ok)
	      query.exec();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("cookies");
}

void dcookies::slotWriteTimeout(void)
{
  if(dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    return;
  else if(m_arePrivate)
    return;

  m_stopMutex.lockForWrite();
  m_stopWriteThread = true;
  m_stopMutex.unlock();
  m_future.waitForFinished();
  m_future = QtConcurrent::run(this, &dcookies::deleteAndSaveCookies,
			       allCookies());
}

void dcookies::makeTimeToWrite(void)
{
  if(m_arePrivate)
    return;

  if(m_writeTimer)
    m_writeTimer->start();
}

void dcookies::clear(void)
{
  bool removed = false;
  QList<QNetworkCookie> all(QNetworkCookieJar::allCookies());

  for(int i = all.size() - 1; i >= 0; i--)
    if(!m_favorites.contains(all.at(i).domain()))
      {
	removed = true;
	all.removeAt(i);
      }

  if(removed)
    {
      setAllCookies(all);
      emit changed();

      if(dmisc::passphraseWasAuthenticated())
	slotWriteTimeout();
    }
}

qint64 dcookies::size(void) const
{
  return QFileInfo(dooble::s_homePath + QDir::separator() + "cookies.db").
    size();
}

void dcookies::slotClear(void)
{
  clear();
}

bool dcookies::arePrivate(void) const
{
  return m_arePrivate;
}

QHash<QString, bool> dcookies::favorites(void) const
{
  return m_favorites;
}

void dcookies::setFavorites(const QHash<QString, bool> &favorites)
{
  m_favorites = favorites;
}

void dcookies::setAllCookies(const QList<QNetworkCookie> &cookies)
{
  QNetworkCookieJar::setAllCookies(cookies);
}

bool dcookies::deleteCookie(const QNetworkCookie &cookie)
{
#if QT_VERSION >= 0x050000
  bool rc = QNetworkCookieJar::deleteCookie(cookie);

  if(rc)
    {
      emit changed();

      if(dmisc::passphraseWasAuthenticated())
	makeTimeToWrite();
    }

  return rc;
#else
  Q_UNUSED(cookie);
  return false;
#endif
}

bool dcookies::insertCookie(const QNetworkCookie &cookie)
{
#if QT_VERSION >= 0x050000
  if(!dooble::s_settings.value("settingsWindow/cookiesEnabled", true).
     toBool())
    return false;

  if(dooble::s_settings.value("settingsWindow/httpOnlyCookies", false).
     toBool())
    {
      emit exceptionRaised
	(dooble::s_httpOnlyExceptionsWindow, cookie.domain());

      /*
      ** We do not have a URL here. Therefore, we will not emit the
      ** httpCookieReceived() signal.
      */

      if(!cookie.isHttpOnly())
	if(!dooble::s_httpOnlyExceptionsWindow->allowed(cookie.domain()))
	  return false;
    }

  bool rc = QNetworkCookieJar::insertCookie(cookie);

  if(rc)
    {
      emit changed();

      if(dmisc::passphraseWasAuthenticated())
	makeTimeToWrite();
    }

  return rc;
#else
  Q_UNUSED(cookie);
  return false;
#endif
}

bool dcookies::updateCookie(const QNetworkCookie &cookie)
{
#if QT_VERSION >= 0x050000
  if(!dooble::s_settings.value("settingsWindow/cookiesEnabled", true).
     toBool())
    return false;

  bool rc = QNetworkCookieJar::updateCookie(cookie);

  if(rc)
    {
      emit changed();

      if(dmisc::passphraseWasAuthenticated())
	makeTimeToWrite();
    }

  return rc;
#else
  Q_UNUSED(cookie);
  return false;
#endif
}
