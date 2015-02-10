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

#ifndef _dfilemanager_h_
#define _dfilemanager_h_

#include <QPointer>
#include <QUrl>
#include <QWidget>

#include "dfilesystemmodel.h"
#include "ui_fileManagerForm.h"

class QWebPage;
class QWebFrame;
class QFileSystemModel;

class dfilemanager: public QWidget
{
  Q_OBJECT

 public:
  dfilemanager(QWidget *parent);
  ~dfilemanager();
  static QPointer<QFileSystemModel> treeModel;
  static QPointer<dfilesystemmodel> tableModel;
  QUrl url(void) const;
  void load(const QUrl &url);
  void stop(void);
  QString html(void) const;
  QString title(void) const;
  QString statusMessage(void) const;
  QWebFrame *mainFrame(void) const;

 private:
  QUrl m_url;
  QString pathLabelPlainText;
  QWebPage *m_webPage;
  Ui_FileManagerForm ui;
  void menuAction(const QString &action, QAbstractItemView *view);
  void prepareLabel(const QUrl &url);
  void keyPressEvent(QKeyEvent *event);
  void deleteSelections(QAbstractItemView *view);

 private slots:
  void slotLoad(const QUrl &url);
  void slotCloseEditor(QWidget *editor,
		       QAbstractItemDelegate::EndEditHint hint);
  void slotTreeClicked(const QModelIndex &index);
  void slotLabelClicked(const QString &link);
  void slotTableClicked(const QModelIndex &index);
  void slotTreeMenuAction(void);
  void slotTableMenuAction(void);
  void slotDirectoryRemoved(const QModelIndex &index, int start, int end);
  void slotSaveTableHeaderState(void);
  void slotTableDirectoryLoaded(const QString &path);
  void slotCustomContextMenuRequest(const QPoint &pos);

 signals:
  void loadPage(const QUrl &url);
  void urlChanged(const QUrl &url);
  void iconChanged(void);
  void loadStarted(void);
  void loadFinished(bool ok);
  void loadProgress(int progress);
  void titleChanged(const QString &title);
};

#endif
