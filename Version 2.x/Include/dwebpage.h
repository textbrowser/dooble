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

#include <QWebEnginePage>
#include <QWebEngineSettings>

#include "dtypes.h"

class QDateTime;
class dexceptionswindow;
class dnetworkaccessmanager;
class dooble;

class dwebpage: public QWebEnginePage
{
  Q_OBJECT

 public:
  dwebpage(QObject *parent = 0);
  ~dwebpage();
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes(void) const;
  bool areWebPluginsEnabled(void) const;
  bool isJavaScriptEnabled(void) const;
  bool isPrivateBrowsingEnabled(void) const;
  void setJavaScriptEnabled(const bool);
  void showWebInspector(void);

 private:
  QUrl m_requestedUrl;
  bool m_isJavaScriptEnabled;
  dnetworkaccessmanager *m_networkAccessManager;
  QString userAgentForUrl(const QUrl &url) const;
  QWebEnginePage *createWindow(WebWindowType type);
  bool javaScriptConfirm(const QUrl &url, const QString &msg);
  bool javaScriptPrompt(const QUrl &url, const QString &msg,
			const QString &defaultValue, QString *result);
  dooble *findDooble(void);
  void downloadFavicon(const QUrl &faviconUrl, const QUrl &url);
  void javaScriptAlert(const QUrl &url, const QString &msg);

 private slots:
  bool shouldInterruptJavaScript(void);
  void slotFinished(QNetworkReply *reply);
  void slotHttpToHttps(void);
  void slotIconDownloadFinished(void);
  void slotIconUrlChanged(const QUrl &url);
  void slotLoadFinished(bool ok);
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
