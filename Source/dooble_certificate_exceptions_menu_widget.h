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

#ifndef dooble_certificate_exceptions_menu_widget_h
#define dooble_certificate_exceptions_menu_widget_h

#include <QSqlDatabase>
#include <QUrl>

#include "ui_dooble_certificate_exceptions_menu_widget.h"

class dooble_certificate_exceptions_menu_widget: public QWidget
{
  Q_OBJECT

 public:
  dooble_certificate_exceptions_menu_widget(QWidget *parent);
  static bool has_exception(const QUrl &url);
  static void exception_accepted(const QString &error, const QUrl &url);
  static void purge(void);
  static void purge_temporary(void);
  void set_url(const QUrl &url);

 private:
  QUrl m_url;
  Ui_dooble_certificate_exceptions_menu_widget m_ui;
  static void create_tables(QSqlDatabase &db);

 private slots:
  void slot_remove_exception(void);

 signals:
  void triggered(void);
};

#endif
