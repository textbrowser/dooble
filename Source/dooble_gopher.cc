/*
** Copyright (c) 2008 - present, Mattias Andrée, Alexis Megas.
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

#include <cctype>

#include "dooble_gopher.h"
#include "dooble_web_engine_view.h"

QByteArray dooble_gopher_implementation::s_eol = "\r\n";

dooble_gopher::dooble_gopher(QObject *parent):
  QWebEngineUrlSchemeHandler(parent)
{
}

void dooble_gopher::requestStarted(QWebEngineUrlRequestJob *request)
{
  if(m_request == request || !request)
    return;

  m_request = request;

  auto gopher_implementation = new dooble_gopher_implementation
    (m_request->requestUrl(),
     qobject_cast<dooble_web_engine_view *> (parent()),
     m_request);

  connect(gopher_implementation,
	  SIGNAL(error(QWebEngineUrlRequestJob::Error)),
	  this,
	  SLOT(slot_error(QWebEngineUrlRequestJob::Error)));
  connect(gopher_implementation,
	  SIGNAL(finished(const QByteArray &, bool, bool)),
	  this,
	  SLOT(slot_finished(const QByteArray &, bool, bool)));
}

void dooble_gopher::slot_error(QWebEngineUrlRequestJob::Error error)
{
  if(m_request)
    m_request->fail(error);
}

void dooble_gopher::slot_finished(const QByteArray &bytes,
				  bool content_type_supported,
				  bool is_image)
{
  if(m_request)
    {
      if(bytes.isEmpty())
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
      else
	{
	  /*
	  ** The buffer object should be deleted when m_request is.
	  */

	  auto buffer = new QBuffer(m_request);

	  if(content_type_supported && !is_image)
	    buffer->setData
	      (QByteArray(bytes).
	       replace(dooble_gopher_implementation::s_eol, "<br>"));
	  else
	    buffer->setData(bytes);

	  if(content_type_supported)
	    {
	      if(is_image)
		{
		  if(bytes.size() >= 2 &&
		     std::tolower(bytes[0]) == 'b' &&
		     std::tolower(bytes[1]) == 'm')
		    m_request->reply("image/bmp", buffer);
		  else if(bytes.size() >= 3 &&
			  std::tolower(bytes[0]) == 'g' &&
			  std::tolower(bytes[1]) == 'i' &&
			  std::tolower(bytes[2]) == 'f')
		    m_request->reply("image/gif", buffer);
		  else if(bytes.size() >= 10 &&
			  std::tolower(bytes[6]) == 'j' &&
			  std::tolower(bytes[7]) == 'f' &&
			  std::tolower(bytes[8]) == 'i' &&
			  std::tolower(bytes[9]) == 'f')
		    m_request->reply("image/jpeg", buffer);
		  else if(bytes.size() >= 4 &&
			  std::tolower(bytes[1]) == 'p' &&
			  std::tolower(bytes[2]) == 'n' &&
			  std::tolower(bytes[3]) == 'g')
		    m_request->reply("image/png", buffer);
		  else if(bytes.size() >= 4 &&
			  std::tolower(bytes[0]) == 'r' &&
			  std::tolower(bytes[1]) == 'i' &&
			  std::tolower(bytes[2]) == 'f' &&
			  std::tolower(bytes[3]) == 'f')
		    m_request->reply("image/webp", buffer);
		  else
		    m_request->reply("application/octet-stream", buffer);
		}
	      else
		m_request->reply("text/html", buffer);
	    }
	  else
	    m_request->reply("application/octet-stream", buffer);
	}
    }
}

dooble_gopher_implementation::dooble_gopher_implementation
(const QUrl &url,
 dooble_web_engine_view *web_engine_view,
 QObject *parent):QTcpSocket(parent)
{
  m_content_type_supported = true;
  m_web_engine_view = web_engine_view;
  m_is_image = false;
  m_item_type = 0;
  m_seven_count = 0;
  m_url = url;

  if(m_url.port() == -1)
    m_url.setPort(70);

  m_write_timer.setSingleShot(true);
  connect(&m_write_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_write_timeout(void)));
  connect(this,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slot_connected(void)));
  connect(this,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_disonnected(void)));
  connect(this,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
  connectToHost(m_url.host(), static_cast<quint16> (m_url.port()));
}

dooble_gopher_implementation::~dooble_gopher_implementation()
{
}

QByteArray dooble_gopher_implementation::plain_to_html(const QByteArray &bytes)
{
  auto b(bytes);

  b.replace("&", "&amp;");
  b.replace("<", "&lt;");
  b.replace(">", "&gt;");
  b.replace(" ", "&nbsp;");
  return b;
}

void dooble_gopher_implementation::slot_connected(void)
{
  QString output("");
  auto path(m_url.path());
  auto query(m_url.query());

  if(path.length() <= 1)
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

  m_output = output;

  if(m_web_engine_view)
    m_web_engine_view->page()->runJavaScript
      ("if(document.getElementById(\"input_value\") != null)"
       "document.getElementById(\"input_value\").value",
       [this] (const QVariant &result)
       {
	 m_search = result.toString();
       });

  m_write_timer.start(500);
}

void dooble_gopher_implementation::slot_disonnected(void)
{
  emit finished(m_html, m_content_type_supported, m_is_image);
}

void dooble_gopher_implementation::slot_ready_read(void)
{
  while(bytesAvailable() > 0)
    m_content.append(readAll());

  if(m_item_type == '0') /* Plaintext */
    {
      m_html.append
	("<html><head></head><body style=\"font-family: monospace\">");
      m_html.append(plain_to_html(m_content));
      m_html.append("</body></html>");
      m_content.clear();
    }
  else if(m_item_type == '4' || /* BinHex Encoded Text File */
	  m_item_type == '5' || /* Binary Archive File */
	  m_item_type == '6' || /* UUEncoded Text File */
	  m_item_type == '9')   /* Binary File */
    {
      m_content_type_supported = false;
      m_html.append(m_content);
      m_content.clear();
    }
  else if(m_item_type == 'I') /* Image File of Unspecified Format */
    {
      m_html.append(m_content);
      m_is_image = true;
      m_content.clear();
    }
  else if(m_item_type == 'g') /* GIF Image */
    {
      m_html.append(m_content);
      m_is_image = true;
      m_content.clear();
    }
  else if(m_item_type == 'h') /* HTML File */
    {
      m_html.append(m_content);
      m_content.clear();
    }
  else if(m_item_type == 's') /* Audio File Format */
    {
      m_content_type_supported = false;
      m_html.append(m_content);
      m_content.clear();
    }
  else
    {
      m_html.append
	("<html><head></head><body style=\"font-family: monospace\">");

      while(m_content.contains(s_eol))
	{
	  auto bytes(m_content.mid(0, m_content.indexOf(s_eol) + 1));

	  m_content.remove(0, bytes.length());
	  bytes = bytes.trimmed();

	  auto c = bytes.length() > 0 ? bytes.at(0) : '0';

	  if(c == '+' ||
	     c == '0' ||
	     c == '1' ||
	     c == '3' ||
	     c == '4' ||
	     c == '5' ||
	     c == '6' ||
	     c == '9' ||
	     c == 'I' ||
	     c == 'g' ||
	     c == 'h' ||
	     c == 'i' ||
	     c == 's')
	    /*
	    ** Some things, we understand.
	    */

	    bytes.remove(0, 1);

	  auto list(bytes.split('\t'));

	  if(c == '+' ||
	     c == '0' ||
	     c == '1' ||
	     c == '4' ||
	     c == '5' ||
	     c == '6' ||
	     c == '9' ||
	     c == 'I' ||
	     c == 'g' ||
	     c == 'h' ||
	     c == 's')
	    {
	      auto port = list.value(3).toInt();

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
		 arg(c == '1' ? "..." : "").toUtf8());
	    }
	  else if(c == '3' || c == 'i')
 	    {
	      auto information(list.value(0));

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
	  else if(c == '7' && m_seven_count == 0)
	    {
	      /*
	      ** Create an input search field.
	      */

	      auto port = list.value(3).toInt();

	      if(port <= 0)
		port = 70;

	      m_html.append
		(QString("<form action=\"gopher://%1:%2/%3%4\" "
			 "method=\"post\">"
			 "<input id=\"input_value\" "
			 "placeholder=\"Search\" type=\"search\" "
			 "value=\"%5\"></input>"
			 "<button type=\"submit\">&#128269;</button>"
			 "</form><br>").
		 arg(list.value(2).trimmed().constData()).
		 arg(port).
		 arg(c).
		 arg(list.value(1).constData() + (list.value(1).
						  mid(0, 1) == "/")).
		 arg(m_search).toUtf8());
	      m_seven_count += 1;
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

void dooble_gopher_implementation::slot_write_timeout(void)
{
  if(m_search.isEmpty())
    write(m_output.toUtf8().append(s_eol));
  else
    write
      (m_output.toUtf8().append("?").append(m_search.toUtf8()).append(s_eol));
}
