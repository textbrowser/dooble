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
#include <QWebSettings>

class dcookies;
class dooble;
class dwebpage;
class dwebview;
class dftpbrowser;
class dfilemanager;
class dcookiewindow;
class dwebviewsimple;
class dnetworkdirreply;
class dnetworkftpreply;
class dexceptionswindow;
class dnetworkblockreply;
class dnetworkerrorreply;
class dnetworksslerrorreply;

class QPrinter;
class QWebView;
class QSslError;
class QWebFrame;
class QNetworkReply;
class QWebHistoryItem;

class dview: public QStackedWidget
{
  Q_OBJECT

 public:
  dview(QWidget *parent, const QByteArray &history, dcookies *cookies,
	const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  ~dview();
  int progress(void) const;
  bool areWebPluginsEnabled(void) const;
  bool isDir(void) const;
  bool isFtp(void) const;
  bool isLoaded(void) const;
  bool isPrivateBrowsingEnabled(void) const;
  bool canGoBack(void) const;
  bool isModified(void) const;
  bool canGoForward(void) const;
  bool isBookmarked(void) const;
  bool isBookmarkWorthy(void) const;
  bool isJavaScriptEnabled(void) const;
  bool hasSecureConnection(void) const;
  bool arePrivateCookiesEnabled(void) const;
  QUrl url(void) const;
  QUrl webviewUrl(void) const;
  void back(void);
  void load(const QUrl &url);
  void post(const QUrl &url, const QString &text);
  void stop(void);
  void print(QPrinter *printer);
  void reload(void);
  void update(void);
  void forward(void);
  void goToItem(const QWebHistoryItem &item);
  void setFocus(void);
  void setTabAction(QAction *action);
  void zoomTextOnly(const bool state);
  void findIpAddress(const QUrl &url);
  void setZoomFactor(const qreal factor);
  void setPrivateCookies(const bool state);
  void viewPrivateCookies(void);
  void initializeFtpBrowser(void);
  void initializeFileManager(void);
  void removeRestorationFiles(void);
  void recordRestorationHistory(void);
  void setJavaScriptEnabled(const bool state);
  void setPrivateBrowsingEnabled(const bool state);
  void setWebPluginsEnabled(const bool state);
  qreal zoomFactor(void) const;
  QIcon icon(void) const;
  QIcon webviewIcon(void) const;
  QAction *tabAction(void) const;
  QString html(void);
  QString title(void) const;
  QString ipAddress(void) const;
  QString description(void) const;
  QString statusMessage(void) const;
  dwebpage *page(void) const;
  QWebFrame *currentFrame(void);
  QByteArray history(void);
  QList<QWebHistoryItem> backItems(const int n) const;
  QList<QWebHistoryItem> forwardItems(const int n) const;

 private:
  int m_percentLoaded;
  int m_lastInfoLookupId;
  bool m_hasSslError;
  bool m_pageLoaded;
  QUrl m_url;
  QUrl m_selectedUrl;
  QUrl selectedImageUrl;
  QString m_title;
  QString m_ipAddress;
  QString m_historyRestorationFileName;
  dwebview *webView;
  QByteArray m_history;
  dftpbrowser *ftpBrowser;
  dfilemanager *fileManager;
  QPointer<QAction> m_action;
  QPointer<dcookies> m_cookies;
  QPointer<dcookiewindow> m_cookieWindow;
  QHash<QWebSettings::WebAttribute, bool> webAttributes(void) const;
  dooble *findDooble(void);
  int tabIndex(void);
  int downloadPrompt(const QString &fileName);
  void enterEvent(QEvent *event);
  void prepareRestorationFileNames(void);
  void setUrlForUnsupportedContent(const QUrl &url);
  void setWebAttributes
    (const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  quint64 parentId(void);

 private slots:
  void slotStop(void);
  void slotPaste(void);
  void slotFinished(dnetworkdirreply *reply);
  void slotFinished(dnetworkftpreply *reply);
  void slotFinished(dnetworkblockreply *reply);
  void slotFinished(dnetworkerrorreply *reply);
  void slotFinished(dnetworksslerrorreply *reply);
  void slotLoadPage(const QUrl &url);
  void slotSaveLink(void);
  void slotSaveLink(const QUrl &url);
  void slotSaveImage(void);
  void slotSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
  void slotViewImage(void);
  void slotUrlChanged(const QUrl &url);
  void slotIconChanged(void);
  void slotLinkClicked(const QUrl &url);
  void slotLoadStarted(void);
  void slotClearCookies(void);
  void slotClearHistory(void);
  void slotHostLookedUp(const QHostInfo &hostInfo);
  void slotLoadFinished(bool ok);
  void slotLoadProgress(int progress);
  void slotTitleChanged(const QString &title);
  void slotLoadErrorPage(const QUrl &url);
  void slotViewPageSource(void);
  void slotCopyLinkLocation(void);
  void slotCopyLinkLocation(const QUrl &url);
  void slotCopySelectedText(void);
  void slotOpenLinkInNewTab(void);
  void slotOpenLinkInNewTab(const QUrl &url);
  void slotSelectionChanged(void);
  void slotCopyImageLocation(void);
  void slotDownloadRequested(const QNetworkRequest &request);
  void slotOpenImageInNewTab(void);
  void slotPrintCurrentFrame(void);
  void slotOpenLinkInNewWindow(void);
  void slotOpenLinkInNewWindow(const QUrl &url);
  void slotOpenImageInNewWindow(void);
  void slotSetTextSizeMultiplier(const qreal multiplier);
  void slotInitialLayoutCompleted(void);
  void slotReencodeRestorationFile(void);
  void slotHandleUnsupportedContent(const QUrl &url);
  void slotHandleUnsupportedContent(QNetworkReply *reply);
  void slotCustomContextMenuRequested(const QPoint &pos);

 signals:
  void goBack(void);
  void saveUrl(const QUrl &url, const int choice);
  void copyLink(const QUrl &url);
  void goReload(void);
  void saveFile(const QString &fileName, const QUrl &url, const int choice);
  void goForward(void);
  void viewImage(const QUrl &url);
  void urlChanged(const QUrl &url);
  void iconChanged(void);
  void loadStarted(void);
  void loadFinished(bool);
  void loadProgress(const int);
  void titleChanged(const QString &title);
  void sslError(const QString &host,
		const QUrl &url,
		const QDateTime &dateTime);
  void printRequested(QWebFrame *frame);
  void viewPageSource(void);
  void exceptionRaised(dexceptionswindow *window,
		       const QUrl &url);
  void ipAddressChanged(const QString &ipAddress);
  void openLinkInNewTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  void selectionChanged(const QString &text);
  void openLinkInNewWindow
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
};

#endif
