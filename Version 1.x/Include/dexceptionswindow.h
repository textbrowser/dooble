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

#ifndef _dexceptionswindow_h_
#define _dexceptionswindow_h_

#include <QMainWindow>

#include "dexceptionsmodel.h"
#include "ui_exceptionsWindow.h"

class QCloseEvent;
class QProgressBar;

class dexceptionswindow: public QMainWindow
{
  Q_OBJECT

 public:
  dexceptionswindow(dexceptionsmodel *model);
  ~dexceptionswindow();
  QString approach(void) const;
  QStringList allowedHosts(void) const;
  bool allowed(const QString &host) const;
  bool contains(const QString &host) const;
  qint64 size(void) const;
  void clear(void);
  void closeEvent(QCloseEvent *event);
  void enableApproachRadioButtons(const bool state);
  void populate(void);
  void reencode(QProgressBar *progress);

 private:
  Ui_exceptionsWindow ui;
  bool event(QEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void saveState(void);

 public slots:
  void slotSetIcons(void);
  void slotShow(void);

 private slots:
  void slotAdd(const QString &host,
	       const QUrl &url,
	       const QDateTime &dateTime);
  void slotAllow(void);
  void slotClose(void);
  void slotDelete(void);
  void slotDeleteAll(void);
  void slotItemsSelected(const QItemSelection &selected,
			 const QItemSelection &deselected);
  void slotRadioToggled(bool state);
  void slotSort(int column, Qt::SortOrder order);
  void slotTextChanged(const QString &text);

 signals:
  void iconsChanged(void);
};

#endif
