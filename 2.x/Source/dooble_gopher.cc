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

#include <QWebEngineUrlRequestJob>

#include "dooble_gopher.h"

QByteArray dooble_gopher::s_eol = "\r\n";

dooble_gopher::dooble_gopher(QObject *parent):QWebEngineUrlSchemeHandler(parent)
{
  m_buffer = new QBuffer(this);
  connect(&m_tcp_socket,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slot_connected(void)));
  connect(&m_tcp_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_disonnected(void)));
  connect(&m_tcp_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
}

QByteArray dooble_gopher::plain_to_html(const QByteArray &bytes)
{
  QByteArray b(bytes);

  b.replace("&", "&amp;");
  b.replace("<", "&lt;");
  b.replace(">", "&gt;");
  b.replace(" ", "&nbsp;");
  return b;
}

void dooble_gopher::requestStarted(QWebEngineUrlRequestJob *request)
{
  if(!request)
    return;

  m_content.clear();
  m_html.clear();
  m_item_type = 0;
  m_request = request;
  m_tcp_socket.abort();
  m_url = m_request->requestUrl();

  if(m_url.port() == -1)
    m_url.setPort(70);

  m_tcp_socket.connectToHost(m_url.host(), m_url.port());
}

void dooble_gopher::slot_connected(void)
{
  QString output("");
  QString path(m_url.path());
  QString query(m_url.query());

  if(path.isEmpty())
    {
      m_item_type = '1';
      output.append("/");
    }
  else
    {
      m_item_type = path.at(1).toLatin1();
      path.remove(1, 1);
      output.append(path);
     }

  if(!query.isEmpty())
    {
      output.append("?");
      output.append(query);
    }

  m_tcp_socket.write(output.toUtf8().append(s_eol));
}

void dooble_gopher::slot_disonnected(void)
{
  if(!m_request)
    return;

  m_buffer->setData(m_html);
  m_request->reply("text/html", m_buffer);
}

void dooble_gopher::slot_ready_read(void)
{
  m_content.append(m_tcp_socket.readAll());

  if(m_item_type == '0') /* Plaintext */
    {
      m_html.append
	("<html><head></head><body style=\"font-family: monospace\">");
      m_html.append(plain_to_html(m_content));
      m_html.append("</body></html>");
      m_content.clear();
    }
  else if(m_item_type == '4') /* BinHex Encoded Text File */
    {
      if(m_request)
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
    }
  else if(m_item_type == '5') /* Binary Archive File */
    {
      if(m_request)
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
    }
  else if(m_item_type == '6') /* UUEncoded Text File */
    {
      if(m_request)
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
    }
  else if(m_item_type == '9') /* Binary File */
    {
      if(m_request)
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
    }
  else if(m_item_type == 'I') /* Image File of Unspecified Format */
    {
      m_html.append(m_content);
      m_content.clear();
    }
  else if(m_item_type == 'g') /* GIF Image */
    {
      m_html.append(m_content);
      m_content.clear();
    }
  else if(m_item_type == 'h') /* HTML File */
    {
      m_html.append(m_content);
      m_content.clear();
    }
  else if(m_item_type == 's') /* Audio File Format */
    {
    }
  else
    {
      m_html.append
	("<html><head></head><body style=\"font-family: monospace\">");

      while(m_content.contains(s_eol))
	{
	  QByteArray bytes(m_content.mid(0, m_content.indexOf(s_eol) + 1));

	  m_content.remove(0, bytes.length());
	  bytes = bytes.trimmed();

	  char c = bytes.at(0);

	  if(c == '+' ||
	     c == '0' || c == '1' || c == '3' || c == '4' || c == '5' ||
	     c == '6' ||
	     c == '9' || c == 'I' || c == 'g' || c == 'h' || c == 'i' ||
	     c == 's')
	    /*
	    ** Some things, we understand.
	    */

	    bytes.remove(0, 1);

	  QList<QByteArray> list(bytes.split('\t'));

	  if(c == '+' ||
	     c == '0' || c == '1' || c == '4' || c == '5' || c == '6' ||
	     c == '9' || c == 'I' || c == 'g' || c == 'h' || c == 's')
	    {
	      int port = list.value(3).toInt();

	      if(port <= 0)
		port = 70;

	      m_html.append
		(QString("<a href=\"gopher://%1:%2/%3%4\" "
			 "style=\"text-decoration: none;\">%5</a>%6<br>").
		 arg(list.value(2).trimmed().constData()).
		 arg(port).
		 arg(c).
		 arg(list.value(1).constData() + (list.value(1).
						  mid(0, 1) == "/")).
		 arg(plain_to_html(list.value(0)).constData()).
		 arg(c == '1' ? "..." : ""));
	    }
	  else if(c == '3' || c == 'i')
 	    {
	      QByteArray information(list.value(0));

	      if(c == 'i')
 		{
		  m_html.append(plain_to_html(information));
		  m_html.append("<br>");
 		}
 	      else
 		{
		  m_html.append("<span style=\"color: red;\">");
		  m_html.append(plain_to_html(information));
		  m_html.append("</span>");
		  m_html.append("<br>");
 		}
 	    }
	  else
 	    {
	      m_html.append(plain_to_html(bytes));
	      m_html.append("<br>");
 	    }
 	}

      m_html.append("</body></html>");
    }
}
