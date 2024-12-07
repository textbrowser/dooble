/*
** Copyright (c) Alexis Megas.
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
**    derived from Swifty without specific prior written permission.
**
** SWIFTY IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SWIFTY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _swifty_h_
#define _swifty_h_

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

class swifty: public QNetworkAccessManager
{
  Q_OBJECT

 public:
  swifty(const QString &current_version,
	 const QString &search_for_string,
	 const QUrl &url,
	 QObject *parent):QNetworkAccessManager(parent)
  {
    m_current_version = m_newest_version = current_version;
    m_search_for_string = search_for_string;
    m_url = url;
  }

  ~swifty()
  {
  }

  QString newest_version(void) const
  {
    return m_newest_version;
  }

 public slots:
  void slot_download()
  {
    m_buffer.clear();

    if(m_reply)
      m_reply->deleteLater();

    m_reply = get(QNetworkRequest(m_url));
    connect(m_reply,
	    SIGNAL(finished(void)),
	    this,
	    SLOT(slot_finished(void)));
    connect(m_reply,
	    SIGNAL(readyRead(void)),
	    this,
	    SLOT(slot_ready_read(void)));
  }

 private:
  QByteArray m_buffer;
  QPointer<QNetworkReply> m_reply;
  QString m_current_version;
  QString m_newest_version;
  QString m_search_for_string;
  QUrl m_url;

 private slots:
  void slot_finished(void)
  {
    if(m_reply)
      m_reply->deleteLater();

    auto const index = m_buffer.indexOf(m_search_for_string.toUtf8());

    if(index >= 0)
      {
	auto version = m_buffer.mid
	  (index + m_search_for_string.toUtf8().length());

	version = version.mid(0, version.indexOf('\n')).replace('"', "").
	  trimmed();

	if(m_current_version != version)
	  {
	    m_newest_version = version;
	    emit different(m_newest_version);
	  }
	else
	  emit same();
      }
  }

  void slot_ready_read(void)
  {
    while(m_reply && m_reply->bytesAvailable() > 0)
      m_buffer.append(m_reply->readAll());
  }

 signals:
  void different(const QString &new_version);
  void same(void);
};

#endif
