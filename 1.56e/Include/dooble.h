/*
** Copyright (c) 2008 - present, Alexis Megas, Bernd Stramm.
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

#ifndef _dooble_h_
#define _dooble_h_

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QWebPage>

#include "dcookies.h"
#include "dcookiewindow.h"
#include "ddesktopwidget.h"
#include "ddownloadwindow.h"
#include "dexceptionswindow.h"
#include "dhistory.h"
#include "dhistorymodel.h"
#include "dhistorysidebar.h"
#include "dmisc.h"
#include "dsettings.h"
#include "dspoton.h"
#include "dsslcipherswindow.h"
#include "dview.h"
#include "ui_dblockedhosts.h"
#include "ui_dmainWindow.h"
#include "ui_dstatusBar.h"

#define DOOBLE_VERSION_STR "1.56e"

class QCloseEvent;
class dbookmarkspopup;
class dbookmarkswindow;
class dclearcontainers;
class derrorlog;
class dnetworkcache;

class dagentstring: public QWebPage
{
  Q_OBJECT

 public:
  dagentstring(void):QWebPage(0)
  {
  }

  QString userAgentForUrl(const QUrl &url) const
  {
    return QWebPage::userAgentForUrl(url);
  }
};

class dooble: public QMainWindow
{
  Q_OBJECT

 public:
  static QHash<QString, QVariant> s_settings;
  static QHash<QString, qint64> s_mostVisitedHosts;
  static QMap<QString, QString> s_applicationsActions;
  static QMutex s_saveHistoryMutex;
  static QPointer<QMenu> s_bookmarksPopupMenu;
  static QPointer<QStandardItemModel> s_bookmarksFolderModel;
  static QPointer<dbookmarkspopup> s_bookmarksPopup;
  static QPointer<dbookmarkswindow> s_bookmarksWindow;
  static QPointer<dclearcontainers> s_clearContainersWindow;
  static QPointer<dcookies> s_cookies;
  static QPointer<dcookiewindow> s_cookieWindow;
  static QPointer<ddownloadwindow> s_downloadWindow;
  static QPointer<derrorlog> s_errorLog;
  static QPointer<dexceptionswindow> s_adBlockWindow;
  static QPointer<dexceptionswindow> s_alwaysHttpsExceptionsWindow;
  static QPointer<dexceptionswindow> s_cacheExceptionsWindow;
  static QPointer<dexceptionswindow> s_cookiesBlockWindow;
  static QPointer<dexceptionswindow> s_dntWindow;
  static QPointer<dexceptionswindow> s_httpOnlyExceptionsWindow;
  static QPointer<dexceptionswindow> s_httpRedirectWindow;
  static QPointer<dexceptionswindow> s_httpReferrerWindow;
  static QPointer<dexceptionswindow> s_imageBlockWindow;
  static QPointer<dexceptionswindow> s_javaScriptExceptionsWindow;
  static QPointer<dexceptionswindow> s_popupsWindow;
  static QPointer<dexceptionswindow> s_sslExceptionsWindow;
  static QPointer<dexceptionswindow> s_userAgentExceptionsWindow;
  static QPointer<dhistory> s_historyWindow;
  static QPointer<dhistorymodel> s_historyModel;
  static QPointer<dnetworkcache> s_networkCache;
  static QPointer<dsettings> s_settingsWindow;
  static QPointer<dspoton> s_spoton;
  static QPointer<dsslcipherswindow> s_sslCiphersWindow;
  static QReadWriteLock s_applicationsActionsLock;
  static QString s_homePath;
  static QUuid s_id; // Unique process ID.
  static const int MAX_HISTORY_ITEMS = 30;
  static const int MAX_MOST_VISITED_ITEMS = 30;
  static const int MAX_NUMBER_OF_MENU_TITLE_CHARACTERS = 100;
  static qint64 s_instances;
  static void s_makeCrashFile(void);
  static void s_removeCrashFile(void);
  /*
  ** Used for JavaScript windows.
  */
  dooble(const QByteArray &history, dooble *d);
  dooble(const QHash<QString, QVariant> &hash, dooble *d);
  dooble(const QUrl &url, dooble *d, dcookies *cookies,
	 const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  dooble(const bool isJavaScriptWindow, dooble *d);
  dooble(dview *p, dooble *d);
  ~dooble();
  QList<QVariantList> tabUrls(void) const;
  dview *newTab(const QByteArray &history);
  qint64 id(void) const;
  void closeEvent(QCloseEvent *event);
  void closeTab(const int index);
  void copyLink(const QUrl &url);
  void setCurrentPage(dview *p);

 private:
  struct
  {
    QString html;
    QUrl url;
    int choice;
  } m_saveDialogHackContainer;

  QList<QByteArray> m_closedTabs;
  QPalette m_findLineEditPalette;
  QPointer<ddesktopwidget> m_desktopWidget;
  QPointer<dooble> m_parentWindow; // Position JavaScript windows.
  QWidget *sbWidget;
  Ui_mainWindow ui;
  Ui_statusBar sb;
  bool m_gridify;
  bool m_isJavaScriptWindow;
  bool showFindFrame;
  dhistorysidebar *m_historySideBar;
  qint64 m_id;
  static Ui_blockedhosts s_blockedHostsUi;
  static QMainWindow *s_blockedhostsWindow;
  bool event(QEvent *event);
  bool promptForPassphrase(const bool override = false);
  bool warnAboutDownloads(void);
  bool warnAboutLeavingModifiedTab(void);
  bool warnAboutTabs(QObject *object);
  dview *newTab(const QUrl &url, dcookies *cookies,
		const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  dview *newTab(dview *p);
  void cleanupBeforeExit(void);
  void clearHistoryWidgets(void);
  void closeDesktop(void);
  void copyDooble(dooble *d);
  void disconnectPageSignals(dview *p, dooble *parent);
  void find(const QWebPage::FindFlags flags);
  void highlightBookmarkButton(void);
  void init_dooble(const bool isJavaScriptWindow);
  void initializeBookmarksMenu(void);
  void initializeHistory(void);
  void keyPressEvent(QKeyEvent *event);
  void launchDooble(const QUrl &url);
  void loadPage(const QUrl &url);
  void newTabInit(dview *p);
  void prepareLocationSpotOnWidget(void);
  void prepareMenuBar(const bool state);
  void prepareMostVisited(void);
  void prepareNavigationButtonMenus(dview *p, QMenu *menu);
  void prepareStatusBarLabel(const QString &text);
  void prepareTabsMenu(void);
  void prepareWidgetsBasedOnView(dview *p);
  void reinstate(void);
  void remindUserToSetPassphrase(void);
  void saveHandler(const QUrl &url, const QString &html,
		   const QString &fileName,const int choice);
  void saveHistoryThread(const QList<QVariant> &list);
  void saveSettings(void);
  void setUrlHandler(dooble *d);
  void unsetUrlHandler(void);

 protected:
  void showEvent(QShowEvent *event);

 private slots:
  void slotAbout(void);
  void slotAboutToShowBackForwardMenu(void);
  void slotAboutToShowBookmarksMenu(void);
  void slotAuthenticate(void);
  void slotAuthenticationRequired(QNetworkReply *reply,
				  QAuthenticator *authenticator);
  void slotBack(void);
  void slotBookmark(const int index);
  void slotBookmark(void);
  void slotBookmarksChanged(void);
  void slotChangeDesktopBackgrounds(void);
  void slotClearContainers(void);
  void slotClearHistory(void);
  void slotClearRecentlyClosedTabs(void);
  void slotClearSpotOnSharedLinks(void);
  void slotClose(void);
  void slotCloseTab(const int index);
  void slotCloseTab(void);
  void slotCloseWindow(void);
  void slotCopy(void);
  void slotCopyLink(const QUrl &url);
  void slotCopyStyleSheet(void);
  void slotDisplayDownloadWindow(void);
  void slotEnablePaste(void);
  void slotErrorLogged(void);
  void slotExceptionRaised(dexceptionswindow *window, const QUrl &url);
  void slotFavoritesToolButtonClicked(void);
  void slotFind(void);
  void slotFindNext(void);
  void slotFindPrevious(void);
  void slotForward(void);
  void slotGeometryChangeRequested(const QRect &geometry);
  void slotGoHome(void);
  void slotGoToBackHistoryItem(void);
  void slotGoToForwardHistoryItem(void);
  void slotGridify(void);
  void slotHandleQuirkySaveDialog(int result);
  void slotHideFind(void);
  void slotHideMainMenus(void);
  void slotHideMenuBar(void);
  void slotHideStatusBar(void);
  void slotHideToolBar(void);
  void slotHistorySideBarClosed(void);
  void slotHistoryTabSplitterMoved(int pos, int index);
  void slotIconChanged(void);
  void slotIconToolButtonClicked(void);
  void slotIpAddressChanged(const QString &ipAddress);
  void slotLinkActionTriggered(void);
  void slotLinkHovered(const QString &link, const QString &title,
		       const QString &textContent);
  void slotLoadFinished(bool ok);
  void slotLoadPage(const QUrl &url);
  void slotLoadPage(void);
  void slotLoadProgress(int progress);
  void slotLoadStarted(void);
  void slotLocationSplitterMoved(int post, int index);
  void slotNewPrivateTab(void);
  void slotNewTab(void);
  void slotNewWindow(void);
  void slotObjectDestroyed(QObject *object);
  void slotOffline(bool state);
  void slotOpenHome(void);
  void slotOpenDirectory(void);
  void slotOpenIrcChannel(void);
  void slotOpenLinkInNewTab(const QUrl &url);
  void slotOpenLinkInNewTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  void slotOpenLinkInNewWindow(const QUrl &url);
  void slotOpenLinkInNewWindow
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
  void slotOpenMyRetrievedFiles(void);
  void slotOpenP2PEmail(void);
  void slotOpenPageInNewWindow(const int index);
  void slotOpenUrl(void);
  void slotOpenUrlsFromDrop(const QList<QUrl> &list);
  void slotPaste(void);
  void slotPrint(void);
  void slotPrintPreview(void);
  void slotPrintRequested(QWebFrame *frame);
  void slotProxyAuthenticationRequired(const QNetworkProxy &proxy,
				       QAuthenticator *authenticator);
  void slotQuit(void);
  void slotQuitAndRestart(void);
  void slotReload(void);
  void slotReloadTab(const int index);
  void slotReopenClosedTab(void);
  void slotRepaintRequested(const QRect &dirtyRect);
  void slotResetStyleSheets(void);
  void slotResetUrl(void);
  void slotSaveBlockedHosts(void);
  void slotSaveFile(const QString &fileName, const QUrl &url, const int choice);
  void slotSavePage(void);
  void slotSaveUrl(const QUrl &url, const int choice);
  void slotSearch(void);
  void slotSelectAllContent(void);
  void slotSelectionChanged(const QString &text);
  void slotSelectionChanged(void);
  void slotSetIcons(void);
  void slotSetStyleSheet(void);
  void slotSetTabBarVisible(const bool state);
  void slotSetWidgetStyleSheet(const QPoint &point);
  void slotSettingsChanged(void);
  void slotShowApplicationCookies(void);
  void slotShowBlockedHosts(void);
  void slotShowBookmarks(void);
  void slotShowDesktopTab(const bool state = true);
  void slotShowFavoritesToolBar(bool checked);
  void slotShowFind(void);
  void slotShowHiddenFiles(bool state);
  void slotShowHistory(void);
  void slotShowHistorySideBar(bool state);
  void slotShowIpAddress(const bool state);
  void slotShowLocationBarButton(bool state);
  void slotShowPageSource(void);
  void slotShowReminder(void);
  void slotShowRestoreSessionTab(void);
  void slotShowSearchWidget(bool state);
  void slotShowSettingsWindow(void);
  void slotShowWebInspector(void);
  void slotStatusBarDisplay(bool state);
  void slotStatusBarMessage(const QString &text);
  void slotStop(void);
  void slotStopTab(const int index);
  void slotSubmitUrlToSpotOn(void);
  void slotTabMoved(int from, int to);
  void slotTabSelected(const int index);
  void slotTextChanged(const QString &text);
  void slotTitleChanged(const QString &title);
  void slotUrlChanged(const QUrl &url);
  void slotViewResetZoom(void);
  void slotViewSiteCookies(void);
  void slotViewZoomIn(void);
  void slotViewZoomOut(void);
  void slotViewZoomTextOnly(bool enable);
  friend class dwebpage;

 signals:
  void bookmarkUpdated(void);
  void clearHistory(void);
  void iconsChanged(void);
  void passphraseWasAuthenticated(const bool state);
  void updateDesktopBackground(void);
};

#endif
