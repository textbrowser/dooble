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

class dooble_gopher: public QWebEngineUrlSchemeHandler
{
  Q_OBJECT

 public:
  dooble_gopher(QObject *parent);

 private:
  QPointer<QWebEngineUrlRequestJob> m_request;
  void requestStarted(QWebEngineUrlRequestJob *request);

 private slots:
  void slot_error(QWebEngineUrlRequestJob::Error error);
  void slot_finished(const QByteArray &bytes);
};

class dooble_gopher_implementation: public QTcpSocket
{
  Q_OBJECT

 public:
  dooble_gopher_implementation(const QUrl &url, QObject *parent);
  ~dooble_gopher_implementation();

 private:
  QByteArray m_content;
  QByteArray m_html;
  QUrl m_url;
  char m_item_type;
  static QByteArray s_eol;
  static QByteArray plain_to_html(const QByteArray &bytes);

 private slots:
  void slot_connected(void);
  void slot_disonnected(void);
  void slot_ready_read(void);

 signals:
  void error(QWebEngineUrlRequestJob::Error error);
  void finished(const QByteArray &bytes);
};

#endif
