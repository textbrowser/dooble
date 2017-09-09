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

#include <QMainWindow>
#include <QTimer>

#include "ui_dooble_downloads.h"

class QWebEngineDownloadItem;

class dooble_downloads: public QMainWindow
{
  Q_OBJECT

 public:
  dooble_downloads(void);
  QString download_path(void) const;
  bool contains(QWebEngineDownloadItem *download) const;
  bool is_finished(void) const;
  static void purge(void);
  void abort(void);
  void record_download(QWebEngineDownloadItem *download);

 public slots:
  void show(void);
  void showNormal(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QHash<QObject *, char> m_downloads;
  QTimer m_download_path_inspection_timer;
  QTimer m_search_timer;
  Ui_dooble_downloads m_ui;
  void delete_selected(void);
  void remove_entry(qint64 oid);
  void save_settings(void);

 private slots:
  void slot_clear_finished_downloads(void);
  void slot_copy_download_location(void);
  void slot_delete_row(void);
  void slot_download_destroyed(void);
  void slot_download_path_inspection_timer_timeout(void);
  void slot_open_download_page(void);
  void slot_populate(void);
  void slot_search_timer_timeout(void);
  void slot_select_path(void);
  void slot_show_context_menu(const QPoint &point);

 signals:
  void open_url(const QUrl &url);
};

#endif
