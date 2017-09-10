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

#ifndef dooble_popup_menu_h
#define dooble_popup_menu_h

#include <QDialog>
#include <QPointer>

#include "ui_dooble_popup_menu.h"

class dooble;
class dooble_page;

class dooble_popup_menu:public QDialog
{
  Q_OBJECT

 public:
  dooble_popup_menu(QWidget *parent);

 private:
  QPointer<dooble_page> m_dooble_page;
  Ui_dooble_popup_menu m_ui;
  dooble *find_parent_dooble(void) const;
  void prepare_icons(void);

 private slots:
  void slot_authenticate(void);
  void slot_dooble_credentials_authenticated(bool state);
  void slot_settings_applied(void);
  void slot_tool_button_clicked(void);

 signals:
  void authenticate(void);
  void quit_dooble(void);
  void show_blocked_domains(void);
  void show_cookies(void);
  void show_history(void);
  void show_settings(void);
};

#endif
