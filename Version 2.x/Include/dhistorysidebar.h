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

#ifndef _dhistorysidebar_h_
#define _dhistorysidebar_h_

#include "ui_dhistorySideBar.h"

class dhistorysidebar: public QWidget
{
  Q_OBJECT

 public:
  dhistorysidebar(QWidget *parent);
  ~dhistorysidebar();
  void search(const QString &text);

 private:
  QTimer *m_searchTimer;
  Ui_historySideBar ui;
  void hideEvent(QHideEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void saveState(void);
  void showEvent(QShowEvent *event);
  void textChanged(const QString &text);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotBookmark(void);
  void slotCopyUrl(void);
  void slotDeletePage(void);
  void slotItemDoubleClicked(const QModelIndex &index);
  void slotModelAboutToBeReset(void);
  void slotModelReset(void);
  void slotOpen(void);
  void slotOpenInNewTab(void);
  void slotOpenInNewWindow(void);
  void slotPopulate(void);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotShare(void);
#endif
  void slotShowContextMenu(const QPoint &point);
  void slotSort(int index);
  void slotTextChanged(const QString &text);
  void slotVisibilityChanged(const bool state);

 signals:
  void bookmark(const QUrl &url,
		const QIcon &icon,
		const QString &title,
		const QString &description,
		const QDateTime &addDate,
		const QDateTime &lastModified);
  void closed(void);
  void createTab(const QUrl &url);
  void iconsChanged(void);
  void open(const QUrl &url);
  void openInNewWindow(const QUrl &url);
  void visibilityChanged(const bool state);
};

#endif
