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

#ifndef _ddownloadwindowitem_h_
#define _ddownloadwindowitem_h_

#include <QDateTime>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPointer>
#include <QWidget>

#include "ui_ddownloadWindowItem.h"

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;

class ddownloadwindowitem: public QWidget
{
  Q_OBJECT

 public:
  ddownloadwindowitem(QWidget *parent);
  ~ddownloadwindowitem();
  QDateTime dateTime(void) const;
  QString fileName(void) const;
  QString text(void) const;
  QUrl url(void) const;
  bool abortedByUser(void) const;
  bool isDownloading(void) const;
  int choice(void) const;
  void abort(void);
  void downloadFile(const QString &srcFileName,
		    const QString &dstFileName,
		    const bool isNew, const QDateTime &dateTime);
  void downloadHtml(const QString &html, const QString &dstFileName);
  void downloadUrl(const QUrl &url,
		   const QString &fileName,
		   const bool isNew,
		   const QDateTime &dateTime,
		   const int choice);

 private:
  QDateTime m_dateTime;
  QPointer<QNetworkAccessManager> m_networkAccessManager;
  QString m_dstFileName;
  QString m_html;
  QString m_srcFileName;
  QTime m_lastTime;
  QUrl m_url;
  Ui_downloadWindowItem ui;
  bool m_abortedByUser;
  bool m_paused;
  int m_choice;
  qint64 m_lastSize;
  qint64 m_rate;
  qint64 m_total;
  void downloadFinished(const qint64 fileSize);
  void init_ddownloadwindowitem(void);
  void updateProgress(const qint64 done, const qint64 total);

 public slots:
  void slotSetIcons(void);

 private slots:
  void slotAbortDownload(void);
  void slotComputeFileHash(void);
  void slotDataTransferProgress(qint64 done, qint64 total);
  void slotDownloadAgain(void);
  void slotDownloadFinished(void);
  void slotError(QNetworkReply::NetworkError code);
  void slotMetaDataChanged(void);
  void slotPauseDownload(void);
  void slotReadyRead(void);

 signals:
  void authenticationRequired(QNetworkReply *reply,
			      QAuthenticator *authenticator);
  void downloadFinished(void);
  void proxyAuthenticationRequired(const QNetworkProxy &proxy,
				   QAuthenticator *authenticator);
  void recordDownload(const QString &fileName,
		      const QUrl &url,
		      const QDateTime &dateTime);
};

#endif
