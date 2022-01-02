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
#include <QTimer>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWidget>

#include "dooble_settings.h"
#include "ui_dooble_page.h"

class QPrinter;
class QShortcut;
class QWebEngineSettings;
class QWebEngineView;
class dooble;
class dooble_address_widget;
class dooble_web_engine_view;

class dooble_page: public QWidget
{
  Q_OBJECT

 public:
  enum ConstantsEnum
    {
     MAXIMUM_HISTORY_ITEMS = 10,
     MAXIMUM_JAVASCRIPT_POPUPS = 100
    };

  dooble_page(QWebEngineProfile *web_engine_profile,
	      dooble_web_engine_view *view,
	      QWidget *parent);
  ~dooble_page();
  QAction *action_close_tab(void) const;
  QAction *full_screen_action(void) const;
  QFrame *frame(void) const;
  QIcon icon(void) const;
  QMenu *menu(void);
  QString title(void) const;
  QUrl url(void) const;
  QWebEngineProfile *web_engine_profile(void) const;
  QWebEngineSettings *web_engine_settings(void) const;
  bool can_go_back(void) const;
  bool can_go_forward(void) const;
  bool is_location_frame_hidden(void) const;
  bool is_location_frame_user_hidden(void) const;
  bool is_private(void) const;
  dooble_address_widget *address_widget(void) const;
  dooble_web_engine_view *view(void) const;
  int reload_periodically_seconds(void) const;
  void download(const QString &file_name, const QUrl &url);
  void enable_web_setting(QWebEngineSettings::WebAttribute setting,
			  bool state);
  void hide_location_frame(bool state);
  void hide_status_bar(bool state);
  void inject_custom_css(void);
  void load(const QUrl &url);
  void prepare_export_as_png(const QString &file_name);
  void print_page(QPrinter *printer);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  void print_page(QPrinter *printer,
		  const QWebEngineCallback<bool> &result_callback);
#endif
  void reload(void);
  void reload_periodically(int seconds);
  void save(const QString &file_name);
  void show_menu(void);
  void stop(void);
  void user_hide_location_frame(bool state);

 protected:
  void resizeEvent(QResizeEvent *event);

 private:
  QLabel *m_progress_label;
  QList<QPixmap> m_pixmaps;
  QList<QShortcut *> m_shortcuts;
  QMenu *m_menu;
  QPointer<QAction> m_action_close_tab;
  QPointer<QAction> m_authentication_action;
  QPointer<QAction> m_find_action;
  QPointer<QAction> m_full_screen_action;
  QPointer<QAction> m_settings_action;
  QPointer<QProgressDialog> m_export_as_png_progress_dialog;
  QString m_export_png_file_name;
  QTimer m_export_png_timer;
  QTimer m_reload_timer;
  QVector<QPointer<dooble_web_engine_view> > m_last_javascript_popups;
  Ui_dooble_page m_ui;
  bool m_export_as_png;
  bool m_is_location_frame_user_hidden;
  bool m_is_private;
  dooble *find_parent_dooble(void) const;
  dooble_web_engine_view *m_view;
  int m_reload_periodically_seconds;
  void find_text(QWebEnginePage::FindFlags find_flags, const QString &text);
  void go_to_backward_item(int index);
  void go_to_forward_item(int index);
  void prepare_icons(void);
  void prepare_progress_label_position(bool process_events = true);
  void prepare_shortcuts(void);
  void prepare_standard_menus(void);
  void prepare_style_sheets(void);
  void prepare_tool_buttons(void);
  void prepare_zoom_toolbutton(qreal zoom_factor);
  void reset_url(void);
  void show_popup_menu(void);

 private slots:
  void slot_about_to_show_standard_menus(void);
  void slot_accepted_or_blocked_add_exception(void);
  void slot_accepted_or_blocked_clicked(void);
  void slot_always_allow_javascript_popup(void);
  void slot_authentication_required(const QUrl &url,
				    QAuthenticator *authenticator);
  void slot_clear_visited_links(void);
  void slot_close_javascript_popup_exception_frame(void);
  void slot_create_dialog_request(dooble_web_engine_view *view);
  void slot_dooble_credentials_authenticated(bool state);
  void slot_dooble_credentials_created(void);
  void slot_downloads_finished(void);
  void slot_downloads_started(void);
  void slot_escape(void);
  void slot_export_as_png_timer_timeout(void);
  void slot_favorite_changed(const QUrl &url, bool state);
  void slot_feature_permission_allow(void);
  void slot_feature_permission_deny(void);
  void slot_feature_permission_request_canceled
    (const QUrl &security_origin, QWebEnginePage::Feature feature);
  void slot_feature_permission_requested(const QUrl &security_origin,
					 QWebEnginePage::Feature feature);
  void slot_find_next(void);
  void slot_find_previous(void);
  void slot_find_text_edited(const QString &text);
  void slot_go_backward(void);
  void slot_go_forward(void);
  void slot_go_home(void);
  void slot_go_to_backward_item(void);
  void slot_go_to_forward_item(void);
  void slot_icon_changed(const QIcon &icon);
  void slot_inject_custom_css(void);
  void slot_javascript_allow_popup_exception(void);
  void slot_link_hovered(const QString &url);
  void slot_load_finished(bool ok);
  void slot_load_page(void);
  void slot_load_progress(int progress);
  void slot_load_started(void);
  void slot_only_now_allow_javascript_popup(void);
  void slot_open_link(const QUrl &url);
  void slot_open_link(void);
  void slot_prepare_backward_menu(void);
  void slot_prepare_forward_menu(void);
  void slot_proxy_authentication_required(const QUrl &url,
					  QAuthenticator *authenticator,
					  const QString &proxy_host);
  void slot_reload(void);
  void slot_reload_or_stop(void);
  void slot_reload_periodically(void);
  void slot_render_pixmap(void);
  void slot_scroll_position_changed(const QPointF &position);
  void slot_scroll_to_top_finished(void);
  void slot_settings_applied(void);
  void slot_show_certificate_exception(void);
  void slot_show_favorites_popup(void);
  void slot_show_find(void);
  void slot_show_popup(void);
  void slot_show_popup_menu(void);
  void slot_show_pull_down_menu(void);
  void slot_show_status_bar(bool state);
  void slot_show_web_settings_panel(void);
  void slot_url_changed(const QUrl &url);
  void slot_zoom_in(void);
  void slot_zoom_out(void);
  void slot_zoom_reset(void);

 signals:
  void authenticate(void);
  void clear_downloads(void);
  void close_tab(void);
  void close_window(void);
  void create_dialog(dooble_web_engine_view *view);
  void create_tab(dooble_web_engine_view *view);
  void create_window(dooble_web_engine_view *view);
  void dooble_credentials_authenticated(bool state);
  void export_as_png(void);
  void iconChanged(const QIcon &icon);
  void javascript_allow_popup_exception(const QUrl &url);
  void loadFinished(bool ok);
  void loadStarted(void);
  void new_private_window(void);
  void new_tab(void);
  void new_window(void);
  void open_link_in_new_private_window(const QUrl &url);
  void open_link_in_new_tab(const QUrl &url);
  void open_link_in_new_window(const QUrl &url);
  void open_local_file(void);
  void print(void);
  void print_preview(void);
  void quit_dooble(void);
  void save(void);
  void show_about(void);
  void show_accepted_or_blocked_domains(void);
  void show_certificate_exceptions(void);
  void show_chart_xyseries(void);
  void show_clear_items(void);
  void show_cookies(void);
  void show_documentation(void);
  void show_downloads(void);
  void show_favorites(void);
  void show_floating_digital_clock(void);
  void show_floating_history_popup(void);
  void show_full_screen(void);
  void show_history(void);
  void show_release_notes(void);
  void show_search_engines(void);
  void show_settings(void);
  void show_settings_panel(dooble_settings::Panels panel);
  void show_site_cookies(void);
  void titleChanged(const QString &title);
  void vacuum_databases(void);
  void windowCloseRequested(void);
  void zoomed(qreal zoom_factor);
};

#endif
