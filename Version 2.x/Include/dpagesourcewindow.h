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

#ifndef _dpagesourcewindow_h_
#define _dpagesourcewindow_h_

#include <QMainWindow>

#include "ui_dpageSourceWindow.h"

class QPrinter;

class dpagesourcewindow: public QMainWindow
{
  Q_OBJECT

 public:
  dpagesourcewindow(QWidget *parent,
		    const QUrl &url, const QString &html);
  ~dpagesourcewindow();
  void closeEvent(QCloseEvent *event);

 private:
  QPalette m_findLineEditPalette;
  QString fileName;
  Ui_pageSourceWindow ui;
  bool event(QEvent *event);
  void keyPressEvent(QKeyEvent *event);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotClose(void);
  void slotHideFind(void);
  void slotNextFind(const QString &text);
  void slotNextFind(void);
  void slotPreviousFind(const QString &text);
  void slotPreviousFind(void);
  void slotPrint(void);
  void slotPrintPreview(void);
  void slotSavePageAs(void);
  void slotShowFind(void);
  void slotTextEditPrintPreview(QPrinter *printer);
  void slotWrapLines(bool checked);
};

#endif
