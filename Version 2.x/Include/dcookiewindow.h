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

#ifndef _dcookiewindow_h_
#define _dcookiewindow_h_

#include <QMainWindow>

#include "ui_dcookieWindow.h"

class QStandardItem;
class QStandardItemModel;
class dcookies;

class dcookiewindow: public QMainWindow
{
  Q_OBJECT

 public:
  dcookiewindow(dcookies *cookies, QWidget *parent = 0);
  ~dcookiewindow();
  void find(const QString &text);
  void populate(void);
  void show(QWidget *parent);

 private:
  QStandardItemModel *m_model;
  Ui_cookieWindow ui;
  dcookies *m_cookies;
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void saveState(void);

 public slots:
  void slotDeleteAll(void);
  void slotDomainsRemoved(const QStringList &list);
  void slotSetIcons(void);

 private slots:
  void slotActionToggled(bool checked);
  void slotCheckBoxItemChanged(QStandardItem *item);
  void slotCollapsed(const QModelIndex &index);
  void slotCookiesChanged(void);
  void slotDeleteCookie(void);
  void slotPopulate(void);
  void slotSort(int column, Qt::SortOrder order);
  void slotTextChanged(const QString &text);

 signals:
  void iconsChanged(void);
};

#endif
