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

#ifndef dooble_favorites_popup_h
#define dooble_favorites_popup_h

#include <QTimer>

#include "ui_dooble_favorites_popup.h"

class dooble_favorites_popup: public QDialog
{
  Q_OBJECT

 public:
  dooble_favorites_popup(QWidget *parent);
  void prepare_viewport_icons(void);
  void show_normal(QWidget *parent);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QTimer m_entries_timer;
  QTimer m_search_timer;
  Ui_dooble_favorites_popup m_ui;
  void prepare_icons(void);
  void save_settings(void);

 private slots:
  void slot_delete_selected(void);
  void slot_double_clicked(const QModelIndex &index);
  void slot_entries_timer_timeout(void);
  void slot_favorites_sorted(void);
  void slot_find(void);
  void slot_search_timer_timeout(void);
  void slot_set_favorites_model(void);
  void slot_settings_applied(void);
  void slot_sort(int index);
  void slot_sort(void);

 signals:
  void favorite_changed(const QUrl &url, bool state);
  void favorites_sorted(void);
  void open_link(const QUrl &url);
  void open_link_in_new_tab(const QUrl &url);
};

#endif
