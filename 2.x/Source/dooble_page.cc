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

#include <QAuthenticator>
#include <QMenu>
#include <QPrinter>
#include <QShortcut>
#include <QStackedWidget>
#include <QWebEngineHistoryItem>
#include <QWebEngineProfile>
#include <QWidgetAction>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_label_widget.h"
#include "dooble_page.h"
#include "dooble_popup_menu.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_page.h"
#include "dooble_web_engine_view.h"
#include "ui_dooble_authentication_dialog.h"

dooble_page::dooble_page(QWebEngineProfile *web_engine_profile,
			 dooble_web_engine_view *view,
			 QWidget *parent):QWidget(parent)
{
  m_is_private = QWebEngineProfile::defaultProfile() != web_engine_profile &&
    web_engine_profile;
  m_menu = new QMenu(this);
  m_ui.setupUi(this);
  m_ui.backward->setEnabled(false);
  m_ui.backward->setMenu(new QMenu(this));
  m_ui.find_frame->setVisible(false);
  m_ui.forward->setEnabled(false);
  m_ui.forward->setMenu(new QMenu(this));

  if(dooble_settings::setting("denote_private_widgets").toBool())
    m_ui.is_private->setVisible(m_is_private);
  else
    m_ui.is_private->setVisible(false);

  m_ui.javascript_popup_message->setVisible(false);
  m_ui.progress->setVisible(false);
  m_ui.status_bar->setVisible
    (dooble_settings::setting("status_bar_visible").toBool());

  if(view)
    {
      m_view = view;
      m_view->setParent(this);
      slot_url_changed(m_view->url());
    }
  else
    m_view = new dooble_web_engine_view(web_engine_profile, this);

  m_ui.frame->layout()->addWidget(m_view);
  connect(dooble::s_history,
	  SIGNAL(populated(void)),
	  m_ui.address,
	  SLOT(slot_populate(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(dooble::s_settings,
	  SIGNAL(dooble_credentials_created(void)),
	  this,
	  SLOT(slot_dooble_credentials_created(void)));
  connect(m_menu,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_standard_menus(void)));
  connect(m_ui.accepted_or_blocked,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_accepted_or_blocked_clicked(void)));
  connect(m_ui.address,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.address,
	  SIGNAL(pull_down_clicked(void)),
	  this,
	  SLOT(slot_show_pull_down_menu(void)));
  connect(m_ui.address,
	  SIGNAL(reset_url(void)),
	  this,
	  SLOT(slot_reset_url(void)));
  connect(m_ui.address,
	  SIGNAL(show_certificate_exception(void)),
	  this,
	  SLOT(slot_show_certificate_exception(void)));
  connect(m_ui.address,
	  SIGNAL(show_cookies(void)),
	  this,
	  SIGNAL(show_cookies(void)));
  connect(m_ui.backward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_backward(void)));
  connect(m_ui.backward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_backward_menu(void)));
  connect(m_ui.close_javascript_popup_exception_frame,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_close_javascript_popup_exception_frame(void)));
  connect(m_ui.downloads,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(show_downloads(void)));
  connect(m_ui.find,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_find_next(void)));
  connect(m_ui.find,
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_find_text_edited(const QString &)));
  connect(m_ui.find_next,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_find_next(void)));
  connect(m_ui.find_previous,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_find_previous(void)));
  connect(m_ui.find_stop,
	  SIGNAL(clicked(void)),
	  m_ui.find_frame,
	  SLOT(hide(void)));
  connect(m_ui.forward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_forward(void)));
  connect(m_ui.forward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_forward_menu(void)));
  connect(m_ui.javascript_allow_popup_exception,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_javascript_allow_popup_exception(void)));
  connect(m_ui.menu,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_show_popup_menu(void)));
  connect(m_ui.reload,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reload_or_stop(void)));
  connect(m_view,
	  SIGNAL(create_dialog(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_dialog(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_dialog_request(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_dialog_request(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_tab(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_window(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SIGNAL(iconChanged(const QIcon &)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SIGNAL(loadFinished(bool)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slot_load_finished(bool)));
  connect(m_view,
	  SIGNAL(loadProgress(int)),
	  this,
	  SLOT(slot_load_progress(int)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  m_ui.address,
	  SLOT(slot_load_started(void)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SIGNAL(loadStarted(void)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
  connect(m_view,
	  SIGNAL(open_link_in_new_private_window(const QUrl &)),
	  this,
	  SIGNAL(open_link_in_new_private_window(const QUrl &)));
  connect(m_view,
	  SIGNAL(open_link_in_new_tab(const QUrl &)),
	  this,
	  SIGNAL(open_link_in_new_tab(const QUrl &)));
  connect(m_view,
	  SIGNAL(open_link_in_new_window(const QUrl &)),
	  this,
	  SIGNAL(open_link_in_new_window(const QUrl &)));
  connect(m_view,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SIGNAL(titleChanged(const QString &)));
  connect(m_view,
	  SIGNAL(urlChanged(const QUrl &)),
	  m_ui.address,
	  SLOT(slot_url_changed(const QUrl &)));
  connect(m_view,
	  SIGNAL(urlChanged(const QUrl &)),
	  this,
	  SLOT(slot_url_changed(const QUrl &)));
  connect(m_view->page(),
	  SIGNAL(authenticationRequired(const QUrl &, QAuthenticator *)),
	  this,
	  SLOT(slot_authentication_required(const QUrl &, QAuthenticator *)));
  connect(m_view->page(),
	  SIGNAL(linkHovered(const QString &)),
	  this,
	  SLOT(slot_link_hovered(const QString &)));
  connect(m_view->page(),
	  SIGNAL(proxyAuthenticationRequired(const QUrl &,
					     QAuthenticator *,
					     const QString &)),
	  this,
	  SLOT(slot_proxy_authentication_required(const QUrl &,
						  QAuthenticator *,
						  const QString &)));
  connect(this,
	  SIGNAL(javascript_allow_popup_exception(const QUrl &)),
	  dooble::s_settings,
	  SLOT(slot_new_javascript_block_popup_exception(const QUrl &)));
  prepare_icons();
  prepare_shortcuts();
  prepare_standard_menus();
  prepare_tool_buttons();
  slot_dooble_credentials_created();
}

dooble_page::~dooble_page()
{
  while(!m_last_javascript_popups.isEmpty())
    {
      QPointer<dooble_web_engine_view> view
	(m_last_javascript_popups.takeFirst());

      if(view && view->parent() == this)
	view->deleteLater();
    }

  while(!m_shortcuts.isEmpty())
    delete m_shortcuts.takeFirst();
}

QAction *dooble_page::action_close_tab(void) const
{
  return m_action_close_tab;
}

QAction *dooble_page::full_screen_action(void) const
{
  return m_full_screen_action;
}

QIcon dooble_page::icon(void) const
{
  return dooble_favicons::icon(m_view->url());
}

QMenu *dooble_page::menu(void) const
{
  return m_menu;
}

QString dooble_page::title(void) const
{
  return m_view->title();
}

QUrl dooble_page::url(void) const
{
  return m_view->url();
}

QWebEngineSettings *dooble_page::web_engine_settings(void) const
{
  return m_view->settings();
}

bool dooble_page::is_private(void) const
{
  return m_is_private;
}

dooble *dooble_page::find_parent_dooble(void) const
{
  QWidget *parent = parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	return qobject_cast<dooble *> (parent);
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  return 0;
}

dooble_address_widget *dooble_page::address_widget(void) const
{
  return m_ui.address;
}

dooble_web_engine_view *dooble_page::view(void) const
{
  return m_view;
}

void dooble_page::enable_web_setting(QWebEngineSettings::WebAttribute setting,
				     bool state)
{
  QWebEngineSettings *settings = m_view->settings();

  if(settings)
    settings->setAttribute(setting, state);
}

void dooble_page::find_text(QWebEnginePage::FindFlags find_flags,
			    const QString &text)
{
  m_view->findText
    (text,
     find_flags,
     [=] (bool found)
     {
       static QPalette s_palette(m_ui.find->palette());

       if(!found)
	 {
	   if(!text.isEmpty())
	     {
	       QColor color(240, 128, 128); // Light Coral
	       QPalette palette(m_ui.find->palette());

	       palette.setColor(m_ui.find->backgroundRole(), color);
	       m_ui.find->setPalette(palette);
	     }
	   else
	     m_ui.find->setPalette(s_palette);
	 }
       else
	 m_ui.find->setPalette(s_palette);
     });
}

void dooble_page::go_to_backward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::go_to_forward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::load(const QUrl &url)
{
  m_view->load(url);
}

void dooble_page::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  if(m_find_action)
    m_find_action->setIcon(QIcon(QString(":/%1/18/find.png").arg(icon_set)));

  if(m_settings_action)
    m_settings_action->setIcon
      (QIcon(QString(":/%1/18/settings.png").arg(icon_set)));

  m_ui.accepted_or_blocked->setIcon
    (QIcon(QString(":/%1/48/blocked_domains.png").arg(icon_set)));
  m_ui.backward->setIcon(QIcon(QString(":/%1/36/backward.png").arg(icon_set)));
  m_ui.downloads->setIcon
    (QIcon(QString(":/%1/36/downloads.png").arg(icon_set)));
  m_ui.find_next->setIcon(QIcon(QString(":/%1/20/next.png").arg(icon_set)));
  m_ui.find_previous->setIcon
    (QIcon(QString(":/%1/20/previous.png").arg(icon_set)));
  m_ui.find_stop->setIcon(QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
  m_ui.forward->setIcon(QIcon(QString(":/%1/36/forward.png").arg(icon_set)));
  m_ui.is_private->setPixmap
    (QIcon(QString(":/%1/18/private.png").arg(icon_set)).pixmap(QSize(16, 16)));
  m_ui.close_javascript_popup_exception_frame->setIcon
    (QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
  m_ui.menu->setIcon(QIcon(QString(":/%1/36/menu.png").arg(icon_set)));
  m_ui.reload->setIcon(QIcon(QString(":/%1/36/reload.png").arg(icon_set)));
}

void dooble_page::prepare_shortcuts(void)
{
  if(m_shortcuts.isEmpty())
    {
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+F")),
		       this,
		       SLOT(slot_show_find(void))));
      m_shortcuts.append
	(new QShortcut(QKeySequence(tr("Ctrl+L")),
		       this,
		       SLOT(slot_open_url(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Ctrl+R")),
				       m_view,
				       SLOT(reload(void))));
      m_shortcuts.append(new QShortcut(QKeySequence(tr("Esc")),
				       this,
				       SLOT(slot_escape(void))));
    }
}

void dooble_page::prepare_standard_menus(void)
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
					    SIGNAL(authenticate(void)));
  m_authentication_action->setEnabled
    (dooble_settings::has_dooble_credentials());
  menu->addSeparator();
  menu->addAction(tr("New &Private Window"),
		  this,
		  SIGNAL(new_private_window(void)));
  menu->addAction(tr("New &Tab"),
		  this,
		  SIGNAL(new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction(tr("&New Window..."),
		  this,
		  SIGNAL(new_window(void)),
		  QKeySequence(tr("Ctrl+N")));
  menu->addAction(tr("&Open URL"),
		  this,
		  SLOT(slot_open_url(void)),
		  QKeySequence(tr("Ctrl+L")));
  menu->addSeparator();
  action = m_action_close_tab = menu->addAction(tr("&Close Tab"),
						this,
						SIGNAL(close_tab(void)),
						QKeySequence(tr("Ctrl+W")));

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    action->setEnabled
      (qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  menu->addSeparator();
  menu->addAction(tr("&Save..."),
		  this,
		  SIGNAL(save(void)),
		  QKeySequence(tr("Ctrl+S")));
  menu->addSeparator();
  menu->addAction(tr("&Print..."),
		  this,
		  SIGNAL(print(void)),
		  QKeySequence(tr("Ctrl+P")));
  menu->addAction(tr("Print Pre&view..."),
		  this,
		  SIGNAL(print_preview(void)))->setEnabled(false);
  menu->addSeparator();
  menu->addAction(tr("E&xit Dooble"),
		  this,
		  SIGNAL(quit_dooble(void)),
		  QKeySequence(tr("Ctrl+Q")));

  /*
  ** Edit Menu
  */

  menu = m_menu->addMenu(tr("&Edit"));
  menu->addAction(tr("&Clear Items..."),
		  this,
		  SIGNAL(show_clear_items(void)));
  m_find_action = menu->addAction
    (QIcon(QString(":/%1/18/find.png").arg(icon_set)),
     tr("&Find"),
     this,
     SLOT(slot_show_find(void)),
     QKeySequence(tr("Ctrl+F")));

  if(dooble_settings::setting("pin_settings_window").toBool())
    m_settings_action = menu->addAction
      (QIcon(QString(":/%1/18/settings.png").arg(icon_set)),
       tr("Settin&gs"),
       this,
       SIGNAL(show_settings(void)),
       QKeySequence(tr("Ctrl+G")));
  else
    m_settings_action = menu->addAction
      (QIcon(QString(":/%1/18/settings.png").arg(icon_set)),
       tr("Settin&gs..."),
       this,
       SIGNAL(show_settings(void)),
       QKeySequence(tr("Ctrl+G")));

  /*
  ** Tools Menu
  */

  menu = m_menu->addMenu(tr("&Tools"));

  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    menu->addAction(tr("&Blocked Domains"),
		    this,
		    SIGNAL(show_accepted_or_blocked_domains(void)));
  else
    menu->addAction(tr("&Blocked Domains..."),
		    this,
		    SIGNAL(show_accepted_or_blocked_domains(void)));

  menu->addAction(tr("&Cookies..."),
		  this,
		  SIGNAL(show_cookies(void)),
		  QKeySequence(tr("Ctrl+K")));

  if(dooble_settings::setting("pin_downloads_window").toBool())
    menu->addAction(tr("&Downloads"),
		    this,
		    SIGNAL(show_downloads(void)),
		    QKeySequence(tr("Ctrl+D")));
  else
    menu->addAction(tr("&Downloads..."),
		    this,
		    SIGNAL(show_downloads(void)),
		    QKeySequence(tr("Ctrl+D")));

  if(dooble_settings::setting("pin_history_window").toBool())
    menu->addAction(tr("&History"),
		    this,
		    SIGNAL(show_history(void)),
		    QKeySequence(tr("Ctrl+H")));
  else
    menu->addAction(tr("&History..."),
		    this,
		    SIGNAL(show_history(void)),
		    QKeySequence(tr("Ctrl+H")));

  /*
  ** View Menu
  */

  menu = m_menu->addMenu(tr("&View"));
  m_full_screen_action = menu->addAction(tr("Show &Full Screen"),
					 this,
					 SIGNAL(show_full_screen(void)),
					 QKeySequence(tr("F11")));
  menu->addSeparator();
  action = menu->addAction(tr("&Status Bar"),
			   this,
			   SLOT(slot_show_status_bar(bool)));
  action->setCheckable(true);
  action->setChecked(dooble_settings::setting("status_bar_visible").toBool());

  /*
  ** Help Menu
  */

  menu = m_menu->addMenu(tr("&Help"));
  menu->addAction(tr("&About..."),
		  this,
		  SIGNAL(show_about(void)));
}

void dooble_page::prepare_tool_buttons(void)
{
#ifdef Q_OS_MACOS
  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    if(m_ui.find_match_case == tool_button)
      {
      }
    else if(m_ui.backward == tool_button ||
	    m_ui.forward == tool_button)
      tool_button->setStyleSheet
	("QToolButton {border: none; padding-right: 10px}"
	 "QToolButton::menu-button {border: none;}");
    else if(m_ui.menu == tool_button)
      tool_button->setStyleSheet
	("QToolButton {border: none;}"
	 "QToolButton::menu-arrow {image: none;}"
	 "QToolButton::menu-button {border: none;}");
    else
      tool_button->setStyleSheet("QToolButton {border: none;}"
				 "QToolButton::menu-button {border: none;}");
#else
  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    if(m_ui.backward == tool_button ||
       m_ui.forward == tool_button)
      tool_button->setStyleSheet
	("QToolButton {padding-right: 10px}"
	 "QToolButton::menu-button {border: none;}");
#endif
}

void dooble_page::print_page(QPrinter *printer)
{
  if(!printer)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_view->page()->print(printer,
			[=] (bool result)
			{
			  delete printer;
			  QApplication::restoreOverrideCursor();
			  Q_UNUSED(result)
			});
}

void dooble_page::reload(void)
{
  m_view->reload();
}

void dooble_page::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  QFontMetrics font_metrics(m_ui.link_hovered->fontMetrics());
  int difference = 15;

  if(m_ui.is_private->isVisible())
    difference += 25;

  if(m_ui.progress->isVisible())
    difference += m_ui.progress->width();

  m_ui.link_hovered->setText
    (font_metrics.
     elidedText(m_ui.link_hovered->property("text").toString().trimmed(),
		Qt::ElideMiddle,
		qAbs(width() - difference)));
  m_ui.link_hovered->setCursorPosition(0);
}

void dooble_page::save(const QString &file_name)
{
  m_view->save(file_name);
}

void dooble_page::show_menu(void)
{
  QPoint point(m_ui.menu->pos());

  m_ui.menu->setChecked(true);
  point.setY(m_ui.menu->size().height() + point.y());
  m_menu->exec(mapToGlobal(point));
  m_ui.menu->setChecked(false);
}

void dooble_page::show_popup_menu(void)
{
  QMenu menu(this);
  QPoint point(m_ui.menu->pos());
  QSize size;
  QWidgetAction widget_action(&menu);
  dooble_popup_menu *popup_menu = new dooble_popup_menu
    (m_view->zoomFactor(), this);

  connect(popup_menu,
	  SIGNAL(accepted(void)),
	  &menu,
	  SLOT(close(void)));
  connect(popup_menu,
	  SIGNAL(authenticate(void)),
	  this,
	  SIGNAL(authenticate(void)));
  connect(popup_menu,
	  SIGNAL(show_accepted_or_blocked_domains(void)),
	  this,
	  SIGNAL(show_accepted_or_blocked_domains(void)));
  connect(popup_menu,
	  SIGNAL(show_cookies(void)),
	  this,
	  SIGNAL(show_cookies(void)));
  connect(popup_menu,
	  SIGNAL(show_history(void)),
	  this,
	  SIGNAL(show_history(void)));
  connect(popup_menu,
	  SIGNAL(quit_dooble(void)),
	  this,
	  SIGNAL(quit_dooble(void)));
  connect(popup_menu,
	  SIGNAL(show_settings(void)),
	  this,
	  SIGNAL(show_settings(void)));
  connect(popup_menu,
	  SIGNAL(zoom_in(void)),
	  this,
	  SLOT(slot_zoom_in(void)));
  connect(popup_menu,
	  SIGNAL(zoom_out(void)),
	  this,
	  SLOT(slot_zoom_out(void)));
  connect(popup_menu,
	  SIGNAL(zoom_reset(void)),
	  this,
	  SLOT(slot_zoom_reset(void)));
  connect(this,
	  SIGNAL(zoomed(qreal)),
	  popup_menu,
	  SLOT(slot_zoomed(qreal)));
  popup_menu->resize(popup_menu->sizeHint());
  size = popup_menu->size();
  widget_action.setDefaultWidget(popup_menu);
  menu.addAction(&widget_action);
  point.setX(m_ui.menu->size().width() + point.x() - size.width());
  point.setY(m_ui.menu->size().height() + point.y());
  menu.exec(mapToGlobal(point));
  m_ui.menu->setChecked(false);
}

void dooble_page::slot_about_to_show_standard_menus(void)
{
  if(m_action_close_tab)
    if(qobject_cast<QStackedWidget *> (parentWidget()))
      m_action_close_tab->setEnabled
	(qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  if(m_full_screen_action)
    {
      dooble *d = find_parent_dooble();

      if(d)
	{
	  if(d->isFullScreen())
	    m_full_screen_action->setText(tr("Show &Normal Screen"));
	  else
	    m_full_screen_action->setText(tr("Show &Full Screen"));
	}
    }
}

void dooble_page::slot_accepted_or_blocked_add_exception(void)
{
}

void dooble_page::slot_accepted_or_blocked_clicked(void)
{
  QMenu menu(this);

  if(!m_view->url().isEmpty() && m_view->url().isValid())
    {
      menu.addAction
	(tr("Add only this page as an exception."),
	 this,
	 SLOT(slot_accepted_or_blocked_add_exception(void)));
      menu.addAction
	(tr("Add the host %1 as an exception.").arg(m_view->url().host()),
	 this,
	 SLOT(slot_accepted_or_blocked_add_exception(void)));
      menu.addSeparator();

      if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
	menu.addAction(tr("Show Accepted / Blocked preferences."),
		       this,
		       SIGNAL(show_accepted_or_blocked_domains(void)));
      else
	menu.addAction(tr("Show Accepted / Blocked preferences..."),
		       this,
		       SIGNAL(show_accepted_or_blocked_domains(void)));
    }
  else
    menu.addAction(tr("The page's URL is empty or invalid."));

  menu.exec(m_ui.accepted_or_blocked->
	    mapToGlobal(m_ui.accepted_or_blocked->rect().bottomLeft()));
  m_ui.accepted_or_blocked->setChecked(false);
}

void dooble_page::slot_always_allow_javascript_popup(void)
{
  m_ui.javascript_popup_message->setVisible(false);

  while(!m_last_javascript_popups.isEmpty())
    {
      QPointer<dooble_web_engine_view> view
	(m_last_javascript_popups.takeFirst());

      if(view && view->parent() == this)
	emit create_dialog(view);
    }

  emit javascript_allow_popup_exception(url());
}

void dooble_page::slot_authentication_required(const QUrl &url,
					       QAuthenticator *authenticator)
{
  if(!authenticator || authenticator->isNull() || !url.isValid())
    return;

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);
  ui.label->setText
    (tr("The site %1 is requesting credentials.").arg(url.toString()));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    m_view->stop();
}

void dooble_page::slot_close_javascript_popup_exception_frame(void)
{
  m_ui.javascript_popup_message->setVisible(false);

  while(!m_last_javascript_popups.isEmpty())
    {
      QPointer<dooble_web_engine_view> view
	(m_last_javascript_popups.takeFirst());

      if(view)
	view->deleteLater();
    }
}

void dooble_page::slot_create_dialog_request(dooble_web_engine_view *view)
{
  if(view)
    {
      view->setParent(this);
      m_last_javascript_popups.append(view);
    }
  else if(m_last_javascript_popups.isEmpty())
    return;

  QFontMetrics font_metrics(m_ui.javascript_popup_exception_url->fontMetrics());
  QString text(tr("A dialog from %1 has been blocked.").arg(url().toString()));

  m_ui.javascript_popup_exception_url->setText
    (font_metrics.elidedText(text, Qt::ElideMiddle, width()));
  m_ui.javascript_popup_message->setVisible(true);
}

void dooble_page::slot_dooble_credentials_authenticated(bool state)
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

void dooble_page::slot_dooble_credentials_created(void)
{
}

void dooble_page::slot_escape(void)
{
  if(m_ui.find->hasFocus())
    m_ui.find_frame->setVisible(false);
  else
    {
      m_ui.address->setText(m_view->url().toString());
      m_view->stop();
    }
}

void dooble_page::slot_find_next(void)
{
  slot_find_text_edited(m_ui.find->text());
}

void dooble_page::slot_find_previous(void)
{
  QString text(m_ui.find->text());

  if(m_ui.find_match_case->isChecked())
    find_text
      (QWebEnginePage::FindFlag(QWebEnginePage::FindBackward |
				QWebEnginePage::FindCaseSensitively),
       text);
  else
    find_text(QWebEnginePage::FindBackward, text);
}

void dooble_page::slot_find_text_edited(const QString &text)
{
  if(m_ui.find_match_case->isChecked())
    find_text(QWebEnginePage::FindCaseSensitively, text);
  else
    find_text(QWebEnginePage::FindFlags(), text);
}

void dooble_page::slot_go_backward(void)
{
  m_view->history()->back();
}

void dooble_page::slot_go_forward(void)
{
  m_view->history()->forward();
}

void dooble_page::slot_go_to_backward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_backward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_forward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_forward_item(action->property("index").toInt());
}

void dooble_page::slot_icon_changed(const QIcon &icon)
{
  if(dooble::s_history && !m_is_private)
    dooble::s_history->save_favicon(icon, m_view->url());

  if(!m_is_private)
    dooble_favicons::save_favicon(icon, m_view->url());

  m_ui.address->set_item_icon(icon, m_view->url());
}

void dooble_page::slot_javascript_allow_popup_exception(void)
{
  QMenu menu(this);

  menu.addAction
    (tr("Always"), this, SLOT(slot_always_allow_javascript_popup(void)));
  menu.addAction
    (tr("Now Only"), this, SLOT(slot_only_now_allow_javascript_popup(void)));
  menu.addSeparator();

  if(dooble_settings::setting("pin_settings_window").toBool())
    menu.addAction(tr("Show pop-up preferences."),
		   this,
		   SLOT(slot_show_web_settings_panel(void)));
  else
    menu.addAction(tr("Show pop-up preferences..."),
		   this,
		   SLOT(slot_show_web_settings_panel(void)));

  if(!m_last_javascript_popups.isEmpty())
    {
      QFontMetrics font_metrics(menu.fontMetrics());

      menu.addSeparator();

      for(int i = 0; i < m_last_javascript_popups.size(); i++)
	{
	  QPointer<dooble_web_engine_view> view(m_last_javascript_popups.at(i));

	  if(view)
	    menu.addAction
	      (font_metrics.elidedText(tr("Show %1").arg(view->url().
							 toString()) + "...",
				       Qt::ElideMiddle,
				       dooble_ui_utilities::
				       context_menu_width(&menu)),
	       this,
	       SLOT(slot_show_popup(void)))->setProperty("index", i);
	}
    }

  menu.setStyleSheet("QMenu {menu-scrollable: 1;}");
  menu.exec
    (m_ui.javascript_allow_popup_exception->
     mapToGlobal(m_ui.javascript_allow_popup_exception->rect().bottomLeft()));
  m_ui.javascript_allow_popup_exception->setChecked(false);
}

void dooble_page::slot_link_hovered(const QString &url)
{
  QFontMetrics font_metrics(m_ui.link_hovered->fontMetrics());
  int difference = 15;

  if(m_ui.is_private->isVisible())
    difference += 25;

  if(m_ui.progress->isVisible())
    difference += m_ui.progress->width();

  m_ui.link_hovered->setProperty("text", url);
  m_ui.link_hovered->setText
    (font_metrics.
     elidedText(url.trimmed(), Qt::ElideMiddle, qAbs(width() - difference)));
  m_ui.link_hovered->setCursorPosition(0);
}

void dooble_page::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  if(dooble::s_history && !m_is_private)
    dooble::s_history->save_item(icon(), m_view->history()->currentItem());

  if(!m_is_private)
    dooble_favicons::save_favicon(icon(), m_view->url());

  m_ui.progress->setVisible(false);

  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.reload->setIcon(QIcon(QString(":/%1/36/reload.png").arg(icon_set)));
  m_ui.reload->setToolTip(tr("Reload"));
  emit iconChanged(icon());
}

void dooble_page::slot_load_page(void)
{
  load(QUrl::fromUserInput(m_ui.address->text().trimmed()));
}

void dooble_page::slot_load_progress(int progress)
{
  m_ui.backward->setEnabled(m_view->history()->canGoBack());
  m_ui.forward->setEnabled(m_view->history()->canGoForward());
  m_ui.progress->setValue(progress);
}

void dooble_page::slot_load_started(void)
{
  emit iconChanged(QIcon());
  emit titleChanged("");

  while(!m_last_javascript_popups.isEmpty())
    {
      QPointer<dooble_web_engine_view> view
	(m_last_javascript_popups.takeFirst());

      if(view && view->parent() == this)
	view->deleteLater();
    }

  m_ui.javascript_popup_message->setVisible(false);
  m_ui.progress->setVisible(true);

  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.reload->setIcon(QIcon(QString(":/%1/36/stop.png").arg(icon_set)));
  m_ui.reload->setToolTip(tr("Stop Page Load"));
}

void dooble_page::slot_only_now_allow_javascript_popup(void)
{
  m_ui.javascript_popup_message->setVisible(false);

  while(!m_last_javascript_popups.isEmpty())
    {
      QPointer<dooble_web_engine_view> view
	(m_last_javascript_popups.takeFirst());

      if(view && view->parent() == this)
	emit create_dialog(view);
    }
}

void dooble_page::slot_open_url(void)
{
  m_ui.address->selectAll();
  m_ui.address->setFocus();
}

void dooble_page::slot_prepare_backward_menu(void)
{
  m_ui.backward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.backward->setEnabled(items.size() > 0);

  for(int i = items.size() - 1; i >= 0; i--)
    {
      QAction *action = 0;
      QIcon icon(dooble_favicons::icon(items.at(i).url()));
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString();

      action = m_ui.backward->menu()->addAction
	(icon, title, this, SLOT(slot_go_to_backward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_prepare_forward_menu(void)
{
  m_ui.forward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.forward->setEnabled(items.size() > 0);

  for(int i = 0; i < items.size(); i++)
    {
      QAction *action = 0;
      QIcon icon(dooble_favicons::icon(items.at(i).url()));
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString();

      action = m_ui.forward->menu()->addAction
	(icon, title, this, SLOT(slot_go_to_forward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_print_preview(QPrinter *printer)
{
  if(!printer)
    return;

  delete printer;
}

void dooble_page::slot_proxy_authentication_required
(const QUrl &url, QAuthenticator *authenticator, const QString &proxy_host)
{
  if(!authenticator ||
     authenticator->isNull() ||
     proxy_host.isEmpty() ||
     !url.isValid())
    return;

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);
  dialog.setWindowTitle(tr("Dooble: Proxy Authentication"));
  ui.label->setText(tr("The proxy %1 is requesting credentials.").
		    arg(proxy_host));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    m_view->stop();
}

void dooble_page::slot_reload_or_stop(void)
{
  if(m_ui.progress->isVisible())
    m_view->stop();
  else
    m_view->reload();
}

void dooble_page::slot_reset_url(void)
{
  m_ui.address->setText(m_view->url().toString());
  m_ui.address->selectAll();
}

void dooble_page::slot_settings_applied(void)
{
  if(dooble_settings::setting("denote_private_widgets").toBool())
    m_ui.is_private->setVisible(m_is_private);
  else
    m_ui.is_private->setVisible(false);

  prepare_icons();
}

void dooble_page::slot_show_certificate_exception(void)
{
  QMenu menu(this);
  QWidget widget(&menu);
  QWidgetAction widget_action(&menu);
  dooble_certificate_exceptions_menu_widget
    *certificate_exceptions_menu_widget =
    new dooble_certificate_exceptions_menu_widget(&widget);

  connect(certificate_exceptions_menu_widget,
	  SIGNAL(triggered(void)),
	  &menu,
	  SLOT(close(void)));
  certificate_exceptions_menu_widget->set_url(m_view->url());
  widget_action.setDefaultWidget(certificate_exceptions_menu_widget);
  menu.addAction(&widget_action);
  menu.exec(m_ui.address->
	    mapToGlobal(m_ui.address->information_rectangle().bottomLeft()));
}

void dooble_page::slot_show_find(void)
{
  m_ui.find->selectAll();
  m_ui.find->setFocus();
  m_ui.find_frame->setVisible(true);
}

void dooble_page::slot_show_popup(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  int index = action->property("index").toInt();

  if(index < 0 || index > m_last_javascript_popups.size())
    return;

  QPointer<dooble_web_engine_view> view(m_last_javascript_popups.at(index));

  if(view)
    emit create_dialog(view);

  m_last_javascript_popups.remove(index);

  if(m_last_javascript_popups.isEmpty())
    m_ui.javascript_popup_message->setVisible(false);
}

void dooble_page::slot_show_popup_menu(void)
{
  show_popup_menu();
}

void dooble_page::slot_show_pull_down_menu(void)
{
  m_ui.address->complete();
}

void dooble_page::slot_show_status_bar(bool state)
{
  m_ui.status_bar->setVisible(state);
  dooble_settings::set_setting("status_bar_visible", state);
}

void dooble_page::slot_show_web_settings_panel(void)
{
  emit show_settings_panel(dooble_settings::WEB_PANEL);
}

void dooble_page::slot_url_changed(const QUrl &url)
{
  m_ui.address->add_item(m_view->icon(), url);
  m_ui.address->setText(url.toString());
}

void dooble_page::slot_zoom_in(void)
{
  qreal zoom_factor = qMin(m_view->zoomFactor() + 0.10, 5.0);

  m_view->setZoomFactor(zoom_factor);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_zoom_out(void)
{
  qreal zoom_factor = qMax(m_view->zoomFactor() - 0.10, 0.25);

  m_view->setZoomFactor(zoom_factor);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_zoom_reset(void)
{
  m_view->setZoomFactor(1.0);
  emit zoomed(m_view->zoomFactor());
}
