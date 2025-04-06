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

#include <QFileDialog>
#include <QWebEngineCertificateError>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_page.h"
#include "ui_dooble_certificate_exceptions_widget.h"

dooble_web_engine_page::dooble_web_engine_page
(QWebEngineProfile *web_engine_profile, bool is_private, QWidget *parent):
  QWebEnginePage(web_engine_profile, parent)
{
  m_certificate_error_url = QUrl();
  m_is_private = is_private;
  connect(this,
	  SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
	  this,
	  SLOT(slot_full_screen_requested(QWebEngineFullScreenRequest)));
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
}

dooble_web_engine_page::dooble_web_engine_page(QWidget *parent):
  QWebEnginePage(dooble::s_default_web_engine_profile, parent)
{
  m_certificate_error_url = QUrl();
  m_is_private = false;
  connect(this,
	  SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
	  this,
	  SLOT(slot_full_screen_requested(QWebEngineFullScreenRequest)));
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
}

dooble_web_engine_page::~dooble_web_engine_page()
{
}

QStringList dooble_web_engine_page::chooseFiles
(FileSelectionMode mode,
 const QStringList &old_files,
 const QStringList &accepted_mime_types)
{
  switch(mode)
    {
    case QWebEnginePage::FileSelectOpen:
      {
	return QStringList() << QFileDialog::getOpenFileName
	  (view(), tr("Select File"), QDir::homePath(), old_files.value(0));
      }
    case QWebEnginePage::FileSelectOpenMultiple:
      {
	return QFileDialog::getOpenFileNames
	  (view(), tr("Select Files"), QDir::homePath());
      }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    case QWebEnginePage::FileSelectSave:
      {
	return QStringList() << QFileDialog::getSaveFileName
	  (view(), tr("Save File"), QDir::homePath(), old_files.value(0));
      }
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    case QWebEnginePage::FileSelectUploadFolder:
      {
	return QStringList() << QFileDialog::getExistingDirectory
	  (view(), tr("Select Directory"), QDir::homePath());
      }
#endif
    default:
      {
	break;
      }
    }

  return QWebEnginePage::chooseFiles(mode, old_files, accepted_mime_types);
}

QUrl dooble_web_engine_page::simplified_url(void) const
{
  return dooble_ui_utilities::simplified_url(url());
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QWidget *dooble_web_engine_page::view(void) const
{
  return qobject_cast<QWidget *> (parent());
}
#endif

bool dooble_web_engine_page::acceptNavigationRequest(const QUrl &url,
						     NavigationType type,
						     bool is_main_frame)
{
  emit loading(url);

  if(dooble::s_accepted_or_blocked_domains->exception(url))
    return true;

  Q_UNUSED(is_main_frame);
  Q_UNUSED(type);

  auto const mode
    (dooble_settings::setting("accepted_or_blocked_domains_mode").toString());
  auto host(url.host());
  auto state = true;
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
(const QWebEngineCertificateError &certificate_error)
{
  if(certificate_error.isOverridable())
    {
      auto const url
	(dooble_ui_utilities::simplified_url(certificate_error.url()));

      if(m_is_private)
	if(profile()->property(("certificate_exception_" + url.toString()).
			       toStdString().data()).isValid() ||
	   profile()->property(("certificate_exception_" +
				url.toString() +
				"/").toStdString().data()).isValid())
	  return true;

      if(dooble_certificate_exceptions_menu_widget::has_exception(url))
	return true;

      if(!view())
	return false;

      auto layout = qobject_cast<QLayout *> (view()->layout());

      if(!layout)
	return false;
      else
	for(int i = 0; i < layout->count(); i++)
	  if(layout->itemAt(i) && layout->itemAt(i)->widget())
	    layout->itemAt(i)->widget()->setVisible(false);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      m_certificate_error = certificate_error.errorDescription();
#else
      m_certificate_error = certificate_error.description();
#endif
      m_certificate_error_url = url;

      if(!m_certificate_error_widget)
	{
	  m_certificate_error_widget = new QWidget(view());
	  m_ui.setupUi(m_certificate_error_widget);
	  connect(m_ui.accept,
		  SIGNAL(clicked(void)),
		  this,
		  SLOT(slot_certificate_exception_accepted(void)));
	}

      if(m_is_private)
	m_ui.label->setText
	  (tr("<html>A certificate error occurred while accessing "
	      "the secure site %1. <b>%2</b> "
	      "Certificate errors may indicate that the server that "
	      "you're attempting to connect to is not trustworthy.<br><br>"
	      "Please accept or decline the temporary "
	      "exception.</html>").
	   arg(url.toString()).
	   arg(
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	       certificate_error.errorDescription()
#else
	       certificate_error.description()
#endif
	       ));
      else
	m_ui.label->setText
	  (tr("<html>A certificate error occurred while accessing "
	      "the secure site %1. <b>%2</b> "
	      "Certificate errors may indicate that the server that "
	      "you're attempting to connect to is not trustworthy.<br><br>"
	      "Please accept or decline the permanent "
	      "exception.<br><br>"
	      "Permanent exceptions may be removed later.</html>").
	   arg(url.toString()).
	   arg(
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	       certificate_error.errorDescription()
#else
	       certificate_error.description()
#endif
	       ));

      m_certificate_error_widget->resize(view()->size());
      m_certificate_error_widget->setVisible(true);
    }
  else
    {
      if(!view())
	return false;

      auto layout = qobject_cast<QLayout *> (view()->layout());

      if(!layout)
	return false;
      else
	for(int i = 0; i < layout->count(); i++)
	  if(layout->itemAt(i) && layout->itemAt(i)->widget())
	    layout->itemAt(i)->widget()->setVisible(false);

      auto const url
	(dooble_ui_utilities::simplified_url(certificate_error.url()));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      m_certificate_error = certificate_error.errorDescription();
#else
      m_certificate_error = certificate_error.description();
#endif
      m_certificate_error_url = url;

      if(!m_certificate_error_widget)
	{
	  m_certificate_error_widget = new QWidget(view());
	  m_ui.setupUi(m_certificate_error_widget);
	  connect(m_ui.accept,
		  SIGNAL(clicked(void)),
		  this,
		  SLOT(slot_certificate_exception_accepted(void)));
	}

      m_ui.accept->setVisible(false);
      m_ui.label->setText
	(tr("<html>A certificate error occurred while accessing "
	    "the secure site %1. <b>%2</b> "
	    "Certificate errors may indicate that the server that "
	    "you're attempting to connect to is not trustworthy.<br><br>"
	    "An exception may not be set because of the severity of the "
	    "certificate error.</html>").
	 arg(url.toString()).
	 arg(
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	     certificate_error.errorDescription()
#else
	     certificate_error.description()
#endif
	     ));
      m_certificate_error_widget->resize(view()->size());
      m_certificate_error_widget->setVisible(true);
    }

  return false;
}

void dooble_web_engine_page::resize_certificate_error_widget(void)
{
  if(m_certificate_error_widget && view())
    m_certificate_error_widget->resize(view()->size());
}

void dooble_web_engine_page::slot_certificate_exception_accepted(void)
{
  if(m_is_private)
    {
      profile()->setProperty
	(("certificate_exception_" + m_certificate_error_url.toString()).
	 toStdString().data(), m_certificate_error);
      profile()->setProperty
	(("certificate_exception_" + m_certificate_error_url.toString() + "/").
	 toStdString().data(), m_certificate_error);
    }
  else
    dooble_certificate_exceptions_menu_widget::exception_accepted
      (m_certificate_error, m_certificate_error_url);

  emit certificate_exception_accepted(url());
}

void dooble_web_engine_page::slot_full_screen_requested
(QWebEngineFullScreenRequest full_screen_request)
{
  if(dooble_settings::setting("full_screen").toBool())
    {
      full_screen_request.accept();
      emit show_full_screen(full_screen_request.toggleOn());
    }
  else
    {
      full_screen_request.reject();
      emit show_full_screen(false);
    }
}

void dooble_web_engine_page::slot_load_started(void)
{
  m_certificate_error = QString();
  m_certificate_error_url = QUrl();

  if(m_certificate_error_widget)
    {
      QLayout *layout = nullptr;

      if(view())
	layout = qobject_cast<QLayout *> (view()->layout());

      if(layout)
	layout->removeWidget(m_certificate_error_widget);

      m_certificate_error_widget->deleteLater();

      if(layout)
	for(int i = 0; i < layout->count(); i++)
	  if(layout->itemAt(i) && layout->itemAt(i)->widget())
	    layout->itemAt(i)->widget()->setVisible(true);
    }
}
