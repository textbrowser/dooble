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

#include "ui_bookmarksWindow.h"

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
  QList<QAction *> actions(QWidget *parent) const;
  bool isBookmarked(const QUrl &url) const;
  qint64 size(void) const;
  void addFolder(void);
  void clear(void);
  void populate(void);
  void reencode(QProgressBar *progress);
  void renameFolder(const QModelIndex &index);
  void show(QWidget *parent);

 private:
  QPointer<QWidget> m_parent;
  QStandardItemModel *m_urlModel;
  Ui_bookmarksWindow ui;
  QUrl locationChanged(void) const;
  QUrl titleChanged(void) const;
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *event);
  void createBookmarksDatabase(void);
  void keyPressEvent(QKeyEvent *event);
  void populateTable(const qint64 folderOid);
  void populateTable(void);
  void purge(void);
  void saveFolder(QStandardItem *item);
  void saveState(void);
  void updateBookmark(const QUrl &originalUrl);
  void updateFolder(const QString &name, const qint64 oid);

 public slots:
  void slotAddBookmark(const QUrl &url,
		       const QIcon &icon,
		       const QString &title,
		       const QString &description,
		       const QDateTime &addDate,
		       const QDateTime &lastModified);
  void slotSetIcons(void);

 private slots:
  void slotAboutToHideMenu(void);
  void slotAboutToShowMenu(void);
  void slotAddFolder(void);
  void slotAddFolderOffParent(void);
  void slotBookmarkDeleted(const int folderOid, const QUrl &url);
  void slotBookmarkReceived(const QModelIndex &index);
  void slotCopyUrl(void);
  void slotDeleteBookmark(void);
  void slotDeleteFolder(void);
  void slotDescriptionChanged(void);
  void slotExport(void);
  void slotFolderDataChanged(const QModelIndex &topLeft,
			     const QModelIndex &bottomRight);
  void slotFolderSelected(const QModelIndex &index);
  void slotImport(void);
  void slotItemDoubleClicked(const QModelIndex &index);
  void slotLocationChanged(void);
  void slotOpen(void);
  void slotOpenInNewTab(void);
  void slotOpenInNewWindow(void);
  void slotParentDestroyed(void);
  void slotRefresh(void);
  void slotSort(int column, Qt::SortOrder order);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotShare(void);
#endif
  void slotShowContextMenu(const QPoint &point);
  void slotTextChanged(const QString &text);
  void slotTitleChanged(const int folderOid, const QUrl &url,
                        const QString &title);
  void slotTitleChanged(void);
  void slotUrlSelected(const QItemSelection &selected,
    const QItemSelection &deselected);

 signals:
  void changed(void);
  void createTab(const QUrl &url);
  void iconsChanged(void);
  void open(const QUrl &url);
  void openInNewWindow(const QUrl &url);
};

#endif
