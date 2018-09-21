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

#ifndef _derrorlog_h_
#define _derrorlog_h_

#include <QMainWindow>

#include "ui_derrorLog.h"

class derrorlog: public QMainWindow
{
  Q_OBJECT

 public:
  derrorlog(void);
  bool hasErrors(void) const;
  void clear(void);
  void logError(const QString &error);

 private:
  QString m_lastError;
  QPalette m_findLineEditPalette;
  Ui_errorLog ui;
  bool event(QEvent *event);
  void saveState(void);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 private slots:
  void slotShow(void);
  void slotClear(void);
  void slotClose(void);
  void slotHideFind(void);
  void slotNextFind(void);
  void slotNextFind(const QString &text);
  void slotSetIcons(void);
  void slotShowFind(void);
  void slotAppendToLog(const QString &error);
  void slotPreviousFind(void);
  void slotPreviousFind(const QString &text);

 signals:
  void appendToLog(const QString &error);
  void errorLogged(void);
  void iconsChanged(void);
};

#endif
