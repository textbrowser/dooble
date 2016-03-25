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

#ifndef _dftpbrowser_h_
#define _dftpbrowser_h_

#include <QPointer>

#include "ui_dftpManagerForm.h"

class QTableWidgetItem;
class QWebFrame;
class QWebPage;

class dftp;
class dftpfileinfo;

class dftpbrowser: public QWidget
{
  Q_OBJECT

 public:
  dftpbrowser(QWidget *parent);
  ~dftpbrowser();
  QUrl url(void) const;
  void load(const QUrl &url, QPointer<dftp> ftp);
  void stop(void);
  QString html(void) const;
  QString title(void) const;
  QString statusMessage(void) const;
  QWebFrame *mainFrame(void) const;

 private:
  quint64 m_fileCount;
  quint64 m_directoryCount;
  QUrl m_url;
  QUrl m_selectedUrl;
  QWebPage *m_webPage;
  QPointer<dftp> m_ftp;
  Qt::MouseButton m_lastButtonPressed;
  Ui_ftpManagerForm ui;
  void mousePressEvent(QMouseEvent *event);

 private slots:
  void slotFinished(const bool ok);
  void slotSaveLink(void);
  void slotListInfos(const QList<dftpfileinfo> &info);
  void slotUrlChanged(const QUrl &url);
  void slotItemDoubleClicked(QTableWidgetItem *item);
  void slotAppendMessage(const QString &text);
  void slotCopyLinkLocation(void);
  void slotOpenLinkInNewTab(void);
  void slotUnsupportedContent(const QUrl &url);
  void slotOpenLinkInNewWindow(void);
  void slotSaveTableHeaderState(void);
  void slotCustomContextMenuRequest(const QPoint &point);

 signals:
  void saveUrl(const QUrl &url);
  void copyLink(const QUrl &url);
  void loadPage(const QUrl &url);
  void urlChanged(const QUrl &url);
  void iconChanged(void);
  void loadStarted(void);
  void loadFinished(bool ok);
  void loadProgress(int progress);
  void titleChanged(const QString &title);
  void openLinkInNewTab(const QUrl &url);
  void unsupportedContent(const QUrl &url);
  void openLinkInNewWindow(const QUrl &url);
  void statusMessageReceived(const QString &message);
};

#endif
