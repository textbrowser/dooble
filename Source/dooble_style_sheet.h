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

#ifndef dooble_style_sheet_h
#define dooble_style_sheet_h

#include "ui_dooble_style_sheet.h"

class dooble_web_engine_page;

class dooble_style_sheet: public QDialog
{
  Q_OBJECT

 public:
  dooble_style_sheet(void);
  dooble_style_sheet(dooble_web_engine_page *web_engine_page, QWidget *parent);
  static void inject(dooble_web_engine_page *web_engine_page);
  static void purge(void);

 protected:
  void keyPressEvent(QKeyEvent *event);

 private:
  dooble_web_engine_page *m_web_engine_page;
  Ui_dooble_style_sheet m_ui;
  static QMap<QPair<QString, QUrl>, QString> s_style_sheets;
  void populate(void);

 private slots:
  void slot_add(void);
  void slot_item_selection_changed(void);
  void slot_populate(void);
  void slot_remove(void);

 signals:
  void populated(void);
};

#endif
