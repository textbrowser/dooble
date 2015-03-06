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
#ifdef DOOBLE_USE_WEBENGINE
#include <QWebEnginePage>
#else
#include <QWebPage>
#endif

#include "dmisc.h"
#include "dview.h"
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
#include "dspoton.h"
#endif
#include "dcookies.h"
#include "dhistory.h"
#include "dsettings.h"
#include "ui_statusBar.h"
#include "dcookiewindow.h"
#include "dhistorymodel.h"
#include "ui_mainWindow.h"
#include "ddesktopwidget.h"
#include "ddownloadwindow.h"
#include "dhistorysidebar.h"
#include "dexceptionswindow.h"
#include "dsslcipherswindow.h"
#include "plugin-spec/extension.h"
#include "plugin-spec/signal-agent.h"

#define DOOBLE_VERSION_STR "2.00"

using namespace simpleplugin;

class QCloseEvent;

class derrorlog;
class dplugintab;
class dnetworkcache;
class dbookmarkspopup;
class dbookmarkswindow;
class dclearcontainers;

class dooble: public QMainWindow
{
  Q_OBJECT

 public:
  static quint64 s_instances;
  static QPointer<QMenu> s_bookmarksPopupMenu;
  static QUuid s_id; // Unique process ID.
  static QMutex s_saveHistoryMutex;
  static QString s_homePath;
  static QPointer<dcookies> s_cookies;
  static const int MAX_HISTORY_ITEMS = 15;
  static const int MAX_MOST_VISITED_ITEMS = 15;
  static const int MAX_NUMBER_OF_MENU_TITLE_CHARACTERS = 100;
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  static QPointer<dspoton> s_spoton;
#endif
  static QPointer<dhistory> s_historyWindow;
  static QPointer<derrorlog> s_errorLog;
  static QPointer<dsettings> s_settingsWindow;
  static QPointer<dcookiewindow> s_cookieWindow;
  static QPointer<dhistorymodel> s_historyModel;
  static QPointer<dnetworkcache> s_networkCache;
  static QPointer<dbookmarkspopup> s_bookmarksPopup;
  static QPointer<ddownloadwindow> s_downloadWindow;
  static QPointer<dbookmarkswindow> s_bookmarksWindow;
  static QPointer<dclearcontainers> s_clearContainersWindow;
  static QPointer<dexceptionswindow> s_dntWindow;
  static QPointer<dexceptionswindow> s_popupsWindow;
  static QPointer<dexceptionswindow> s_adBlockWindow;
  static QPointer<dexceptionswindow> s_httpOnlyExceptionsWindow;
  static QPointer<dexceptionswindow> s_httpReferrerWindow;
  static QPointer<dexceptionswindow> s_imageBlockWindow;
  static QPointer<dexceptionswindow> s_cookiesBlockWindow;
  static QPointer<dexceptionswindow> s_httpRedirectWindow;
  static QPointer<dexceptionswindow> s_cacheExceptionsWindow;
  static QPointer<dexceptionswindow> s_javaScriptExceptionsWindow;
  static QPointer<dexceptionswindow> s_alwaysHttpsExceptionsWindow;
  static QPointer<dexceptionswindow> s_sslExceptionsWindow;
  static QPointer<QStandardItemModel> s_bookmarksFolderModel;
  static QPointer<dsslcipherswindow> s_sslCiphersWindow;
  static QHash<QString, qint64> s_mostVisitedHosts;
  static QMap<QString, QString> s_applicationsActions;
  static QHash<QString, QVariant> s_settings;
  static void s_makeCrashFile(void);
  static void s_removeCrashFile(void);
  /*
  ** Used for JavaScript windows.
  */
  dooble(const bool isJavaScriptWindow, dooble *d);
#ifdef DOOBLE_USE_WEBENGINE
  dooble(const QUrl &url, dooble *d, dcookies *cookies,
	 const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
#else
  dooble(const QUrl &url, dooble *d, dcookies *cookies,
	 const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
#endif
  dooble(const QByteArray &history, dooble *d);
  dooble(dview *p, dooble *d);
  dooble(const QHash<QString, QVariant> &hash, dooble *d);
  ~dooble();
  void copyLink(const QUrl &url);
  void closeTab(const int index);
  void closeEvent(QCloseEvent *event);
  void setCurrentPage(dview *p);
  dview *newTab(const QByteArray &history);
  quint64 id(void) const;
  QList<QVariantList> tabUrls(void) const;

 private:
  struct
  {
    int choice;
    QUrl url;
    QString html;
  } m_saveDialogHackContainer;

  bool m_isJavaScriptWindow;
  bool showFindFrame;
  quint64 m_id;
  QWidget *sbWidget;
  Ui_statusBar sb;
  Ui_mainWindow ui;
  dhistorysidebar *m_historySideBar;
  QPointer<dooble> m_parentWindow; /*
				   ** Used for positioning JavaScript
				   ** windows.
				   */
  QList<QByteArray> m_closedTabs;
  QPointer<ddesktopwidget> m_desktopWidget;

  struct PluginRec
  {
    PluginRec(dplugintab *t = 0, const QString &fn = QString("")):tab(t),
      fileName(fn)
    {
    }

    dplugintab *tab;
    QString fileName;
  };

  QMap<QWidget *, PluginRec> pluginMap;
  QMap<QString, int>         pluginLoadCount;
  bool event(QEvent *event);
  bool warnAboutTabs(QObject *object);
  bool warnAboutDownloads(void);
  bool promptForPassphrase(const bool override = false);
  bool warnAboutLeavingModifiedTab(void);
  QSize m_sizeForFullScreen;
#ifdef DOOBLE_USE_WEBENGINE
  void find(const QWebEnginePage::FindFlags flags);
#else
  void find(const QWebPage::FindFlags flags);
#endif
  void loadPage(const QUrl &url);
  void reinstate(void);
  void copyDooble(dooble *d);
  void newTabInit(dview *p);
  void init_dooble(const bool isJavaScriptWindow);
  void saveHandler(const QUrl &url,
		   const QString &html,
		   const QString &fileName,
		   const int choice);
  void saveHistoryThread(const QList<QVariant> &list);
  void closeDesktop(void);
  void disconnectPageSignals(dview *p, dooble *parent);
  void launchDooble(const QUrl &url);
  void saveSettings(void);
  void keyPressEvent(QKeyEvent *event);
  void setUrlHandler(dooble *d);
  void prepareMenuBar(const bool state);
  void prepareTabsMenu(void);
  void unsetUrlHandler(void);
  void cleanupBeforeExit(void);
  void initializeHistory(void);
  void prepareMostVisited(void);
  void clearHistoryWidgets(void);
  void highlightBookmarkButton(void);
  void initializeBookmarksMenu(void);
  void prepareWidgetsBasedOnView(dview *p);
  void remindUserToSetPassphrase(void);
  void prepareNavigationButtonMenus(dview *p, QMenu *menu);
#ifdef DOOBLE_USE_WEBENGINE
  dview *newTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
#else
  dview *newTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
#endif
  dview *newTab(dview *p);
  void startPlugin(const QString &pluginName);
  Extension *loadPlugin(const QString &pluginName);
  void addPluginAction(const QString &pluginFileName);
  QString pluginFileName(const QString &plugName);
  QString pluginPath(void);

 protected:
  void showEvent(QShowEvent *event);

 private slots:
  void slotBack(void);
  void slotCopy(void);
  void slotFind(void);
  void slotQuit(void);
  void slotStop(void);
  void slotAbout(void);
  void slotClose(void);
  void slotPaste(void);
  void slotPrint(void);
  void slotGoHome(void);
  void slotNewTab(void);
  void slotReload(void);
  void slotSearch(void);
  void slotForward(void);
  void slotOffline(bool state);
  void slotOpenUrl(void);
  void slotSaveUrl(const QUrl &url, const int choice);
  void slotBookmark(void);
  void slotBookmark(const int index);
  void slotCloseTab(void);
  void slotCloseTab(const int index);
  void slotCopyLink(const QUrl &url);
  void slotFindNext(void);
  void slotHideFind(void);
  void slotHideMainMenus(void);
  void slotLoadPage(void);
  void slotLoadPage(const QUrl &url);
  void slotResetUrl(void);
  void slotSaveFile(const QString &fileName, const QUrl &url,
		    const int choice);
  void slotSavePage(void);
  void slotSetIcons(void);
  void slotShowFind(void);
  void slotShowLocationToolBar(bool state);
  void slotTabMoved(int from, int to);
  void slotNewWindow(void);
  void slotReloadTab(const int index);
  void slotShowAddOns(void);
  void slotUrlChanged(const QUrl &url);
  void slotViewSiteCookies(void);
  void slotViewZoomIn(void);
  void slotCloseWindow(void);
  void slotEnablePaste(void);
  void slotErrorLogged(void);
  void slotHideMenuBar(void);
  void slotHideToolBar(void);
  void slotIconChanged(void);
  void slotIconToolButtonClicked(void);
#ifdef DOOBLE_USE_WEBENGINE
  void slotLinkHovered(const QString &link);
#else
  void slotLinkHovered(const QString &link, const QString &title,
		       const QString &textContent);
#endif
  void slotShowHistory(void);
  void slotViewZoomOut(void);
  void slotLoadStarted(void);
  void slotTabSelected(const int index);
  void slotTextChanged(const QString &text);
  void slotViewEntered(void);
  void slotAuthenticate(void);
  void slotClearHistory(void);
  void slotFindPrevious(void);
  void slotLoadFinished(bool ok);
  void slotLoadProgress(int progress);
  void slotOpenP2PEmail(void);
  void slotPrintPreview(void);
  void slotTitleChanged(const QString &title);
  void slotMarkerEntered(void);
  void slotOpenDirectory(void);
  void slotHideStatusBar(void);
  void slotShowBookmarks(void);
  void slotShowIpAddress(const bool state);
  void slotViewResetZoom(void);
  void slotFullScreenMode(void);
  void slotOpenIrcChannel(void);
#ifdef DOOBLE_USE_WEBENGINE
  void slotPrintRequested(QWebEnginePage *frame);
#else
  void slotPrintRequested(QWebFrame *frame);
#endif
  void slotQuitAndRestart(void);
  void slotShowDesktopTab(const bool state = true);
  void slotShowPageSource(void);
  void slotClearContainers(void);
  void slotExceptionRaised(dexceptionswindow *window,
			   const QUrl &url);
  void slotObjectDestroyed(QObject *object);
  void slotReopenClosedTab(void);
  void slotShowHiddenFiles(bool state);
  void slotBookmarksChanged(void);
  void slotIpAddressChanged(const QString &ipAddress);
  void slotOpenLinkInNewTab(const QUrl &url);
#ifdef DOOBLE_USE_WEBENGINE
  void slotOpenLinkInNewTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
#else
  void slotOpenLinkInNewTab
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
#endif
  void slotOpenUrlsFromDrop(const QList<QUrl> &list);
  void slotRepaintRequested(const QRect &dirtyRect);
  void slotSelectAllContent(void);
  void slotSelectionChanged(void);
  void slotSelectionChanged(const QString &text);
  void slotSetTabBarVisible(const bool state);
  void slotStatusBarDisplay(bool state);
  void slotStatusBarMessage(const QString &text);
  void slotViewZoomTextOnly(bool enable);
  void slotShowSettingsWindow(void);
  void slotShowHistorySideBar(bool state);
  void slotGoToBackHistoryItem(void);
  void slotLinkActionTriggered(void);
  void slotOpenLinkInNewWindow(const QUrl &url);
#ifdef DOOBLE_USE_WEBENGINE
  void slotOpenLinkInNewWindow
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes);
#else
  void slotOpenLinkInNewWindow
    (const QUrl &url, dcookies *cookies,
     const QHash<QWebSettings::WebAttribute, bool> &webAttributes);
#endif
  void slotOpenPageInNewWindow(const int index);
  void slotHistorySideBarClosed(void);
  void slotOpenMyRetrievedFiles(void);
  void slotShowFavoritesToolBar(bool checked);
  void slotDisplayDownloadWindow(void);
  void slotLocationSplitterMoved(int post, int index);
  void slotShowRestoreSessionTab(void);
#ifdef DOOBLE_USE_WEBENGINE
  void slotAuthenticationRequired(const QUrl &url,
				  QAuthenticator *authenticator);
#else
  void slotAuthenticationRequired(QNetworkReply *reply,
				  QAuthenticator *authenticator);
#endif
  void slotGoToForwardHistoryItem(void);
  void slotHandleQuirkySaveDialog(int result);
  void slotShowApplicationCookies(void);
  void slotShowWebInspector(void);
  void slotClearRecentlyClosedTabs(void);
  void slotGeometryChangeRequested(const QRect &geometry);
  void slotHistoryTabSplitterMoved(int pos, int index);
  void slotAboutToShowBookmarksMenu(void);
  void slotChangeDesktopBackgrounds(void);
  void slotAboutToShowBackForwardMenu(void);
  void slotFavoritesToolButtonClicked(void);
#ifdef DOOBLE_USE_WEBENGINE
  void slotProxyAuthenticationRequired(const QUrl &url,
				       QAuthenticator *authenticator,
				       const QString &proxyHost);
#else
  void slotProxyAuthenticationRequired(const QNetworkProxy &proxy,
				       QAuthenticator *authenticator);
#endif
  void slotRefreshPlugins(void);
  void slotPluginAction(QAction *plugAction);
  void slotPluginExiting(Extension *plugin, int status);
  void slotSubmitUrlToSpotOn(void);
  friend class dwebpage;

 signals:
  void clearHistory(void);
  void iconsChanged(void);
  void bookmarkUpdated(void);
  void updateDesktopBackground(void);
  void passphraseWasAuthenticated(const bool state);
};

#endif
