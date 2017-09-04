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

#ifndef dooble_h
#define dooble_h

#include <QMainWindow>
#include <QTimer>

#include "dooble_settings.h"
#include "ui_dooble.h"

class dooble_about;
class dooble_accepted_or_blocked_domains;
class dooble_application;
class dooble_cookies;
class dooble_cookies_window;
class dooble_cryptography;
class dooble_history;
class dooble_history_window;
class dooble_page;
class dooble_popup_menu;
class dooble_web_engine_url_request_interceptor;
class dooble_web_engine_view;

class dooble: public QMainWindow
{
  Q_OBJECT

 public:
  dooble(dooble_page *page);
  dooble(dooble_web_engine_view *view);
  dooble(void);
  ~dooble();
  dooble_page *current_page(void) const;
  static QPointer<dooble_history> s_history;
  static dooble_about *s_about;
  static dooble_accepted_or_blocked_domains *s_accepted_or_blocked_domains;
  static dooble_application *s_application;
  static dooble_cookies *s_cookies;
  static dooble_cookies_window *s_cookies_window;
  static dooble_cryptography *s_cryptography;
  static dooble_history_window *s_history_window;
  static dooble_popup_menu *s_popup_menu;
  static dooble_settings *s_settings;
  static dooble_web_engine_url_request_interceptor *s_url_request_interceptor;
  void new_page(bool is_private);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 private:
  QTimer m_populate_containers_timer;
  Ui_dooble m_ui;
  static bool s_containers_populated;
  void connect_signals(void);
  void initialize_static_members(void);
  void new_page(dooble_page *page);
  void new_page(dooble_web_engine_view *view);
  void prepare_page_connections(dooble_page *page);

 public slots:
  void show(void);

 private slots:
  void slot_about_to_hide_main_menu(void);
  void slot_about_to_show_main_menu(void);
  void slot_close_tab(void);
  void slot_create_tab(dooble_web_engine_view *view);
  void slot_create_window(dooble_web_engine_view *view);
  void slot_icon_changed(const QIcon &icon);
  void slot_load_finished(bool ok);
  void slot_load_started(void);
  void slot_new_private_tab(void);
  void slot_new_tab(void);
  void slot_new_window(void);
  void slot_open_tab_as_new_window(int index);
  void slot_open_url(const QUrl &url);
  void slot_populate_containers_timer_timeout(void);
  void slot_print(void);
  void slot_quit_dooble(void);
  void slot_settings_applied(void);
  void slot_show_about(void);
  void slot_show_blocked_domains(void);
  void slot_show_clear_items(void);
  void slot_show_history(void);
  void slot_show_popup_menu(void);
  void slot_show_settings(void);
  void slot_show_settings_panel(dooble_settings::Panels panel);
  void slot_tab_close_requested(int index);
  void slot_tab_index_changed(int index);
  void slot_title_changed(const QString &title);

 signals:
  void dooble_credentials_authenticated(bool state);
};

#endif
