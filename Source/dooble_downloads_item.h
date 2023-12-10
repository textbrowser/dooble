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

#ifndef dooble_downloads_item_h
#define dooble_downloads_item_h

#include <QPointer>
#include <QPropertyAnimation>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include "ui_dooble_downloads_item.h"

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
class QWebEngineDownloadItem;
#else
class QWebEngineDownloadRequest;
#endif
class QWebEngineProfile;

class dooble_downloads_item: public QWidget
{
  Q_OBJECT

 public:
  dooble_downloads_item
    (
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
     QWebEngineDownloadItem *download,
#else
     QWebEngineDownloadRequest *download,
#endif
     const bool is_private,
     qintptr oid,
     QWidget *parent
     );
  dooble_downloads_item(const QString &download_path,
			const QString &file_name,
			const QString &information,
			const QUrl &url,
			qintptr oid,
			QWidget *parent);
  ~dooble_downloads_item();
  QPointer<QWebEngineProfile> profile(void) const;
  QString download_path(void) const;
  QUrl url(void) const;
  bool is_finished(void) const;
  qintptr oid(void) const;
  void cancel(void);

 private:
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QPointer<QWebEngineDownloadItem> m_download;
#else
  QPointer<QWebEngineDownloadRequest> m_download;
#endif
  QPointer<QWebEngineProfile> m_profile;
  QPropertyAnimation m_progress_bar_animation;
  QString m_download_path;
  QString m_file_name;
  QTime m_last_time;
  QTimer m_stalled_timer;
  QUrl m_url;
  Ui_dooble_downloads_item m_ui;
  bool m_is_private;
  qint64 m_last_bytes_received;
  qint64 m_rate;
  qintptr m_oid;
  void prepare_icons(void);
  void record(void);
  void record_information(void);

 private slots:
  void slot_cancel(void);
  void slot_download_progress(qint64 bytes_received, qint64 bytes_total);
  void slot_download_progress(void);
  void slot_finished(void);
  void slot_pause_or_resume(void);
  void slot_reload(void);
  void slot_settings_applied(void);
  void slot_stalled(void);

 signals:
  void finished(void);
  void reload(const QString &file_name, const QUrl &url);
};

#endif
