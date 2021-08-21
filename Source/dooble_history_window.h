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

#ifndef dooble_history_window_h
#define dooble_history_window_h

#include <QTimer>
#include <QWebEngineHistoryItem>

#include "dooble_main_window.h"
#include "ui_dooble_history_window.h"

class dooble_history_window: public dooble_main_window
{
  Q_OBJECT

 public:
  dooble_history_window(bool floating = false);
  void prepare_viewport_icons(void);
  void show(QWidget *parent);
  void show_normal(QWidget *parent);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  enum TableColumns
    {
     FAVORITE = 0,
     TITLE = 1,
     LOCATION = 2,
     LAST_VISITED = 3
    };

  QHash<QUrl, QTableWidgetItem *> m_items;
  QTimer m_save_settings_timer;
  QTimer m_search_timer;
  QWidget *m_parent;
  Ui_dooble_history_window m_ui;
  bool m_floating;
  void discover_m_parent(void);
  void save_settings(void);
  void set_row_hidden(int i);

 private slots:
  void slot_copy_location(void);
  void slot_delete_pages(void);
  void slot_delete_rows(bool favorites_included, const QModelIndexList &list);
  void slot_enter_pressed(void);
  void slot_favorite_changed(const QUrl &url, bool state);
  void slot_favorites_cleared(void);
  void slot_find(void);
  void slot_history_cleared(void);
  void slot_horizontal_header_section_resized
    (int logicalIndex, int oldSize, int newSize);
  void slot_icon_updated(const QIcon &icon, const QUrl &url);
  void slot_item_changed(QTableWidgetItem *item);
  void slot_item_double_clicked(QTableWidgetItem *item);
  void slot_item_updated(const QIcon &icon, const QWebEngineHistoryItem &item);
  void slot_new_item(const QIcon &icon, const QWebEngineHistoryItem &item);
  void slot_parent_destroyed(void);
  void slot_populate(void);
  void slot_save_settings_timeout(void);
  void slot_search_timer_timeout(void);
  void slot_show_context_menu(const QPoint &point);
  void slot_splitter_moved(int pos, int index);

 signals:
  void delete_rows(bool favorites_included, const QModelIndexList &list);
  void favorite_changed(const QUrl &url, bool state);
  void open_link(const QUrl &url);
};

#endif
