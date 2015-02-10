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

#ifndef _dwebpage_h_
#define _dwebpage_h_

#ifdef DOOBLE_USE_WEBENGINE
#include <QWebEnginePage>
#include <QWebEngineSettings>
#else
#include <QWebPage>
#endif

#include "dtypes.h"

class QDateTime;
#ifdef DOOBLE_USE_WEBENGINE
#else
class QWebInspector;
#endif

class dexceptionswindow;
class dnetworkaccessmanager;
class dooble;

#ifdef DOOBLE_USE_WEBENGINE
class dwebpage: public QWebEnginePage
#else
class dwebpage: public QWebPage
#endif
{
  Q_OBJECT

 public:
  dwebpage(QObject *parent = 0);
  ~dwebpage();
#ifdef DOOBLE_USE_WEBENGINE
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes(void) const;
#else
  QHash<QWebSettings::WebAttribute, bool> webAttributes(void) const;
#endif
  bool areWebPluginsEnabled(void) const;
  bool isJavaScriptEnabled(void) const;
  bool isPrivateBrowsingEnabled(void) const;
  void setJavaScriptEnabled(const bool);
  void showWebInspector(void);

 private:
  QUrl m_requestedUrl;
#ifdef DOOBLE_USE_WEBENGINE
#else
  QWebInspector *m_webInspector;
#endif
  bool m_isJavaScriptEnabled;
  dnetworkaccessmanager *m_networkAccessManager;
  QString userAgentForUrl(const QUrl &url) const;
#ifdef DOOBLE_USE_WEBENGINE
  QWebEnginePage *createWindow(WebWindowType type);
#else
  QWebPage *createWindow(WebWindowType type);
#endif
#ifdef DOOBLE_USE_WEBENGINE
  bool javaScriptConfirm(const QUrl &url, const QString &msg);
  bool javaScriptPrompt(const QUrl &url, const QString &msg,
			const QString &defaultValue, QString *result);
#else
  bool acceptNavigationRequest(QWebFrame *frame,
			       const QNetworkRequest &request,
			       NavigationType type);
  bool javaScriptConfirm(QWebFrame *frame, const QString &msg);
  bool javaScriptPrompt(QWebFrame *frame, const QString &msg,
			const QString &defaultValue, QString *result);
#endif
  dooble *findDooble(void);
  void downloadFavicon(const QUrl &faviconUrl, const QUrl &url);
#ifdef DOOBLE_USE_WEBENGINE
  void javaScriptAlert(const QUrl &url, const QString &msg);
#else
  void fetchFaviconPathFromFrame(QWebFrame *frame);
  void javaScriptAlert(QWebFrame *frame, const QString &msg);
#endif

 private slots:
  bool shouldInterruptJavaScript(void);
  void slotFinished(QNetworkReply *reply);
#ifdef DOOBLE_USE_WEBENGINE
#else
  void slotFrameCreated(QWebFrame *frame);
#endif
  void slotHttpToHttps(void);
  void slotIconDownloadFinished(void);
#ifdef DOOBLE_USE_WEBENGINE
  void slotIconUrlChanged(const QUrl &url);
#endif
  void slotInitialLayoutCompleted(void);
  void slotUrlChanged(const QUrl &url);

 signals:
  void alwaysHttpsException(const QString &host,
			    const QUrl &url,
			    const QDateTime &dateTime);
  void exceptionRaised(dexceptionswindow *window,
		       const QUrl &url);
  void iconChanged(void);
  void loadErrorPage(const QUrl &url);
  void popupRequested(const QString &host,
		      const QUrl &url,
		      const QDateTime &dateTime);
  void urlRedirectionRequest(const QString &host,
			     const QUrl &url,
			     const QDateTime &dateTime);
};

#endif
