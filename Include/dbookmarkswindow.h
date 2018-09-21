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

#ifndef _dbookmarkswindow_h_
#define _dbookmarkswindow_h_

#include <QMainWindow>
#include <QPointer>

#include "ui_dbookmarksWindow.h"

class QAction;
class QProgressBar;
class QStandardItem;
class QStandardItemModel;

class dbookmarkswindow: public QMainWindow
{
  Q_OBJECT

 public:
  dbookmarkswindow(void);
  ~dbookmarkswindow();
  bool isBookmarked(const QUrl &url) const;
  void show(QWidget *parent);
  void clear(void);
  void populate(void);
  void reencode(QProgressBar *progress);
  void addFolder(void);
  void renameFolder(const QModelIndex &index);
  qint64 size(void) const;
  QList<QAction *> actions(QWidget *parent) const;

 private:
  QPointer<QWidget> m_parent;
  Ui_bookmarksWindow ui;
  QStandardItemModel *m_urlModel;
  QUrl titleChanged(void) const;
  QUrl locationChanged(void) const;
  bool event(QEvent *event);
  void purge(void);
  void saveState(void);
  void closeEvent(QCloseEvent *event);
  void saveFolder(QStandardItem *item);
  void updateFolder(const QString &name,
		    const qint64 oid);
  void keyPressEvent(QKeyEvent *event);
  void populateTable(void);
  void populateTable(const qint64 folderOid);
  void updateBookmark(const QUrl &originalUrl);
  void createBookmarksDatabase(void);

 public slots:
  void slotSetIcons(void);
  void slotAddBookmark(const QUrl &url,
		       const QIcon &icon,
		       const QString &title,
		       const QString &description,
		       const QDateTime &addDate,
		       const QDateTime &lastModified);

 private slots:
  void slotOpen(void);
  void slotSort(int column, Qt::SortOrder order);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotShare(void);
#endif
  void slotExport(void);
  void slotImport(void);
  void slotCopyUrl(void);
  void slotRefresh(void);
  void slotAddFolder(void);
  void slotTextChanged(const QString &text);
  void slotUrlSelected(const QItemSelection &selected,
		       const QItemSelection &deselected);
  void slotDeleteFolder(void);
  void slotOpenInNewTab(void);
  void slotTitleChanged(void);
  void slotTitleChanged(const int folderOid,
			const QUrl &url,
			const QString &title);
  void slotDeleteBookmark(void);
  void slotFolderSelected(const QModelIndex &index);
  void slotAboutToHideMenu(void);
  void slotAboutToShowMenu(void);
  void slotBookmarkDeleted(const int folderOid,
			   const QUrl &url);
  void slotLocationChanged(void);
  void slotOpenInNewWindow(void);
  void slotParentDestroyed(void);
  void slotShowContextMenu(const QPoint &point);
  void slotBookmarkReceived(const QModelIndex &index);
  void slotFolderDataChanged(const QModelIndex &topLeft,
			     const QModelIndex &bottomRight);
  void slotItemDoubleClicked(const QModelIndex &index);
  void slotAddFolderOffParent(void);
  void slotDescriptionChanged(void);

 signals:
  void open(const QUrl &url);
  void changed(void);
  void createTab(const QUrl &url);
  void iconsChanged(void);
  void openInNewWindow(const QUrl &url);
};

#endif
