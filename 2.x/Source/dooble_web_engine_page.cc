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

#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_blocked_domains.h"
#include "dooble_web_engine_page.h"

dooble_web_engine_page::dooble_web_engine_page
(QWebEngineProfile *web_engine_profile, bool is_private, QWidget *parent):
  QWebEnginePage(web_engine_profile, parent)
{
  m_is_private = is_private;
}

dooble_web_engine_page::dooble_web_engine_page(QWidget *parent):
  QWebEnginePage(parent)
{
  m_is_private = false;
}

dooble_web_engine_page::~dooble_web_engine_page()
{
}

bool dooble_web_engine_page::acceptNavigationRequest(const QUrl &url,
						     NavigationType type,
						     bool isMainFrame)
{
  Q_UNUSED(type);
  Q_UNUSED(isMainFrame);

  QString host(url.host().toLower());
  int index = -1;

  while(!host.isEmpty())
    if(dooble::s_blocked_domains->contains(host))
      return false;
    else if((index = host.indexOf('.')) > 0)
      host.remove(0, index + 1);
    else
      break;

  return true;
}
