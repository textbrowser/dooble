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

#ifndef _dhistory_h_
#define _dhistory_h_

#include <QMainWindow>
#include <QPointer>

#include "ui_dhistoryWindow.h"

class QProgressBar;
class QStandardItemModel;
class QTimer;

class dhistory: public QMainWindow
{
  Q_OBJECT

 public:
  dhistory(void);
  ~dhistory();
  qint64 size(void) const;
  void populate(void);
  void reencode(QProgressBar *progress);
  void show(QWidget *parent);

 private:
  QPointer<QWidget> m_parent;
  QStandardItemModel *m_model;
  QTimer *m_searchTimer;
  QTimer *m_timer;
  Ui_historyWindow ui;
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void purge(void);
  void saveState(void);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotBookmark(void);
  void slotCopyUrl(void);
  void slotDeleteAll(void);
  void slotDeletePage(void);
  void slotItemDoubleClicked(const QModelIndex &index);
  void slotItemSelectionChanged(void);
  void slotItemsSelected(const QItemSelection &selected,
			 const QItemSelection &deselected);
  void slotOpen(void);
  void slotOpenInNewTab(void);
  void slotOpenInNewWindow(void);
  void slotParentDestroyed(void);
  void slotPopulate(void);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotShare(void);
#endif
  void slotShowContextMenu(const QPoint &point);
  void slotSort(int column, Qt::SortOrder order);
  void slotTextChanged(const QString &text);
  void slotTimeout(void);

 signals:
  void bookmark(const QUrl &url,
		const QIcon &icon,
		const QString &title,
		const QString &description,
		const QDateTime &addDate,
		const QDateTime &lastModified);
  void createTab(const QUrl &url);
  void iconsChanged(void);
  void open(const QUrl &url);
  void openInNewWindow(const QUrl &url);
};

#endif
