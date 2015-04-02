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

#ifndef _dview_h_
#define _dview_h_

#include <QHostInfo>
#include <QIcon>
#include <QNetworkRequest>
#include <QPointer>
#include <QStackedWidget>
#include <QWebEngineSettings>

class QNetworkReply;
class QPrinter;
class QSslError;
class QWebEngineHistoryItem;
class QWebEnginePage;
class QWebEngineView;
class dcookies;
class dcookiewindow;
class dexceptionswindow;
class dfilemanager;
class dnetworkblockreply;
class dnetworkdirreply;
class dnetworkerrorreply;
class dnetworkftpreply;
class dnetworksslerrorreply;
class dooble;
class dwebpage;
class dwebview;
class dwebviewsimple;

class dview: public QStackedWidget
{
  Q_OBJECT

 public:
  dview(QWidget *parent, const QByteArray &history, dcookies *cookies,
	const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
  ~dview();
  QAction *tabAction(void) const;
  QByteArray history(void);
  QIcon icon(void) const;
  QIcon webviewIcon(void) const;
  QList<QWebEngineHistoryItem> backItems(const int n) const;
  QList<QWebEngineHistoryItem> forwardItems(const int n) const;
  QString description(void) const;
  QString html(void);
  QString ipAddress(void) const;
  QString statusMessage(void) const;
  QString title(void) const;
  QUrl url(void) const;
  QUrl webviewUrl(void) const;
  QWebEnginePage *currentFrame(void);
  bool arePrivateCookiesEnabled(void) const;
  bool areWebPluginsEnabled(void) const;
  bool canGoBack(void) const;
  bool canGoForward(void) const;
  bool hasSecureConnection(void) const;
  bool isBookmarkWorthy(void) const;
  bool isBookmarked(void) const;
  bool isDir(void) const;
  bool isFtp(void) const;
  bool isJavaScriptEnabled(void) const;
  bool isLoaded(void) const;
  bool isModified(void) const;
  bool isPrivateBrowsingEnabled(void) const;
  dwebpage *page(void) const;
  int progress(void) const;
  qreal zoomFactor(void) const;
  void back(void);
  void findIpAddress(const QUrl &url);
  void forward(void);
  void goToItem(const QWebEngineHistoryItem &item);
  void load(const QUrl &url);
  void post(const QUrl &url, const QString &text);
  void print(QPrinter *printer);
  void recordRestorationHistory(void);
  void reload(void);
  void removeRestorationFiles(void);
  void setFocus(void);
  void setJavaScriptEnabled(const bool state);
  void setPrivateBrowsingEnabled(const bool state);
  void setPrivateCookies(const bool state);
  void setTabAction(QAction *action);
  void setWebPluginsEnabled(const bool state);
  void setZoomFactor(const qreal factor);
  void stop(void);
  void update(void);
  void viewPrivateCookies(void);
  void zoomTextOnly(const bool state);

 private:
  QByteArray m_history;
  QPointer<QAction> m_action;
  QPointer<dcookies> m_cookies;
  QPointer<dcookiewindow> m_cookieWindow;
  QString m_historyRestorationFileName;
  QString m_html;
  QString m_ipAddress;
  QString m_title;
  QUrl m_selectedUrl;
  QUrl m_url;
  QUrl selectedImageUrl;
  bool m_hasSslError;
  bool m_pageLoaded;
  dwebview *webView;
  int m_lastInfoLookupId;
  int m_percentLoaded;
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes(void) const;
  dooble *findDooble(void);
  int downloadPrompt(const QString &fileName);
  int tabIndex(void);
  quint64 parentId(void);
  void enterEvent(QEvent *event);
  void prepareRestorationFileNames(void);
  void setUrlForUnsupportedContent(const QUrl &url);
  void setWebAttributes
    (const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);

 private slots:
    void slotClearHistory(void);
  void slotCopyImageLocation(void);
  void slotCopyLinkLocation(const QUrl &url);
  void slotCopyLinkLocation(void);
  void slotCopySelectedText(void);
  void slotCustomContextMenuRequested(const QPoint &pos);
  void slotDownloadRequested(const QNetworkRequest &request);
  void slotFinished(dnetworkblockreply *reply);
  void slotFinished(dnetworkdirreply *reply);
  void slotFinished(dnetworkerrorreply *reply);
  void slotFinished(dnetworkftpreply *reply);
  void slotFinished(dnetworksslerrorreply *reply);
  void slotHandleUnsupportedContent(QNetworkReply *reply);
  void slotHandleUnsupportedContent(const QUrl &url);
  void slotHostLookedUp(const QHostInfo &hostInfo);
  void slotIconChanged(void);
  void slotInitialLayoutCompleted(void);
  void slotLinkClicked(const QUrl &url);
  void slotLoadErrorPage(const QUrl &url);
  void slotLoadFinished(bool ok);
  void slotLoadPage(const QUrl &url);
  void slotLoadProgress(int progress);
  void slotLoadStarted(void);
  void slotOpenImageInNewTab(void);
  void slotOpenImageInNewWindow(void);
  void slotOpenLinkInNewTab(const QUrl &url);
  void slotOpenLinkInNewTab(void);
  void slotOpenLinkInNewWindow(const QUrl &url);
  void slotOpenLinkInNewWindow(void);
  void slotPaste(void);
  void slotPrintCurrentFrame(void);
  void slotReencodeRestorationFile(void);
  void slotSaveImage(void);
  void slotSaveLink(const QUrl &url);
  void slotSaveLink(void);
  void slotSelectionChanged(void);
  void slotSetTextSizeMultiplier(const qreal multiplier);
  void slotSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
  void slotStop(void);
  void slotTitleChanged(const QString &title);
  void slotUrlChanged(const QUrl &url);
  void slotViewImage(void);
  void slotViewPageSource(void);
void slotClearCookies(void);

 signals:
  void copyLink(const QUrl &url);
  void exceptionRaised(dexceptionswindow *window, const QUrl &url);
  void goBack(void);
  void goForward(void);
  void goReload(void);
  void iconChanged(void);
  void ipAddressChanged(const QString &ipAddress);
  void loadFinished(bool);
  void loadProgress(const int);
  void loadStarted(void);
  void openLinkInNewTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
  void openLinkInNewWindow
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
  void printRequested(QWebEnginePage *frame);
  void saveFile(const QString &fileName, const QUrl &url, const int choice);
  void saveUrl(const QUrl &url, const int choice);
  void selectionChanged(const QString &text);
  void sslError
    (const QString &host, const QUrl &url, const QDateTime &dateTime);
  void titleChanged(const QString &title);
  void urlChanged(const QUrl &url);
  void viewImage(const QUrl &url);
  void viewPageSource(void);
};

#endif
