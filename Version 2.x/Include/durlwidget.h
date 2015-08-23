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

#ifndef _durlwidget_h_
#define _durlwidget_h_

#include <QCompleter>
#include <QLineEdit>
#include <QTableView>
#include <QToolButton>

class QMouseEvent;
class QStandardItem;
class QStandardItemModel;

class durlcompleterview: public QTableView
{
  Q_OBJECT

 public:
  durlcompleterview(QLineEdit *lineEdit);

 private:
  QLineEdit *m_lineEdit;
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
};

class durlcompleter: public QCompleter
{
  Q_OBJECT

 public:
  durlcompleter(QWidget *parent);
  ~durlcompleter();
  bool exists(const QString &text) const;
  void clear(void);
  void copyContentsOf(durlcompleter *c);
  void saveItemUrl(const QString &url);
  void setCompletion(const QString &completion);
  void setModel(QStandardItemModel *model);

 private:
  QList<QStandardItem *> m_purgedItems;
  QStandardItemModel *m_model;
  QStringList m_allUrls;
  durlcompleterview *m_tableView;
};

class durlwidgettoolbutton: public QToolButton
{
  Q_OBJECT

 public:
  durlwidgettoolbutton(QWidget *parent);

 private:
  void mousePressEvent(QMouseEvent *event);
};

class durlwidget: public QLineEdit
{
  Q_OBJECT

 public:
  durlwidget(QWidget *parent);
  QPoint bookmarkButtonPopupPosition(void) const;
  QPoint iconButtonPopupPosition(void) const;
  bool isBookmarkButtonEnabled(void) const;
  bool isIconButtonEnabled(void) const;
  int findText(const QString &text) const;
  void addItem(const QString &text);
  void addItem(const QString &text, const QIcon &icon);
  void appendItem(const QString &text, const QIcon &icon);
  void appendItems(const QMultiMap<QDateTime, QVariantList> &items);
  void clearHistory(void);
  void copyContentsOf(durlwidget *c);
  void popdown(void) const;
  void resizeEvent(QResizeEvent *);
  void selectAll(void);
  void setBookmarkButtonEnabled(const bool state);
  void setBookmarkColor(const bool isBookmarked);
  void setIcon(const QIcon &icon);
  void setIconButtonEnabled(const bool state);
  void setItemIcon(const int index, const QIcon &icon);
  void setSecureColor(const bool isHttps);
  void setSpotOnButtonEnabled(const bool state);
  void setSpotOnColor(const bool isLoaded);
  void setText(const QString &text);
  void updateToolTips(void);

 private:
  QIcon m_bookmarkButtonIcon;
  QIcon m_secureButtonIcon;
  QIcon m_spotonIcon;
  QToolButton *bookmarkToolButton;
  QToolButton *goToolButton;
  QToolButton *m_fakePulldownMenu;
  QToolButton *m_iconToolButton;
  QToolButton *m_spotonButton;
  durlcompleter *m_completer;
  int m_counter;
  bool event(QEvent *e);
  void keyPressEvent(QKeyEvent *event);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotBookmark(void);
  void slotLoadPage(const QString &url);
  void slotLoadPage(void);
  void slotPulldownClicked(void);
  void slotReturnPressed(void);

 signals:
  void bookmark(void);
  void iconToolButtonClicked(void);
  void loadPage(const QUrl &url);
  void openLinkInNewTab(const QUrl &url);
  void resetUrl(void);
  void submitUrlToSpotOn(void);
};

#endif
