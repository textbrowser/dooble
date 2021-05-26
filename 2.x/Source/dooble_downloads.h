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

#ifndef dooble_downloads_h
#define dooble_downloads_h

#include <QPointer>
#include <QSqlDatabase>
#include <QTimer>

#include "dooble_main_window.h"
#include "ui_dooble_downloads.h"

class QWebEngineDownloadItem;
class QWebEngineProfile;

class dooble_downloads: public dooble_main_window
{
  Q_OBJECT

 public:
  dooble_downloads(QWebEngineProfile *web_engine_profile, QWidget *parent);
  QString download_path(void) const;
  bool contains(QWebEngineDownloadItem *download) const;
  bool is_finished(void) const;
  bool is_private(void) const;
  int size(void) const;
  static void create_tables(QSqlDatabase &db);
  void abort(void);
  void clear(void);
  void purge(void);
  void record_download(QWebEngineDownloadItem *download);
  void show_normal(QWidget *parent);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QHash<QObject *, char> m_downloads;
  QPointer<QWebEngineProfile> m_web_engine_profile;
  QTimer m_download_path_inspection_timer;
  QTimer m_search_timer;
  Ui_dooble_downloads m_ui;
  void delete_selected(void);
  void remove_entry(qintptr oid);
  void save_settings(void);

 private slots:
  void slot_clear_finished_downloads(void);
  void slot_copy_download_location(void);
  void slot_delete_row(void);
  void slot_download_destroyed(void);
  void slot_download_finished(void);
  void slot_download_path_inspection_timer_timeout(void);
  void slot_download_requested(QWebEngineDownloadItem *download);
  void slot_find(void);
  void slot_open_download_page(void);
  void slot_populate(void);
  void slot_reload(const QString &file_name, const QUrl &url);
  void slot_search_timer_timeout(void);
  void slot_select_path(void);
  void slot_show_context_menu(const QPoint &point);

 signals:
  void finished(void);
  void open_link(const QUrl &url);
  void populated(void);
  void started(void);
};

#endif
