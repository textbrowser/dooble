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

#ifndef dooble_address_widget_h
#define dooble_address_widget_h

#include <QLineEdit>

class QToolButton;
class dooble_address_widget_completer;

class dooble_address_widget: public QLineEdit
{
  Q_OBJECT

 public:
  dooble_address_widget(QWidget *parent);
  void add_item(const QIcon &icon, const QUrl &url);
  void complete(void);
  void setText(const QString &text);
  void set_item_icon(const QIcon &icon, const QUrl &url);

 protected:
  bool event(QEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QFrame *m_line;
  QMenu *m_menu;
  QToolButton *m_bookmark;
  QToolButton *m_information;
  QToolButton *m_pull_down;
  dooble_address_widget_completer *m_completer;
  void prepare_icons(void);

 private slots:
  void slot_populate(void);
  void slot_settings_applied(void);
  void slot_show_site_information_menu(void);

 signals:
  void pull_down_clicked(void);
  void reset_url(void);
  void show_cookies(void);
};

#endif
