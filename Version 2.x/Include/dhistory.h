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

#include <QPointer>
#include <QMainWindow>

#include "ui_historyWindow.h"

class QTimer;
class QProgressBar;
class QStandardItemModel;

class dhistory: public QMainWindow
{
  Q_OBJECT

 public:
  dhistory(void);
  ~dhistory();
  void show(QWidget *parent);
  void populate(void);
  void reencode(QProgressBar *progress);
  qint64 size(void) const;

 private:
  QTimer *m_timer;
  QTimer *m_searchTimer;
  Ui_historyWindow ui;
  QPointer<QWidget> m_parent;
  QStandardItemModel *m_model;
  bool event(QEvent *event);
  void purge(void);
  void saveState(void);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotOpen(void);
  void slotSort(int column, Qt::SortOrder order);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotShare(void);
#endif
  void slotCopyUrl(void);
  void slotTimeout(void);
  void slotBookmark(void);
  void slotPopulate(void);
  void slotDeleteAll(void);
  void slotDeletePage(void);
  void slotTextChanged(const QString &text);
  void slotOpenInNewTab(void);
  void slotItemsSelected(const QItemSelection &selected,
			 const QItemSelection &deselected);
  void slotOpenInNewWindow(void);
  void slotParentDestroyed(void);
  void slotShowContextMenu(const QPoint &point);
  void slotItemDoubleClicked(const QModelIndex &index);
  void slotItemSelectionChanged(void);

 signals:
  void open(const QUrl &url);
  void bookmark(const QUrl &url,
		const QIcon &icon,
		const QString &title,
		const QString &description,
		const QDateTime &addDate,
		const QDateTime &lastModified);
  void createTab(const QUrl &url);
  void iconsChanged(void);
  void openInNewWindow(const QUrl &url);
};

#endif
