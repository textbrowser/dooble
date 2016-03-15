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

#ifndef _dcookies_h_
#define _dcookies_h_

#include <QHash>
#include <QMutex>
#include <QFuture>
#include <QNetworkCookieJar>
#include <QReadWriteLock>

class QProgressBar;
class QTimer;
class dexceptionswindow;

class dcookies: public QNetworkCookieJar
{
  Q_OBJECT

 public:
  dcookies(const bool arePrivate, QObject *parent = 0);
  ~dcookies();
  bool arePrivate(void) const;
  bool deleteCookie(const QNetworkCookie &cookie);
  bool insertCookie(const QNetworkCookie &cookie);
  bool isFavorite(const QString &domain) const;
  bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
			 const QUrl &url);
  bool updateCookie(const QNetworkCookie &cookie);
  void allowDomain(const QString &domain,
		   const bool allowed);
  void clear(void);
  void populate(void);
  void reencode(QProgressBar *progress);
  void removeCookie(const QNetworkCookie &cookie);
  void removeDomains(const QStringList &domains);
  void setAllCookies(const QList<QNetworkCookie> &cookies);
  void setFavorites(const QHash<QString, bool> &favorites);
  qint64 size(void) const;
  QHash<QString, bool> favorites(void) const;
  QList<QNetworkCookie> allCookies(void) const;
  QList<QNetworkCookie> allCookiesAndFavorites(void) const;
  QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;

 private:
  QMutex m_writeMutex;
  QReadWriteLock m_stopMutex;
  QTimer *m_timer;
  QTimer *m_writeTimer;
  QFuture<void> m_future;
  QHash<QString, bool> m_favorites;
  bool m_arePrivate;
  bool m_stopWriteThread;
  int computedTimerInterval(void) const;
  void makeTimeToWrite(void);
  void createCookieDatabase(void);
  void deleteAndSaveCookies(const QList<QNetworkCookie> &all);
  void deleteAndSaveFavorites(void);

 public slots:
  void slotCookieTimerChanged(void);
  void slotWriteTimeout(void);

 private slots:
  void slotClear(void);
  void slotRemoveDomains(void);

 signals:
  void changed(void);
  void cookieReceived(const QString &host,
		      const QUrl &url,
		      const QDateTime &dateTime);
  void domainsRemoved(const QStringList &list);
  void exceptionRaised(dexceptionswindow *window,
		       const QUrl &url);
  void httpCookieReceived(const QString &host,
			  const QUrl &url,
			  const QDateTime &dateTime);
};

#endif
