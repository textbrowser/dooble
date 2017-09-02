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

#include <QStackedLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_web_engine_page.h"
#include "ui_dooble_certificate_exceptions_widget.h"

dooble_web_engine_page::dooble_web_engine_page
(QWebEngineProfile *web_engine_profile, bool is_private, QWidget *parent):
  QWebEnginePage(web_engine_profile, parent)
{
  m_certificate_error_url = QUrl();
  m_certificate_error_widget = 0;
  m_is_private = is_private;
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
}

dooble_web_engine_page::dooble_web_engine_page(QWidget *parent):
  QWebEnginePage(parent)
{
  m_certificate_error_url = QUrl();
  m_certificate_error_widget = 0;
  m_is_private = false;
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
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

  QString host(url.host());
  QString mode
    (dooble_settings::setting("accepted_or_blocked_domains_mode").toString());
  bool state = true;
  int index = -1;

  if(mode == "accept")
    state = true;
  else
    state = false;

  while(!host.isEmpty())
    if(dooble::s_accepted_or_blocked_domains->contains(host))
      return state;
    else if((index = host.indexOf('.')) > 0)
      host.remove(0, index + 1);
    else
      break;

  return true;
}

bool dooble_web_engine_page::certificateError
(const QWebEngineCertificateError &certificateError)
{
  if(certificateError.isOverridable())
    {
      if(dooble_certificate_exceptions_menu_widget::
	 has_exception(certificateError.url()))
	return true;

      m_certificate_error_url = certificateError.url();

      if(!m_certificate_error_widget)
	{
	  m_certificate_error_widget = new QWidget(view());
	  m_ui.setupUi(m_certificate_error_widget);
	  connect(m_ui.accept,
		  SIGNAL(clicked(void)),
		  this,
		  SLOT(slot_certificate_exception_accepted(void)));
	  connect(m_ui.confirm_exception,
		  SIGNAL(toggled(bool)),
		  m_ui.accept,
		  SLOT(setEnabled(bool)));
	}

      m_certificate_error_widget->resize(view()->size());
      m_certificate_error_widget->setVisible(true);
      m_ui.accept->setEnabled(false);
      m_ui.confirm_exception->setChecked(false);
      m_ui.label->setText
	(tr("<html>A certificate error occurred while accessing "
	    "the secure site %1. <b>%2</b> "
	    "Certificate errors may indicate that the server that "
	    "you're attempting to connect to is not trustworthy.<br><br>"
	    "Please accept or decline the permanent "
	    "exception.<br><br>"
	    "Permanent exceptions may be removed later.</html>").
	 arg(certificateError.url().toString()).
	 arg(certificateError.errorDescription()));
      view()->layout()->removeWidget(m_certificate_error_widget);
      view()->layout()->addWidget(m_certificate_error_widget);
      qobject_cast<QStackedLayout *> (view()->layout())->
	setCurrentWidget(m_certificate_error_widget);
    }
  else if(m_certificate_error_widget)
    {
      m_certificate_error_widget->setVisible(false);
      view()->layout()->removeWidget(m_certificate_error_widget);
    }

  return false;
}

void dooble_web_engine_page::slot_certificate_exception_accepted(void)
{
  dooble_certificate_exceptions_menu_widget::exception_accepted
    (m_certificate_error_url);
  emit certificate_exception_accepted(m_certificate_error_url);
}

void dooble_web_engine_page::slot_load_started(void)
{
  m_certificate_error_url = QUrl();

  if(m_certificate_error_widget)
    view()->layout()->removeWidget(m_certificate_error_widget);
}
