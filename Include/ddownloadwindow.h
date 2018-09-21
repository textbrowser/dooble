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

#ifndef _ddownloadwindow_h_
#define _ddownloadwindow_h_

#include <QMainWindow>

#include "ddownloadwindowitem.h"
#include "ui_ddownloadWindow.h"

class QCloseEvent;
class QNetworkReply;
class QProgressBar;
class QString;
class QUrl;

class ddownloadwindow: public QMainWindow
{
  Q_OBJECT

 public:
  ddownloadwindow(void);
  ~ddownloadwindow();
  bool isActive(void) const;
  void show(QWidget *parent);
  void abort(void);
  void clear(void);
  void populate(void);
  void reencode(QProgressBar *progress);
  void addUrlItem(const QUrl &url, const QString &fileName,
		  const bool isNew,
		  const QDateTime &dateTime,
		  const int choice);
  void addFileItem(const QString &srcFileName, const QString &dstFileName,
		   const bool isNew, const QDateTime &dateTime);
  void addHtmlItem(const QString &html, const QString &dstFileName);
  void closeEvent(QCloseEvent *event);
  qint64 size(void) const;

 private:
  Ui_downloadWindow ui;
  bool event(QEvent *event);
  void purge(void);
  void saveState(void);
  void keyPressEvent(QKeyEvent *event);
  void createDownloadDatabase(void);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotClear(void);
  void slotClose(void);
  void slotEnterUrl(void);
  void slotClearList(void);
  void slotDownloadUrl(void);
  void slotTextChanged(const QString &text);
  void slotBitRateChanged(int state);
  void slotRecordDownload(const QString &fileName,
			  const QUrl &url,
			  const QDateTime &dateTime);
  void slotDownloadFinished(void);
  void slotCancelDownloadUrl(void);
  void slotCellDoubleClicked(int row, int col);
  void slotAuthenticationRequired(QNetworkReply *reply,
				  QAuthenticator *authenticator);
  void slotProxyAuthenticationRequired(const QNetworkProxy &proxy,
				       QAuthenticator *authenticator);

 signals:
  void saveUrl(const QUrl &url, const int choice);
  void iconsChanged(void);
};

#endif
