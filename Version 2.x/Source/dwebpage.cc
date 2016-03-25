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

#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include <QWebEngineHistory>
#include <QWebEnginePage>

#include "dexceptionswindow.h"
#include "dmisc.h"
#include "dnetworkaccessmanager.h"
#include "dooble.h"
#include "dwebpage.h"
#include "dwebview.h"

dwebpage::dwebpage(QObject *parent):QWebEnginePage(parent)
{
  m_isJavaScriptEnabled = dooble::s_settings.
    value("settingsWindow/javascriptEnabled", false).
    toBool();
  m_networkAccessManager = new dnetworkaccessmanager(this);

  /*
  ** The initialLayoutCompleted() signal is not emitted by every
  ** page.
  */

  connect(this,
	  SIGNAL(iconUrlChanged(const QUrl &)),
	  this,
	  SLOT(slotIconUrlChanged(const QUrl &)));
  connect(this,
	  SIGNAL(popupRequested(const QString &,
				const QUrl &,
				const QDateTime &)),
	  dooble::s_popupsWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(this,
	  SIGNAL(alwaysHttpsException(const QString &,
				      const QUrl &,
				      const QDateTime &)),
	  dooble::s_alwaysHttpsExceptionsWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(this,
	  SIGNAL(urlRedirectionRequest(const QString &,
				       const QUrl &,
				       const QDateTime &)),
	  dooble::s_httpRedirectWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(doNotTrack(const QString &,
			    const QUrl &,
			    const QDateTime &)),
	  dooble::s_dntWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(blockThirdPartyHost(const QString &,
				     const QUrl &,
				     const QDateTime &)),
	  dooble::s_adBlockWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(suppressHttpReferrer(const QString &,
				      const QUrl &,
				      const QDateTime &)),
	  dooble::s_httpReferrerWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(urlRedirectionRequest(const QString &,
				       const QUrl &,
				       const QDateTime &)),
	  this,
	  SIGNAL(urlRedirectionRequest(const QString &,
				       const QUrl &,
				       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(loadImageRequest(const QString &,
				  const QUrl &,
				  const QDateTime &)),
	  dooble::s_imageBlockWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));
  connect(m_networkAccessManager,
	  SIGNAL(finished(QNetworkReply *)),
	  this,
	  SLOT(slotFinished(QNetworkReply *)));
}

void dwebpage::javaScriptAlert(const QUrl &url, const QString &msg)
{
  /*
  ** The default implementation of javaScriptAlert() causes
  ** segmentation faults when viewing certain Web pages.
  */

  Q_UNUSED(url);

  if(settings()->testAttribute(QWebEngineSettings::JavascriptEnabled) &&
     !dooble::s_settings.value("settingsWindow/javascriptAcceptAlerts",
			       true).toBool())
    return;

  QMessageBox *mb = new QMessageBox(view());

  connect(mb,
	  SIGNAL(finished(int)),
	  mb,
	  SLOT(deleteLater(void)));
#ifdef Q_OS_MAC
  mb->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  mb->setIcon(QMessageBox::Information);
  mb->setWindowTitle(tr("JavaScript Alert"));
  mb->setWindowModality(Qt::WindowModal);
  mb->setStandardButtons(QMessageBox::Ok);
  mb->setText(msg);

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  for(int i = 0; i < mb->buttons().size(); i++)
    if(mb->buttonRole(mb->buttons().at(i)) == QMessageBox::AcceptRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::ApplyRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::YesRole)
      {
	mb->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	mb->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  mb->setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));
  mb->exec();
}

bool dwebpage::javaScriptConfirm(const QUrl &url, const QString &msg)
{
  Q_UNUSED(url);

  if(settings()->testAttribute(QWebEngineSettings::JavascriptEnabled) &&
     !dooble::s_settings.value("settingsWindow/javascriptAcceptConfirmations",
			       true).toBool())
    return false;

  QMessageBox *mb = new QMessageBox(view());

  connect(mb,
	  SIGNAL(finished(int)),
	  mb,
	  SLOT(deleteLater(void)));
#ifdef Q_OS_MAC
  mb->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  mb->setIcon(QMessageBox::Question);
  mb->setWindowTitle(tr("JavaScript Confirm"));
  mb->setWindowModality(Qt::WindowModal);
  mb->setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb->setText(msg);

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  for(int i = 0; i < mb->buttons().size(); i++)
    if(mb->buttonRole(mb->buttons().at(i)) == QMessageBox::AcceptRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::ApplyRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::YesRole)
      {
	mb->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	mb->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	mb->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	mb->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  mb->setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  if(mb->exec() == QMessageBox::Yes)
    return true;
  else
    return false;
}

bool dwebpage::shouldInterruptJavaScript(void)
{
  int value = qBound
    (0, dooble::s_settings.
     value("settingsWindow/javascriptStagnantScripts",
	   2).toInt(), 2);

  if(value == 0) // Continue.
    return false;
  else if(value == 1) // Interrupt.
    return true;
  else
    return javaScriptConfirm
      (QUrl(), tr("Dooble has detected a stagnant JavaScript "
		  "script. Should the script be terminated?"));
}

bool dwebpage::javaScriptPrompt(const QUrl &url, const QString &msg,
				const QString &defaultValue, QString *result)
{
  Q_UNUSED(url);

  if(settings()->testAttribute(QWebEngineSettings::JavascriptEnabled) &&
     !dooble::s_settings.value("settingsWindow/javascriptAcceptPrompts",
			       true).toBool())
    return false;

  if(!result)
    return false;

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);
  QPointer<QInputDialog> id = new QInputDialog(view());

  connect(id,
	  SIGNAL(finished(int)),
	  id,
	  SLOT(deleteLater(void)));
#ifdef Q_OS_MAC
  id->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  id->setInputMode(QInputDialog::TextInput);
  id->setLabelText(msg);
  id->setWindowTitle(tr("JavaScript Prompt"));
  id->setWindowModality(Qt::WindowModal);
  id->setTextValue(defaultValue);
  id->setOkButtonText(tr("OK"));
  id->setCancelButtonText(tr("Cancel"));
  id->setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  foreach(QPushButton *button, id->findChildren<QPushButton *> ())
    if(button->text() == tr("OK"))
      {
	button->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	button->setIconSize(QSize(16, 16));
      }
    else
      {
	button->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	button->setIconSize(QSize(16, 16));
      }

  if(id->exec() == QDialog::Accepted)
    {
      if(id)
	*result = id->textValue();
      else
	*result = "";

      return true;
    }
  else
    {
      *result = "";
      return false;
    }
}

QWebEnginePage *dwebpage::createWindow(WebWindowType type)
{
  Q_UNUSED(type);

  if(dooble::s_settings.value("settingsWindow/blockPopups", true).toBool())
    {
      /*
      ** This is a difficult situation. The createWindow() method
      ** doesn't provide us with a URL.
      */
    }

  if(settings()->testAttribute(QWebEngineSettings::JavascriptEnabled))
    {
      if(dooble::s_settings.value("settingsWindow/javascriptAllowNewWindows",
				  true).toBool())
	{
	  dooble *dbl = 0;

	  if(type == QWebEnginePage::WebBrowserTab)
	    dbl = findDooble();

	  if(!dbl)
	    dbl = new dooble(true, findDooble());

	  dview *v = dbl->newTab
	    (m_requestedUrl,
	     qobject_cast<dcookies *> (m_networkAccessManager->
				       cookieJar()),
	     webAttributes());

	  return v->page();
	}
      else
	return 0;
    }

  /*
  ** A new window will not be created.
  */

  return 0;
}

dooble *dwebpage::findDooble(void)
{
  dooble *dbl = 0;
  QObject *prnt(this);

  do
    {
      prnt = prnt->parent();
      dbl = qobject_cast<dooble *> (prnt);
    }
  while(prnt != 0 && dbl == 0);

  return dbl;
}

QString dwebpage::userAgentForUrl(const QUrl &url) const
{
  Q_UNUSED(url);

  QString agent("");

  agent.remove(QString("%1/%2").arg("Dooble").arg(DOOBLE_VERSION_STR));
  return agent;
}

dwebpage::~dwebpage()
{
}

void dwebpage::downloadFavicon(const QUrl &faviconUrl, const QUrl &url)
{
  if(dooble::s_settings.value("settingsWindow/enableFaviconsDatabase",
			      false).toBool())
    if(faviconUrl.scheme().toLower().trimmed() == "http" ||
       faviconUrl.scheme().toLower().trimmed() == "https" ||
       faviconUrl.scheme().toLower().trimmed() == "qrc")
      {
	QNetworkProxy proxy(m_networkAccessManager->proxy());
	QNetworkReply *reply = 0;
	QNetworkRequest request(faviconUrl);

	request.setAttribute(QNetworkRequest::User, "dooble-favicon");
	m_networkAccessManager->setProxy(dmisc::proxyByUrl(faviconUrl));
	reply = m_networkAccessManager->get(request);
	m_networkAccessManager->setProxy(proxy);
	reply->setProperty("dooble-favicon", true);
	reply->setProperty("dooble-favicon-for-url", url);
	connect(reply, SIGNAL(finished(void)),
		this, SLOT(slotIconDownloadFinished(void)));
      }
}

void dwebpage::slotIconDownloadFinished(void)
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> (sender());

  if(!dooble::s_settings.value("settingsWindow/enableFaviconsDatabase",
			       false).toBool())
    {
      if(reply)
	reply->deleteLater();

      return;
    }

  if(reply)
    {
      QUrl url(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).
	       toUrl());

      /*
      ** The URL may be relative.
      */

      url = reply->url().resolved(url);

      if(!url.isEmpty() && url.isValid() &&
	 url.toString(QUrl::StripTrailingSlash) !=
	 reply->url().toString(QUrl::StripTrailingSlash))
	{
	  if(dooble::s_httpRedirectWindow->allowed(url.host()))
	    {
	      if(url.path().isEmpty())
		url.setPath("/favicon.ico");

	      QNetworkProxy proxy(m_networkAccessManager->proxy());
	      QNetworkReply *r = 0;
	      QNetworkRequest request(url);

	      request.setAttribute(QNetworkRequest::User, "dooble-favicon");
	      m_networkAccessManager->setProxy(dmisc::proxyByUrl(url));
	      r = m_networkAccessManager->get(request);
	      m_networkAccessManager->setProxy(proxy);
	      r->setProperty("dooble-favicon", true);
	      r->setProperty("dooble-favicon-for-url",
			     reply->property("dooble-favicon-for-url"));
	      connect(r, SIGNAL(finished(void)),
		      this, SLOT(slotIconDownloadFinished(void)));
	    }
	  else
	    {
	      emit exceptionRaised(dooble::s_httpRedirectWindow, url);
	      emit urlRedirectionRequest(url.host(),
					 reply->url(),
					 QDateTime::currentDateTime());
	    }
	}
      else
	{
	  QIcon icon;
	  QPixmap pixmap;
	  QByteArray bytes(reply->readAll());

	  if(pixmap.loadFromData(bytes, "gif"))
	    icon = QIcon(pixmap);
	  else if(pixmap.loadFromData(bytes, "ico"))
	    icon = QIcon(pixmap);
	  else if(pixmap.loadFromData(bytes, "jpg"))
	    icon = QIcon(pixmap);
	  else if(pixmap.loadFromData(bytes, "jpeg"))
	    icon = QIcon(pixmap);
	  else if(pixmap.loadFromData(bytes, "png"))
	    icon = QIcon(pixmap);
	  else
	    icon = QIcon();

	  if(!icon.isNull())
	    {
	      QUrl urlForFavicon
		(reply->property("dooble-favicon-for-url").toUrl());

	      dmisc::saveIconForUrl(icon, urlForFavicon);
	      emit iconChanged();
	    }
	}

      reply->deleteLater();
    }
}

void dwebpage::slotInitialLayoutCompleted(void)
{
  if(!dooble::s_settings.value("settingsWindow/enableFaviconsDatabase",
			       false).toBool())
    return;
}

void dwebpage::slotFinished(QNetworkReply *reply)
{
  if(!reply)
    return;
  else if(reply->property("dooble-favicon").toBool())
    return;
  else if(!reply->url().isValid() || !m_requestedUrl.isValid())
    return;
  else if(!dmisc::isSchemeAcceptedByDooble(reply->url().scheme()) ||
	  !dmisc::isSchemeAcceptedByDooble(m_requestedUrl.scheme()))
    return;

  int status = reply->attribute
    (QNetworkRequest::HttpStatusCodeAttribute).toInt();

  if(status >= 400 && status <= 599 &&
     dmisc::s_httpStatusCodes.contains(status) &&
     dmisc::s_httpStatusCodes.value(status, 1) == 0)
    {
      QVariant variant(reply->header(QNetworkRequest::ContentLengthHeader));

      dmisc::logError
	(QString("dwebpage::slotFinished(): "
		 "The URL %1 generated an error (HTTP %2).").
	 arg(reply->url().toString(QUrl::StripTrailingSlash)).
	 arg(status));

      if(dooble::s_settings.value("settingsWindow/alwaysHttps", false).
	 toBool())
	if(reply->url().scheme().toLower().trimmed() == "https")
	  {
	    emit exceptionRaised(dooble::s_alwaysHttpsExceptionsWindow,
				 reply->url());
	    emit alwaysHttpsException(reply->url().host(), reply->url(),
				      QDateTime::currentDateTime());
	  }

      if(variant.isNull())
	return;
      else if(variant.toInt() > 512)
	return;
    }

  switch(reply->error())
    {
    case QNetworkReply::NoError:
    case QNetworkReply::OperationCanceledError:
      {
	return;
      }
    default:
      {
	dmisc::logError
	  (QString("dwebpage::slotFinished(): "
		   "The URL %1 generated an error (%2:%3).").
	   arg(reply->url().toString(QUrl::StripTrailingSlash)).
	   arg(reply->error()).
	   arg(reply->errorString()));

	if(dooble::s_settings.value("settingsWindow/alwaysHttps", false).
	   toBool())
	  if(reply->url().scheme().toLower().trimmed() == "https")
	    {
	      emit exceptionRaised
		(dooble::s_alwaysHttpsExceptionsWindow, reply->url());
	      emit alwaysHttpsException
		(reply->url().host(), reply->url(),
		 QDateTime::currentDateTime());
	    }

	break;
      }
    }

  if(reply->url().toString(QUrl::StripTrailingSlash) ==
     m_requestedUrl.toString(QUrl::StripTrailingSlash))
    emit loadErrorPage(m_requestedUrl);
}

void dwebpage::slotHttpToHttps(void)
{
  if(!dooble::s_settings.value("settingsWindow/alwaysHttps", false).toBool())
    return;
}

void dwebpage::slotUrlChanged(const QUrl &url)
{
  Q_UNUSED(url);
}

QHash<QWebEngineSettings::WebAttribute, bool> dwebpage::
webAttributes(void) const
{
  QHash<QWebEngineSettings::WebAttribute, bool> hash;

  hash[QWebEngineSettings::JavascriptEnabled] = m_isJavaScriptEnabled;
  return hash;
}

bool dwebpage::areWebPluginsEnabled(void) const
{
  return false;
}

bool dwebpage::isJavaScriptEnabled(void) const
{
  return m_isJavaScriptEnabled;
}

bool dwebpage::isPrivateBrowsingEnabled(void) const
{
  return false;
}

void dwebpage::setJavaScriptEnabled(const bool state)
{
  /*
  ** If a JavaScript exception is set for a particular site,
  ** we cannot disable the tab's JavaScript option because the
  ** user may navigate to a site that should have JavaScript enabled.
  */

  m_isJavaScriptEnabled = state;
  settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, state);
}

void dwebpage::showWebInspector(void)
{
}

void dwebpage::slotIconUrlChanged(const QUrl &url)
{
  downloadFavicon(url, this->url());
}
