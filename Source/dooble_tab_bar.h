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

#ifndef dooble_tab_bar_h
#define dooble_tab_bar_h

#include <QTabBar>

class QToolButton;
class dooble;
class dooble_page;

class dooble_tab_bar: public QTabBar
{
  Q_OBJECT

 public:
  dooble_tab_bar(QWidget *parent);
  void set_corner_widget(QWidget *widget);

 protected:
  QSize tabSizeHint(int index) const;
  void hideEvent(QHideEvent *event);
  void showEvent(QShowEvent *event);
  void tabLayoutChange(void);

 private:
  QToolButton *m_next_tool_button;
  QToolButton *m_previous_tool_button;
  QWidget *m_corner_widget;
  bool is_private(void) const;
  dooble_page *page(int index) const;
  void mouseDoubleClickEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void prepare_icons(void);
  void prepare_style_sheets(void);

 private slots:
  void slot_anonymous_tab_headers(void);
  void slot_application_locked(void);
  void slot_clone_tab(void);
  void slot_close_other_tabs(void);
  void slot_close_tab(void);
  void slot_decouple_tab(void);
  void slot_hide_location_frame(void);
  void slot_javascript(void);
  void slot_next_tab(void);
  void slot_open_tab_as_new_cute_window(void);
  void slot_open_tab_as_new_private_window(void);
  void slot_open_tab_as_new_window(void);
  void slot_pin_tab(void);
  void slot_previous_tab(void);
  void slot_reload(void);
  void slot_reload_periodically(void);
  void slot_settings_applied(void);
  void slot_show_context_menu(const QPoint &point);
  void slot_web_plugins(void);
  void slot_webgl(void);

 signals:
  void anonymous_tab_headers(bool state);
  void application_locked(bool state, dooble *d);
  void clone_tab(int index);
  void decouple_tab(int index);
  void new_tab(void);
  void open_tab_as_new_cute_window(int index);
  void open_tab_as_new_private_window(int index);
  void open_tab_as_new_window(int index);
  void pin_tab(bool state, int index);
  void reload_tab(int index);
  void reload_tab_periodically(int index, int seconds);
  void set_visible_corner_button(bool state);
  void show_corner_widget(bool state);
};

#endif
