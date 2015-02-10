/*
** Copyright (c) 2008 - present, Alexis Megas, Bernd Stramm.
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

#ifndef _dtabwidget_h_
#define _dtabwidget_h_

#include <QTabBar>
#include <QTabWidget>
#include <QUrl>

class QToolButton;

class dtabbar: public QTabBar
{
  Q_OBJECT

 public:
  dtabbar(QWidget *parent);
  QSize tabSizeHint(int index) const;

 private:
  void dropEvent(QDropEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);

 signals:
  void createTab(void);
  void urlsReceivedViaDrop(const QList<QUrl> &list);
};

class dtabwidget: public QTabWidget
{
  Q_OBJECT

 public:
  dtabwidget(QWidget *parent);
  ~dtabwidget();
  void animateIndex(const int index, const bool state, const QIcon &icon);
  void setTabButton(int index);
  void setBarVisible(const bool state);

 private:
  int m_selectedTabIndex;
  QString m_spinningIconPath;
  dtabbar *m_tabBar;
  void dropEvent(QDropEvent *event);
  void tabRemoved(int index);
  void resizeEvent(QResizeEvent *event);
  void tabInserted(int index);
  void dragMoveEvent(QDragMoveEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);

 public slots:
  void slotSetIcons(void);
  void slotIconChange(QWidget *tab, const QIcon &icon);
  void slotTitleChange(QWidget *tab, const QString &title);

 private slots:
  void slotBookmark(void);
  void slotCloseTab(void);
  void slotCloseTab(int index);
  void slotCreateTab(void);
  void slotJavaScript(void);
  void slotReloadTab(void);
  void slotCloseOtherTabs(void);
  void slotPrivateBrowsing(void);
  void slotPrivateCookies(void);
  void slotOpenInNewWindow(void);
  void slotShowContextMenu(const QPoint &point);
  void slotViewPrivateCookies(void);
  void slotWebPlugins(void);

 signals:
  void bookmark(const int index);
  void closeTab(const int index);
  void tabMoved(int from, int to);
  void createTab(void);
  void reloadTab(const int index);
  void openInNewWindow(const int index);
  void openLinkInNewTab(const QUrl &url);
  void urlsReceivedViaDrop(const QList<QUrl> &list);
};

#endif
