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

#include <QBuffer>
#include <QFileInfo>
#include <QStandardPaths>

#include "dooble_web_engine_view.h"
#include "dooble_zip.h"

dooble_zip::dooble_zip(QObject *parent):QWebEngineUrlSchemeHandler(parent)
{
}

void dooble_zip::requestStarted(QWebEngineUrlRequestJob *request)
{
  if(!request || m_request == request)
    return;

  m_request = request;

  auto zip_implementation = new dooble_zip_implementation
    (m_request->requestUrl(), m_web_engine_view, m_request);

  connect(zip_implementation,
	  SIGNAL(error(QWebEngineUrlRequestJob::Error)),
	  this,
	  SLOT(slot_error(QWebEngineUrlRequestJob::Error)));
  connect(zip_implementation,
	  SIGNAL(finished(const QByteArray &, const bool)),
	  this,
	  SLOT(slot_finished(const QByteArray &, const bool)));
}

void dooble_zip::set_web_engine_view(dooble_web_engine_view *web_engine_view)
{
  m_web_engine_view = web_engine_view;
}

void dooble_zip::slot_error(QWebEngineUrlRequestJob::Error error)
{
  if(m_request)
    m_request->fail(error);
}

void dooble_zip::slot_finished(const QByteArray &bytes, const bool file)
{
  if(m_request)
    {
      if(bytes.isEmpty())
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
      else
	{
	  if(file)
	    {
	      QUrl const url;

	      m_request->redirect(url);
	    }
	  else
	    {
	      auto buffer = new QBuffer(m_request);

	      buffer->setData(bytes);
	      m_request->reply("text/html", buffer);
	    }
	}
    }
}

dooble_zip_implementation::dooble_zip_implementation
(const QUrl &url,
 dooble_web_engine_view *web_engine_view,
 QObject *parent):QProcess(parent)
{
  connect(this,
	  SIGNAL(finished(int, QProcess::ExitStatus)),
	  this,
	  SLOT(slot_finished(int, QProcess::ExitStatus)));
  connect(this,
	  SIGNAL(readyReadStandardOutput(void)),
	  this,
	  SLOT(slot_ready_read(void)));
  m_url = url;
  m_web_engine_view = web_engine_view;
  setWorkingDirectory
    (QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).
     value(0));

  if(m_url.hasQuery())
    start
      ("jar", QStringList() << "-f" << m_url.path() << "-x" << m_url.query());
  else
    start("unzip", QStringList() << "-v" << m_url.path());
}

dooble_zip_implementation::~dooble_zip_implementation()
{
  kill();
}

void dooble_zip_implementation::slot_finished
(int exit_code, QProcess::ExitStatus exit_status)
{
  Q_UNUSED(exit_code);
  Q_UNUSED(exit_status);

  if(m_url.hasQuery())
    {
      /*
      ** Redirect the request.
      */

      emit finished(m_url.query().toUtf8(), true);
      return;
    }

  m_html = "<html>\n";
  m_html += "<head>\n";
  m_html += "<style>\n";
  m_html += "td {padding-left: 0px; padding-right: 10px}\n";
  m_html += "</style>\n";
  m_html += "</head>\n";
  m_html += "<body bgcolor=\"white\" style=\"font-family: monospace\">\n";
  m_html += "<title>";
  m_html += m_url.path().toUtf8();
  m_html += "</title>\n";

  if(QFileInfo(m_url.path()).isReadable())
    {
      /*
      ** Archive: File
      ** Length Method Size Cmpr Date Time CRC-32 Name
      ** ------ ------ ---- ---- ---- ---- ------ ----
      ** ...
      ** ------ ------ ---- ---- ---- ---- ------ ----
      ** Total         Total %                    Count
      */

      auto const output(m_content.split('\n'));

      m_html += "<table>\n";
      m_html += "<tr>\n";

      foreach(auto const &i, output.value(1).split(' '))
	if(i.trimmed().size() > 0)
	  m_html += "<th>" + i.trimmed() + "</th>\n";

      m_html += "</tr>\n";

      for(int i = 3; i < output.size() - 3; i++)
	{
	  auto const bytes(output.at(i).trimmed());

	  if(bytes.size() > 0)
	    {
	      m_html += "<tr>\n";

	      auto const list(bytes.split(' '));

	      for(int j = 0; j < list.size(); j++)
		if(list.at(j).trimmed().size() > 0)
		  {
		    if(j == list.size() - 1)
		      {
			QString file("");

			file = QString("<a href=\"zip://%1?%2\">%2</a>").
			  arg(m_url.path()).arg(list.value(j).constData());

			m_html += "<td>" + file.toUtf8() + "</td>\n";
		      }
		    else
		      m_html += "<td>" + list.at(j).trimmed() + "</td>\n";
		  }

	      m_html += "</tr>\n";
	    }
	}

      m_html += "</table>\n";
    }
  else
    m_html += "The file " + m_url.path().toUtf8() + " is not readable.\n";

  m_html += "</body></html>";
  emit finished(m_html, false);
}

void dooble_zip_implementation::slot_ready_read(void)
{
  while(bytesAvailable() > 0)
    m_content.append(readAll());
}
