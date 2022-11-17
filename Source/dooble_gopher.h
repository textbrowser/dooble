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

#ifndef dooble_gopher_h
#define dooble_gopher_h

#include <QBuffer>
#include <QPointer>
#include <QTcpSocket>
#include <QUrl>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

#include "dooble_web_engine_view.h"

class dooble_gopher: public QWebEngineUrlSchemeHandler
{
  Q_OBJECT

 public:
  dooble_gopher(QObject *parent);
  void set_web_engine_view(dooble_web_engine_view *web_engine_view);

 private:
  QPointer<QWebEngineUrlRequestJob> m_request;
  QPointer<dooble_web_engine_view> m_web_engine_view;
  void requestStarted(QWebEngineUrlRequestJob *request);

 private slots:
  void slot_error(QWebEngineUrlRequestJob::Error error);
  void slot_finished(const QByteArray &bytes,
		     bool content_type_supported,
		     bool is_image);
};

class dooble_gopher_implementation: public QTcpSocket
{
  Q_OBJECT

 public:
  dooble_gopher_implementation(const QUrl &url,
			       dooble_web_engine_view *web_engine_view,
			       QObject *parent);
  ~dooble_gopher_implementation();
  static QByteArray s_eol;

 private:
  QByteArray m_content;
  QByteArray m_html;
  QPointer<dooble_web_engine_view> m_web_engine_view;
  QString m_output;
  QString m_search;
  QTimer m_write_timer;
  QUrl m_url;
  bool m_content_type_supported;
  bool m_is_image;
  char m_item_type;
  int m_seven_count;
  static QByteArray plain_to_html(const QByteArray &bytes);

 private slots:
  void slot_connected(void);
  void slot_disonnected(void);
  void slot_ready_read(void);
  void slot_write_timeout(void);

 signals:
  void error(QWebEngineUrlRequestJob::Error error);
  void finished(const QByteArray &bytes,
		bool content_type_supported,
		bool is_image);
};

#endif
