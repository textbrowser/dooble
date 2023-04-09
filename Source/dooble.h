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
#include <QPrinter>
#include <QTimer>
#include <QWebEngineCookieStore>

#include "dooble_settings.h"
#include "ui_dooble.h"
#include "ui_dooble_floating_digital_clock.h"

class QDialog;
class QPrinter;
class QWebEngineDownloadItem;
class QWebEngineProfile;
class dooble_about;
class dooble_accepted_or_blocked_domains;
class dooble_application;
class dooble_certificate_exceptions;
class dooble_charts;
class dooble_cookies;
class dooble_cookies_window;
class dooble_cryptography;
class dooble_downloads;
class dooble_favorites_popup;
class dooble_gopher;
class dooble_history;
class dooble_history_window;
class dooble_page;
class dooble_popup_menu;
class dooble_search_engines_popup;
class dooble_style_sheet;
class dooble_web_engine_url_request_interceptor;
class dooble_web_engine_view;

class dooble: public QMainWindow
{
  Q_OBJECT

 public:
  enum Limits
    {
     MAXIMUM_TITLE_LENGTH = 1024,
     MAXIMUM_SQL_TEXT_LENGTH = 5000,
     MAXIMUM_URL_LENGTH = 2048
    };

  dooble(QWidget *widget);
  dooble(const QList<QUrl> &urls, bool is_private);
  dooble(dooble_page *page);
  dooble(dooble_web_engine_view *view);
  ~dooble();
  bool anonymous_tab_headers(void) const;
  bool initialized(void) const;
  bool is_private(void) const;
  dooble_page *current_page(void) const;
  dooble_page *new_page(const QUrl &url, bool is_private);
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
  static QPointer<dooble_gopher> s_gopher;
  static QPointer<dooble_history> s_history;
  static QPointer<dooble_history_window> s_history_popup;
  static QPointer<dooble_history_window> s_history_window;
  static QPointer<dooble_search_engines_popup> s_search_engines_window;
  static QPointer<dooble_settings> s_settings;
  static QPointer<dooble_style_sheet> s_style_sheet;
  static QPointer<dooble_web_engine_url_request_interceptor>
    s_url_request_interceptor;
  static QString ABOUT_BLANK;
  static void clean(void);
  static void print(QWidget *parent, dooble_charts *chart);
  static void print_preview(QPrinter *printer, dooble_charts *chart);
  void print_current_page(void);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 private:
  enum CanExit
    {
     CAN_EXIT_CLOSE_EVENT = 0,
     CAN_EXIT_SLOT_QUIT_DOOBLE
    };

  QDialog *m_floating_digital_clock_dialog;
  QEventLoop m_event_loop;
  QFuture<QList<QByteArray> > m_pbkdf2_future;
  QFutureWatcher<QList<QByteArray> > m_pbkdf2_future_watcher;
#ifdef Q_OS_MACOS
  QHash<QTimer *, QShortcut *> m_disabled_shortcuts;
#endif
  QList<QPair<QPointer<dooble_page>, QUrl> > m_delayed_pages;
  QList<QPointer<QAction> > m_standard_menu_actions;
  QList<QShortcut *> m_shortcuts;
  QList<QShortcut *> m_tab_widget_shortcuts;
  QList<QUrl> all_open_tab_urls(void) const;
  QMenu *m_menu;
  QPointer<QAction> m_action_close_tab;
  QPointer<QAction> m_authentication_action;
  QPointer<QAction> m_full_screen_action;
  QPointer<QAction> m_settings_action;
  QPointer<QProgressDialog> m_pbkdf2_dialog;
  QPointer<QWebEngineProfile> m_web_engine_profile;
  QPointer<dooble_cookies> m_cookies;
  QPointer<dooble_cookies_window> m_cookies_window;
  QPointer<dooble_downloads> m_downloads;
  QPointer<dooble_popup_menu> m_popup_menu;
  QPrinter m_printer;
  QTimer m_floating_digital_clock_timer;
  QTimer m_populate_containers_timer;
  Ui_dooble m_ui;
  Ui_dooble_floating_digital_clock m_floating_digital_clock_ui;
  bool m_anonymous_tab_headers;
  bool m_is_javascript_dialog;
  bool m_is_private;
  bool m_print_preview;
  static QPointer<dooble> s_favorites_popup_opened_from_dooble_window;
  static QPointer<dooble> s_search_engines_popup_opened_from_dooble_window;
  static bool s_containers_populated;
  QStringList chart_names(void) const;
  static bool cookie_filter
    (const QWebEngineCookieStore::FilterRequest &filter_request);
  bool can_exit(const dooble::CanExit can_exit);
  bool tabs_closable(void) const;
  void add_tab(QWidget *widget, const QString &title);
  void connect_signals(void);
  void decouple_support_windows(void);
  void delayed_load(const QUrl &url, dooble_page *page);
  void initialize_static_members(void);
  void new_page(dooble_charts *chart);
  void new_page(dooble_page *page);
  void new_page(dooble_web_engine_view *view);
  void open_tab_as_new_window(bool is_private, int index);
  void prepare_control_w_shortcut(void);
  void prepare_icons(void);
  void prepare_page_connections(dooble_page *page);
  void prepare_private_web_engine_profile_settings(void);
  void prepare_shortcuts(void);
  void prepare_standard_menus(void);
  void prepare_style_sheets(void);
  void prepare_tab_icons_text_tool_tips(void);
  void prepare_tab_shortcuts(void);
  void print(dooble_page *page);
  void remove_page_connections(dooble_page *page);
  void setWindowTitle(const QString &text);

 private slots:
  void slot_about_to_hide_main_menu(void);
  void slot_about_to_show_history_menu();
  void slot_about_to_show_main_menu(void);
  void slot_about_to_show_tabs_menu(void);
  void slot_about_to_show_view_menu(void);
  void slot_anonymous_tab_headers(bool state);
  void slot_application_locked(bool state, dooble *d);
  void slot_authenticate(void);
  void slot_clear_downloads(void);
  void slot_clear_history(void);
  void slot_clear_visited_links(void);
  void slot_close_tab(void);
  void slot_create_dialog(dooble_web_engine_view *view);
  void slot_create_tab(dooble_web_engine_view *view);
  void slot_create_window(dooble_web_engine_view *view);
  void slot_decouple_tab(int index);
  void slot_dooble_credentials_authenticated(bool state);
  void slot_dooble_credentials_created(void);
  void slot_downloads_started(void);
#ifdef Q_OS_MACOS
  void slot_enable_shortcut(void);
#endif
  void slot_export_as_png(void);
  void slot_floating_digital_dialog_timeout(void);
  void slot_history_action_triggered(void);
  void slot_history_favorites_populated(void);
  void slot_icon_changed(const QIcon &icon);
  void slot_inject_custom_css(void);
  void slot_load_finished(bool ok);
  void slot_new_private_window(void);
  void slot_new_tab(const QUrl &url);
  void slot_new_tab(void);
  void slot_new_window(void);
  void slot_open_chart(void);
  void slot_open_favorites_link(const QUrl &url);
  void slot_open_favorites_link_in_new_tab(const QUrl &url);
  void slot_open_link(const QUrl &url);
  void slot_open_link_in_new_private_window(const QUrl &url);
  void slot_open_link_in_new_tab(const QUrl &url);
  void slot_open_link_in_new_window(const QUrl &url);
  void slot_open_local_file(void);
  void slot_open_previous_session_tabs(void);
  void slot_open_tab_as_new_private_window(int index);
  void slot_open_tab_as_new_window(int index);
  void slot_pbkdf2_future_finished(void);
  void slot_populate_containers_timer_timeout(void);
  void slot_populated(void);
  void slot_print(void);
  void slot_print_finished(bool ok);
  void slot_print_preview(QPrinter *printer);
  void slot_print_preview(void);
  void slot_quit_dooble(void);
  void slot_reload_tab(int index);
  void slot_reload_tab_periodically(int index, int seconds);
  void slot_remove_tab_widget_shortcut(void);
  void slot_save(void);
  void slot_set_current_tab(void);
  void slot_settings_applied(void);
#ifdef Q_OS_MACOS
  void slot_shortcut_activated(void);
#endif
  void slot_show_about(void);
  void slot_show_accepted_or_blocked_domains(void);
  void slot_show_certificate_exceptions(void);
  void slot_show_chart_xyseries(void);
  void slot_show_clear_items(void);
  void slot_show_cookies(void);
  void slot_show_documentation(void);
  void slot_show_downloads(void);
  void slot_show_favorites(void);
  void slot_show_floating_digital_clock(void);
  void slot_show_floating_history_popup(void);
  void slot_show_floating_menu(void);
  void slot_show_full_screen(bool state);
  void slot_show_full_screen(void);
  void slot_show_history(void);
  void slot_show_main_menu(void);
  void slot_show_release_notes(const QUrl &url);
  void slot_show_release_notes(void);
  void slot_show_search_engines(void);
  void slot_show_settings(void);
  void slot_show_settings_panel(dooble_settings::Panels panel);
  void slot_show_site_cookies(void);
  void slot_tab_close_requested(int index);
  void slot_tab_index_changed(int index);
  void slot_tab_widget_shortcut_activated(void);
  void slot_tabs_menu_button_clicked(void);
  void slot_title_changed(const QString &title);
  void slot_vacuum_databases(void);
  void slot_warn_of_missing_sqlite_driver(void);
  void slot_window_close_requested(void);

 signals:
  void add_session_url(void);
  void application_locked(bool state, dooble *d);
  void dooble_credentials_authenticated(bool state);
  void history_cleared(void);
};

#endif
