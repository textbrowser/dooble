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

#ifndef dooble_page_h
#define dooble_page_h

#include <QPointer>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWidget>

#include "dooble_settings.h"
#include "ui_dooble_page.h"

class QWebEngineSettings;
class dooble_web_engine_view;

class dooble_page: public QWidget
{
  Q_OBJECT

 public:
  enum ConstantsEnum
  {
    MAXIMUM_HISTORY_ITEMS = 20
  };

  dooble_page(bool is_private, dooble_web_engine_view *view, QWidget *parent);
  QIcon icon(void) const;
  QString title(void) const;
  QToolButton *menu(void) const;
  QUrl url(void) const;
  QWebEngineSettings *web_engine_settings(void) const;
  dooble_web_engine_view *view(void) const;
  void enable_web_setting(QWebEngineSettings::WebAttribute setting,
			  bool state);

 protected:
  void resizeEvent(QResizeEvent *event);

 private:
  QPointer<QAction> m_action_close_tab;
  QPointer<QAction> m_authentication_action;
  QPointer<QAction> m_find_action;
  QPointer<QAction> m_settings_action;
  Ui_dooble_page m_ui;
  bool m_is_private;
  bool m_shortcuts_prepared;
  dooble_web_engine_view *m_view;
  void find_text(QWebEnginePage::FindFlags find_flags, const QString &text);
  void go_to_backward_item(int index);
  void go_to_forward_item(int index);
  void load_page(const QUrl &url);
  void prepare_icons(void);
  void prepare_shortcuts(void);
  void prepare_standard_menus(void);
  void prepare_tool_buttons_for_mac(void);

 private slots:
  void slot_about_to_show_standard_menus(void);
  void slot_authenticate(void);
  void slot_authentication_required(const QUrl &url,
				    QAuthenticator *authenticator);
  void slot_dooble_credentials_authenticated(bool state);
  void slot_dooble_credentials_created(void);
  void slot_escape(void);
  void slot_find_next(void);
  void slot_find_previous(void);
  void slot_find_text_edited(const QString &text);
  void slot_go_backward(void);
  void slot_go_forward(void);
  void slot_go_to_backward_item(void);
  void slot_go_to_forward_item(void);
  void slot_icon_changed(const QIcon &icon);
  void slot_link_hovered(const QString &url);
  void slot_load_finished(bool ok);
  void slot_load_page(void);
  void slot_load_progress(int progress);
  void slot_load_started(void);
  void slot_open_url(void);
  void slot_prepare_backward_menu(void);
  void slot_prepare_forward_menu(void);
  void slot_proxy_authentication_required(const QUrl &url,
					  QAuthenticator *authenticator,
					  const QString &proxy_host);
  void slot_reload_or_stop(void);
  void slot_reset_url(void);
  void slot_settings_applied(void);
  void slot_show_cookies(void);
  void slot_show_find(void);
  void slot_show_pull_down_menu(void);
  void slot_url_changed(const QUrl &url);

 signals:
  void close_tab(void);
  void create_tab(dooble_web_engine_view *view);
  void create_window(dooble_web_engine_view *view);
  void dooble_credentials_authenticated(bool state);
  void iconChanged(const QIcon &icon);
  void loadFinished(bool ok);
  void loadStarted(void);
  void new_private_tab(void);
  void new_tab(void);
  void new_window(void);
  void quit_dooble(void);
  void show_blocked_domains(void);
  void show_history(void);
  void show_settings(void);
  void show_settings_panel(dooble_settings::Panels panel);
  void titleChanged(const QString &title);
};

#endif
