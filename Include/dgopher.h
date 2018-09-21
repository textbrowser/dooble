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

#ifndef _dgopher_h_
#define _dgopher_h_

#include <QNetworkReply>
#include <QTcpSocket>

class dgopher: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dgopher(void):QNetworkReply(0)
  {
    initialize();
  }

  dgopher(const dgopher &reply):QNetworkReply(0)
  {
    initialize();
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  dgopher(QObject *parent, const QNetworkRequest &request);
  ~dgopher();
  QByteArray html(void) const;
  QByteArray plainToHtml(const QByteArray &bytes) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void download(void);
  void load(void);

 private:
  QByteArray m_content;
  QByteArray m_html;
  QString m_path;
  QTcpSocket *m_socket;
  bool m_download;
  char m_itemType;
  qint64 m_offset;
  static QByteArray s_eol;
  void initialize(void);

 private slots:
  void slotConnected(void);
  void slotConnectedForDownload(void);
  void slotConnectedForText(void);
  void slotDisonnected(void);
  void slotReadyRead(void);
  void slotReadyReadForDownload(void);
  void slotReadyReadForText(void);

 protected:
  qint64 readData(char *data, qint64 maxSize);
};

#endif
