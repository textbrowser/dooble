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
#include <QDir>
#include <QPainter>
#include <QProcess>
#include <QToolTip>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QWebEngineFindTextResult>
#endif
#include <QWebEngineHistoryItem>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
#include <QWebEngineLoadingInfo>
#endif
#include <QWebEngineProfile>
#include <QWidgetAction>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_favorites_popup.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_javascript.h"
#include "dooble_page.h"
#include "dooble_popup_menu.h"
#include "dooble_search_engines_popup.h"
#include "dooble_style_sheet.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_page.h"
#include "dooble_web_engine_view.h"
#include "ui_dooble_authentication_dialog.h"

dooble_page::dooble_page(QWebEngineProfile *web_engine_profile,
			 dooble_web_engine_view *view,
			 QWidget *parent):QWidget(parent)
{
  auto zoom_factor = dooble_settings::setting("zoom", 100.0).toDouble() / 100.0;

  m_export_as_png = false;
  m_export_png_timer.setSingleShot(true);
  m_is_location_frame_user_hidden = false;
  m_is_private = QWebEngineProfile::defaultProfile() != web_engine_profile &&
    web_engine_profile;
  m_menu = new QMenu(this);
  m_popup_menu = new dooble_popup_menu(zoom_factor, this);
  m_popup_menu->resize(m_popup_menu->minimumSize());
  m_popup_menu->set_accept_on_click(false);
  m_reload_periodically_seconds = 0;
  m_ui.setupUi(this);
  m_ui.backward->setEnabled(false);
  m_ui.backward->setMenu(new QMenu(this));
  m_ui.downloads->setMenu(new QMenu(this));
  m_ui.downloads->menu()->addAction(tr("Clear Downloads"),
				    this,
				    SIGNAL(clear_downloads(void)));
  m_ui.feature_permission_popup_message->setVisible(false);
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
  m_ui.zoom_value->setVisible(false);

  if(view)
    {
      m_view = view;
      m_view->setParent(this);
      slot_url_changed(m_view->url());
    }
  else
    m_view = new dooble_web_engine_view(web_engine_profile, this);

  m_ui.address->set_view(m_view);
  m_ui.frame->layout()->addWidget(m_view);

  if(parent)
    m_view->resize(parent->size());
  else
    {
      auto d = find_parent_dooble();

      if(d)
	m_view->resize(d->size());
    }

  connect(&m_export_png_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_export_as_png_timer_timeout(void)));
  connect(&m_reload_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_reload_periodically(void)));
  connect(dooble::s_downloads,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_downloads_finished(void)));
  connect(dooble::s_downloads,
	  SIGNAL(started(void)),
	  this,
	  SLOT(slot_downloads_started(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  m_popup_menu,
	  SLOT(slot_settings_applied(void)));
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
  connect(m_popup_menu,
	  SIGNAL(authenticate(void)),
	  this,
	  SIGNAL(authenticate(void)));
  connect(m_popup_menu,
	  SIGNAL(save(void)),
	  this,
	  SIGNAL(save(void)));
  connect(m_popup_menu,
	  SIGNAL(show_accepted_or_blocked_domains(void)),
	  this,
	  SIGNAL(show_accepted_or_blocked_domains(void)));
  connect(m_popup_menu,
	  SIGNAL(show_cookies(void)),
	  this,
	  SIGNAL(show_cookies(void)));
  connect(m_popup_menu,
	  SIGNAL(show_history(void)),
	  this,
	  SIGNAL(show_history(void)));
  connect(m_popup_menu,
	  SIGNAL(quit_dooble(void)),
	  this,
	  SIGNAL(quit_dooble(void)));
  connect(m_popup_menu,
	  SIGNAL(show_settings(void)),
	  this,
	  SIGNAL(show_settings(void)));
  connect(m_popup_menu,
	  SIGNAL(zoom_in(void)),
	  this,
	  SLOT(slot_zoom_in(void)));
  connect(m_popup_menu,
	  SIGNAL(zoom_out(void)),
	  this,
	  SLOT(slot_zoom_out(void)));
  connect(m_popup_menu,
	  SIGNAL(zoom_reset(void)),
	  this,
	  SLOT(slot_zoom_reset(void)));
  connect(m_ui.accepted_or_blocked,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_accepted_or_blocked_clicked(void)));
  connect(m_ui.address,
	  SIGNAL(favorite_changed(const QUrl &, bool)),
	  this,
	  SLOT(slot_favorite_changed(const QUrl &, bool)));
  connect(m_ui.address,
	  SIGNAL(inject_custom_css(void)),
	  this,
	  SLOT(slot_inject_custom_css(void)));
  connect(m_ui.address,
	  SIGNAL(javascript_console(void)),
	  this,
	  SLOT(slot_javascript_console(void)));
  connect(m_ui.address,
	  SIGNAL(load_page(const QUrl &)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.address,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.address,
	  SIGNAL(pull_down_clicked(void)),
	  this,
	  SLOT(slot_show_pull_down_menu(void)));
  connect(m_ui.address,
	  SIGNAL(show_certificate_exception(void)),
	  this,
	  SLOT(slot_show_certificate_exception(void)));
  connect(m_ui.address,
	  SIGNAL(show_site_cookies(void)),
	  this,
	  SIGNAL(show_site_cookies(void)));
  connect(m_ui.address,
	  SIGNAL(zoom_reset(void)),
	  this,
	  SLOT(slot_zoom_reset(void)));
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
  connect(m_ui.feature_permission_allow,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_feature_permission_allow(void)));
  connect(m_ui.feature_permission_deny,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_feature_permission_deny(void)));
  connect(m_ui.favorites,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_show_favorites_popup(void)));
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
  connect(m_ui.home,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_home(void)));
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
  connect(m_ui.zoom_value,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_zoom_reset(void)));
  connect(m_view,
	  SIGNAL(create_dialog(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_dialog(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_dialog_request(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_dialog_request(dooble_web_engine_view *)),
	  Qt::QueuedConnection);
  connect(m_view,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_tab(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_window(dooble_web_engine_view *)));
  connect
    (m_view,
     SIGNAL(featurePermissionRequestCanceled(const QUrl &,
					     QWebEnginePage::Feature)),
     this,
     SLOT(slot_feature_permission_request_canceled(const QUrl &,
						   QWebEnginePage::Feature)));
  connect(m_view,
	  SIGNAL(featurePermissionRequested(const QUrl &,
					    QWebEnginePage::Feature)),
	  this,
	  SLOT(slot_feature_permission_requested(const QUrl &,
						 QWebEnginePage::Feature)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SIGNAL(iconChanged(const QIcon &)),
	  Qt::QueuedConnection); // Prevent favicon flicker.
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  m_ui.address,
	  SLOT(slot_load_finished(bool)));
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
	  SIGNAL(open_link_in_current_page(const QUrl &)),
	  this,
	  SLOT(slot_open_link(const QUrl &)));
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
	  SIGNAL(peekaboo_text(const QString &)),
	  this,
	  SIGNAL(peekaboo_text(const QString &)));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  connect(m_view,
	  SIGNAL(printRequested(void)),
	  this,
	  SIGNAL(print(void)));
#endif
  connect(m_view,
	  SIGNAL(show_full_screen(bool)),
	  this,
	  SIGNAL(show_full_screen(bool)));
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
  connect(m_view,
	  SIGNAL(windowCloseRequested(void)),
	  this,
	  SIGNAL(windowCloseRequested(void)));
  connect(m_view->page(),
	  SIGNAL(authenticationRequired(const QUrl &, QAuthenticator *)),
	  this,
	  SLOT(slot_authentication_required(const QUrl &, QAuthenticator *)));
  connect(m_view->page(),
	  SIGNAL(linkHovered(const QString &)),
	  this,
	  SLOT(slot_link_hovered(const QString &)));
  connect(m_view->page(),
	  SIGNAL(loading(const QUrl &)),
	  this,
	  SLOT(slot_loading(const QUrl &)));
#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
  connect(m_view->page(),
	  SIGNAL(loadingChanged(const QWebEngineLoadingInfo &)),
	  this,
	  SLOT(slot_loading_changed(const QWebEngineLoadingInfo &)));
#endif
  connect(m_view->page(),
	  SIGNAL(proxyAuthenticationRequired(const QUrl &,
					     QAuthenticator *,
					     const QString &)),
	  this,
	  SLOT(slot_proxy_authentication_required(const QUrl &,
						  QAuthenticator *,
						  const QString &)));
  connect(m_view->page(),
	  SIGNAL(scrollPositionChanged(const QPointF &)),
	  this,
	  SLOT(slot_scroll_position_changed(const QPointF &)));
  connect(this,
	  SIGNAL(javascript_allow_popup_exception(const QUrl &)),
	  dooble::s_settings,
	  SLOT(slot_new_javascript_block_popup_exception(const QUrl &)));
  connect(this,
	  SIGNAL(zoomed(qreal)),
	  m_popup_menu,
	  SLOT(slot_zoomed(qreal)));
  connect(this,
	  SIGNAL(zoomed(qreal)),
	  m_ui.address,
	  SLOT(slot_zoomed(qreal)));
  m_progress_label = new QLabel(m_ui.frame);
  m_progress_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_progress_label->setIndent(5);
  m_progress_label->setMinimumHeight
    (10 + m_progress_label->sizeHint().height());
  m_progress_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_progress_label->setStyleSheet("QLabel {background-color: #e0e0e0;}");
  m_progress_label->setVisible(false);
  prepare_icons();
  prepare_shortcuts();
  prepare_standard_menus();
  prepare_style_sheets();
  prepare_tool_buttons();
  m_view->setZoomFactor(zoom_factor);
  prepare_zoom_toolbutton(zoom_factor);
  slot_dooble_credentials_created();
  QTimer::singleShot(1000, this, SLOT(slot_zoomed(void)));
}

dooble_page::~dooble_page()
{
  foreach(const auto &view, m_last_javascript_popups)
    if(view && view->parent() == this)
      view->deleteLater();

  foreach(auto shortcut, m_shortcuts)
    delete shortcut;
}

QAction *dooble_page::action_close_tab(void) const
{
  return m_action_close_tab;
}

QAction *dooble_page::full_screen_action(void) const
{
  return m_full_screen_action;
}

QFrame *dooble_page::frame(void) const
{
  return m_ui.frame;
}

QIcon dooble_page::icon(void) const
{
  if(m_view->icon().isNull())
    return dooble_favicons::icon(m_view->url());
  else
    return m_view->icon();
}

QMenu *dooble_page::menu(void)
{
  prepare_standard_menus();
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

QWebEngineProfile *dooble_page::web_engine_profile(void) const
{
  return m_view->page()->profile();
}

QWebEngineSettings *dooble_page::web_engine_settings(void) const
{
  return m_view->settings();
}

bool dooble_page::can_go_back(void) const
{
  return m_view->history()->canGoBack();
}

bool dooble_page::can_go_forward(void) const
{
  return m_view->history()->canGoForward();
}

bool dooble_page::is_location_frame_user_hidden(void) const
{
  return m_is_location_frame_user_hidden;
}

bool dooble_page::is_private(void) const
{
  return m_is_private;
}

bool dooble_page::is_web_setting_enabled
(QWebEngineSettings::WebAttribute setting) const
{
  auto settings = m_view->settings();

  if(settings)
    return settings->testAttribute(setting);
  else
    return false;
}

dooble *dooble_page::find_parent_dooble(void) const
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto parent = parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	{
	  QApplication::restoreOverrideCursor();
	  return qobject_cast<dooble *> (parent);
	}
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  QApplication::restoreOverrideCursor();
  return nullptr;
}

dooble_address_widget *dooble_page::address_widget(void) const
{
  return m_ui.address;
}

dooble_popup_menu *dooble_page::popup_menu(void) const
{
  return m_popup_menu;
}

dooble_web_engine_view *dooble_page::view(void) const
{
  return m_view;
}

int dooble_page::reload_periodically_seconds(void) const
{
  return m_reload_periodically_seconds;
}

void dooble_page::download(const QString &file_name, const QUrl &url)
{
  m_view->download(file_name, url);
}

void dooble_page::enable_web_setting
(QWebEngineSettings::WebAttribute setting, bool state)
{
  auto settings = m_view->settings();

  if(settings)
    settings->setAttribute(setting, state);
}

void dooble_page::find_text(QWebEnginePage::FindFlags find_flags,
			    const QString &text)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_view->findText
    (text,
     find_flags,
     [=] (bool found)
#else
  m_view->findText
    (text,
     find_flags,
     [=] (const QWebEngineFindTextResult &result)
#endif
     {
       static QPalette s_palette(m_ui.find->palette());

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
       if(!found)
#else
       if(result.numberOfMatches() == 0)
#endif
	 {
	   if(!text.isEmpty())
	     {
	       QColor color(240, 128, 128); // Light Coral
	       auto palette(m_ui.find->palette());

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
  auto items
    (m_view->history()->
     backItems(static_cast<int> (dooble_page::ConstantsEnum::
				 MAXIMUM_HISTORY_ITEMS)));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::go_to_forward_item(int index)
{
  auto items
    (m_view->history()->
     forwardItems(static_cast<int> (dooble_page::ConstantsEnum::
				    MAXIMUM_HISTORY_ITEMS)));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::hide_location_frame(bool state)
{
  m_ui.top_frame->setVisible(!state);
}

void dooble_page::hide_status_bar(bool state)
{
  m_ui.status_bar->setVisible(!state);
  prepare_progress_label_position(false);
}

void dooble_page::inject_custom_css(void)
{
  slot_inject_custom_css();
}

 void dooble_page::javascript_console(void)
{
  slot_javascript_console();
}

void dooble_page::load(const QUrl &url)
{
  m_view->stop();
  m_view->load(url);
  m_view->setUrl(url); // Set the address widget's text.
}

void dooble_page::prepare_export_as_png(const QString &file_name)
{
  if(!m_export_as_png_progress_dialog)
    {
      m_export_as_png_progress_dialog = new QProgressDialog
	(tr("Exporting the page. Please remain calm."),
	 QString(),
	 0,
	 0,
	 this,
	 Qt::Dialog);
      m_export_as_png_progress_dialog->setWindowModality(Qt::ApplicationModal);
      m_export_as_png_progress_dialog->setWindowTitle
	(tr("Dooble: Exporting Page"));
    }

  m_export_as_png = false;
  m_export_as_png_progress_dialog->show();
  m_export_png_file_name = file_name;
  m_view->page()->runJavaScript("window.scrollTo(0, 0);");
  QTimer::singleShot(2500, this, SLOT(slot_scroll_to_top_finished(void)));
}

void dooble_page::prepare_icons(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(m_find_action)
    m_find_action->setIcon
      (QIcon::fromTheme(use_material_icons + "edit-find",
			QIcon(QString(":/%1/18/find.png").arg(icon_set))));

  if(m_settings_action)
    m_settings_action->setIcon
      (QIcon::fromTheme(use_material_icons + "preferences-system",
			QIcon(QString(":/%1/18/settings.png").arg(icon_set))));

  m_ui.accepted_or_blocked->setIcon
    (QIcon::fromTheme(use_material_icons + "process-stop",
		      QIcon(QString(":/%1/36/blocked_domains.png").
			    arg(icon_set))));
  m_ui.backward->setIcon
    (QIcon::fromTheme(use_material_icons + "go-previous",
		      QIcon(QString(":/%1/36/backward.png").arg(icon_set))));

  if(dooble::s_downloads->is_finished())
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads.png").arg(icon_set))));
  else
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads_active.png").
			      arg(icon_set))));

  m_ui.favorites->setIcon
    (QIcon::fromTheme(use_material_icons + "emblem-favorite",
		      QIcon(QString(":/%1/36/favorites.png").arg(icon_set))));
  m_ui.find_next->setIcon
    (QIcon::fromTheme(use_material_icons + "go-next",
		      QIcon(QString(":/%1/20/next.png").arg(icon_set))));
  m_ui.find_previous->setIcon
    (QIcon::fromTheme(use_material_icons + "go-previous",
		      QIcon(QString(":/%1/20/previous.png").arg(icon_set))));
  m_ui.find_stop->setIcon(QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
  m_ui.forward->setIcon
    (QIcon::fromTheme(use_material_icons + "go-next",
		      QIcon(QString(":/%1/36/forward.png").arg(icon_set))));
  m_ui.is_private->setPixmap
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/18/private.png").
			    arg(icon_set))).pixmap(QSize(16, 16)));
  m_ui.close_javascript_popup_exception_frame->setIcon
    (QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
  m_ui.home->setIcon
    (QIcon::fromTheme(use_material_icons + "go-home",
		      QIcon(QString(":/%1/36/home.png").arg(icon_set))));
  m_ui.menu->setIcon
    (QIcon::fromTheme(use_material_icons + "application-menu",
		      QIcon(QString(":/%1/36/menu.png").arg(icon_set))));
  m_ui.reload->setIcon
    (QIcon::fromTheme(use_material_icons + "view-refresh",
		      QIcon(QString(":/%1/36/reload.png").arg(icon_set))));
}

void dooble_page::prepare_progress_label_position(bool process_events)
{
  if(process_events)
    QApplication::processEvents();

  auto y = m_ui.frame->height() - m_progress_label->height() - 1;

  m_progress_label->move(1, y);
}

void dooble_page::prepare_shortcuts(void)
{
  if(m_shortcuts.isEmpty())
    {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      m_shortcuts << new QShortcut
	(QKeySequence(Qt::AltModifier + Qt::Key_Left),
	 this,
	 SLOT(slot_go_backward(void)));
      m_shortcuts << new QShortcut
	(QKeySequence(Qt::AltModifier + Qt::Key_Right),
	 this,
	 SLOT(slot_go_forward(void)));
#else
      m_shortcuts << new QShortcut
	(QKeySequence(Qt::AltModifier | Qt::Key_Left),
	 this,
	 SLOT(slot_go_backward(void)));
      m_shortcuts << new QShortcut
	(QKeySequence(Qt::AltModifier | Qt::Key_Right),
	 this,
	 SLOT(slot_go_forward(void)));
#endif
      m_shortcuts << new QShortcut(QKeySequence(Qt::Key_Escape),
				   this,
				   SLOT(slot_escape(void)));
      m_shortcuts << new QShortcut(QKeySequence(Qt::Key_F5),
				   this,
				   SLOT(slot_reload(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+=")),
				   this,
				   SLOT(slot_zoom_in(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+-")),
				   this,
				   SLOT(slot_zoom_out(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+0")),
				   this,
				   SLOT(slot_zoom_reset(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+F")),
				   this,
				   SLOT(slot_show_find(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+L")),
				   this,
				   SLOT(slot_open_link(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+O")),
				   this,
				   SIGNAL(open_local_file(void)));
      m_shortcuts << new QShortcut(QKeySequence(tr("Ctrl+R")),
				   this,
				   SLOT(slot_reload(void)));
    }
}

void dooble_page::prepare_standard_menus(void)
{
  if(!m_menu->actions().isEmpty())
    return;

  QAction *action = nullptr;
  QMenu *menu = nullptr;
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  /*
  ** File Menu
  */

  menu = m_menu->addMenu(tr("&File"));
  m_authentication_action = menu->addAction
    (QIcon::fromTheme(use_material_icons + "dialog-password",
		      QIcon(QString(":/%1/36/authenticate.png").arg(icon_set))),
     tr("&Authenticate..."),
     this,
     SIGNAL(authenticate(void)));

  if(dooble_settings::has_dooble_credentials())
    m_authentication_action->setEnabled
      (dooble::s_cryptography && !dooble::s_cryptography->authenticated());
  else
    m_authentication_action->setEnabled(false);

  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/48/new_private_window.png").
			    arg(icon_set))),
     tr("New P&rivate Window..."),
     QKeySequence(tr("Ctrl+Shift+P")),
     this,
     SIGNAL(new_private_window(void)));
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "folder-new",
		      QIcon(QString(":/%1/48/new_tab.png").arg(icon_set))),
     tr("New &Tab"),
     QKeySequence(tr("Ctrl+T")),
     this,
     SIGNAL(new_tab(void)));
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "window-new",
		      QIcon(QString(":/%1/48/new_window.png").arg(icon_set))),
     tr("&New Window..."),
     QKeySequence(tr("Ctrl+N")),
     this,
     SIGNAL(new_window(void)));
  menu->addAction(tr("&Open File..."),
		  QKeySequence(tr("Ctrl+O")),
		  this,
		  SIGNAL(open_local_file(void)));
  menu->addAction(tr("Open UR&L"),
		  QKeySequence(tr("Ctrl+L")),
		  this,
		  SLOT(slot_open_link(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/48/new_private_window.png").
			    arg(icon_set))),
     tr("New P&rivate Window..."),
     this,
     SIGNAL(new_private_window(void)),
     QKeySequence(tr("Ctrl+Shift+P")));
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "folder-new",
		      QIcon(QString(":/%1/48/new_tab.png").arg(icon_set))),
     tr("New &Tab"),
     this,
     SIGNAL(new_tab(void)),
     QKeySequence(tr("Ctrl+T")));
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "window-new",
		      QIcon(QString(":/%1/48/new_window.png").arg(icon_set))),
     tr("&New Window..."),
     this,
     SIGNAL(new_window(void)),
     QKeySequence(tr("Ctrl+N")));
  menu->addAction(tr("&Open File..."),
		  this,
		  SIGNAL(open_local_file(void)),
		  QKeySequence(tr("Ctrl+O")));
  menu->addAction(tr("Open UR&L"),
		  this,
		  SLOT(slot_open_link(void)),
		  QKeySequence(tr("Ctrl+L")));
#endif
  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  action = m_action_close_tab = menu->addAction(tr("&Close Tab"),
						QKeySequence(tr("Ctrl+W")),
						this,
						SIGNAL(close_tab(void)));
#else
  action = m_action_close_tab = menu->addAction(tr("&Close Tab"),
						this,
						SIGNAL(close_tab(void)),
						QKeySequence(tr("Ctrl+W")));
#endif

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    {
      if(qobject_cast<QStackedWidget *> (parentWidget())->count() == 1)
	action->setEnabled
	  (dooble::s_settings->setting("allow_closing_of_single_tab").toBool());
      else
	action->setEnabled
	  (qobject_cast<QStackedWidget *> (parentWidget())->count() > 0);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction(tr("Close Window"),
		  QKeySequence(tr("Ctrl+Shift+W")),
		  this,
		  SIGNAL(close_window(void)));
#else
  menu->addAction(tr("Close Window"),
		  this,
		  SIGNAL(close_window(void)),
		  QKeySequence(tr("Ctrl+Shift+W")));
#endif
  menu->addSeparator();
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "document-export",
		      QIcon(QString(":/%1/48/export.png").arg(icon_set))),
     tr("&Export As PNG..."),
     this,
     SIGNAL(export_as_png(void)));
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "document-save",
		      QIcon(QString(":/%1/48/save.png").arg(icon_set))),
     tr("&Save"),
     QKeySequence(tr("Ctrl+S")),
     this,
     SIGNAL(save(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "document-save",
		      QIcon(QString(":/%1/48/save.png").arg(icon_set))),
     tr("&Save"),
     this,
     SIGNAL(save(void)),
     QKeySequence(tr("Ctrl+S")));
#endif
  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "document-print",
		      QIcon(QString(":/%1/48/print.png").arg(icon_set))),
     tr("&Print..."),
     QKeySequence(tr("Ctrl+P")),
     this,
     SIGNAL(print(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "document-print",
		      QIcon(QString(":/%1/48/print.png").arg(icon_set))),
     tr("&Print..."),
     this,
     SIGNAL(print(void)),
     QKeySequence(tr("Ctrl+P")));
#endif
  menu->addAction(tr("Print Pre&view..."),
		  this,
		  SIGNAL(print_preview(void)));
  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "application-exit",
		      QIcon(QString(":/%1/48/exit_dooble.png").arg(icon_set))),
     tr("E&xit Dooble"),
     QKeySequence(tr("Ctrl+Q")),
     this,
     SIGNAL(quit_dooble(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "application-exit",
		      QIcon(QString(":/%1/48/exit_dooble.png").arg(icon_set))),
     tr("E&xit Dooble"),
     this,
     SIGNAL(quit_dooble(void)),
     QKeySequence(tr("Ctrl+Q")));
#endif

  if(dooble::s_settings)
    {
      foreach(auto action, menu->actions())
	if(action)
	  dooble::s_settings->add_shortcut(action);
    }

  /*
  ** Edit Menu
  */

  menu = m_menu->addMenu(tr("&Edit"));
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "edit-clear",
		      QIcon(QString(":/%1/48/clear_items.png").arg(icon_set))),
     tr("&Clear Items..."),
     this,
     SIGNAL(show_clear_items(void)));
  menu->addAction(tr("Clear &Visited Links"),
		  this,
		  SLOT(slot_clear_visited_links(void)));
  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  m_find_action = menu->addAction
    (QIcon::fromTheme(use_material_icons + "edit-find",
		      QIcon(QString(":/%1/18/find.png").arg(icon_set))),
     tr("&Find"),
     QKeySequence(tr("Ctrl+F")),
     this,
     SLOT(slot_show_find(void)));
#else
  m_find_action = menu->addAction
    (QIcon::fromTheme(use_material_icons + "edit-find",
		      QIcon(QString(":/%1/18/find.png").arg(icon_set))),
     tr("&Find"),
     this,
     SLOT(slot_show_find(void)),
     QKeySequence(tr("Ctrl+F")));
#endif
  menu->addSeparator();

  if(dooble_settings::setting("pin_settings_window").toBool())
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      m_settings_action = menu->addAction
	(QIcon::fromTheme(use_material_icons + "preferences-system",
			  QIcon(QString(":/%1/18/settings.png").arg(icon_set))),
	 tr("Settin&gs"),
	 QKeySequence(tr("Ctrl+G")),
	 this,
	 SIGNAL(show_settings(void)));
#else
      m_settings_action = menu->addAction
	(QIcon::fromTheme(use_material_icons + "preferences-system",
			  QIcon(QString(":/%1/18/settings.png").arg(icon_set))),
	 tr("Settin&gs"),
	 this,
	 SIGNAL(show_settings(void)),
	 QKeySequence(tr("Ctrl+G")));
#endif
    }
  else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      m_settings_action = menu->addAction
	(QIcon::fromTheme(use_material_icons + "preferences-system",
			  QIcon(QString(":/%1/18/settings.png").arg(icon_set))),
	 tr("Settin&gs..."),
	 QKeySequence(tr("Ctrl+G")),
	 this,
	 SIGNAL(show_settings(void)));
#else
      m_settings_action = menu->addAction
	(QIcon::fromTheme(use_material_icons + "preferences-system",
			  QIcon(QString(":/%1/18/settings.png").arg(icon_set))),
	 tr("Settin&gs..."),
	 this,
	 SIGNAL(show_settings(void)),
	 QKeySequence(tr("Ctrl+G")));
#endif
    }

  menu->addSeparator();
  menu->addAction(tr("Vacuum Databases"),
		  this,
		  SIGNAL(vacuum_databases(void)));

  if(dooble::s_settings)
    {
      foreach(auto action, menu->actions())
	if(action)
	  dooble::s_settings->add_shortcut(action);
    }

  /*
  ** Tools Menu
  */

  menu = m_menu->addMenu(tr("&Tools"));

  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    menu->addAction
      (QIcon::fromTheme(use_material_icons + "process-stop",
			QIcon(QString(":/%1/36/blocked_domains.png").
			      arg(icon_set))),
       tr("Accepted / &Blocked Domains"),
       this,
       SIGNAL(show_accepted_or_blocked_domains(void)));
  else
    menu->addAction
      (QIcon::fromTheme(use_material_icons + "process-stop",
			QIcon(QString(":/%1/36/blocked_domains.png").
			      arg(icon_set))),
       tr("Accepted / &Blocked Domains..."),
       this,
       SIGNAL(show_accepted_or_blocked_domains(void)));

  menu->addAction(tr("Certificate &Exceptions..."),
		  this,
		  SIGNAL(show_certificate_exceptions(void)));
  menu->addSeparator();

  QMenu *sub_menu = new QMenu(tr("Charts"));

  menu->addMenu(sub_menu);
  action = sub_menu->addAction(tr("XY Series"),
			       this,
			       SIGNAL(show_chart_xyseries(void)));
#ifndef DOOBLE_QTCHARTS_PRESENT
  action->setEnabled(false);
#endif
  menu->addSeparator();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "preferences-web-browser-cookies",
		      QIcon(QString(":/%1/48/cookies.png").arg(icon_set))),
     tr("Coo&kies..."),
     QKeySequence(tr("Ctrl+K")),
     this,
     SIGNAL(show_cookies(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "preferences-web-browser-cookies",
		      QIcon(QString(":/%1/48/cookies.png").arg(icon_set))),
     tr("Coo&kies..."),
     this,
     SIGNAL(show_cookies(void)),
     QKeySequence(tr("Ctrl+K")));
#endif

  if(dooble_settings::setting("pin_downloads_window").toBool())
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "folder-download",
			  QIcon(QString(":/%1/36/downloads.png").
				arg(icon_set))),
	 tr("&Downloads"),
	 QKeySequence(tr("Ctrl+D")),
	 this,
	 SIGNAL(show_downloads(void)));
#else
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "folder-download",
			  QIcon(QString(":/%1/36/downloads.png").
				arg(icon_set))),
	 tr("&Downloads"),
	 this,
	 SIGNAL(show_downloads(void)),
	 QKeySequence(tr("Ctrl+D")));
#endif
    }
  else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "folder-download",
			  QIcon(QString(":/%1/36/downloads.png").
				arg(icon_set))),
	 tr("&Downloads..."),
	 QKeySequence(tr("Ctrl+D")),
	 this,
	 SIGNAL(show_downloads(void)));
#else
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "folder-download",
			  QIcon(QString(":/%1/36/downloads.png").
				arg(icon_set))),
	 tr("&Downloads..."),
	 this,
	 SIGNAL(show_downloads(void)),
	 QKeySequence(tr("Ctrl+D")));
#endif
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "emblem-favorite",
		      QIcon(QString(":/%1/36/favorites.png").arg(icon_set))),
     tr("&Favorites..."),
     QKeySequence(tr("Ctrl+B")),
     this,
     SIGNAL(show_favorites(void)));
#else
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "emblem-favorite",
		      QIcon(QString(":/%1/36/favorites.png").arg(icon_set))),
     tr("&Favorites..."),
     this,
     SIGNAL(show_favorites(void)),
     QKeySequence(tr("Ctrl+B")));
#endif
  menu->addSeparator();
  menu->addAction(tr("Floating Digital &Clock..."),
		  this,
		  SIGNAL(show_floating_digital_clock(void)));
  menu->addSeparator();
  menu->addAction(tr("Floating History Popup..."),
		  this,
		  SIGNAL(show_floating_history_popup(void)));

  if(dooble_settings::setting("pin_history_window").toBool())
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "deep-history",
			  QIcon(QString(":/%1/36/history.png").arg(icon_set))),
	 tr("&History"),
	 QKeySequence(tr("Ctrl+H")),
	 this,
	 SIGNAL(show_history(void)));
#else
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "deep-history",
			  QIcon(QString(":/%1/36/history.png").arg(icon_set))),
	 tr("&History"),
	 this,
	 SIGNAL(show_history(void)),
	 QKeySequence(tr("Ctrl+H")));
#endif
    }
  else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "deep-history",
			  QIcon(QString(":/%1/36/history.png").arg(icon_set))),
	 tr("&History..."),
	 QKeySequence(tr("Ctrl+H")),
	 this,
	 SIGNAL(show_history(void)));
#else
      menu->addAction
	(QIcon::fromTheme(use_material_icons + "deep-history",
			  QIcon(QString(":/%1/36/history.png").arg(icon_set))),
	 tr("&History..."),
	 this,
	 SIGNAL(show_history(void)),
	 QKeySequence(tr("Ctrl+H")));
#endif
    }

  menu->addSeparator();
  menu->addAction(tr("Inject Custom Style Sheet..."),
		  this,
		  SLOT(slot_inject_custom_css(void)));
  menu->addAction(tr("JavaScript Console..."),
		  this,
		  SLOT(slot_javascript_console(void)));
  menu->addSeparator();
  menu->addAction
    (QIcon::fromTheme(use_material_icons + "application-menu",
		      QIcon(QString(":/%1/36/menu.png").arg(icon_set))),
     tr("Page Floating &Menu..."),
     this,
     SIGNAL(show_floating_menu(void)));
  menu->addSeparator();
  menu->addAction(tr("&Search Engines..."),
		  this,
		  SIGNAL(show_search_engines(void)));
  menu->addSeparator();
  menu->addAction(tr("Translate Page (Google)"),
		  this,
		  SIGNAL(translate_page(void)))->setEnabled
    (QUrl::fromUserInput(dooble::s_google_translate_url).isValid());

  if(dooble::s_settings)
    {
      foreach(auto action, menu->actions())
	if(action)
	  dooble::s_settings->add_shortcut(action);
    }

  menu->addSeparator();

  auto m = menu->addMenu(tr("Current URL Executable(s)"));

  if(dooble::current_url_executables().isEmpty())
    m->addAction(tr("Empty"));
  else
    {
      QSetIterator<QString> it(dooble::current_url_executables());
      QStringList list;

      while(it.hasNext())
	{
	  auto string(it.next().trimmed());

	  if(!string.isEmpty())
	    list << string;
	}

      std::sort(list.begin(), list.end());

      for(int i = 0; i < list.size(); i++)
	m->addAction(list.at(i),
		     this,
		     SLOT(slot_current_url_executable(void)));
    }

  /*
  ** View Menu
  */

  menu = m_menu->addMenu(tr("&View"));
  connect(menu,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_view_menu(void)));
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  m_full_screen_action = menu->addAction(tr("Show &Full Screen"),
					 QKeySequence(tr("Ctrl+F11")),
					 this,
					 SIGNAL(show_full_screen(void)));
#else
  m_full_screen_action = menu->addAction(tr("Show &Full Screen"),
					 this,
					 SIGNAL(show_full_screen(void)),
					 QKeySequence(tr("Ctrl+F11")));
#endif
  menu->addSeparator();
  action = menu->addAction(tr("&Status Bar"),
			   this,
			   SLOT(slot_show_status_bar(bool)));
  action->setCheckable(true);
  action->setChecked(dooble_settings::setting("status_bar_visible").toBool());

  if(dooble::s_settings)
    {
      foreach(auto action, menu->actions())
	if(action)
	  dooble::s_settings->add_shortcut(action);
    }

  /*
  ** Help Menu
  */

  menu = m_menu->addMenu(tr("&Help"));
  menu->addAction(QIcon(":/Logo/dooble.png"),
		  tr("&About..."),
		  this,
		  SIGNAL(show_about(void)));
  menu->addSeparator();
  menu->addAction(tr("&Documentation"),
		  this,
		  SIGNAL(show_documentation(void)));
  menu->addAction(tr("&Release Notes"),
		  this,
		  SIGNAL(show_release_notes(void)));

  if(dooble::s_settings)
    {
      foreach(auto action, menu->actions())
	if(action)
	  dooble::s_settings->add_shortcut(action);
    }
}

void dooble_page::prepare_style_sheets(void)
{
  if(dooble::s_application->style_name() == "fusion" ||
     dooble::s_application->style_name().contains("windows"))
    {
      auto theme_color(dooble_settings::setting("theme_color").toString());
      static auto link_hovered_style_sheet(m_ui.link_hovered->styleSheet());

      if(theme_color == "default")
	{
	  m_ui.find_frame->setStyleSheet("");
	  m_ui.link_hovered->setStyleSheet(link_hovered_style_sheet);
	  m_ui.status_bar->setStyleSheet("");
	  m_ui.top_frame->setStyleSheet("");
	}
      else
	{
	  m_ui.find_frame->setStyleSheet
	    (QString("QFrame {background-color: %1;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-selected-tab-color").arg(theme_color)).
		 name()));
	  m_ui.link_hovered->setStyleSheet
	    (QString("QLineEdit {background-color: %1; color: %2;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-selected-tab-color").arg(theme_color)).
		 name()).
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-status-bar-text-color").
		       arg(theme_color)).name()));
	  m_ui.status_bar->setStyleSheet
	    (QString("QFrame {background-color: %1;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-selected-tab-color").arg(theme_color)).
		 name()));
	  m_ui.top_frame->setStyleSheet
	    (QString("QFrame {background-color: %1;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-selected-tab-color").arg(theme_color)).
		 name()));
	}
    }
}

void dooble_page::prepare_tool_buttons(void)
{
#ifdef Q_OS_MACOS
  foreach(auto tool_button, findChildren<QToolButton *> ())
    if(m_ui.find_match_case == tool_button || m_ui.zoom_value == tool_button)
      {
      }
    else if(m_ui.backward == tool_button ||
	    m_ui.downloads == tool_button ||
	    m_ui.forward == tool_button)
      tool_button->setStyleSheet
	("QToolButton {border: none;}"
	 "QToolButton::menu-indicator {image: none;}");
    else if(m_ui.menu == tool_button)
      tool_button->setStyleSheet
	("QToolButton {border: none;}"
	 "QToolButton::menu-button {border: none;}"
	 "QToolButton::menu-indicator {image: none;}");
    else
      tool_button->setStyleSheet("QToolButton {border: none;}"
				 "QToolButton::menu-button {border: none;}");
#else
  foreach(auto tool_button, findChildren<QToolButton *> ())
    if(m_ui.backward == tool_button ||
       m_ui.forward == tool_button ||
       m_ui.downloads == tool_button)
      tool_button->setStyleSheet("QToolButton::menu-indicator {image: none;}");
#endif
}

void dooble_page::prepare_zoom_toolbutton(qreal zoom_factor)
{
  m_ui.zoom_value->setText
    (tr("%1%").arg(static_cast<int> (100.0 * zoom_factor)));
  m_ui.zoom_value->setVisible(zoom_factor < 1 || zoom_factor > 1);
}

void dooble_page::print_page(QPrinter *printer)
{
  if(!printer)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_view->page()->print(printer,
			[=] (bool result)
			{
			  Q_UNUSED(result);
			  delete printer;
			  QApplication::restoreOverrideCursor();
			});
#else
  m_view->print(printer);
  QApplication::restoreOverrideCursor();
#endif
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
void dooble_page::print_page
(QPrinter *printer, const QWebEngineCallback<bool> &result_callback)
{
  m_view->page()->print(printer, result_callback);
}
#endif

void dooble_page::reload(void)
{
  m_ui.address->setText(m_view->url().toString());
  m_view->reload();
}

void dooble_page::reload_periodically(int seconds)
{
  if(seconds <= 0)
    {
      m_reload_periodically_seconds = 0;
      m_reload_timer.stop();
    }
  else
    {
      m_reload_periodically_seconds = seconds;
      m_reload_timer.start(1000 * m_reload_periodically_seconds);
    }
}

void dooble_page::reset_url(void)
{
  m_ui.address->setText(m_view->url().toString());
  m_ui.address->selectAll();

  if(m_ui.address->isVisible())
    m_ui.address->setFocus();
}

void dooble_page::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  prepare_progress_label_position(false);

  auto font_metrics(m_ui.link_hovered->fontMetrics());
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
  auto point(m_ui.menu->pos());

  m_ui.menu->setChecked(true);
  point.setY(m_ui.menu->size().height() + point.y());
  m_menu->exec(mapToGlobal(point));
  m_ui.menu->setChecked(false);
}

void dooble_page::show_popup_menu(void)
{
  QMenu menu(this);
  QSize size;
  QWidgetAction widget_action(&menu);
  auto point(m_ui.menu->pos());
  auto popup_menu = new dooble_popup_menu(m_view->zoomFactor(), this);

  connect(popup_menu,
	  SIGNAL(accepted(void)),
	  &menu,
	  SLOT(close(void)));
  connect(popup_menu,
	  SIGNAL(authenticate(void)),
	  this,
	  SIGNAL(authenticate(void)));
  connect(popup_menu,
	  SIGNAL(save(void)),
	  this,
	  SIGNAL(save(void)));
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
      {
	if(qobject_cast<QStackedWidget *> (parentWidget())->count() == 1)
	  m_action_close_tab->setEnabled
	    (dooble::s_settings->setting("allow_closing_of_single_tab").
	     toBool());
	else
	  m_action_close_tab->setEnabled
	    (qobject_cast<QStackedWidget *> (parentWidget())->count() > 0);
      }

  if(m_full_screen_action)
    {
      auto d = find_parent_dooble();

      if(d)
	{
	  if(d->isFullScreen())
	    m_full_screen_action->setText(tr("Show &Normal Screen"));
	  else
	    m_full_screen_action->setText(tr("Show &Full Screen"));
	}
    }
}

void dooble_page::slot_about_to_show_view_menu(void)
{
  /*
  ** Please also review dooble.cc.
  */

#ifdef Q_OS_MACOS
  auto menu = qobject_cast<QMenu *> (sender());

  if(menu)
    menu->setMinimumWidth(menu->sizeHint().width() + 25);
#endif
}

void dooble_page::slot_accepted_or_blocked_add_exception(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  if(action->property("host").isValid())
    {
      dooble::s_accepted_or_blocked_domains->new_exception
	(action->property("host").toString());
      m_view->reload();
    }
  else if(action->property("url").isValid())
    {
      dooble::s_accepted_or_blocked_domains->new_exception
	(action->property("url").toUrl().toString());
      m_view->reload();
    }
}

void dooble_page::slot_accepted_or_blocked_clicked(void)
{
  QMenu menu(this);

  if(!m_view->url().isEmpty() && m_view->url().isValid())
    {
      menu.addAction
	(tr("Add only this page as an exception."),
	 this,
	 SLOT(slot_accepted_or_blocked_add_exception(void)))->setProperty
	("url", m_view->url());
      menu.addAction
	(tr("Add the host %1 as an exception.").arg(m_view->url().host()),
	 this,
	 SLOT(slot_accepted_or_blocked_add_exception(void)))->setProperty
	("host", m_view->url().host());
    }
  else
    menu.addAction(tr("The page's URL is empty or invalid."));

  menu.addSeparator();

  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(dooble_settings::setting("pin_accepted_or_blocked_window").toBool())
    menu.addAction
      (QIcon::fromTheme(use_material_icons + "process-stop",
			QIcon(QString(":/%1/36/blocked_domains.png").
			      arg(icon_set))),
       tr("Show Accepted / Blocked Domains preferences."),
       this,
       SIGNAL(show_accepted_or_blocked_domains(void)));
  else
    menu.addAction
      (QIcon::fromTheme(use_material_icons + "process-stop",
			QIcon(QString(":/%1/36/blocked_domains.png").
			      arg(icon_set))),
       tr("Show Accepted / Blocked Domains preferences..."),
       this,
       SIGNAL(show_accepted_or_blocked_domains(void)));

  menu.exec(m_ui.accepted_or_blocked->
	    mapToGlobal(m_ui.accepted_or_blocked->rect().bottomLeft()));
  m_ui.accepted_or_blocked->setChecked(false);
}

void dooble_page::slot_always_allow_javascript_popup(void)
{
  m_ui.javascript_popup_message->setVisible(false);
  prepare_progress_label_position();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(const auto &view, m_last_javascript_popups)
    if(view && view->parent() == this)
      emit create_dialog(view);

  m_last_javascript_popups.clear();
  QApplication::restoreOverrideCursor();

  auto action = qobject_cast<QAction *> (sender());

  if(action && action->property("url").isValid())
    emit javascript_allow_popup_exception(action->property("url").toUrl());
  else
    emit javascript_allow_popup_exception(m_view->url());
}

void dooble_page::slot_authentication_required(const QUrl &url,
					       QAuthenticator *authenticator)
{
  if(!authenticator || authenticator->isNull() || !url.isValid())
    {
      if(authenticator)
	*authenticator = QAuthenticator();

      return;
    }

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);

  foreach(auto widget, ui.button_box->findChildren<QWidget *> ())
    widget->setMinimumSize(QSize(125, 30));

  ui.label->setText
    (tr("The site <b>%1</b> is requesting credentials.").arg(url.toString()));
  dialog.setWindowTitle(tr("Dooble: Authentication"));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    {
      QApplication::processEvents();
      *authenticator = QAuthenticator();
    }
}

void dooble_page::slot_clear_visited_links(void)
{
  QWebEngineProfile::defaultProfile()->clearAllVisitedLinks();
}

void dooble_page::slot_close_javascript_popup_exception_frame(void)
{
  m_ui.javascript_popup_message->setVisible(false);
  prepare_progress_label_position();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(const auto &view, m_last_javascript_popups)
    if(view && view->parent() == this)
      view->deleteLater();

  m_last_javascript_popups.clear();
  QApplication::restoreOverrideCursor();
}

void dooble_page::slot_create_dialog_request(dooble_web_engine_view *view)
{
  if(view)
    {
      if(!m_last_javascript_popups.contains(view))
	{
	  auto size = m_last_javascript_popups.size();

	  if(size >=
	     static_cast<decltype(size)> (dooble_page::ConstantsEnum::
					  MAXIMUM_JAVASCRIPT_POPUPS))
	    {
	      view->deleteLater();
	      return;
	    }
	  else
	    {
	      m_last_javascript_popups << view;
	      view->setParent(this);
	    }
	}
      else
	{
	  view->setParent(this);
	  return;
	}
    }
  else
    return;

  QString text("");
  auto font_metrics(m_ui.javascript_popup_exception_url->fontMetrics());

  if(m_last_javascript_popups.size() == 1)
    text = tr("A dialog from <b>%1</b> has been blocked.").
      arg(m_view->url().toString());
  else
    text = tr("Dooble blocked %1 dialogs from <b>%2</b>.").
      arg(m_last_javascript_popups.size()).arg(m_view->url().toString());

  m_ui.javascript_popup_exception_url->setText
    (font_metrics.elidedText(text, Qt::ElideMiddle, width()));
  m_ui.javascript_popup_message->setVisible(true);
  prepare_progress_label_position();
}

void dooble_page::slot_current_url_executable(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  qputenv("DOOBLE_CURRENT_URL_UTF8", url().toString().toUtf8());
  QProcess::startDetached(action->text(), QStringList() << url().toString());
  qunsetenv("DOOBLE_CURRENT_URL_UTF8");
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

  m_menu->clear();
}

void dooble_page::slot_dooble_credentials_created(void)
{
  m_menu->clear();
}

void dooble_page::slot_downloads_finished(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(dooble::s_downloads->is_finished())
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads.png").arg(icon_set))));
  else
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads_active.png").
			      arg(icon_set))));
}

void dooble_page::slot_downloads_started(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(dooble::s_downloads->is_finished())
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads.png").arg(icon_set))));
  else
    m_ui.downloads->setIcon
      (QIcon::fromTheme(use_material_icons + "folder-download",
			QIcon(QString(":/%1/36/downloads_active.png").
			      arg(icon_set))));
}

void dooble_page::slot_enable_javascript(void)
{
  enable_web_setting(QWebEngineSettings::JavascriptEnabled, true);
}

void dooble_page::slot_escape(void)
{
  auto d = find_parent_dooble();

  if(d && d->isFullScreen())
    {
      auto action = m_view->pageAction(QWebEnginePage::ExitFullScreen);

      if(action)
	action->trigger();
      else
	emit show_full_screen(false);
    }
  else
    {
      if(m_ui.find->hasFocus())
	m_ui.find_frame->setVisible(false);
      else
	{
	  m_ui.address->hide_popup();
	  m_ui.address->prepare_containers_for_url(m_view->url());
	  m_view->stop();
	  reset_url();
	}
    }
}

void dooble_page::slot_export_as_png_timer_timeout(void)
{
  m_export_as_png = false;

  QPainter painter;
  QPixmap pixmap(m_view->page()->contentsSize().toSize());
  auto remainder = m_view->page()->contentsSize().toSize().height() %
    m_view->size().height();
  int y = 0;

  painter.begin(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  for(int i = 0; i < m_pixmaps.size(); i++)
    {
      auto p(m_pixmaps.at(i));

      if(i == m_pixmaps.size() - 1)
	painter.drawPixmap
	  (QRect(0, y, p.width(), p.height()),
	   p,
	   QRect(0, p.height() - remainder, p.width(), p.height()));
      else
	painter.drawPixmap
	  (QRect(0, y, p.width(), p.height()),
	   p,
	   QRect(0, 0, p.width(), p.height()));

      y += p.height();
    }

  m_pixmaps.clear();
  painter.end();
  pixmap.save(m_export_png_file_name, "PNG", 100);
  m_view->page()->runJavaScript("window.scrollTo(0, 0);");

  if(m_export_as_png_progress_dialog)
    m_export_as_png_progress_dialog->close();
}

void dooble_page::slot_favorite_changed(const QUrl &url, bool state)
{
  if(state)
    if(m_view->history()->currentItem().url() == url)
      dooble::s_history->save_item
	(m_view->icon(), m_view->history()->currentItem(), true);
}

void dooble_page::slot_feature_permission_allow(void)
{
  auto feature = m_ui.feature_permission_url->property("feature").toInt();

  m_ui.feature_permission_popup_message->setVisible(false);
  prepare_progress_label_position();

  if(feature != -1)
    m_view->set_feature_permission
      (m_ui.feature_permission_url->property("security_origin").toUrl(),
       QWebEnginePage::Feature(feature),
       QWebEnginePage::PermissionGrantedByUser);

  m_ui.feature_permission_url->setProperty("feature", -1);
  m_ui.feature_permission_url->setProperty("security_origin", QUrl());
}

void dooble_page::slot_feature_permission_deny(void)
{
  auto feature = m_ui.feature_permission_url->property("feature").toInt();

  m_ui.feature_permission_popup_message->setVisible(false);
  prepare_progress_label_position();

  if(feature != -1)
    m_view->set_feature_permission
      (m_ui.feature_permission_url->property("security_origin").toUrl(),
       QWebEnginePage::Feature(feature),
       QWebEnginePage::PermissionDeniedByUser);

  m_ui.feature_permission_url->setProperty("feature", -1);
  m_ui.feature_permission_url->setProperty("security_origin", QUrl());
}

void dooble_page::slot_feature_permission_request_canceled
(const QUrl &security_origin, QWebEnginePage::Feature feature)
{
  if(feature == m_ui.feature_permission_url->property("feature").toInt() &&
     m_ui.feature_permission_url->property("security_origin").toUrl() ==
     security_origin)
    {
      m_ui.feature_permission_popup_message->setVisible(false);
      prepare_progress_label_position();
    }
}

void dooble_page::slot_feature_permission_requested
(const QUrl &security_origin, QWebEnginePage::Feature feature)
{
  if(!dooble::s_settings->setting("features_permissions").toBool())
    {
      m_view->set_feature_permission
	(security_origin, feature, QWebEnginePage::PermissionDeniedByUser);
      return;
    }
  else if(security_origin.isEmpty() || !security_origin.isValid())
    {
      m_view->set_feature_permission
	(security_origin, feature, QWebEnginePage::PermissionDeniedByUser);
      return;
    }
  else if(m_ui.feature_permission_popup_message->isVisible())
    {
      /*
      ** Deny the feature.
      */

      m_view->set_feature_permission
	(security_origin, feature, QWebEnginePage::PermissionDeniedByUser);
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto policy = dooble_settings::site_feature_permission
    (security_origin, feature);

  QApplication::restoreOverrideCursor();

  switch(policy)
    {
    case 0:
      {
	m_ui.feature_permission_popup_message->setVisible(false);
	m_view->set_feature_permission
	  (security_origin, feature, QWebEnginePage::PermissionDeniedByUser);
	prepare_progress_label_position();
	return;
      }
    case 1:
      {
	m_ui.feature_permission_popup_message->setVisible(false);
	m_view->set_feature_permission
	  (security_origin, feature, QWebEnginePage::PermissionGrantedByUser);
	prepare_progress_label_position();
	return;
      }
    }

  m_ui.feature_permission_url->setProperty("feature", feature);
  m_ui.feature_permission_url->setProperty("security_origin", security_origin);

  switch(feature)
    {
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
    case QWebEnginePage::DesktopAudioVideoCapture:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting "
	      "Desktop Audio Video Capture access.").
	   arg(security_origin.toString()));
	break;
      }
#endif
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
    case QWebEnginePage::DesktopVideoCapture:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Desktop Video Capture access.").
	   arg(security_origin.toString()));
	break;
      }
#endif
    case QWebEnginePage::Geolocation:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Geo Location access.").
	   arg(security_origin.toString()));
	break;
      }
    case QWebEnginePage::MediaAudioCapture:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Media Audio Capture access.").
	   arg(security_origin.toString()));
	break;
      }
    case QWebEnginePage::MediaAudioVideoCapture:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting "
	      "Media Audio Video Capture access.").
	   arg(security_origin.toString()));
	break;
      }
    case QWebEnginePage::MediaVideoCapture:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Media Video Capture access.").
	   arg(security_origin.toString()));
	break;
      }
    case QWebEnginePage::MouseLock:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Mouse Lock access.").
	   arg(security_origin.toString()));
	break;
      }
    case QWebEnginePage::Notifications:
      {
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting Notifications access.").
	   arg(security_origin.toString()));
	break;
      }
    default:
      {
	m_ui.feature_permission_url->setProperty("feature", -1);
	m_ui.feature_permission_url->setText
	  (tr("The URL <b>%1</b> is requesting access to an unknown feature.").
	   arg(security_origin.toString()));
	break;
      }
    }

  m_ui.feature_permission_popup_message->setVisible(true);
  prepare_progress_label_position();
}

void dooble_page::slot_find_next(void)
{
  slot_find_text_edited(m_ui.find->text());
}

void dooble_page::slot_find_previous(void)
{
  auto text(m_ui.find->text());

  if(m_ui.find_match_case->isChecked())
    find_text
      (QWebEnginePage::FindFlags(QWebEnginePage::FindBackward |
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

void dooble_page::slot_go_home(void)
{
  load(QUrl::fromEncoded(dooble_settings::setting("home_url").toByteArray()));
}

void dooble_page::slot_go_to_backward_item(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_backward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_forward_item(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_forward_item(action->property("index").toInt());
}

void dooble_page::slot_icon_changed(const QIcon &icon)
{
  Q_UNUSED(icon);

  if(dooble::s_history->is_favorite(m_view->url()) || !m_is_private)
    dooble::s_history->save_favicon(m_view->icon(), m_view->url());

  if(!m_is_private)
    dooble_favicons::save_favicon(m_view->icon(), m_view->url());

  m_ui.address->set_item_icon(m_view->icon(), m_view->url());
}

void dooble_page::slot_inject_custom_css(void)
{
  dooble_style_sheet dialog
    (qobject_cast<dooble_web_engine_page *> (m_view->page()), this);

  dialog.exec();
}

void dooble_page::slot_javascript_allow_popup_exception(void)
{
  QMenu menu(this);
  auto url(QUrl::fromUserInput(m_view->url().host()));

  url.setScheme(m_view->url().scheme());
  menu.addAction
    (tr("Always"), this, SLOT(slot_always_allow_javascript_popup(void)));
  menu.addAction
    (tr("Always from %1.").arg(url.toString()),
     this,
     SLOT(slot_always_allow_javascript_popup(void)))->setProperty("url", url);
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
      auto font_metrics(menu.fontMetrics());

      menu.addSeparator();

      for(int i = 0; i < m_last_javascript_popups.size(); i++)
	{
	  auto view(m_last_javascript_popups.at(i));

	  if(view)
	    {
	      auto action = menu.addAction
		(font_metrics.elidedText(tr("Show %1").arg(view->url().
							   toString()) + "...",
					 Qt::ElideMiddle,
					 dooble_ui_utilities::
					 context_menu_width(&menu)),
		 this,
		 SLOT(slot_show_popup(void)));

	      action->setProperty("index", i);
	      action->setToolTip(view->url().toString());
	    }
	}
    }

  menu.setStyleSheet("QMenu {menu-scrollable: 1;}");
  menu.exec
    (m_ui.javascript_allow_popup_exception->
     mapToGlobal(m_ui.javascript_allow_popup_exception->rect().bottomLeft()));
  m_ui.javascript_allow_popup_exception->setChecked(false);
}

void dooble_page::slot_javascript_console(void)
{
  if(!m_javascript_console)
    {
      m_javascript_console = new dooble_javascript(this);
      m_javascript_console->set_page(m_view->page());
    }

  m_javascript_console->showNormal();
  m_javascript_console->raise();
  m_javascript_console->activateWindow();
}

void dooble_page::slot_link_hovered(const QString &url)
{
  if(url.trimmed().isEmpty())
    {
      if(!property("is_loading").toBool())
	{
	  m_progress_label->clear();
	  m_progress_label->setVisible(false);
	}

      m_ui.link_hovered->setProperty("text", "");
      m_ui.link_hovered->clear();
      return;
    }

  if(dooble_settings::setting("show_hovered_links_tool_tips").toBool())
    QToolTip::showText
      (QCursor::pos(), "<html>" + url.trimmed() + "</html>", this);

  if(m_ui.status_bar->isVisible())
    {
      auto font_metrics(m_ui.link_hovered->fontMetrics());
      int difference = 15;

      if(m_ui.is_private->isVisible())
	difference += 25;

      if(m_ui.progress->isVisible())
	difference += m_ui.progress->width();

      m_ui.link_hovered->setProperty("text", url.trimmed());
      m_ui.link_hovered->setText
	(font_metrics.
	 elidedText(url.trimmed(),
		    Qt::ElideMiddle,
		    qAbs(width() - difference)));
      m_ui.link_hovered->setCursorPosition(0);
    }
  else if(!property("is_loading").toBool())
    {
      auto font_metrics(m_progress_label->fontMetrics());

      m_progress_label->setText
	(font_metrics.
	 elidedText(url.trimmed(), Qt::ElideMiddle, qAbs(width() - 15)));
      m_progress_label->resize
	(QSize(m_progress_label->sizeHint().width() + 5,
	       m_progress_label->sizeHint().height()));
      m_progress_label->setVisible(true);
    }
}

void dooble_page::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);
  dooble_style_sheet::inject
    (qobject_cast<dooble_web_engine_page *> (m_view->page()));
  setProperty("is_loading", false);

  if(m_ui.address->text() != m_view->url().toString())
    m_ui.address->setText(m_view->url().toString());

  /*
  ** Do not save the favicon. The current page's favicon and the page's
  ** url may be unrelated.
  */

  if(dooble::s_history->
     is_favorite(m_view->history()->currentItem().url()) || !m_is_private)
    dooble::s_history->save_item
      (QIcon(), m_view->history()->currentItem(), true);

  if(!dooble_ui_utilities::allowed_url_scheme(m_view->url()) ||
     !m_view->url().isValid() ||
     m_view->url().isEmpty())
    {
      m_ui.address->selectAll();
      m_ui.address->setFocus();
    }

  m_progress_label->clear();
  m_progress_label->setVisible(false);
  m_ui.progress->setVisible(false);

  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  m_ui.reload->setIcon
    (QIcon::fromTheme(use_material_icons + "view-refresh",
		      QIcon(QString(":/%1/36/reload.png").arg(icon_set))));
  m_ui.reload->setToolTip(tr("Reload"));
  emit iconChanged(icon());
  emit titleChanged(title());

  if(dooble_settings::setting("temporarily_disable_javascript", false).toBool())
    {
      auto javascript_enabled = true;
      auto settings = m_view->settings();

      if(settings)
	javascript_enabled = settings->testAttribute
	  (QWebEngineSettings::JavascriptEnabled);

      enable_web_setting(QWebEngineSettings::JavascriptEnabled, false);

      if(javascript_enabled)
	QTimer::singleShot(100, this, SLOT(slot_enable_javascript(void)));
    }
}

void dooble_page::slot_load_page(void)
{
  auto keyboard_modifiers(QGuiApplication::keyboardModifiers());
  auto string(m_ui.address->text().trimmed());

  if(dooble::s_search_engines_window)
    {
      auto url(dooble::s_search_engines_window->search_url(string));

      if(!url.isEmpty() && url.isValid())
	{
	  load(url);
	  return;
	}
    }

  auto url((QUrl(string))); // Special parentheses for compilers.

  if((!url.isValid() ||
      Qt::ControlModifier & keyboard_modifiers ||
      string.contains(' ') ||
      string.contains('\t') ||
      url.scheme().isEmpty()) &&
     dooble::s_search_engines_window)
    {
      if(Qt::ControlModifier & keyboard_modifiers ||
	 string.contains(' ') ||
	 string.contains('\t'))
	{
	search_label:

	  auto url
	    (dooble::s_search_engines_window->default_address_bar_engine_url());

	  if(!url.isEmpty() && url.isValid())
	    {
	      if(url.hasQuery())
		url.setQuery(url.query().append(QString("%1").arg(string)));
	      else
		url = QUrl::fromUserInput(url.toString().append(string));

	      load(url);
	      return;
	    }
	  else // Prevent an endless loop.
	    {
	      load(QUrl::fromUserInput(string));
	      return;
	    }
	}

      auto index = string.lastIndexOf('.');

      if(index < string.size() && index > -1)
	if(string.at(index + 1).isLetterOrNumber())
	  {
	    url = QUrl::fromUserInput(string);

	    if(!url.isValid() || url.scheme().isEmpty())
	      goto search_label;

	    goto done_label;
	  }

      goto search_label;
    }
  else if(!dooble_ui_utilities::allowed_url_scheme(url) &&
	  !url.scheme().isEmpty() &&
	  dooble::s_search_engines_window)
    goto search_label;

 done_label:

  if(!dooble_ui_utilities::allowed_url_scheme(url))
    {
      url = QUrl::fromUserInput(string);
      url.setScheme("https");
    }

  auto character
    (dooble_settings::setting("relative_location_character").toString());

  if(character.length() && url.toString().endsWith(character))
    {
      auto scheme(url.scheme());

      if(url.isLocalFile())
	{
	  QDir directory(url.toLocalFile());
	  auto current(directory.absolutePath());

	  do
	    {
	      if(!directory.isRoot())
		current = directory.absolutePath();
	    }
	  while(directory.cdUp());

	  url = QUrl::fromUserInput(current);
	  url.setScheme(scheme);
	}
      else
	{
	  url = QUrl::fromUserInput(url.host());
	  url.setScheme(scheme);
	}
    }

  load(url);
}

void dooble_page::slot_load_progress(int progress)
{
  m_progress_label->setVisible(progress > 0 && progress < 100);
  m_ui.backward->setEnabled(m_view->history()->canGoBack());
  m_ui.forward->setEnabled(m_view->history()->canGoForward());
  m_ui.progress->setValue(progress);
  m_ui.progress->setVisible(progress > 0 && progress < 100);
#ifndef Q_OS_MACOS
  static auto s_address_palette(m_ui.address->palette());

  if(dooble_settings::setting("status_bar_visible").toBool())
    m_ui.address->setPalette(s_address_palette);
  else if(progress < 100)
    {
      if(dooble_settings::setting("show_loading_gradient").toBool())
	{
	  QLinearGradient linear_gradient
	    (0,
	     m_ui.address->height(),
	     m_ui.address->width(),
	     m_ui.address->height());

	  linear_gradient.setColorAt(progress / 100.0, QColor(144, 238, 144));
	  linear_gradient.setColorAt
	    (qBound(progress / 100.0, progress / 100.0 + 0.15, 1.0),
	     QColor(Qt::white));

	  auto palette(m_ui.address->palette());

	  palette.setBrush
	    (m_ui.address->backgroundRole(), QBrush(linear_gradient));
	  m_ui.address->setPalette(palette);
	}
      else if(m_ui.address->palette() != s_address_palette)
	m_ui.address->setPalette(s_address_palette);
    }
  else if(m_ui.address->palette() != s_address_palette)
    m_ui.address->setPalette(s_address_palette);
#endif
}

void dooble_page::slot_load_started(void)
{
  emit iconChanged(QIcon());
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setProperty("is_loading", true);

  foreach(const auto &view, m_last_javascript_popups)
    if(view && view->parent() == this)
      view->deleteLater();

  m_last_javascript_popups.clear();
  QApplication::restoreOverrideCursor();

  if(url().host().isEmpty())
    m_progress_label->setText(tr("Waiting for page..."));
  else
    {
      auto url_1(QUrl::fromUserInput(m_ui.address->text()));
      auto url_2(url());

      if(url_1.host() != url_2.host())
	m_progress_label->setText(tr("Loading %1...").arg(url_1.host()));
      else
	m_progress_label->setText(tr("Loading %1...").arg(url_2.host()));
    }

  m_progress_label->resize(QSize(m_progress_label->sizeHint().width() + 5,
				 m_progress_label->sizeHint().height()));
  m_progress_label->setVisible(true);
  m_ui.feature_permission_popup_message->setVisible(false);
  m_ui.javascript_popup_message->setVisible(false);
  prepare_progress_label_position();

  auto icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.reload->setIcon
    (QIcon(QString(":/%1/36/stop.png").arg(icon_set)));
  m_ui.reload->setToolTip(tr("Stop Page Load"));
}

void dooble_page::slot_loading(const QUrl &url)
{
  if(url.host().isEmpty())
    return;

  m_progress_label->setText(tr("Loading %1...").arg(url.host()));
  m_progress_label->resize
    (QSize(m_progress_label->sizeHint().width() + 5,
	   m_progress_label->sizeHint().height()));
  m_progress_label->setVisible(true);
  prepare_progress_label_position();

  if(!property("is_loading").toBool())
    QTimer::singleShot(2500, m_progress_label, SLOT(hide(void)));
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
void dooble_page::slot_loading_changed(const QWebEngineLoadingInfo &info)
{
  slot_loading(info.url());
}
#endif

void dooble_page::slot_only_now_allow_javascript_popup(void)
{
  m_ui.javascript_popup_message->setVisible(false);
  prepare_progress_label_position();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(const auto &view, m_last_javascript_popups)
    if(view && view->parent() == this)
      emit create_dialog(view);

  m_last_javascript_popups.clear();
  QApplication::restoreOverrideCursor();
}

void dooble_page::slot_open_link(const QUrl &url)
{
  load(url);
}

void dooble_page::slot_open_link(void)
{
  m_ui.address->selectAll();

  if(m_ui.address->isVisible())
    m_ui.address->setFocus();
}

void dooble_page::slot_prepare_backward_menu(void)
{
  m_ui.backward->menu()->clear();

  QFontMetrics font_metrics(m_ui.backward->menu()->font());
  auto items
    (m_view->history()->
     backItems(static_cast<int> (dooble_page::ConstantsEnum::
				 MAXIMUM_HISTORY_ITEMS)));

  m_ui.backward->setEnabled(!items.empty());

  for(int i = items.size() - 1; i >= 0; i--)
    {
      QAction *action = nullptr;
      auto icon(dooble_favicons::icon(items.at(i).url()));
      auto title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString();

      action = m_ui.backward->menu()->addAction
	(icon,
	 font_metrics.elidedText(title + "...",
				 Qt::ElideMiddle,
				 dooble_ui_utilities::
				 context_menu_width(m_ui.backward->menu())),
	 this,
	 SLOT(slot_go_to_backward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_prepare_forward_menu(void)
{
  m_ui.forward->menu()->clear();

  QFontMetrics font_metrics(m_ui.forward->menu()->font());
  auto items
    (m_view->history()->
     forwardItems(static_cast<int> (dooble_page::ConstantsEnum::
				    MAXIMUM_HISTORY_ITEMS)));

  m_ui.forward->setEnabled(!items.empty());

  for(int i = 0; i < items.size(); i++)
    {
      QAction *action = nullptr;
      auto icon(dooble_favicons::icon(items.at(i).url()));
      auto title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString();

      action = m_ui.forward->menu()->addAction
	(icon,
	 font_metrics.elidedText(title + "...",
				 Qt::ElideMiddle,
				 dooble_ui_utilities::
				 context_menu_width(m_ui.forward->menu())),
	 this,
	 SLOT(slot_go_to_forward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_proxy_authentication_required
(const QUrl &url, QAuthenticator *authenticator, const QString &proxy_host)
{
  if(!authenticator ||
     authenticator->isNull() ||
     proxy_host.isEmpty() ||
     !url.isValid())
    {
      if(authenticator)
	*authenticator = QAuthenticator();

      return;
    }

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);

  foreach(auto widget, ui.button_box->findChildren<QWidget *> ())
    widget->setMinimumSize(QSize(125, 30));

  ui.label->setText(tr("The proxy <b>%1</b> is requesting credentials.").
		    arg(proxy_host));
  dialog.setWindowTitle(tr("Dooble: Proxy Authentication"));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    {
      QApplication::processEvents();
      *authenticator = QAuthenticator();
    }
}

void dooble_page::slot_reload(void)
{
  reload();
}

void dooble_page::slot_reload_or_stop(void)
{
  if(m_ui.reload->toolTip() == tr("Stop Page Load"))
    m_view->stop();
  else
    reload();
}

void dooble_page::slot_reload_periodically(void)
{
  if(!m_ui.progress->isVisible() &&
     !m_view->url().isEmpty() &&
     m_view->url().isValid())
    {
      stop();
      reload();
    }
}

void dooble_page::slot_scroll_position_changed(const QPointF &position)
{
  Q_UNUSED(position);

  if(!m_export_as_png)
    return;

  QTimer::singleShot(50, this, SLOT(slot_render_pixmap(void)));
}

void dooble_page::slot_scroll_to_top_finished(void)
{
  m_export_as_png = true;
  m_export_png_timer.start
    (250 *
     qMax(m_view->page()->contentsSize().toSize().height(),
	  m_view->size().height()) / m_view->size().height());
  m_pixmaps.clear();
  m_view->page()->runJavaScript
    (QString("window.scrollBy(0, %1);").arg(m_view->size().height()));

  QPixmap pixmap(m_view->size());

  m_view->render(&pixmap);
  m_pixmaps << pixmap.copy();
}

void dooble_page::slot_render_pixmap(void)
{
  QPixmap pixmap(m_view->size());

  m_view->render(&pixmap);
  m_pixmaps << pixmap.copy();
  m_view->page()->runJavaScript
    (QString("window.scrollBy(0, %1);").arg(m_view->size().height()));
}

void dooble_page::slot_settings_applied(void)
{
  m_menu->clear();

  if(dooble_settings::setting("denote_private_widgets").toBool())
    m_ui.is_private->setVisible(m_is_private);
  else
    m_ui.is_private->setVisible(false);

  auto zoom_factor = dooble_settings::setting("zoom").toDouble() / 100.0;

  m_view->setZoomFactor(zoom_factor);
  prepare_icons();
  prepare_style_sheets();
  prepare_zoom_toolbutton(zoom_factor);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_show_certificate_exception(void)
{
  QMenu menu(this);
  QWidget widget(&menu);
  QWidgetAction widget_action(&menu);
  auto certificate_exceptions_menu_widget = new
    dooble_certificate_exceptions_menu_widget(&widget);

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

void dooble_page::slot_show_favorites_popup(void)
{
  QMenu menu(this);
  QSize size;
  QWidget widget(&menu);
  QWidgetAction widget_action(&menu);
  auto favorites_popup = new dooble_favorites_popup(&widget);
  auto point(m_ui.favorites->pos());

  connect(favorites_popup,
	  SIGNAL(favorites_sorted(void)),
	  dooble::s_application,
	  SIGNAL(favorites_sorted(void)));
  connect(favorites_popup,
	  SIGNAL(accepted(void)),
	  &menu,
	  SLOT(close(void)));
  connect(favorites_popup,
	  SIGNAL(open_link(const QUrl &)),
	  this,
	  SLOT(slot_open_link(const QUrl &)));
  connect(favorites_popup,
	  SIGNAL(open_link_in_new_tab(const QUrl &)),
	  this,
	  SIGNAL(open_link_in_new_tab(const QUrl &)));
  favorites_popup->prepare_viewport_icons();
  favorites_popup->resize(favorites_popup->sizeHint());
  size = favorites_popup->size();
  point.setX(m_ui.favorites->size().width() + point.x() - size.width());
  point.setY(m_ui.favorites->size().height() + point.y());
  widget_action.setDefaultWidget(favorites_popup);
  menu.addAction(&widget_action);
  menu.exec(mapToGlobal(point));
  m_ui.favorites->setChecked(false);
}

void dooble_page::slot_show_find(void)
{
  m_ui.find->selectAll();
  m_ui.find->setFocus();
  m_ui.find_frame->setVisible(true);
}

void dooble_page::slot_show_popup(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto index = action->property("index").toInt();

  if(index < 0 || index >= m_last_javascript_popups.size())
    return;

  auto view(m_last_javascript_popups.at(index));

  if(view)
    emit create_dialog(view);

  m_last_javascript_popups.remove(index);

  if(m_last_javascript_popups.isEmpty())
    {
      m_ui.javascript_popup_message->setVisible(false);
      prepare_progress_label_position();
    }
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
  prepare_progress_label_position();
}

void dooble_page::slot_show_web_settings_panel(void)
{
  emit show_settings_panel(dooble_settings::Panels::WEB_PANEL);
}

void dooble_page::slot_url_changed(const QUrl &url)
{
  auto length = url.toString().length();

  if(length >
     static_cast<decltype(length)> (dooble::Limits::MAXIMUM_URL_LENGTH))
    return;

  /*
  ** Cannot assume that the view's icon has been loaded.
  */

  m_ui.address->add_item(QIcon(), m_view->url());
  m_ui.address->setText(m_view->url().toString());
}

void dooble_page::slot_zoom_in(void)
{
  auto zoom_factor = qMin(m_view->zoomFactor() + 0.10, 5.0);

  m_view->setZoomFactor(zoom_factor);
  prepare_zoom_toolbutton(zoom_factor);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_zoom_out(void)
{
  auto zoom_factor = qMax(m_view->zoomFactor() - 0.10, 0.25);

  m_view->setZoomFactor(zoom_factor);
  prepare_zoom_toolbutton(zoom_factor);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_zoom_reset(void)
{
  m_view->setZoomFactor(1.0);
  prepare_zoom_toolbutton(1.0);
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::slot_zoomed(void)
{
  emit zoomed(m_view->zoomFactor());
}

void dooble_page::stop(void)
{
  m_view->stop();
}

void dooble_page::user_hide_location_frame(bool state)
{
  m_is_location_frame_user_hidden = state;
  m_ui.top_frame->setVisible(!state);
}
