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
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QUrl>
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
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_hmac.h"
#include "dooble_page.h"
#include "dooble_pbkdf2.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_url_request_interceptor.h"
#include "dooble_web_engine_view.h"

QPointer<dooble_history> dooble::s_history;
bool dooble::s_containers_populated = false;
dooble_about *dooble::s_about = 0;
dooble_accepted_or_blocked_domains *dooble::s_accepted_or_blocked_domains = 0;
dooble_application *dooble::s_application = 0;
dooble_cookies *dooble::s_cookies = 0;
dooble_cookies_window *dooble::s_cookies_window = 0;
dooble_cryptography *dooble::s_cryptography = 0;
dooble_downloads *dooble::s_downloads = 0;
dooble_history_window *dooble::s_history_window = 0;
dooble_settings *dooble::s_settings = 0;
dooble_web_engine_url_request_interceptor *dooble::s_url_request_interceptor =
  0;

dooble::dooble(QWidget *widget):QMainWindow()
{
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#else
  setMenuBar(0);
#endif

  if(widget)
    {
      m_ui.tab->addTab(widget, widget->windowTitle());
      m_ui.tab->setCurrentWidget(widget);
      m_ui.tab->setTabToolTip(0, widget->windowTitle());
    }
  else
    new_page(false);

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
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#else
  setMenuBar(0);
#endif
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
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  connect_signals();
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#else
  setMenuBar(0);
#endif
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

dooble::dooble(void):dooble(static_cast<dooble_web_engine_view *> (0))
{
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

dooble_page *dooble::current_page(void) const
{
  return qobject_cast<dooble_page *> (m_ui.tab->currentWidget());
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

  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting("dooble_geometry", saveGeometry().toBase64());

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; i < list.size(); i++)
    if(list.at(i) != this && qobject_cast<dooble *> (list.at(i)))
      {
	decouple_support_windows();
	deleteLater();
	QApplication::restoreOverrideCursor();
	return;
      }

  if(s_cookies_window)
    s_cookies_window->close();

  if(s_downloads)
    s_downloads->abort();

  if(s_history)
    s_history->deleteLater();

  QApplication::restoreOverrideCursor();
  QMainWindow::closeEvent(event);
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
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slot_tab_close_requested(int)),
	  Qt::UniqueConnection);
  connect(s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)),
	  Qt::UniqueConnection);
  connect(this,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  Qt::UniqueConnection);
  connect(this,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  this,
	  SLOT(slot_dooble_credentials_authenticated(bool)),
	  Qt::UniqueConnection);
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
  if(!s_accepted_or_blocked_domains)
    s_accepted_or_blocked_domains = new dooble_accepted_or_blocked_domains();

  if(!s_about)
    s_about = new dooble_about();

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
	s_cryptography = new dooble_cryptography();
      else
	s_cryptography = new dooble_cryptography(QByteArray(), QByteArray());
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
#ifndef Q_OS_MACOS
  if(!menuBar()->isVisible())
#endif
    if(event && event->modifiers() == Qt::AltModifier)
      {
	dooble_page *page = qobject_cast<dooble_page *>
	  (m_ui.tab->currentWidget());

	if(page)
	  page->show_menu();
      }

  QMainWindow::keyPressEvent(event);
}

void dooble::new_page(bool is_private)
{
  dooble_page *page = new dooble_page(is_private, 0, m_ui.tab);

  prepare_page_connections(page);
  m_ui.tab->addTab(page, dooble_favicons::icon(QUrl()), tr("Dooble"));
  m_ui.tab->setCurrentWidget(page);
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();
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
    title = tr("Dooble");

  m_ui.tab->addTab(page, title);

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page);

  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();
}

void dooble::new_page(dooble_web_engine_view *view)
{
  dooble_page *page = new dooble_page
    (view ? view->is_private() : false, view, m_ui.tab);

  prepare_page_connections(page);
  m_ui.tab->addTab(page, tr("Dooble"));

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page);

  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);

  if(m_ui.tab->currentWidget() == page)
    page->address_widget()->setFocus();
}

void dooble::prepare_page_connections(dooble_page *page)
{
  if(!page)
    return;

  connect(dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  page,
	  SLOT(slot_dooble_credentials_authenticated(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
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
	  dooble::s_application,
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
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_private_tab(void)),
	  this,
	  SLOT(slot_new_private_tab(void)),
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
}

void dooble::prepare_shortcuts(void)
{
  if(m_shortcuts.isEmpty())
    {
      m_shortcuts.append
	(new QShortcut (QKeySequence(tr("Ctrl+A")),
			this,
			SLOT(slot_authenticate(void))));
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
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+N")),
				       this,
				       SLOT(slot_new_window(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+P")),
				       this,
				       SLOT(slot_print(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+Q")),
				       this,
				       SLOT(slot_quit_dooble(void))));
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
					    SLOT(slot_authenticate(void)),
					    QKeySequence(tr("Ctrl+A")));
  m_authentication_action->setEnabled
    (dooble_settings::has_dooble_credentials());
  menu->addSeparator();
  menu->addAction(tr("New &Private Tab"),
		  this,
		  SLOT(slot_new_private_tab(void)));
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
  m_settings_action = menu->addAction
    (QIcon(QString(":/%1/16/settings.png").arg(icon_set)),
     tr("Settin&gs..."),
     this,
     SLOT(slot_show_settings(void)),
     QKeySequence(tr("Ctrl+G")));

  /*
  ** Tools Menu
  */

  menu = m_menu->addMenu(tr("&Tools"));
  menu->addAction(tr("&Blocked Domains..."),
		  this,
		  SLOT(slot_show_accepted_or_blocked_domains(void)));
  menu->addAction(tr("&Downloads..."),
		  this,
		  SLOT(slot_show_downloads(void)),
		  QKeySequence(tr("Ctrl+D")));
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
	    m_ui.menu_view->addActions(m->actions()[3]->menu()->actions());
	}
    }
}

void dooble::slot_authenticate(void)
{
  if(m_pbkdf2_dialog || m_pbkdf2_future.isRunning())
    return;

  if(!dooble_settings::has_dooble_credentials())
    slot_show_settings_panel(dooble_settings::PRIVACY_PANEL);
  else
    {
      QInputDialog dialog(this);

      dialog.setLabelText(tr("Dooble Password"));
      dialog.setTextEchoMode(QLineEdit::Password);
      dialog.setWindowIcon(windowIcon());
      dialog.setWindowTitle(tr("Dooble: Password"));

      if(dialog.exec() != QDialog::Accepted)
	return;

      QString text = dialog.textValue();

      if(text.isEmpty())
	return;

      QByteArray salt
	(QByteArray::fromHex(dooble_settings::setting("authentication_salt").
			     toByteArray()));
      QByteArray salted_password
	(QByteArray::fromHex(dooble_settings::
			     setting("authentication_salted_password").
			     toByteArray()));
      int iteration_count = dooble_settings::setting
	("authentication_iteration_count").toInt();

      dooble::s_cryptography->authenticate(salt, salted_password, text);

      if(dooble::s_cryptography->authenticated())
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
      main_window->show();
      dooble_ui_utilities::center_window_widget(this, main_window);
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
  dialog.setDirectory(dooble::s_downloads->download_path());
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setLabelText(QFileDialog::Accept, tr("Save"));
  dialog.setWindowTitle(tr("Dooble: Download File"));
  dialog.selectFile(fileInfo.fileName());

  if(dialog.exec() == QDialog::Accepted)
    {
      download->setPath(dialog.selectedFiles().value(0));
      s_downloads->record_download(download);

      if(dooble_settings::setting("pin_downloads_window").toBool())
	{
	  if(m_ui.tab->indexOf(s_downloads) == -1)
	    m_ui.tab->addTab(s_downloads, s_downloads->windowTitle());

	  m_ui.tab->setCurrentWidget(s_downloads);
	  m_ui.tab->setTabToolTip
	    (m_ui.tab->count() - 1, s_downloads->windowTitle());
	  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
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

  if(!page)
    return;

  if(dooble::s_history && !page->is_private())
    dooble::s_history->save_favicon(icon, page->url());

  if(!page->is_private())
    dooble_favicons::save_favicon(icon, page->url());

  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), icon);
}

void dooble::slot_load_started(void)
{
}

void dooble::slot_new_private_tab(void)
{
  new_page(true);
}

void dooble::slot_new_tab(void)
{
  new_page(false);
}

void dooble::slot_new_window(void)
{
  (new dooble())->show();
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
}

void dooble::slot_open_url(const QUrl &url)
{
  new_page(false);

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->currentWidget());

  if(page)
    page->load(url);
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

      if(list.size() == 4)
	{
	  dooble::s_cryptography->setAuthenticated(true);
	  dooble::s_cryptography->setKeys
	    (list.at(0).mid(0, 64), list.at(0).mid(64, 32));
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

void dooble::slot_settings_applied(void)
{
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#endif

  if(!dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_accepted_or_blocked_domains));
      s_accepted_or_blocked_domains->setParent(0);
    }

  if(!dooble_settings::setting("pin_downloads_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_downloads));
      s_downloads->setParent(0);
    }

  if(!dooble_settings::setting("pin_history_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_history_window));
      s_history_window->setParent(0);
    }

  if(!dooble_settings::setting("pin_settings_window").toBool())
    {
      m_ui.tab->removeTab(m_ui.tab->indexOf(s_settings));
      s_settings->setParent(0);
    }
}

void dooble::slot_show_about(void)
{
  s_about->resize(s_about->sizeHint());
  s_about->show();
  dooble_ui_utilities::center_window_widget(this, s_about);
}

void dooble::slot_show_accepted_or_blocked_domains(void)
{
  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    {
      if(m_ui.tab->indexOf(s_accepted_or_blocked_domains) == -1)
	m_ui.tab->addTab(s_accepted_or_blocked_domains,
			 s_accepted_or_blocked_domains->windowTitle());

      m_ui.tab->setCurrentWidget(s_accepted_or_blocked_domains);
      m_ui.tab->setTabToolTip
	(m_ui.tab->count() - 1, s_accepted_or_blocked_domains->windowTitle());
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
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
	  dooble::s_application,
	  SIGNAL(containers_cleared(void)));
  clear_items.exec();
}

void dooble::slot_show_downloads(void)
{
  if(dooble_settings::setting("pin_downloads_window").toBool())
    {
      if(m_ui.tab->indexOf(s_downloads) == -1)
	m_ui.tab->addTab(s_downloads, s_downloads->windowTitle());

      m_ui.tab->setCurrentWidget(s_downloads);
      m_ui.tab->setTabToolTip
	(m_ui.tab->count() - 1, s_downloads->windowTitle());
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
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
	m_ui.tab->addTab(s_history_window, s_history_window->windowTitle());

      m_ui.tab->setCurrentWidget(s_history_window);
      m_ui.tab->setTabToolTip
	(m_ui.tab->count() - 1, s_history_window->windowTitle());
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
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
	m_ui.tab->addTab(s_settings, s_settings->windowTitle());

      m_ui.tab->setCurrentWidget(s_settings);
      m_ui.tab->setTabToolTip(m_ui.tab->count() - 1, s_settings->windowTitle());
      m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
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
}

void dooble::slot_tab_index_changed(int index)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;
  else if(page != m_ui.tab->currentWidget())
    return;

  if(page->title().trimmed().isEmpty())
    setWindowTitle(tr("Dooble"));
  else
    setWindowTitle(tr("%1 - Dooble").arg(page->title().trimmed()));

  page->view()->setFocus();
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
