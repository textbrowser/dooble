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
#include <QStringList>
#include <QtDebug>

#include "dgopher.h"

QByteArray dgopher::s_eol = "\r\n";

dgopher::dgopher
(QObject *parent, const QNetworkRequest &req):QNetworkReply(parent)
{
  initialize();
  m_hasBeenPreFetched = false;

  QNetworkRequest request(req);
  QUrl url(request.url());

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
  connect(this,
	  SIGNAL(finished(dgopher *)),
	  this,
	  SIGNAL(finished(void)));
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

void dgopher::download(void)
{
  disconnect(m_socket, SIGNAL(connected(void)), 0, 0);
  disconnect(m_socket, SIGNAL(disconnected(void)), 0, 0);
  disconnect(m_socket, SIGNAL(readyRead(void)), 0, 0);
  abort();
  initialize();
  connect(m_socket,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slotConnectedForDownload(void)));
  connect(m_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slotDisonnected(void)));
  connect(m_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slotReadyReadForDownload(void)));
  m_socket->connectToHost(url().host(), url().port());
}

void dgopher::initialize(void)
{
  m_content.clear();
  m_download = false;
  m_html.clear();
  m_itemType = 0;
  m_offset = 0;
  m_path.clear();
  m_preFetch = false;

  if(!isOpen())
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void dgopher::load(void)
{
  disconnect(m_socket, SIGNAL(connected(void)), 0, 0);
  disconnect(m_socket, SIGNAL(disconnected(void)), 0, 0);
  disconnect(m_socket, SIGNAL(readyRead(void)), 0, 0);
  abort();
  initialize();
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
  m_socket->connectToHost(url().host(), url().port());
}

void dgopher::slotConnected(void)
{
  QString path(url().path());

  if(path.isEmpty())
    {
      m_html.append("<html><head></head><p><body>");
      m_socket->write(s_eol);
    }
  else
    {
      /*
      ** Determine the current path's type by proceeding to its
      ** parent directory.
      */

      if(m_hasBeenPreFetched)
	m_socket->write(url().path().toUtf8().append(s_eol));
      else
	{
	  QStringList list(path.split("/", QString::SkipEmptyParts));

	  if(!list.isEmpty())
	    {
	      m_path = list.at(list.length() - 1);
	      m_preFetch = true;
	    }

	  path.clear();

	  for(int i = 0; i < list.size() - 1; i++)
	    {
	      path.append(list.at(i));
	      path.append("/");
	    }

	  m_socket->write(path.toUtf8().append(s_eol));
	}
    }
}

void dgopher::slotConnectedForDownload(void)
{
  m_socket->write(url().path().toUtf8().append(s_eol));
}

void dgopher::slotConnectedForText(void)
{
  connect(m_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slotReadyReadForText(void)));
  m_socket->write(url().path().toUtf8().append(s_eol));
}

void dgopher::slotDisonnected(void)
{
  if(m_preFetch)
    m_html.append("</body></p></html>");

  m_hasBeenPreFetched = false;
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

      char c = bytes.at(0);

      bytes.remove(0, 1);

      QList<QByteArray> list(bytes.split('\t'));

      if(!m_hasBeenPreFetched && m_preFetch)
	{
	  if(list.value(1).replace("/", "").
	     endsWith(m_path.toUtf8().constData()))
	    {
	      m_hasBeenPreFetched = true;

	      if(c == '0' || c == 'h' || c == 'i')
		{
		  disconnect(m_socket, SIGNAL(connected(void)), 0, 0);
		  disconnect(m_socket, SIGNAL(disconnected(void)), 0, 0);
		  disconnect(m_socket, SIGNAL(readyRead(void)), 0, 0);
		  abort();
		  initialize();
		  connect(m_socket,
			  SIGNAL(connected(void)),
			  this,
			  SLOT(slotConnectedForText(void)));
		  connect(m_socket,
			  SIGNAL(disconnected(void)),
			  this,
			  SLOT(slotDisonnected(void)));
		  m_socket->connectToHost(url().host(), url().port());
		  return;
		}
	      else if(c == '1')
		{
		  load();
		  return;
		}
	      else
		{
		  setError
		    (QNetworkReply::UnknownContentError,
		     "Unknown content error.");
		  abort();
		  return;
		}
	    }
	}

      if(c == '+' ||
	 c == '0' || c == '1' || c == '4' || c == '5' || c == '6' ||
	 c == '9' || c == 'g' || c == 'h' || c == 'l' || c == 's')
	{
	  int port = list.value(3).toInt();

	  if(port <= 0)
	    port = 70;

	  m_html.append
	    (QString("<a href=\"gopher://%1:%2%3\">%4</a>%5<br>\n").
	     arg(list.value(2).trimmed().constData()).
	     arg(port).
	     arg(list.value(1).constData()).
	     arg(list.value(0).constData()).
	     arg(c == '1' ? "..." : ""));
	}
      else if(c == '3' || c == 'i')
	{
	  QByteArray information(list.value(0).trimmed());

	  if(!information.isEmpty())
	    {
	      m_html.append(information);
	      m_html.append("<br>\n");
	    }
	}
    }
}

void dgopher::slotReadyReadForDownload(void)
{
  m_content.append(m_socket->readAll());
  m_html.append(m_content);
  m_content.clear();
  emit readyRead();
}

void dgopher::slotReadyReadForText(void)
{
  m_content.append(m_socket->readAll());
  m_content.replace("\n", "<br>");
  m_html.append(m_content);
  m_content.clear();
}
