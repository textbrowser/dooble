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

#include <QDialog>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkCookie>
#include <QPointer>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_about.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_clear_items.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_hmac.h"
#include "dooble_page.h"
#include "dooble_pbkdf2.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_url_request_interceptor.h"
#include "dooble_web_engine_view.h"
#include "ui_dooble_authenticate.h"

QColor dooble::s_private_tab_text_color = QColor(148, 0, 211);
QPointer<dooble_history> dooble::s_history;
bool dooble::s_containers_populated = false;
QPointer<dooble_about> dooble::s_about;
QPointer<dooble_accepted_or_blocked_domains>
dooble::s_accepted_or_blocked_domains;
QPointer<dooble_application> dooble::s_application;
QPointer<dooble_cookies> dooble::s_cookies;
QPointer<dooble_cookies_window> dooble::s_cookies_window;
QPointer<dooble_cryptography> dooble::s_cryptography;
QPointer<dooble_downloads> dooble::s_downloads;
QPointer<dooble_history_window> dooble::s_history_window;
QPointer<dooble_settings> dooble::s_settings;
QPointer<dooble_web_engine_url_request_interceptor>
dooble::s_url_request_interceptor;

dooble::dooble(QWidget *widget):QMainWindow()
{
  initialize_static_members();
  m_is_javascript_dialog = false;
  m_is_private = false;
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();

  if(!isFullScreen())
    m_ui.menu_bar->setVisible
      (dooble_settings::setting("main_menu_bar_visible").toBool());

  if(widget)
    {
      m_ui.tab->addTab(widget, widget->windowTitle());
      m_ui.tab->setCurrentWidget(widget);
      m_ui.tab->setTabIcon(0, widget->windowIcon());
      m_ui.tab->setTabToolTip(0, widget->windowTitle());
      prepare_tab_icons();
      prepare_tab_shortcuts();
    }
  else
    new_page(QUrl(), false);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(500);
	s_containers_populated = true;
      }

  connect(QWebEngineProfile::defaultProfile(),
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)));
  prepare_shortcuts();
  prepare_standard_menus();
}

dooble::dooble(const QUrl &url, bool is_private):QMainWindow()
{
  initialize_static_members();
  m_is_javascript_dialog = false;
  m_is_private = is_private;
  m_menu = new QMenu(this);

  if(m_is_private)
    {
      m_cookies = new dooble_cookies(m_is_private, this);
      m_cookies_window = new dooble_cookies_window(m_is_private, this);
      m_cookies_window->setCookies(m_cookies);
      m_web_engine_profile = new QWebEngineProfile(this);
      m_web_engine_profile->setRequestInterceptor
	(dooble::s_url_request_interceptor);
      m_web_engine_profile->setSpellCheckEnabled(true);
      m_web_engine_profile->setSpellCheckLanguages
	(dooble::s_settings->s_spell_checker_dictionaries);
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::FullScreenSupportEnabled, true);
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::JavascriptCanAccessClipboard,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard));
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::JavascriptCanOpenWindows,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptCanOpenWindows));
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::JavascriptEnabled,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptEnabled));
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::LocalStorageEnabled, false);
      m_web_engine_profile->settings()->setAttribute
	(QWebEngineSettings::PluginsEnabled,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::PluginsEnabled));
      connect(dooble::s_settings,
	      SIGNAL(applied(void)),
	      this,
	      SLOT(slot_settings_applied(void)));
      connect(m_cookies,
	      SIGNAL(cookie_added(const QNetworkCookie &, bool)),
	      m_cookies_window,
	      SLOT(slot_cookie_added(const QNetworkCookie &, bool)));
      connect(m_cookies,
	      SIGNAL(cookie_removed(const QNetworkCookie &)),
	      m_cookies_window,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
      connect(m_web_engine_profile->cookieStore(),
	      SIGNAL(cookieAdded(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_added(const QNetworkCookie &)));
      connect(m_web_engine_profile->cookieStore(),
	      SIGNAL(cookieRemoved(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
      copy_default_profile_settings();
      m_cookies_window->setCookieStore(m_web_engine_profile->cookieStore());
    }

  m_ui.setupUi(this);
  connect_signals();

  if(!isFullScreen())
    m_ui.menu_bar->setVisible
      (dooble_settings::setting("main_menu_bar_visible").toBool());

  new_page(url, is_private);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(500);
	s_containers_populated = true;
      }

  connect(QWebEngineProfile::defaultProfile(),
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)));
  prepare_shortcuts();
  prepare_standard_menus();
}

dooble::dooble(dooble_page *page):QMainWindow()
{
  initialize_static_members();
  m_is_javascript_dialog = false;
  m_is_private = page ? page->is_private() : false;
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();

  if(!isFullScreen())
    m_ui.menu_bar->setVisible
      (dooble_settings::setting("main_menu_bar_visible").toBool());

  new_page(page);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(500);
	s_containers_populated = true;
      }

  connect(QWebEngineProfile::defaultProfile(),
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)));
  prepare_shortcuts();
  prepare_standard_menus();
}

dooble::dooble(dooble_web_engine_view *view):QMainWindow()
{
  initialize_static_members();
  m_is_javascript_dialog = false;
  m_is_private = view ? view->is_private() : false;
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();

  if(!isFullScreen())
    m_ui.menu_bar->setVisible
      (dooble_settings::setting("main_menu_bar_visible").toBool());

  new_page(view);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(500);
	s_containers_populated = true;
      }

  connect(QWebEngineProfile::defaultProfile(),
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)));
  prepare_shortcuts();
  prepare_standard_menus();
}

dooble::~dooble()
{
  while(!m_shortcuts.isEmpty())
    delete m_shortcuts.takeFirst();
}

bool dooble::can_exit(void)
{
  if(s_downloads->is_finished())
    return true;
  else
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Downloads are in progress. Are you sure that you wish to exit? "
	    "If you exit, downloads will be aborted."));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	return false;

      return true;
    }
}

bool dooble::is_private(void) const
{
  return m_is_private;
}

dooble_page *dooble::current_page(void) const
{
  return qobject_cast<dooble_page *> (m_ui.tab->currentWidget());
}

dooble_page *dooble::new_page(const QUrl &url, bool is_private)
{
  dooble_page *page = new dooble_page(m_web_engine_profile, 0, m_ui.tab);

  prepare_page_connections(page);
  m_ui.tab->addTab(page, tr("New Page"));
  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!

  if(dooble_settings::setting("denote_private_widgets").toBool())
    if(is_private)
      m_ui.tab->setTabTextColor
	(m_ui.tab->count() - 1, s_private_tab_text_color);

  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(dooble_settings::setting("access_new_tabs").toBool() ||
     qobject_cast<QShortcut *> (sender()) ||
     qobject_cast<dooble_tab_widget *> (sender()) || !sender())
    m_ui.tab->setCurrentWidget(page); // Order is important.

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();

  page->load(url);
  prepare_tab_shortcuts();
  return page;
}

void dooble::closeEvent(QCloseEvent *event)
{
  if(event)
    if(!can_exit())
      {
	event->ignore();
	return;
      }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(!m_is_javascript_dialog)
    if(dooble_settings::setting("save_geometry").toBool())
      dooble_settings::set_setting
	("dooble_geometry", saveGeometry().toBase64());

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; i < list.size(); i++)
    if(list.at(i) != this && qobject_cast<dooble *> (list.at(i)))
      {
	decouple_support_windows();
	deleteLater();
	QApplication::restoreOverrideCursor();
	return;
      }

  s_cookies_window->close();
  s_downloads->abort();
  s_history->abort();
  QApplication::restoreOverrideCursor();
  QApplication::exit(0);
}

void dooble::connect_signals(void)
{
  connect(&m_pbkdf2_future_watcher,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_pbkdf2_future_finished(void)));
  connect(&m_populate_containers_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_populate_containers_timer_timeout(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_edit,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_edit,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_file,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_file,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_help,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_help,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_tools,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_tools,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_view,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_view,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(currentChanged(int)),
	  this,
	  SLOT(slot_tab_index_changed(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(decouple_tab(int)),
	  this,
	  SLOT(slot_decouple_tab(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(empty_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(new_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SLOT(slot_open_tab_as_new_window(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(reload_tab(int)),
	  this,
	  SLOT(slot_reload_tab(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slot_tab_close_requested(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(tabs_menu_button_clicked(void)),
	  this,
	  SLOT(slot_tabs_menu_button_clicked(void)),
	  Qt::UniqueConnection);
  connect(s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)),
	  Qt::UniqueConnection);
  connect(this,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  Qt::UniqueConnection);
  connect(this,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  this,
	  SLOT(slot_dooble_credentials_authenticated(bool)),
	  Qt::UniqueConnection);
}

void dooble::copy_default_profile_settings(void)
{
  if(!m_is_private || !m_web_engine_profile)
    return;

  /*
  ** PluginsEnabled is not copied.
  */

  m_web_engine_profile->setHttpCacheMaximumSize
    (QWebEngineProfile::defaultProfile()->httpCacheMaximumSize());
  m_web_engine_profile->setHttpCacheType
    (QWebEngineProfile::defaultProfile()->httpCacheType());
  m_web_engine_profile->setHttpUserAgent
    (QWebEngineProfile::defaultProfile()->httpUserAgent());
  m_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     QWebEngineSettings::defaultSettings()->
     testAttribute(QWebEngineSettings::ScrollAnimatorEnabled));
  m_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled,
     QWebEngineSettings::defaultSettings()->
     testAttribute(QWebEngineSettings::XSSAuditingEnabled));
}

void dooble::decouple_support_windows(void)
{
  if(dooble_ui_utilities::
     find_parent_dooble(s_accepted_or_blocked_domains) == this)
    s_accepted_or_blocked_domains->setParent(0);

  if(dooble_ui_utilities::find_parent_dooble(s_downloads) == this)
    s_downloads->setParent(0);

  if(dooble_ui_utilities::find_parent_dooble(s_history_window) == this)
    s_history_window->setParent(0);

  if(dooble_ui_utilities::find_parent_dooble(s_settings) == this)
    s_settings->setParent(0);
}

void dooble::initialize_static_members(void)
{
  if(!s_about)
    s_about = new dooble_about();

  if(!s_accepted_or_blocked_domains)
    s_accepted_or_blocked_domains = new dooble_accepted_or_blocked_domains();

  if(!s_cookies)
    s_cookies = new dooble_cookies(false, 0);

  if(!s_cookies_window)
    {
      s_cookies_window = new dooble_cookies_window(false, 0);
      s_cookies_window->setCookieStore
	(QWebEngineProfile::defaultProfile()->cookieStore());
      s_cookies_window->setCookies(s_cookies);
    }

  if(!s_cryptography)
    {
      if(dooble_settings::setting("credentials_enabled").toBool())
	s_cryptography = new dooble_cryptography
	  (dooble_settings::setting("block_cipher_type").toString());
      else
	s_cryptography = new dooble_cryptography
	  (QByteArray(), QByteArray(), "AES-256");
    }

  if(!s_downloads)
    s_downloads = new dooble_downloads();

  if(!s_history)
    s_history = new dooble_history();

  if(!s_history_window)
    s_history_window = new dooble_history_window();

  if(!s_url_request_interceptor)
    {
      s_url_request_interceptor = new
	dooble_web_engine_url_request_interceptor(0);
      QWebEngineProfile::defaultProfile()->setRequestInterceptor
	(s_url_request_interceptor);
    }
}

void dooble::keyPressEvent(QKeyEvent *event)
{
  if(event)
    if((event->modifiers() & Qt::AltModifier) &&
       (event->modifiers() & Qt::ControlModifier))
      if(!dooble_settings::setting("main_menu_bar_visible").toBool())
	{
	  if(m_ui.menu_bar->isVisible())
	    m_ui.menu_bar->setVisible(false);
	  else
	    {
	      m_ui.menu_bar->setVisible(true);
	      m_ui.menu_bar->setFocus();
	    }
	}

  QMainWindow::keyPressEvent(event);
}

void dooble::new_page(dooble_page *page)
{
  if(!page)
    return;

  page->setParent(m_ui.tab);
  prepare_page_connections(page);

  /*
  ** The page's icon and title may not be meaningful.
  */

  QString title(page->title().trimmed());

  if(title.isEmpty())
    title = page->url().toString();

  if(title.isEmpty())
    title = tr("New Page");

  m_ui.tab->addTab(page, title);
  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!

  if(dooble_settings::setting("denote_private_widgets").toBool())
    if(page->is_private())
      m_ui.tab->setTabTextColor
	(m_ui.tab->count() - 1, s_private_tab_text_color);

  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page); // Order is important.

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();

  page->address_widget()->selectAll();
  prepare_tab_shortcuts();
}

void dooble::new_page(dooble_web_engine_view *view)
{
  dooble_page *page = new dooble_page
    (view ? view->web_engine_profile() : m_web_engine_profile.data(),
     view,
     m_ui.tab);

  prepare_page_connections(page);

  QString title(page->title().trimmed());

  if(title.isEmpty())
    title = page->url().toString();

  m_ui.tab->addTab(page, tr("New Page"));
  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page); // Order is important.

  if(dooble_settings::setting("denote_private_widgets").toBool())
    if(page->is_private())
      m_ui.tab->setTabTextColor
	(m_ui.tab->count() - 1, s_private_tab_text_color);

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();

  prepare_tab_shortcuts();
}

void dooble::prepare_page_connections(dooble_page *page)
{
  if(!page)
    return;

  connect(page,
	  SIGNAL(authenticate(void)),
	  this,
	  SLOT(slot_authenticate(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(close_tab(void)),
	  this,
	  SLOT(slot_close_tab(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(create_dialog(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_dialog(dooble_web_engine_view *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_tab(dooble_web_engine_view *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_window(dooble_web_engine_view *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadFinished(bool)),
	  m_ui.tab,
	  SLOT(slot_load_finished(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadStarted(void)),
	  m_ui.tab,
	  SLOT(slot_load_started(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_private_window(void)),
	  this,
	  SLOT(slot_new_private_window(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_window(void)),
	  this,
	  SLOT(slot_new_window(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(open_link_in_new_private_window(const QUrl &)),
	  this,
	  SLOT(slot_open_link_in_new_private_window(const QUrl &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(open_link_in_new_tab(const QUrl &)),
	  this,
	  SLOT(slot_open_link_in_new_tab(const QUrl &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(open_link_in_new_window(const QUrl &)),
	  this,
	  SLOT(slot_open_link_in_new_window(const QUrl &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(print(void)),
	  this,
	  SLOT(slot_print(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(print_preview(void)),
	  this,
	  SLOT(slot_print_preview(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(quit_dooble(void)),
	  this,
	  SLOT(slot_quit_dooble(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(save(void)),
	  this,
	  SLOT(slot_save(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_about(void)),
	  this,
	  SLOT(slot_show_about(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_accepted_or_blocked_domains(void)),
	  this,
	  SLOT(slot_show_accepted_or_blocked_domains(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_clear_items(void)),
	  this,
	  SLOT(slot_show_clear_items(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_cookies(void)),
	  this,
	  SLOT(slot_show_cookies(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_downloads(void)),
	  this,
	  SLOT(slot_show_downloads(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_full_screen(void)),
	  this,
	  SLOT(slot_show_full_screen(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_history(void)),
	  this,
	  SLOT(slot_show_history(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_settings(void)),
	  this,
	  SLOT(slot_show_settings(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_settings_panel(dooble_settings::Panels)),
	  this,
	  SLOT(slot_show_settings_panel(dooble_settings::Panels)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SLOT(slot_title_changed(const QString &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  page,
	  SLOT(slot_dooble_credentials_authenticated(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
}

void dooble::prepare_shortcuts(void)
{
  if(m_shortcuts.isEmpty())
    {
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+D")),
		       this,
		       SLOT(slot_show_downloads(void))));
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+G")),
		       this,
		       SLOT(slot_show_settings(void))));
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+H")),
		       this,
		       SLOT(slot_show_history(void))));
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+K")),
		       this,
		       SLOT(slot_show_cookies(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+N")),
				       this,
				       SLOT(slot_new_window(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+P")),
				       this,
				       SLOT(slot_print(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+Q")),
				       this,
				       SLOT(slot_quit_dooble(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+S")),
				       this,
				       SLOT(slot_save(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+T")),
				       this,
				       SLOT(slot_new_tab(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+W")),
				       this,
				       SLOT(slot_close_tab(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("F11")),
				       this,
				       SLOT(slot_show_full_screen(void))));
    }
}

void dooble::prepare_standard_menus(void)
{
  m_menu->clear();

  QAction *action = 0;
  QMenu *menu = 0;
  QString icon_set(dooble_settings::setting("icon_set").toString());

  /*
  ** File Menu
  */

  menu = m_menu->addMenu(tr("&File"));
  m_authentication_action = menu->addAction(tr("&Authenticate..."),
					    this,
					    SLOT(slot_authenticate(void)));
  m_authentication_action->setEnabled
    (dooble_settings::has_dooble_credentials());
  menu->addSeparator();
  menu->addAction(tr("New &Private Window"),
		  this,
		  SLOT(slot_new_private_window(void)));
  menu->addAction(tr("New &Tab"),
		  this,
		  SLOT(slot_new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction(tr("&New Window..."),
		  this,
		  SLOT(slot_new_window(void)),
		  QKeySequence(tr("Ctrl+N")));
  menu->addSeparator();
  action = m_action_close_tab = menu->addAction(tr("&Close Tab"),
						this,
						SLOT(slot_close_tab(void)),
						QKeySequence(tr("Ctrl+W")));

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    action->setEnabled
      (qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  menu->addSeparator();
  menu->addAction(tr("E&xit Dooble"),
		  this,
		  SLOT(slot_quit_dooble(void)),
		  QKeySequence(tr("Ctrl+Q")));

  /*
  ** Edit Menu
  */

  menu = m_menu->addMenu(tr("&Edit"));
  menu->addAction(tr("&Clear Items..."),
		  this,
		  SLOT(slot_show_clear_items(void)));

  if(dooble_settings::setting("pin_settings_window").toBool())
    m_settings_action = menu->addAction
      (QIcon(QString(":/%1/18/settings.png").arg(icon_set)),
       tr("Settin&gs"),
       this,
       SLOT(slot_show_settings(void)),
       QKeySequence(tr("Ctrl+G")));
  else
    m_settings_action = menu->addAction
      (QIcon(QString(":/%1/18/settings.png").arg(icon_set)),
       tr("Settin&gs..."),
       this,
       SLOT(slot_show_settings(void)),
       QKeySequence(tr("Ctrl+G")));

  /*
  ** Tools Menu
  */

  menu = m_menu->addMenu(tr("&Tools"));

  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    menu->addAction(tr("&Blocked Domains"),
		    this,
		    SLOT(slot_show_accepted_or_blocked_domains(void)));
  else
    menu->addAction(tr("&Blocked Domains..."),
		    this,
		    SLOT(slot_show_accepted_or_blocked_domains(void)));

  menu->addAction(tr("&Cookies..."),
		  this,
		  SLOT(slot_show_cookies(void)),
		  QKeySequence(tr("Ctrl+K")));

  if(dooble_settings::setting("pin_downloads_window").toBool())
    menu->addAction(tr("&Downloads"),
		    this,
		    SLOT(slot_show_downloads(void)),
		    QKeySequence(tr("Ctrl+D")));
  else
    menu->addAction(tr("&Downloads..."),
		    this,
		    SLOT(slot_show_downloads(void)),
		    QKeySequence(tr("Ctrl+D")));

  if(dooble_settings::setting("pin_history_window").toBool())
    menu->addAction(tr("&History"),
		    this,
		    SLOT(slot_show_history(void)),
		    QKeySequence(tr("Ctrl+H")));
  else
    menu->addAction(tr("&History..."),
		    this,
		    SLOT(slot_show_history(void)),
		    QKeySequence(tr("Ctrl+H")));

  /*
  ** View Menu
  */

  menu = m_menu->addMenu(tr("&View"));
  m_full_screen_action = menu->addAction(tr("Show &Full Screen"),
					 this,
					 SLOT(slot_show_full_screen(void)),
					 QKeySequence(tr("F11")));

  /*
  ** Help Menu
  */

  menu = m_menu->addMenu(tr("&Help"));
  menu->addAction(tr("&About..."),
		  this,
		  SLOT(slot_show_about(void)));
}

void dooble::prepare_tab_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  for(int i = 0; i < m_ui.tab->count(); i++)
    {
      QMainWindow *main_window = qobject_cast<QMainWindow *>
	(m_ui.tab->widget(i));

      if(!main_window)
	continue;

      if(main_window == s_accepted_or_blocked_domains)
	m_ui.tab->setTabIcon
	  (i, QIcon(QString(":/%1/48/blocked_domains.png").arg(icon_set)));
      else if(main_window == s_downloads)
	m_ui.tab->setTabIcon
	  (i, QIcon(QString(":/%1/36/downloads.png").arg(icon_set)));
      else if(main_window == s_history_window)
	m_ui.tab->setTabIcon
	  (i, QIcon(QString(":/%1/48/history.png").arg(icon_set)));
      else if(main_window == s_settings)
	m_ui.tab->setTabIcon
	  (i, QIcon(QString(":/%1/48/settings.png").arg(icon_set)));
    }
}

void dooble::prepare_tab_shortcuts(void)
{
  while(!m_tab_widget_shortcuts.isEmpty())
    delete m_tab_widget_shortcuts.takeFirst();

  for(int i = 0; i < qMin(m_ui.tab->count(), 10); i++)
    {
      QWidget *widget = m_ui.tab->widget(i);

      if(!widget)
	continue;

      QShortcut *shortcut = 0;

      if(i == 9)
	{
	  shortcut = new QShortcut
	    (Qt::AltModifier + Qt::Key_0,
	     this,
	     SLOT(slot_tab_widget_shortcut_activated(void)));
	  m_tab_widget_shortcuts.append(shortcut);
	}
      else
	{
	  shortcut = new QShortcut
	    (Qt::AltModifier + Qt::Key(Qt::Key_1 + i),
	     this,
	     SLOT(slot_tab_widget_shortcut_activated(void)));
	  m_tab_widget_shortcuts.append(shortcut);

	  if(i == m_ui.tab->count() - 1)
	    {
	      shortcut = new QShortcut
		(Qt::AltModifier + Qt::Key_0,
		 this,
		 SLOT(slot_tab_widget_shortcut_activated(void)));
	      m_tab_widget_shortcuts.append(shortcut);
	    }
	}
    }
}

void dooble::print(dooble_page *page)
{
  if(!page)
    return;

  QPrintDialog *print_dialog = 0;
  QPrinter *printer = new QPrinter();

  print_dialog = new QPrintDialog(printer, this);

  if(print_dialog->exec() == QDialog::Accepted)
    page->print_page(printer);
  else
    delete printer;

  print_dialog->deleteLater();
}

void dooble::print_current_page(void)
{
  print(current_page());
}

void dooble::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("dooble_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble::slot_about_to_hide_main_menu(void)
{
  QMenu *menu = qobject_cast<QMenu *> (sender());

  if(menu)
    menu->clear();
}

void dooble::slot_about_to_show_main_menu(void)
{
  QMenu *menu = qobject_cast<QMenu *> (sender());

  if(menu)
    {
      menu->clear();

      QMenu *m = 0;
      dooble_page *page = qobject_cast<dooble_page *>
	(m_ui.tab->currentWidget());

      if(page && page->menu())
	m = page->menu();
      else
	m = m_menu;

      if(m && m->actions().size() >= 5)
	{
	  if(m_ui.menu_edit == menu && m->actions()[1]->menu())
	    m_ui.menu_edit->addActions(m->actions()[1]->menu()->actions());
	  else if(m_ui.menu_file == menu && m->actions()[1]->menu())
	    {
	      m_ui.menu_file->addActions(m->actions()[0]->menu()->actions());

	      if(page && page->action_close_tab())
		page->action_close_tab()->setEnabled(m_ui.tab->count() > 1);
	      else if(m_action_close_tab)
		m_action_close_tab->setEnabled(m_ui.tab->count() > 1);
	    }
	  else if(m_ui.menu_help == menu && m->actions()[4]->menu())
	    m_ui.menu_help->addActions(m->actions()[4]->menu()->actions());
	  else if(m_ui.menu_tools == menu && m->actions()[2]->menu())
	    m_ui.menu_tools->addActions(m->actions()[2]->menu()->actions());
	  else if(m_ui.menu_view == menu && m->actions()[3]->menu())
	    {
	      m_ui.menu_view->addActions(m->actions()[3]->menu()->actions());

	      if(page && page->full_screen_action())
		{
		  if(isFullScreen())
		    page->full_screen_action()->setText
		      (tr("Show &Normal Screen"));
		  else
		    page->full_screen_action()->setText
		      (tr("Show &Full Screen"));
		}
	      else if(m_full_screen_action)
		{
		  if(isFullScreen())
		    m_full_screen_action->setText(tr("Show &Normal Screen"));
		  else
		    m_full_screen_action->setText(tr("Show &Full Screen"));
		}
	    }
	}
    }
}

void dooble::slot_authenticate(void)
{
  if(m_pbkdf2_dialog || m_pbkdf2_future.isRunning())
    return;
  else if(s_cryptography && s_cryptography->authenticated())
    return;

  if(!dooble_settings::has_dooble_credentials())
    slot_show_settings_panel(dooble_settings::PRIVACY_PANEL);
  else
    {
      QDialog dialog(this);
      Ui_dooble_authenticate ui;

      ui.setupUi(&dialog);

      if(s_application->style_name() == "macintosh")
	ui.password->setAttribute(Qt::WA_MacShowFocusRect, false);

      connect(ui.authenticate,
	      SIGNAL(clicked(void)),
	      &dialog,
	      SLOT(accept(void)));
      connect(ui.password,
	      SIGNAL(returnPressed(void)),
	      &dialog,
	      SLOT(accept(void)));

      if(dialog.exec() != QDialog::Accepted)
	return;

      QByteArray salt
	(QByteArray::fromHex(dooble_settings::setting("authentication_salt").
			     toByteArray()));
      QByteArray salted_password
	(QByteArray::fromHex(dooble_settings::
			     setting("authentication_salted_password").
			     toByteArray()));
      QString text(ui.password->text());
      int block_cipher_type_index = dooble_settings::setting
	("block_cipher_type_index").toInt();
      int iteration_count = dooble_settings::setting
	("authentication_iteration_count").toInt();

      s_cryptography->authenticate(salt, salted_password, text);

      if(s_cryptography->authenticated())
	{
	  m_pbkdf2_dialog = new QProgressDialog(this);
	  m_pbkdf2_dialog->setCancelButtonText(tr("Interrupt"));
	  m_pbkdf2_dialog->setLabelText(tr("Preparing credentials..."));
	  m_pbkdf2_dialog->setMaximum(0);
	  m_pbkdf2_dialog->setMinimum(0);
	  m_pbkdf2_dialog->setWindowIcon(windowIcon());
	  m_pbkdf2_dialog->setWindowModality(Qt::ApplicationModal);
	  m_pbkdf2_dialog->setWindowTitle(tr("Dooble: Preparing Credentials"));

	  QScopedPointer<dooble_pbkdf2> pbkdf2;

	  pbkdf2.reset(new dooble_pbkdf2(text.toUtf8(),
					 salt,
					 block_cipher_type_index,
					 iteration_count,
					 1024));
	  m_pbkdf2_future = QtConcurrent::run
	    (pbkdf2.data(),
	     &dooble_pbkdf2::pbkdf2,
	     &dooble_hmac::sha3_512_hmac);
	  m_pbkdf2_future_watcher.setFuture(m_pbkdf2_future);
	  connect(m_pbkdf2_dialog,
		  SIGNAL(canceled(void)),
		  pbkdf2.data(),
		  SLOT(slot_interrupt(void)));
	  m_pbkdf2_dialog->exec();
	}
      else
	QMessageBox::critical
	  (this,
	   tr("Dooble: Error"),
	   tr("Unable to authenticate the provided password."));
    }
}

void dooble::slot_close_tab(void)
{
  if(m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->currentWidget());

  if(page)
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(page));
      page->deleteLater();
    }
  else
    m_ui.tab->removeTab(m_ui.tab->indexOf(m_ui.tab->currentWidget()));

  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  prepare_tab_shortcuts();
}

void dooble::slot_create_dialog(dooble_web_engine_view *view)
{
  if(!view)
    return;

  dooble *d = new dooble(view);

  d->m_is_javascript_dialog = true;
  d->m_ui.tab->setTabBarAutoHide(true);
  d->resize(640, 480); // VGA
  d->showNormal();
  dooble_ui_utilities::center_window_widget(this, d);
}

void dooble::slot_create_tab(dooble_web_engine_view *view)
{
  new_page(view);
}

void dooble::slot_create_window(dooble_web_engine_view *view)
{
  dooble *d = new dooble(view);

  d->show();
}

void dooble::slot_decouple_tab(int index)
{
  QMainWindow *main_window = qobject_cast<QMainWindow *>
    (m_ui.tab->widget(index));

  if(main_window)
    {
      m_ui.tab->removeTab(index);
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
      main_window->setParent(0);
      main_window->resize(main_window->sizeHint());
      main_window->show();
      dooble_ui_utilities::center_window_widget(this, main_window);
      prepare_tab_shortcuts();
    }
}

void dooble::slot_dooble_credentials_authenticated(bool state)
{
  if(state)
    {
      if(m_authentication_action)
	m_authentication_action->setEnabled(false);
    }
  else
    {
      if(m_authentication_action)
	m_authentication_action->setEnabled
	  (dooble_settings::has_dooble_credentials());
    }
}

void dooble::slot_download_requested(QWebEngineDownloadItem *download)
{
  if(!download)
    return;
  else if(s_downloads->contains(download))
    {
      download->cancel();
      return;
    }

  QFileDialog dialog(this);
  QFileInfo fileInfo(download->path());

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setConfirmOverwrite(true);
  dialog.setDirectory(s_downloads->download_path());
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setLabelText(QFileDialog::Accept, tr("Save"));
  dialog.setWindowTitle(tr("Dooble: Download File"));
  dialog.selectFile(fileInfo.fileName());

  if(dialog.exec() == QDialog::Accepted)
    {
      if(download->state() == QWebEngineDownloadItem::DownloadRequested)
	download->setPath(dialog.selectedFiles().value(0));

      s_downloads->record_download(download);

      if(dooble_settings::setting("pin_downloads_window").toBool())
	{
	  if(m_ui.tab->indexOf(s_downloads) == -1)
	    {
	      m_ui.tab->addTab(s_downloads, s_downloads->windowTitle());
	      m_ui.tab->setTabToolTip
		(m_ui.tab->count() - 1, s_downloads->windowTitle());
	      m_ui.tab->setTabIcon
		(m_ui.tab->count() - 1, s_downloads->windowIcon());
	      prepare_tab_icons();
	    }

	  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
	  m_ui.tab->setCurrentWidget(s_downloads); // Order is important.
	  prepare_tab_shortcuts();
	}
      else if(!s_downloads->isVisible())
	{
	  s_downloads->activateWindow();
	  s_downloads->showNormal();
	}

      download->accept();
    }
  else
    download->cancel();
}

void dooble::slot_icon_changed(const QIcon &icon)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(page)
    m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), icon);
}

void dooble::slot_new_private_window(void)
{
  (new dooble(QUrl(), true))->show();
}

void dooble::slot_new_tab(void)
{
  new_page(QUrl(), m_is_private);
}

void dooble::slot_new_window(void)
{
  (new dooble(QUrl(), false))->show();
}

void dooble::slot_open_link_in_new_private_window(const QUrl &url)
{
  (new dooble(url, true))->show();
}

void dooble::slot_open_link_in_new_tab(const QUrl &url)
{
  new_page(url, m_is_private);
}

void dooble::slot_open_link_in_new_window(const QUrl &url)
{
  (new dooble(url, false))->show();
}

void dooble::slot_open_tab_as_new_window(int index)
{
  if(m_ui.tab->count() == 1)
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(page)
    {
      dooble *d = new dooble(page);

      d->show();
      m_ui.tab->removeTab(m_ui.tab->indexOf(page));
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
    }
  else
    {
      dooble *d = new dooble(m_ui.tab->widget(index));

      d->show();
      m_ui.tab->removeTab(index);
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
    }

  prepare_tab_shortcuts();
}

void dooble::slot_open_url(const QUrl &url)
{
  new_page(url, false);
}

void dooble::slot_pbkdf2_future_finished(void)
{
  bool was_canceled = false;

  if(m_pbkdf2_dialog)
    {
      if(m_pbkdf2_dialog->wasCanceled())
	was_canceled = true;

      m_pbkdf2_dialog->cancel();
      m_pbkdf2_dialog->deleteLater();
    }

  if(!was_canceled)
    {
      QList<QByteArray> list(m_pbkdf2_future.result());

      if(!list.isEmpty())
	{
	  s_cryptography->set_authenticated(true);

	  if(list.value(1).toInt() == 0)
	    s_cryptography->set_block_cipher_type("AES-256");
	  else
	    s_cryptography->set_block_cipher_type("Threefish-256");

	  s_cryptography->set_keys
	    (list.value(0).mid(0, 64), list.value(0).mid(64, 32));
	  emit dooble_credentials_authenticated(true);
	}
    }
}

void dooble::slot_populate_containers_timer_timeout(void)
{
  emit dooble_credentials_authenticated(true);
}

void dooble::slot_print(void)
{
  print(qobject_cast<dooble_page *> (m_ui.tab->currentWidget()));
}

void dooble::slot_print_preview(void)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->currentWidget());

  if(!page)
    return;

  QPrintPreviewDialog *print_preview_dialog = 0;
  QPrinter *printer = new QPrinter();

  print_preview_dialog = new QPrintPreviewDialog(printer, this);
  connect(print_preview_dialog,
	  SIGNAL(paintRequested(QPrinter *)),
	  page,
	  SLOT(slot_print_preview(QPrinter *)));
  print_preview_dialog->exec();
  print_preview_dialog->deleteLater();
}

void dooble::slot_quit_dooble(void)
{
  /*
  ** May require some confirmation from the user.
  */

  if(!close())
    return;

  QApplication::exit(0);
}

void dooble::slot_reload_tab(int index)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(page)
    page->reload();
}

void dooble::slot_remove_tab_widget_shortcut(void)
{
  prepare_tab_shortcuts();
}

void dooble::slot_save(void)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->currentWidget());

  if(!page)
    return;

  QString file_name(page->url().fileName());

  if(file_name.isEmpty())
    file_name = page->url().host();

  page->save(s_downloads->download_path() + QDir::separator() + file_name);
}

void dooble::slot_set_current_tab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    m_ui.tab->setCurrentIndex(action->property("index").toInt());
}

void dooble::slot_settings_applied(void)
{
  copy_default_profile_settings();
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());

  if(!dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_accepted_or_blocked_domains));
      prepare_tab_shortcuts();
      s_accepted_or_blocked_domains->setParent(0);
    }

  if(!dooble_settings::setting("pin_downloads_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_downloads));
      prepare_tab_shortcuts();
      s_downloads->setParent(0);
    }

  if(!dooble_settings::setting("pin_history_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_history_window));
      prepare_tab_shortcuts();
      s_history_window->setParent(0);
    }

  if(!dooble_settings::setting("pin_settings_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_settings));
      prepare_tab_shortcuts();
      s_settings->setParent(0);
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < m_ui.tab->count(); i++)
    {
      dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(i));

      if(!page)
	continue;

      if(dooble_settings::setting("denote_private_widgets").toBool())
	{
	  if(page->is_private())
	    m_ui.tab->setTabTextColor(i, s_private_tab_text_color);
	  else
	    m_ui.tab->setTabTextColor(i, QColor());
	}
      else
	m_ui.tab->setTabTextColor(i, QColor());
    }

  prepare_tab_icons();
  QApplication::restoreOverrideCursor();
}

void dooble::slot_show_about(void)
{
  connect(s_about,
	  SIGNAL(link_activated(const QUrl &)),
	  this,
	  SLOT(slot_open_url(const QUrl &)),
	  Qt::UniqueConnection);
  s_about->show();
  dooble_ui_utilities::center_window_widget(this, s_about);
}

void dooble::slot_show_accepted_or_blocked_domains(void)
{
  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    {
      if(m_ui.tab->indexOf(s_accepted_or_blocked_domains) == -1)
	{
	  m_ui.tab->addTab(s_accepted_or_blocked_domains,
			   s_accepted_or_blocked_domains->windowTitle());
	  m_ui.tab->setTabIcon
	    (m_ui.tab->count() - 1,
	     s_accepted_or_blocked_domains->windowIcon());
	  m_ui.tab->setTabToolTip
	    (m_ui.tab->count() - 1,
	     s_accepted_or_blocked_domains->windowTitle());
	  prepare_tab_icons();
	}

      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
      m_ui.tab->setCurrentWidget
	(s_accepted_or_blocked_domains); // Order is important.
      prepare_tab_shortcuts();
      return;
    }

  if(s_accepted_or_blocked_domains->isVisible())
    {
      s_accepted_or_blocked_domains->activateWindow();
      s_accepted_or_blocked_domains->raise();
      return;
    }

  s_accepted_or_blocked_domains->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget
      (this, s_accepted_or_blocked_domains);

  s_accepted_or_blocked_domains->activateWindow();
  s_accepted_or_blocked_domains->raise();
}

void dooble::slot_show_clear_items(void)
{
  dooble_clear_items clear_items(this);

  connect(&clear_items,
	  SIGNAL(containers_cleared(void)),
	  s_application,
	  SIGNAL(containers_cleared(void)));
  clear_items.exec();
}

void dooble::slot_show_cookies(void)
{
  if(m_cookies_window)
    {
      dooble_page *page = qobject_cast<dooble_page *>
	(m_ui.tab->currentWidget());

      if(page)
	m_cookies_window->filter(page->url().host());

      if(m_cookies_window->isVisible())
	{
	  m_cookies_window->activateWindow();
	  m_cookies_window->raise();
	  return;
	}

      m_cookies_window->showNormal();

      if(dooble_settings::setting("center_child_windows").toBool())
	dooble_ui_utilities::center_window_widget(this, m_cookies_window);

      m_cookies_window->activateWindow();
      m_cookies_window->raise();
      return;
    }

  if(!qobject_cast<QShortcut *> (sender()))
    {
      /*
      ** Display this site's cookies.
      */

      dooble_page *page = qobject_cast<dooble_page *>
	(m_ui.tab->currentWidget());

      if(page)
	s_cookies_window->filter(page->url().host());
    }

  if(s_cookies_window->isVisible())
    {
      s_cookies_window->activateWindow();
      s_cookies_window->raise();
      return;
    }

  s_cookies_window->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, s_cookies_window);

  s_cookies_window->activateWindow();
  s_cookies_window->raise();
}

void dooble::slot_show_downloads(void)
{
  if(dooble_settings::setting("pin_downloads_window").toBool())
    {
      if(m_ui.tab->indexOf(s_downloads) == -1)
	{
	  m_ui.tab->addTab(s_downloads, s_downloads->windowTitle());
	  m_ui.tab->setTabIcon
	    (m_ui.tab->count() - 1, s_downloads->windowIcon());
	  m_ui.tab->setTabToolTip
	    (m_ui.tab->count() - 1, s_downloads->windowTitle());
	  prepare_tab_icons();
	}

      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
      m_ui.tab->setCurrentWidget(s_downloads); // Order is important.
      prepare_tab_shortcuts();
      return;
    }

  if(s_downloads->isVisible())
    {
      s_downloads->activateWindow();
      s_downloads->raise();
      return;
    }

  s_downloads->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget
      (this, s_downloads);

  s_downloads->activateWindow();
  s_downloads->raise();
}

void dooble::slot_show_full_screen(void)
{
  if(!isFullScreen())
    showFullScreen();
  else
    showNormal();
}

void dooble::slot_show_history(void)
{
  if(dooble_settings::setting("pin_history_window").toBool())
    {
      if(m_ui.tab->indexOf(s_history_window) == -1)
	{
	  m_ui.tab->addTab(s_history_window, s_history_window->windowTitle());
	  m_ui.tab->setTabIcon
	    (m_ui.tab->count() - 1, s_history_window->windowIcon());
	  m_ui.tab->setTabToolTip
	    (m_ui.tab->count() - 1, s_history_window->windowTitle());
	  prepare_tab_icons();
	}

      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
      m_ui.tab->setCurrentWidget(s_history_window); // Order is important.
      prepare_tab_shortcuts();
      return;
    }

  if(s_history_window->isVisible())
    {
      s_history_window->activateWindow();
      s_history_window->raise();
      return;
    }

  s_history_window->showNormal(this);

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, s_history_window);

  s_history_window->activateWindow();
  s_history_window->raise();
}

void dooble::slot_show_settings(void)
{
  if(dooble_settings::setting("pin_settings_window").toBool())
    {
      if(m_ui.tab->indexOf(s_settings) == -1)
	{
	  m_ui.tab->addTab(s_settings, s_settings->windowTitle());
	  m_ui.tab->setTabIcon
	    (m_ui.tab->count() - 1, s_settings->windowIcon());
	  m_ui.tab->setTabToolTip
	    (m_ui.tab->count() - 1, s_settings->windowTitle());
	  prepare_tab_icons();
	  s_settings->restore();
	}

      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
      m_ui.tab->setCurrentWidget(s_settings); // Order is important.
      prepare_tab_shortcuts();
      return;
    }

  if(s_settings->isVisible())
    {
      s_settings->activateWindow();
      s_settings->raise();
      return;
    }

  s_settings->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, s_settings);

  s_settings->activateWindow();
  s_settings->raise();
}

void dooble::slot_show_settings_panel(dooble_settings::Panels panel)
{
  slot_show_settings();
  s_settings->show_panel(panel);
}

void dooble::slot_tab_close_requested(int index)
{
  if(index < 0 || m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(page)
    page->deleteLater();

  m_ui.tab->removeTab(index);
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  prepare_tab_shortcuts();
}

void dooble::slot_tab_index_changed(int index)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    {
      QMainWindow *main_window = qobject_cast<QMainWindow *>
	(m_ui.tab->widget(index));

      if(main_window)
	setWindowTitle(main_window->windowTitle());

      return;
    }
  else if(page != m_ui.tab->currentWidget())
    return;

  if(page->title().trimmed().isEmpty())
    setWindowTitle(tr("Dooble"));
  else
    setWindowTitle(tr("%1 - Dooble").arg(page->title().trimmed()));

  page->view()->setFocus();
}

void dooble::slot_tab_widget_shortcut_activated(void)
{
  QShortcut *shortcut = qobject_cast<QShortcut *> (sender());

  if(!shortcut)
    return;

  QKeySequence key(shortcut->key());
  int index = -1;

  if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_1)))
    index = 0;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_2)))
    index = 1;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_3)))
    index = 2;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_4)))
    index = 3;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_5)))
    index = 4;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_6)))
    index = 5;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_7)))
    index = 6;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_8)))
    index = 7;
  else if(key.matches(QKeySequence(Qt::AltModifier + Qt::Key_9)))
    index = 8;
  else
    index = m_ui.tab->count() - 1;

  m_ui.tab->setCurrentIndex(index);
}

void dooble::slot_tabs_menu_button_clicked(void)
{
  QMenu menu(this);

  menu.setStyleSheet("QMenu {menu-scrollable: 1;}");

  QFontMetrics font_metrics(menu.fontMetrics());

  for(int i = 0; i < m_ui.tab->count(); i++)
    {
      QAction *action = 0;
      QString text(m_ui.tab->tabText(i));
      dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(i));

      if(page)
	action = menu.addAction
	  (page->icon(),
	   font_metrics.elidedText(text,
				   Qt::ElideRight,
				   dooble_ui_utilities::
				   context_menu_width(&menu)));
      else
	action = menu.addAction
	  (m_ui.tab->tabIcon(i),
	   font_metrics.elidedText(text,
				   Qt::ElideRight,
				   dooble_ui_utilities::
				   context_menu_width(&menu)));

      action->setProperty("index", i);
      connect(action,
	      SIGNAL(triggered(void)),
	      this,
	      SLOT(slot_set_current_tab(void)));
    }

  menu.exec
    (m_ui.tab->tabs_menu_button()->
     mapToGlobal(m_ui.tab->tabs_menu_button()->rect().bottomLeft()));
  m_ui.tab->tabs_menu_button()->setChecked(false);
}

void dooble::slot_title_changed(const QString &title)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  QString text(title.trimmed());

  if(text.isEmpty())
    text = page->url().toString();

  if(text.isEmpty())
    text = tr("Dooble");
  else
    text = tr("%1 - Dooble").arg(text);

  if(page == m_ui.tab->currentWidget())
    setWindowTitle(text);

  m_ui.tab->setTabText(m_ui.tab->indexOf(page), text);
  m_ui.tab->setTabToolTip(m_ui.tab->indexOf(page), text);
}
