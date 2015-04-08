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

#include <QDateTime>
#include <QtDebug>

#include "dgopher.h"

QByteArray dgopher::s_eol = "\r\n";

dgopher::dgopher
(QObject *parent, const QNetworkRequest &req):QNetworkReply(parent)
{
  QNetworkRequest request(req);
  QUrl url(request.url());

  m_offset = 0;
  m_socket = new QTcpSocket(this);

  if(url.port() == -1)
    url.setPort(70);

  request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
  request.setUrl(url);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::LocationHeader, url);
  setOperation(QNetworkAccessManager::GetOperation);
  setRequest(request);
  setUrl(url);
  connect(m_socket,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slotConnected(void)));
  connect(m_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slotDisonnected(void)));
  connect(m_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slotReadyRead(void)));
  connect(this,
	  SIGNAL(finished(dgopher *)),
	  this,
	  SIGNAL(finished(void)));
  open(QIODevice::ReadOnly | QIODevice::Unbuffered);
  m_html.append("<html><head></head><p><body>");
}

QByteArray dgopher::html(void) const
{
  return m_html;
}

qint64 dgopher::bytesAvailable(void) const
{
  return m_html.size();
}

qint64 dgopher::readData(char *data, qint64 maxSize)
{
  if(!data || maxSize < 0)
    return -1;
  else if(maxSize == 0)
    return 0;

  qint64 number = qMin
    (maxSize, static_cast<qint64> (m_html.size()) - m_offset);

  if(number < 0)
    return -1;

  memcpy(data, m_html.constData() + m_offset, number);
  m_offset += number;
  return number;
}

void dgopher::abort(void)
{
  close();
  m_socket->abort();
}

void dgopher::load(void)
{
  m_socket->abort();
  m_socket->connectToHost(url().host(), url().port());
}

void dgopher::slotConnected(void)
{
  m_socket->write((url().path()).toUtf8().append(s_eol));
}

void dgopher::slotDisonnected(void)
{
  m_html.append("</body></p></html>");
  emit finished(this);
}

void dgopher::slotReadyRead(void)
{
  m_content.append(m_socket->readAll());

  while(m_content.contains(s_eol))
    {
      QByteArray bytes(m_content.mid(0, m_content.indexOf(s_eol) + 1));

      m_content.remove(0, bytes.length());
      bytes = bytes.trimmed();

      if(bytes == "." || bytes.isEmpty())
	break;

      char c = bytes.at(0);

      if(c == '0' || c == '1' || c == '2' || c == '6' || c == '7' ||
	 c == '+')
	{
	  bytes.remove(0, 1);

	  QList<QByteArray> list(bytes.split('\t'));
	  QUrl url;

	  url.setHost(list.value(2));
	  url.setPath(list.value(1));
	  url.setPort(list.value(3).toInt());

	  if(url.port() == -1)
	    url.setPort(70);

	  url.setScheme("gopher");
	  m_html.append
	    (QString("<a href=\"%1\">%2%3</a><br>").
	     arg(url.toEncoded().constData()).
	     arg(list.value(0).constData()).
	     arg(c == '1' ? "..." : ""));
	}
      else if(c == '3')
	abort();
    }
}
