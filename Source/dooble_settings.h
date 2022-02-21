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

#ifndef dooble_settings_h
#define dooble_settings_h

#include <QFuture>
#include <QFutureWatcher>
#include <QPointer>
#include <QProgressDialog>
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QUrl>
#include <QWebEnginePage>

#include "dooble_main_window.h"
#include "ui_dooble_settings.h"

class dooble_settings: public dooble_main_window
{
  Q_OBJECT

 public:
  enum Panels
    {
     CACHE_PANEL = 0,
     DISPLAY_PANEL,
     HISTORY_PANEL,
     PRIVACY_PANEL,
     WEB_PANEL,
     WINDOWS_PANEL
    };

  dooble_settings(void);
  static QString cookie_policy_string(int index);
  static QString use_material_icons(void);
  static QString zoom_frame_location_string(int index);
  static QStringList s_spell_checker_dictionaries;
  static QVariant getenv(const QString &n);
  static QVariant setting(const QString &k,
			  const QVariant &default_value = QVariant(""));
  static bool has_dooble_credentials(void);
  static bool has_dooble_credentials_temporary(void);
  static bool set_setting(const QString &key, const QVariant &value);
  static bool site_has_javascript_block_popup_exception(const QUrl &url);
  static int site_feature_permission(const QUrl &url,
				     QWebEnginePage::Feature feature);
  static void remove_setting(const QString &key);
  void restore(bool read_database);
  void set_site_feature_permission(const QUrl &url,
				   QWebEnginePage::Feature feature,
				   bool state);
  void show_normal(QWidget *parent);
  void show_panel(dooble_settings::Panels panel);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QFuture<QList<QByteArray> > m_pbkdf2_future;
  QFutureWatcher<QList<QByteArray> > m_pbkdf2_future_watcher;
  QPointer<QProgressDialog> m_pbkdf2_dialog;
  Ui_dooble_settings m_ui;
  static QHash<QUrl, char> s_javascript_block_popup_exceptions;
  static QMap<QString, QVariant> s_getenv;
  static QMap<QString, QVariant> s_settings;
  static QMultiMap<QUrl, QPair<int, bool> > s_site_features_permissions;
  static QReadWriteLock s_getenv_mutex;
  static QReadWriteLock s_settings_mutex;
  static QString s_http_user_agent;
  static void create_tables(QSqlDatabase &db);
  void new_javascript_block_popup_exception(const QUrl &url);
  void prepare_fonts(void);
  void prepare_icons(void);
  void prepare_proxy(bool save);
  void prepare_table_statistics(void);
  void prepare_web_engine_settings(void);
  void purge_database_data(void);
  void purge_features_permissions(void);
  void purge_javascript_block_popup_exceptions(void);
  void save_fonts(void);
  void save_javascript_block_popup_exception(const QUrl &url, bool state);
  void save_settings(void);

 private slots:
  void slot_apply(void);
  void slot_clear_cache(void);
  void slot_features_permissions_item_changed(QTableWidgetItem *item);
  void slot_javascript_block_popups_exceptions_item_changed
    (QTableWidgetItem *item);
  void slot_new_javascript_block_popup_exception(const QUrl &url);
  void slot_new_javascript_block_popup_exception(void);
  void slot_page_button_clicked(void);
  void slot_password_changed(void);
  void slot_pbkdf2_future_finished(void);
  void slot_populate(void);
  void slot_proxy_type_changed(int index);
  void slot_remove_all_features_permissions(void);
  void slot_remove_all_javascript_block_popup_exceptions(void);
  void slot_remove_selected_features_permissions(void);
  void slot_remove_selected_javascript_block_popup_exceptions(void);
  void slot_reset(void);
  void slot_reset_credentials(void);
  void slot_reset_user_agent(void);
  void slot_save_credentials(void);

 signals:
  void applied(void);
  void dooble_credentials_authenticated(bool state);
  void dooble_credentials_created(void);
  void populated(void);
};

#endif
