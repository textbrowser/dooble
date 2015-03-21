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

class QFileSystemModel;
class QWebEnginePage;

class dfilemanager: public QWidget
{
  Q_OBJECT

 public:
  dfilemanager(QWidget *parent);
  ~dfilemanager();
  QString html(void) const;
  QString statusMessage(void) const;
  QString title(void) const;
  QUrl url(void) const;
  QWebEnginePage *mainFrame(void) const;
  static QPointer<QFileSystemModel> treeModel;
  static QPointer<dfilesystemmodel> tableModel;
  void load(const QUrl &url);
  void stop(void);

 private:
  QString pathLabelPlainText;
  QUrl m_url;
  QWebEnginePage *m_webPage;
  Ui_FileManagerForm ui;
  void deleteSelections(QAbstractItemView *view);
  void keyPressEvent(QKeyEvent *event);
  void menuAction(const QString &action, QAbstractItemView *view);
  void prepareLabel(const QUrl &url);

 private slots:
  void slotCloseEditor(QWidget *editor,
		       QAbstractItemDelegate::EndEditHint hint);
  void slotCustomContextMenuRequest(const QPoint &pos);
  void slotDirectoryRemoved(const QModelIndex &index, int start, int end);
  void slotLabelClicked(const QString &link);
  void slotLoad(const QUrl &url);
  void slotSaveTableHeaderState(void);
  void slotTableClicked(const QModelIndex &index);
  void slotTableDirectoryLoaded(const QString &path);
  void slotTableMenuAction(void);
  void slotTreeClicked(const QModelIndex &index);
  void slotTreeMenuAction(void);

 signals:
  void iconChanged(void);
  void loadFinished(bool ok);
  void loadPage(const QUrl &url);
  void loadProgress(int progress);
  void loadStarted(void);
  void titleChanged(const QString &title);
  void urlChanged(const QUrl &url);
};

#endif
