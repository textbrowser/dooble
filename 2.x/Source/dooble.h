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

#include <QFuture>
#include <QMainWindow>
#include <QShortcut>
#include <QPointer>
#include <QTimer>

#include "dooble_settings.h"
#include "ui_dooble.h"

#define DOOBLE_DATE_VERSION_STRING "2018.03.15"
#define DOOBLE_VERSION_STRING "2.1.8"

class QWebEngineDownloadItem;
class QWebEngineProfile;
class dooble_about;
class dooble_accepted_or_blocked_domains;
class dooble_application;
class dooble_certificate_exceptions;
class dooble_cookies;
class dooble_cookies_window;
class dooble_cryptography;
class dooble_downloads;
class dooble_favorites_popup;
class dooble_history;
class dooble_history_window;
class dooble_page;
class dooble_web_engine_url_request_interceptor;
class dooble_web_engine_view;

class dooble: public QMainWindow
{
  Q_OBJECT

 public:
  enum Limits
  {
    MAXIMUM_TITLE_LENGTH = 1024,
    MAXIMUM_URL_LENGTH = 2048
  };

  dooble(QWidget *widget);
  dooble(const QUrl &url, bool is_private);
  dooble(dooble_page *page);
  dooble(dooble_web_engine_view *view);
  ~dooble();
  bool initialized(void) const;
  bool is_private(void) const;
  dooble_page *current_page(void) const;
  dooble_page *new_page(const QUrl &url, bool is_private);
  static QPointer<dooble_history> s_history;
  static QPointer<dooble_about> s_about;
  static QPointer<dooble_accepted_or_blocked_domains>
    s_accepted_or_blocked_domains;
  static QPointer<dooble_application> s_application;
  static QPointer<dooble_certificate_exceptions> s_certificate_exceptions;
  static QPointer<dooble_cookies> s_cookies;
  static QPointer<dooble_cookies_window> s_cookies_window;
  static QPointer<dooble_cryptography> s_cryptography;
  static QPointer<dooble_downloads> s_downloads;
  static QPointer<dooble_favorites_popup> s_favorites_window;
  static QPointer<dooble_history_window> s_history_window;
  static QPointer<dooble_settings> s_settings;
  static QPointer<dooble_web_engine_url_request_interceptor>
    s_url_request_interceptor;
  void print_current_page(void);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 private:
  QFuture<QList<QByteArray> > m_pbkdf2_future;
  QFutureWatcher<QList<QByteArray> > m_pbkdf2_future_watcher;
  QHash<QTimer *, QShortcut *> m_disabled_shortcuts;
  QList<QShortcut *> m_shortcuts;
  QList<QShortcut *> m_tab_widget_shortcuts;
  QMenu *m_menu;
  QPointer<QAction> m_action_close_tab;
  QPointer<QAction> m_authentication_action;
  QPointer<QAction> m_full_screen_action;
  QPointer<QAction> m_settings_action;
  QPointer<QProgressDialog> m_pbkdf2_dialog;
  QPointer<QWebEngineProfile> m_web_engine_profile;
  QPointer<dooble_cookies> m_cookies;
  QPointer<dooble_cookies_window> m_cookies_window;
  QTimer m_populate_containers_timer;
  Ui_dooble m_ui;
  bool m_is_javascript_dialog;
  bool m_is_private;
  static QPointer<dooble> s_favorites_popup_opened_from_dooble_window;
  static bool s_containers_populated;
  bool can_exit(void);
  void connect_signals(void);
  void copy_default_profile_settings(void);
  void decouple_support_windows(void);
  void initialize_static_members(void);
  void new_page(dooble_page *page);
  void new_page(dooble_web_engine_view *view);
  void open_tab_as_new_window(bool is_private, int index);
  void prepare_page_connections(dooble_page *page);
  void prepare_shortcuts(void);
  void prepare_standard_menus(void);
  void prepare_style_sheets(void);
  void prepare_tab_icons(void);
  void prepare_tab_shortcuts(void);
  void print(dooble_page *page);
  void remove_page_connections(dooble_page *page);

 private slots:
  void slot_about_to_hide_main_menu(void);
  void slot_about_to_show_main_menu(void);
  void slot_authenticate(void);
  void slot_clear_visited_links(void);
  void slot_close_tab(void);
  void slot_create_dialog(dooble_web_engine_view *view);
  void slot_create_tab(dooble_web_engine_view *view);
  void slot_create_window(dooble_web_engine_view *view);
  void slot_decouple_tab(int index);
  void slot_dooble_credentials_authenticated(bool state);
  void slot_download_requested(QWebEngineDownloadItem *download);
  void slot_enable_shortcut(void);
  void slot_icon_changed(const QIcon &icon);
  void slot_load_finished(bool ok);
  void slot_new_private_window(void);
  void slot_new_tab(void);
  void slot_new_window(void);
  void slot_open_favorites_link(const QUrl &url);
  void slot_open_favorites_link_in_new_tab(const QUrl &url);
  void slot_open_link(const QUrl &url);
  void slot_open_link_in_new_private_window(const QUrl &url);
  void slot_open_link_in_new_tab(const QUrl &url);
  void slot_open_link_in_new_window(const QUrl &url);
  void slot_open_tab_as_new_private_window(int index);
  void slot_open_tab_as_new_window(int index);
  void slot_pbkdf2_future_finished(void);
  void slot_populate_containers_timer_timeout(void);
  void slot_populated(void);
  void slot_print(void);
  void slot_print_preview(void);
  void slot_quit_dooble(void);
  void slot_reload_tab(int index);
  void slot_remove_tab_widget_shortcut(void);
  void slot_save(void);
  void slot_set_current_tab(void);
  void slot_settings_applied(void);
  void slot_shortcut_activated(void);
  void slot_show_about(void);
  void slot_show_accepted_or_blocked_domains(void);
  void slot_show_certificate_exceptions(void);
  void slot_show_clear_items(void);
  void slot_show_cookies(void);
  void slot_show_documentation(void);
  void slot_show_downloads(void);
  void slot_show_favorites(void);
  void slot_show_full_screen(void);
  void slot_show_history(void);
  void slot_show_main_menu(void);
  void slot_show_settings(void);
  void slot_show_settings_panel(dooble_settings::Panels panel);
  void slot_tab_close_requested(int index);
  void slot_tab_index_changed(int index);
  void slot_tab_widget_shortcut_activated(void);
  void slot_tabs_menu_button_clicked(void);
  void slot_title_changed(const QString &title);
  void slot_window_close_requested(void);

 signals:
  void dooble_credentials_authenticated(bool state);
};

#endif
