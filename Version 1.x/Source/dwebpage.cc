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
#include <QSslConfiguration>
#include <QUrl>
#include <QWebElementCollection>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebInspector>

#include "dexceptionswindow.h"
#include "dmisc.h"
#include "dnetworkaccessmanager.h"
#include "dooble.h"
#include "dwebpage.h"
#include "dwebview.h"

dwebpage::dwebpage(QObject *parent):QWebPage(parent)
{
  m_webInspector = new QWebInspector();
  m_webInspector->setPage(this);
  m_isJavaScriptEnabled = dooble::s_settings.
    value("settingsWindow/javascriptEnabled", false).
    toBool();
  m_networkAccessManager = new dnetworkaccessmanager(this);
  setNetworkAccessManager(m_networkAccessManager);
  history()->setMaximumItemCount(dooble::MAX_HISTORY_ITEMS);

  /*
  ** The initialLayoutCompleted() signal is not emitted by every
  ** page.
  */

  connect(mainFrame(),
	  SIGNAL(initialLayoutCompleted(void)),
	  this,
	  SLOT(slotHttpToHttps(void)));
  connect(mainFrame(),
	  SIGNAL(initialLayoutCompleted(void)),
	  this,
	  SLOT(slotInitialLayoutCompleted(void)));
  connect(mainFrame(),
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slotHttpToHttps(void)));
  connect(mainFrame(),
	  SIGNAL(urlChanged(const QUrl &)),
	  this,
	  SLOT(slotUrlChanged(const QUrl &)));
  connect(this,
	  SIGNAL(frameCreated(QWebFrame *)),
	  this,
	  SLOT(slotFrameCreated(QWebFrame *)));
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
  setForwardUnsupportedContent(true);
}

void dwebpage::javaScriptAlert(QWebFrame *frame, const QString &msg)
{
  /*
  ** The default implementation of javaScriptAlert() causes
  ** segmentation faults when viewing certain Web pages.
  */

  Q_UNUSED(frame);

  if(settings()->testAttribute(QWebSettings::JavascriptEnabled) &&
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

bool dwebpage::javaScriptConfirm(QWebFrame *frame, const QString &msg)
{
  Q_UNUSED(frame);

  if(settings()->testAttribute(QWebSettings::JavascriptEnabled) &&
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
      (0, tr("Dooble has detected a stagnant JavaScript "
	     "script. Should the script be terminated?"));
}

bool dwebpage::javaScriptPrompt(QWebFrame *frame, const QString &msg,
				const QString &defaultValue, QString *result)
{
  Q_UNUSED(frame);

  if(settings()->testAttribute(QWebSettings::JavascriptEnabled) &&
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

QWebPage *dwebpage::createWindow(WebWindowType type)
{
  Q_UNUSED(type);

  if(dooble::s_settings.value("settingsWindow/blockPopups", true).toBool())
    {
      /*
      ** This is a difficult situation. The createWindow() method
      ** doesn't provide us with a URL.
      */

      bool allowPopup = false;
      dwebview *v = qobject_cast<dwebview *> (parent());

      if(v)
	allowPopup = v->checkAndClearPopup();

      if(!allowPopup)
	{
	  if(!dooble::s_popupsWindow->allowed(m_requestedUrl.host()))
	    {
	      emit exceptionRaised(dooble::s_popupsWindow, m_requestedUrl);
	      emit popupRequested(m_requestedUrl.host(),
				  m_requestedUrl,
				  QDateTime::currentDateTime());
	      return 0;
	    }
	  else
	    allowPopup = true;
	}

      if(!allowPopup)
	return 0;
    }

  if(settings()->testAttribute(QWebSettings::JavascriptEnabled))
    {
      if(dooble::s_settings.value("settingsWindow/javascriptAllowNewWindows",
				  true).toBool())
	{
	  dooble *dbl = 0;
	  dwebview *webview = qobject_cast<dwebview *> (parent());
	  Qt::MouseButton lastMouseButton = Qt::NoButton;

	  if(webview)
	    lastMouseButton = webview->mouseButtonPressed();

	  if(lastMouseButton == Qt::MiddleButton || (lastMouseButton !=
						     Qt::NoButton &&
						     QApplication::
						     keyboardModifiers() ==
						     Qt::ControlModifier))
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
  QObject *prnt(this);
  dooble *dbl = 0;

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
  QString agent
    (dooble::s_settings.value("settingsWindow/user_agent_string").
     toString().trimmed());

  if(agent.isEmpty())
    agent = QWebPage::userAgentForUrl(url);

  return agent;
}

dwebpage::~dwebpage()
{
  m_webInspector->deleteLater();
}

bool dwebpage::acceptNavigationRequest(QWebFrame *frame,
				       const QNetworkRequest &request,
				       NavigationType type)
{
  if(request.url().toString() == "dooble://open-ssl-errors-exceptions")
    {
      if(type == QWebPage::NavigationTypeLinkClicked)
	emit openSslErrorsExceptions();

      return false;
    }

  if(!request.url().isValid())
    return false;

  m_requestedUrl = request.url();

  if(frame == mainFrame())
    {
      if(dooble::s_javaScriptExceptionsWindow->allowed(m_requestedUrl.host()))
	/*
	** Exempt hosts must be prevented from executing JavaScript.
	*/

	settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
      else
	settings()->setAttribute
	  (QWebSettings::JavascriptEnabled, isJavaScriptEnabled());
    }

  if(m_requestedUrl.scheme().toLower().trimmed() == "data")
    /*
    ** Disable the page's network access manager.
    */

    networkAccessManager()->setNetworkAccessible
      (QNetworkAccessManager::NotAccessible);

  if(type == QWebPage::NavigationTypeLinkClicked)
    {
      if(!frame)
	{
	  if(dooble::s_settings.value("settingsWindow/openInNewTab",
				      true).toBool())
	    {
	      dooble *dbl = findDooble();

	      if(!dbl)
		dbl = new dooble(false, 0);

	      dview *v = dbl->newTab
		(m_requestedUrl,
		 qobject_cast<dcookies *> (m_networkAccessManager->
					   cookieJar()),
		 webAttributes());

	      if(dooble::s_settings.value("settingsWindow/proceedToNewTab",
					  true).toBool())
		dbl->setCurrentPage(v);

	      return false;
	    }
	  else
	    {
	      dooble *dbl = new dooble(false, findDooble());

	      dbl->newTab(m_requestedUrl,
			  qobject_cast<dcookies *> (m_networkAccessManager->
						    cookieJar()),
			  webAttributes());
	      return false;
	    }
	}
      else
	{
	  dwebview *v = qobject_cast<dwebview *> (parent());
	  Qt::MouseButton lastMouseButton = Qt::NoButton;

	  if(v)
	    lastMouseButton = v->mouseButtonPressed();

	  if(lastMouseButton == Qt::MiddleButton ||
	     (lastMouseButton != Qt::NoButton &&
	      QApplication::keyboardModifiers() == Qt::ControlModifier))
	    {
	      dooble *dbl = findDooble();

	      if(!dbl)
		dbl = new dooble(false, 0);

	      dview *v = dbl->newTab
		(m_requestedUrl,
		 qobject_cast<dcookies *> (m_networkAccessManager->
					   cookieJar()),
		 webAttributes());

	      if(dooble::s_settings.value("settingsWindow/proceedToNewTab",
					  true).toBool())
		dbl->setCurrentPage(v);

	      return false;
	    }
	}
    }

  m_networkAccessManager->setLinkClicked(m_requestedUrl);

  if(frame == mainFrame())
    /*
    ** Let's set the proxy, but only if the navigation request
    ** occurs within the main frame.
    */

    m_networkAccessManager->setProxy(dmisc::proxyByUrl(m_requestedUrl));

  return QWebPage::acceptNavigationRequest(frame, request, type);
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

	if(faviconUrl.scheme().toLower().trimmed() == "https")
	  if(dooble::s_sslCiphersWindow)
	    {
	      QSslConfiguration configuration = request.sslConfiguration();

	      configuration.setProtocol
		(dooble::s_sslCiphersWindow->protocol());
	      request.setSslConfiguration(configuration);
	    }

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

	      if(url.scheme().toLower().trimmed() == "https")
		if(dooble::s_sslCiphersWindow)
		  {
		    QSslConfiguration configuration =
		      request.sslConfiguration();

		    configuration.setProtocol
		      (dooble::s_sslCiphersWindow->protocol());
		    request.setSslConfiguration(configuration);
		  }

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

  fetchFaviconPathFromFrame(qobject_cast<QWebFrame *> (sender()));
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

  if(reply->error() == QNetworkReply::AuthenticationRequiredError)
    status = 204;
  else if(reply->error() == QNetworkReply::ContentNotFoundError)
    status = 203;

  if((status == 203 || status == 204 || (status >= 400 && status <= 599)) &&
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

  if(mainFrame() !=
     qobject_cast<QWebFrame *> (reply->request().originatingObject()))
    /*
    ** We only wish to process replies that originated from the page's
    ** main frame.
    */

    return;

  if(reply->url().toString(QUrl::StripTrailingSlash) ==
     m_requestedUrl.toString(QUrl::StripTrailingSlash))
    emit loadErrorPage(m_requestedUrl);

  if(reply->url().scheme().toLower().trimmed() == "gopher")
    if(reply->error() == QNetworkReply::UnknownContentError)
      emit unsupportedContent(reply);
}

void dwebpage::slotFrameCreated(QWebFrame *frame)
{
  if(!frame)
    return;

  disconnect(frame,
	     SIGNAL(initialLayoutCompleted(void)),
	     this,
	     SLOT(slotHttpToHttps(void)));
  disconnect(frame,
	     SIGNAL(loadFinished(bool)),
	     this,
	     SLOT(slotHttpToHttps(void)));
  connect(frame,
	  SIGNAL(initialLayoutCompleted(void)),
	  this,
	  SLOT(slotHttpToHttps(void)));
  connect(frame,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slotHttpToHttps(void)));
}

void dwebpage::slotHttpToHttps(void)
{
  if(!dooble::s_settings.value("settingsWindow/alwaysHttps", false).toBool())
    return;

  QWebFrame *frame = qobject_cast<QWebFrame *> (sender());

  if(!frame)
    return;

  foreach(QWebElement element, frame->findAllElements("a").toList())
    {
      QString href(element.attribute("href"));

      if(!href.isEmpty())
	{
	  QUrl url(href);
	  QUrl baseUrl(frame->baseUrl());

	  url = baseUrl.resolved(url);

	  if(!dooble::s_alwaysHttpsExceptionsWindow->allowed(url.host()))
	    if(url.scheme().toLower().trimmed() == "http")
	      {
		url.setScheme("https");

		if(url.port() == 80)
		  url.setPort(443);

		element.setAttribute
		  ("href", url.toString(QUrl::StripTrailingSlash));
	      }
	}
    }
}

void dwebpage::slotUrlChanged(const QUrl &url)
{
  Q_UNUSED(url);
  fetchFaviconPathFromFrame(qobject_cast<QWebFrame *> (sender()));
}

void dwebpage::fetchFaviconPathFromFrame(QWebFrame *frame)
{
  if(frame)
    {
      bool relFound = false;
      QWebElementCollection elements(frame->findAllElements("link"));

      foreach(QWebElement element, elements)
	{
	  QStringList attributesList = element.attributeNames();

	  qSort(attributesList.begin(), attributesList.end(),
		qGreater<QString> ());

	  foreach(QString attributeName, attributesList)
	    {
	      if(attributeName.toLower() == "rel" && (element.
						      attribute(attributeName).
						      toLower() == "icon" ||
						      element.
						      attribute(attributeName).
						      toLower() ==
						      "shortcut icon"))
		relFound = true;

	      if(relFound)
		if(attributeName.toLower() == "href")
		  {
		    QUrl url;
		    QString href(element.attribute(attributeName).trimmed());

		    if(href.toLower().startsWith("http://") ||
		       href.toLower().startsWith("https://"))
		      url = QUrl::fromUserInput(href);
		    else if(href.startsWith("//"))
		      {
			url = QUrl::fromUserInput(href);
			url.setScheme(frame->url().scheme().toLower().
				      trimmed());
		      }
		    else
		      {
			url.setHost(frame->url().host());

			if(!href.startsWith("/"))
			  url.setPath("/" + href);
			else
			  url.setPath(href);

			url.setScheme(frame->url().scheme().toLower().
				      trimmed());
		      }

		    if(!url.isEmpty() && url.isValid())
		      {
			downloadFavicon
			  (dmisc::correctedUrlPath(url), frame->url());
			break;
		      }
		  }
	    }

	  if(relFound)
	    break;
	}

      if(!relFound)
	{
	  QUrl url;

	  url.setHost(frame->url().host());
	  url.setPath("/favicon.ico");
	  url.setScheme(frame->url().scheme().toLower().trimmed());
	  downloadFavicon(url, frame->url());
	}
    }
}

QHash<QWebSettings::WebAttribute, bool> dwebpage::webAttributes(void) const
{
  QHash<QWebSettings::WebAttribute, bool> hash;

  hash[QWebSettings::JavascriptEnabled] = m_isJavaScriptEnabled;
  hash[QWebSettings::PluginsEnabled] = settings()->testAttribute
    (QWebSettings::PluginsEnabled);
  hash[QWebSettings::PrivateBrowsingEnabled] = settings()->testAttribute
    (QWebSettings::PrivateBrowsingEnabled);
  return hash;
}

bool dwebpage::areWebPluginsEnabled(void) const
{
  return settings()->testAttribute(QWebSettings::PluginsEnabled);
}

bool dwebpage::isJavaScriptEnabled(void) const
{
  return m_isJavaScriptEnabled;
}

bool dwebpage::isPrivateBrowsingEnabled(void) const
{
  return settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled);
}

void dwebpage::setJavaScriptEnabled(const bool state)
{
  /*
  ** If a JavaScript exception is set for a particular site,
  ** we cannot disable the tab's JavaScript option because the
  ** user may navigate to a site that should have JavaScript enabled.
  */

  m_isJavaScriptEnabled = state;
  settings()->setAttribute(QWebSettings::JavascriptEnabled, state);
}

void dwebpage::showWebInspector(void)
{
  m_webInspector->setVisible(true);
}
