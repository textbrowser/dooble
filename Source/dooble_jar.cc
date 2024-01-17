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

#include "dooble_jar.h"
#include "dooble_web_engine_view.h"

dooble_jar::dooble_jar(QObject *parent):QWebEngineUrlSchemeHandler(parent)
{
}

void dooble_jar::requestStarted(QWebEngineUrlRequestJob *request)
{
  if(!request || m_request == request)
    return;

  m_request = request;

  auto jar_implementation = new dooble_jar_implementation
    (m_request->requestUrl(), m_web_engine_view, m_request);

  connect(jar_implementation,
	  SIGNAL(error(QWebEngineUrlRequestJob::Error)),
	  this,
	  SLOT(slot_error(QWebEngineUrlRequestJob::Error)));
  connect(jar_implementation,
	  SIGNAL(finished(const QByteArray &)),
	  this,
	  SLOT(slot_finished(const QByteArray &)));
}

void dooble_jar::set_web_engine_view(dooble_web_engine_view *web_engine_view)
{
  m_web_engine_view = web_engine_view;
}

void dooble_jar::slot_error(QWebEngineUrlRequestJob::Error error)
{
  if(m_request)
    m_request->fail(error);
}

void dooble_jar::slot_finished(const QByteArray &bytes)
{
  if(m_request)
    {
      if(bytes.isEmpty())
	m_request->fail(QWebEngineUrlRequestJob::RequestFailed);
      else
	{
	}
    }
}

dooble_jar_implementation::dooble_jar_implementation
(const QUrl &url,
 dooble_web_engine_view *web_engine_view,
 QObject *parent):QProcess(parent)
{
  m_url = url;
  m_web_engine_view = web_engine_view;
  m_write_timer.setSingleShot(true);
  connect(&m_write_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_write_timeout(void)));
  connect(this,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
}

dooble_jar_implementation::~dooble_jar_implementation()
{
}

void dooble_jar_implementation::slot_ready_read(void)
{
  while(bytesAvailable() > 0)
    m_content.append(readAll());
}

void dooble_jar_implementation::slot_write_timeout(void)
{
}
