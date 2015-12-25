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

#include <QDialog>
#include <QSslConfiguration>
#include <QWebFrame>

#include "dftp.h"
#include "dgopher.h"
#include "dmisc.h"
#include "dnetworkaccessmanager.h"
#include "dnetworkcache.h"
#include "dooble.h"
#include "dwebpage.h"

/*
** The dnetworkblockreply, dnetworkdirreply, dnetworkerrorreply,
** dnetworkftpreply, and dnetworksslerrorreply classes were created in
** order to inject entries into a QWebPage's history.
*/

dnetworkblockreply::dnetworkblockreply
(QObject *parent, const QNetworkRequest &req):QNetworkReply(parent)
{
  QNetworkRequest request(req);
  QUrl url(request.url());

  request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
  setRequest(request);
  setUrl(url);
  open(ReadOnly | Unbuffered);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::ContentLengthHeader, m_content.size());
  setHeader(QNetworkRequest::LocationHeader, url);
  setHeader(QNetworkRequest::LastModifiedHeader, QDateTime::currentDateTime());
  setOperation(QNetworkAccessManager::GetOperation);
  connect(this,
	  SIGNAL(finished(dnetworkblockreply *)),
	  this,
	  SIGNAL(finished(void)));
}

void dnetworkblockreply::load(void)
{
  QMetaObject::invokeMethod(this,
			    "finished",
			    Qt::QueuedConnection,
			    Q_ARG(dnetworkblockreply *, this));
}

void dnetworkblockreply::abort(void)
{
}

bool dnetworkblockreply::isSequential(void) const
{
  return true;
}

qint64 dnetworkblockreply::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return 0;
}

qint64 dnetworkblockreply::bytesAvailable(void) const
{
  return 0;
}

QByteArray dnetworkblockreply::html(void) const
{
  return m_content;
}

dnetworkdirreply::dnetworkdirreply
(QObject *parent, const QUrl &url):QNetworkReply(parent)
{
  setUrl(url);
  open(ReadOnly | Unbuffered);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::ContentLengthHeader, 0);
  setHeader(QNetworkRequest::LocationHeader, url);
  setHeader(QNetworkRequest::LastModifiedHeader, QDateTime::currentDateTime());
  setOperation(QNetworkAccessManager::GetOperation);
  connect(this,
	  SIGNAL(finished(dnetworkdirreply *)),
	  this,
	  SIGNAL(finished(void)));
}

void dnetworkdirreply::load(void)
{
  QMetaObject::invokeMethod(this,
			    "readyRead",
			    Qt::QueuedConnection);
  QMetaObject::invokeMethod(this,
			    "finished",
			    Qt::QueuedConnection,
			    Q_ARG(dnetworkdirreply *, this));
}

void dnetworkdirreply::abort(void)
{
}

bool dnetworkdirreply::isSequential(void) const
{
  return true;
}

qint64 dnetworkdirreply::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return 0;
}

qint64 dnetworkdirreply::bytesAvailable(void) const
{
  return 0;
}

dnetworkerrorreply::dnetworkerrorreply
(QObject *parent, const QNetworkRequest &req):QNetworkReply(parent)
{
  QNetworkRequest request(req);
  QUrl url(request.url());
  QString scheme(url.scheme().toLower().trimmed());

  request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
  setRequest(request);
  url.setScheme(scheme.mid(static_cast<int> (qstrlen("dooble-"))));
  m_content.append("<html>");
  m_content.append("<head>");
  m_content.append(QString("<title>%1</title>").
		   arg(url.toString(QUrl::StripTrailingSlash)));

  QFile file(":/error-pages.css");

  if(file.open(QIODevice::ReadOnly))
    m_content.append(file.readAll());

  file.close();
  m_content.append("</head>");
  m_content.append("<body>");

  QUrl fileUrl
    (QUrl::fromLocalFile(QString("%1/Icons/64x64/dooble.png").
			 arg(QDir::currentPath())));

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    m_content.append
      (QString("<div id=\"block\">"
	       "<table><tbody><tr><td>"
	       "<img src=\"%1\">"
	       "</td><td><h1>Offline Mode</h1>").
       arg(fileUrl.toString(QUrl::StripTrailingSlash)));
  else
    m_content.append
      (QString("<div id=\"block\">"
	       "<table><tbody><tr><td>"
	       "<img src=\"%1\">"
	       "</td><td><h1>Load Error</h1>").
       arg(fileUrl.toString(QUrl::StripTrailingSlash)));

  if(url.host().isEmpty())
    m_content.append("<h2>Dooble cannot establish a connection to "
		     "the requested site.</h2>"
		     "</td>");
  else
    m_content.append
      (QString("<h2>Dooble cannot establish a connection to "
	       "%1.</h2>"
	       "</td>").arg(url.host()));

  m_content.append("</tr></tbody></table>");

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    m_content.append
      ("<h3><ul>"
       "<li>You may disable the offline mode via the File menu.</li>"
       "</ul></h3>");
  else
    {
      m_content.append("<h3><ul>");
      m_content.append
	("<li>Malformed cookies may be present.</li>"
	 "<li>Please review your configuration settings as "
	 "Dooble may be intentionally restricting access.</li>"
	 "<li>Please verify that your firewall settings "
	 "are not preventing Dooble from accessing the "
	 "site.</li>"
	 "<li>Please verify that your proxy settings "
	 "are correct.</li>"
	 "<li>The site may be attempting to redirect to "
	 "a different URL and is being prevented from doing "
	 "so.</li>"
	 "<li>The site may be unavailable.</li>");

      if(dooble::s_settings.value("settingsWindow/diskCacheEnabled",
				  false).toBool())
	m_content.append("<li>You have the disk cache enabled. Please "
			 "try reloading the page.</li>");

      m_content.append("</ul></h3>");
    }

  m_content.append("</div>");
  m_content.append("</body></html>");
  setUrl(url);
  open(ReadOnly | Unbuffered);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::ContentLengthHeader, m_content.size());
  setHeader(QNetworkRequest::LocationHeader, url);
  setHeader(QNetworkRequest::LastModifiedHeader, QDateTime::currentDateTime());
  setOperation(QNetworkAccessManager::GetOperation);
  connect(this,
	  SIGNAL(finished(dnetworkerrorreply *)),
	  this,
	  SIGNAL(finished(void)));
}

void dnetworkerrorreply::load(void)
{
  QMetaObject::invokeMethod(this,
			    "readyRead",
			    Qt::QueuedConnection);
  QMetaObject::invokeMethod(this,
			    "finished",
			    Qt::QueuedConnection,
			    Q_ARG(dnetworkerrorreply *, this));
}

void dnetworkerrorreply::abort(void)
{
}

bool dnetworkerrorreply::isSequential(void) const
{
  return true;
}

qint64 dnetworkerrorreply::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return 0;
}

qint64 dnetworkerrorreply::bytesAvailable(void) const
{
  return 0;
}

QByteArray dnetworkerrorreply::html(void) const
{
  return m_content;
}

dnetworkftpreply::dnetworkftpreply
(QObject *parent, const QUrl &url):QNetworkReply(parent)
{
  m_ftp = new dftp(this);
  connect(m_ftp,
	  SIGNAL(finished(bool)),
	  this,
	  SLOT(slotFtpFinished(bool)));
  connect(m_ftp,
	  SIGNAL(unsupportedContent(const QUrl &)),
	  this,
	  SLOT(slotUnsupportedContent(const QUrl &)));
  m_ftp->fetchList(url);
  setUrl(url);
  open(ReadOnly | Unbuffered);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::ContentLengthHeader, 0);
  setHeader(QNetworkRequest::LocationHeader, url);
  setHeader(QNetworkRequest::LastModifiedHeader, QDateTime::currentDateTime());
  setOperation(QNetworkAccessManager::GetOperation);
  connect(this,
	  SIGNAL(finished(dnetworkftpreply *)),
	  this,
	  SIGNAL(finished(void)));
}

void dnetworkftpreply::slotFtpFinished(bool ok)
{
  if(!ok)
    if(error() == QNetworkReply::NoError)
      setError
	(QNetworkReply::ProtocolFailure, "QNetworkReply::ProtocolFailure");

  QMetaObject::invokeMethod(this,
			    "finished",
			    Qt::QueuedConnection,
			    Q_ARG(dnetworkftpreply *, this));
}

void dnetworkftpreply::slotUnsupportedContent(const QUrl &url)
{
  Q_UNUSED(url);
  setError
    (QNetworkReply::UnknownContentError, "QNetworkReply::UnknownContentError");
}

void dnetworkftpreply::load(void)
{
  QMetaObject::invokeMethod(this,
			    "readyRead",
			    Qt::QueuedConnection);
}

void dnetworkftpreply::abort(void)
{
}

bool dnetworkftpreply::isSequential(void) const
{
  return true;
}

qint64 dnetworkftpreply::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return 0;
}

qint64 dnetworkftpreply::bytesAvailable(void) const
{
  return 0;
}

QPointer<dftp> dnetworkftpreply::ftp(void) const
{
  return m_ftp;
}

dnetworksslerrorreply::dnetworksslerrorreply
(QObject *parent, const QNetworkRequest &req):QNetworkReply(parent)
{
  QNetworkRequest request(req);
  QUrl url(request.url());
  QString scheme(url.scheme().toLower().trimmed());

  request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
  setRequest(request);
  url.setScheme(scheme.mid(static_cast<int> (qstrlen("dooble-ssl-"))));
  m_content.append("<html>");
  m_content.append("<head>");
  m_content.append(QString("<title>%1</title>").
		   arg(url.toString(QUrl::StripTrailingSlash)));

  QFile file(":/error-pages.css");

  if(file.open(QIODevice::ReadOnly))
    m_content.append(file.readAll());

  file.close();
  m_content.append("</head>");
  m_content.append("<body>");

  QUrl fileUrl
    (QUrl::fromLocalFile(QString("%1/Icons/64x64/dooble.png").
			 arg(QDir::currentPath())));

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    m_content.append
      (QString("<div id=\"block\">"
	       "<table><tbody><tr><td>"
	       "<img src=\"%1\">"
	       "</td><td><h1>Offline Mode</h1>").
       arg(fileUrl.toString(QUrl::StripTrailingSlash)));
  else
    m_content.append
      (QString("<div id=\"block\">"
	       "<table><tbody><tr><td>"
	       "<img src=\"%1\">"
	       "</td><td><h1>Load Error</h1>").
       arg(fileUrl.toString(QUrl::StripTrailingSlash)));

  if(url.host().isEmpty())
    m_content.append("<h2>Dooble cannot establish a secure connection to "
		     "the requested site. Please visit the Allowed "
		     "SSL Ciphers panel and review your selections. "
		     "You may also wish to prepare exceptions.</h2>"
		     "</td>");
  else
    m_content.append
      (QString("<h2>Dooble cannot establish a secure connection to "
	       "%1. Please visit the Allowed SSL Ciphers panel "
	       "and review your selections. You may also wish "
	       "to prepare exceptions.</h2>"
	       "</td>").arg(url.host()));

  m_content.append("</tr></tbody></table>");

  QStringList errors;

  for(int i = 0; i < request.rawHeaderList().size(); i++)
    {
      QByteArray header(request.rawHeaderList().at(i));

      if(header.startsWith("SSL Error"))
	errors.append(request.rawHeader(header).constData());
    }

  if(!errors.isEmpty())
    {
      m_content.append("<h3><ul>");

      while(!errors.isEmpty())
	{
	  QString error(errors.takeFirst());

	  if(error.at(error.length() - 1).isPunct())
	    m_content.append(QString("<li>%1</li>").arg(error));
	  else
	    m_content.append(QString("<li>%1.</li>").arg(error));
	}

      m_content.append("</ul></h3>");
    }

  m_content.append("<ul><a href=\"dooble://open-ssl-errors-exceptions\">"
		   "SSL Errors Exceptions</a></ul>");
  m_content.append("</div>");
  m_content.append("</body></html>");
  setUrl(url);
  open(ReadOnly | Unbuffered);
  setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
  setHeader(QNetworkRequest::ContentLengthHeader, m_content.size());
  setHeader(QNetworkRequest::LocationHeader, url);
  setHeader(QNetworkRequest::LastModifiedHeader, QDateTime::currentDateTime());
  setOperation(QNetworkAccessManager::GetOperation);
  connect(this,
	  SIGNAL(finished(dnetworksslerrorreply *)),
	  this,
	  SIGNAL(finished(void)));
}

void dnetworksslerrorreply::load(void)
{
  QMetaObject::invokeMethod(this,
			    "readyRead",
			    Qt::QueuedConnection);
  QMetaObject::invokeMethod(this,
			    "finished",
			    Qt::QueuedConnection,
			    Q_ARG(dnetworksslerrorreply *, this));
}

void dnetworksslerrorreply::abort(void)
{
}

bool dnetworksslerrorreply::isSequential(void) const
{
  return true;
}

qint64 dnetworksslerrorreply::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return 0;
}

qint64 dnetworksslerrorreply::bytesAvailable(void) const
{
  return 0;
}

QByteArray dnetworksslerrorreply::html(void) const
{
  return m_content;
}

dnetworkaccessmanager::dnetworkaccessmanager(QObject *parent):
  QNetworkAccessManager(parent)
{
  connect(this,
	  SIGNAL(finished(QNetworkReply *)),
	  this,
	  SLOT(slotFinished(QNetworkReply *)));

  if(dooble::s_cookies)
    {
      setCookieJar(dooble::s_cookies);
      dooble::s_cookies->setParent(0);
    }

  if(dooble::s_networkCache)
    {
      setCache(dooble::s_networkCache);
      dooble::s_networkCache->setParent(0);
    }
}

QNetworkReply *dnetworkaccessmanager::createRequest
(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
  QNetworkRequest request(req);

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
			 QNetworkRequest::AlwaysCache);

  if(dooble::s_settings.value("settingsWindow/automaticallyLoadImages",
			      true).toBool())
    {
      QString path(request.url().path().toLower().trimmed());

      if(path.endsWith(".bmp") ||
	 path.endsWith(".gif") ||
	 path.endsWith(".jpeg") ||
	 path.endsWith(".jpg") ||
	 path.endsWith(".png"))
	{
	  /*
	  ** Let's not block images that are accessed by the user.
	  */

	  bool userRequested = false;
	  QWebFrame *frame = qobject_cast<QWebFrame *>
	    (request.originatingObject());

	  if(frame)
	    {
	      dwebpage *page = qobject_cast<dwebpage *> (frame->parent());

	      if(page &&
		 request.url().toString(QUrl::StripTrailingSlash) ==
		 page->mainFrame()->requestedUrl().
		 toString(QUrl::StripTrailingSlash))
		userRequested = true;
	    }

	  if(!userRequested)
	    if(dooble::s_imageBlockWindow->allowed(request.url().host()))
	      {
		emit exceptionRaised(dooble::s_adBlockWindow, request.url());
		emit loadImageRequest(request.url().host(), request.url(),
				      QDateTime::currentDateTime());

		QPointer<QNetworkReply> reply =
		  QNetworkAccessManager::createRequest
		  (op, QNetworkRequest(), outgoingData);

		setLinkClicked(QUrl());
		return reply;
	      }
	}
    }

  QString scheme(request.url().scheme().toLower().trimmed());

  if(scheme.startsWith("dooble-ssl-"))
    {
      QPointer<dnetworksslerrorreply> reply = new dnetworksslerrorreply
	(this, req);

      connect(reply,
	      SIGNAL(finished(dnetworksslerrorreply *)),
	      this,
	      SIGNAL(finished(dnetworksslerrorreply *)));
      reply->load();
      setLinkClicked(QUrl());
      return reply;
    }
  else if(scheme.startsWith("dooble-"))
    {
      QPointer<dnetworkerrorreply> reply = new dnetworkerrorreply
	(this, req);

      connect(reply,
	      SIGNAL(finished(dnetworkerrorreply *)),
	      this,
	      SIGNAL(finished(dnetworkerrorreply *)));
      reply->load();
      setLinkClicked(QUrl());
      return reply;
    }
  else if(scheme == "ftp")
    {
      QPointer<dnetworkftpreply> reply = new dnetworkftpreply
	(this, request.url());

      connect(reply,
	      SIGNAL(finished(dnetworkftpreply *)),
	      this,
	      SIGNAL(finished(dnetworkftpreply *)));
      reply->load();
      setLinkClicked(QUrl());
      return reply;
    }
  else if(scheme == "gopher")
    {
      QPointer<dgopher> reply = new dgopher(this, req);

      reply->load();
      setLinkClicked(QUrl());
      return reply;
    }
  else if(QFileInfo(request.url().toLocalFile()).isDir())
    {
      QPointer<dnetworkdirreply> reply = new dnetworkdirreply
	(this, request.url());

      connect(reply,
	      SIGNAL(finished(dnetworkdirreply *)),
	      this,
	      SIGNAL(finished(dnetworkdirreply *)));
      reply->load();
      setLinkClicked(QUrl());
      return reply;
    }

  if(scheme == "http" || scheme == "https")
    {
      if(scheme == "https")
	if(dooble::s_sslCiphersWindow)
	  {
	    QSslConfiguration configuration = request.sslConfiguration();

	    configuration.setProtocol
	      (dooble::s_sslCiphersWindow->protocol());
	    request.setSslConfiguration(configuration);
	  }

      if(op == QNetworkAccessManager::PostOperation)
	if(request.header(QNetworkRequest::ContentTypeHeader).isNull())
	  request.setHeader(QNetworkRequest::ContentTypeHeader,
			    "application/x-www-form-urlencoded");

      request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute,
			   true);

#if QT_VERSION >= 0x050300
      if(scheme == "http" || scheme == "https")
	request.setAttribute
	  (QNetworkRequest::SpdyAllowedAttribute,
	   dooble::s_settings.value("settingsWindow/speedy", false).
	   toBool());
#endif

      if(request.attribute(QNetworkRequest::User) != "dooble-favicon" &&
	 request.url() != m_linkClicked &&
	 dooble::s_settings.value("settingsWindow/blockThirdPartyContent",
				  true).toBool())
	{
	  bool block = false;

	  if(dmisc::hostblocked(request.url().host()))
	    block = true;
	  else
	    {
	      QWebFrame *frame = qobject_cast<QWebFrame *>
		(request.originatingObject());

	      if(frame && !qobject_cast<dwebpage *> (frame->parent()))
		{
		  /*
		  ** If the frame's parent is not a dwebpage, we'll
		  ** assume that the request should be blocked unless
		  ** the frame's URL is consistent with the request's
		  ** referer.
		  */

		  QString domain1(request.url().host());

		  if(!dooble::s_adBlockWindow->allowed(domain1))
		    {
		      QString domain2
			(QUrl::fromUserInput(request.rawHeader("Referer")).
			 host());

		      if(!domain2.isEmpty())
			{
			  QStringList list1
			    (domain1.split(".", QString::SkipEmptyParts));
			  QStringList list2
			    (domain2.split(".", QString::SkipEmptyParts));

			  if(list1.size() > 1)
			    domain1 = list1.value
			      (qMax(0, list1.size() - 2)) + "." +
			      list1.value(qMax(0, list1.size() - 1));

			  if(list2.size() > 1)
			    domain2 = list2.value
			      (qMax(0, list2.size() - 2)) + "." +
			      list2.value(qMax(0, list2.size() - 1));

			  if(!((domain1.contains(domain2) &&
				!domain2.isEmpty()) ||
			       (domain2.contains(domain1) &&
				!domain1.isEmpty())))
			    block = true;
			}
		    }
		}
	    }

	  if(block)
	    {
	      QUrl url(QUrl::fromUserInput(request.rawHeader("Referer")));

	      url = QUrl::fromEncoded
		(url.toEncoded(QUrl::StripTrailingSlash));
	      emit exceptionRaised(dooble::s_adBlockWindow, request.url());
	      emit blockThirdPartyHost
		(request.url().host(), url, QDateTime::currentDateTime());

	      QPointer<dnetworkblockreply> reply = new
		dnetworkblockreply(this, req);

	      connect(reply,
		      SIGNAL(finished(dnetworkblockreply *)),
		      this,
		      SIGNAL(finished(dnetworkblockreply *)));
	      reply->load();
	      setLinkClicked(QUrl());
	      return reply;
	    }
	}

      if(dooble::s_settings.value("settingsWindow/suppressHttpReferrer1",
				  false).toBool())
	if(!dooble::s_httpReferrerWindow->allowed(request.url().host()))
	  {
	    emit exceptionRaised
	      (dooble::s_httpReferrerWindow, request.url());
	    emit suppressHttpReferrer(request.url().host(), request.url(),
				      QDateTime::currentDateTime());

	    QUrl url(QUrl::fromUserInput(request.url().host()));

	    if(!url.isEmpty() && url.isValid())
	      {
		request.setRawHeader("Origin", url.toEncoded());
		request.setRawHeader("Referer", url.toEncoded());
	      }
	    else
	      {
		request.setRawHeader("Origin", 0);
		request.setRawHeader("Referer", 0);
	      }
	  }

      if(dooble::s_settings.value("settingsWindow/doNotTrack", false).toBool())
	if(!dooble::s_dntWindow->allowed(request.url().host()))
	  {
	    emit exceptionRaised(dooble::s_dntWindow, request.url());
	    emit doNotTrack(request.url().host(), request.url(),
			    QDateTime::currentDateTime());
	    request.setRawHeader("DNT", "1");
	  }
    }

  QNetworkReply *reply =
    QNetworkAccessManager::createRequest(op, request, outgoingData);

  setLinkClicked(QUrl());
  return reply;
}

void dnetworkaccessmanager::slotFinished(QNetworkReply *reply)
{
  if(!reply)
    return;

  if(qobject_cast<dnetworkblockreply *> (reply) ||
     qobject_cast<dnetworkerrorreply *> (reply) ||
     qobject_cast<dnetworkftpreply *> (reply) ||
     qobject_cast<dnetworksslerrorreply *> (reply) ||
     qobject_cast<dnetworkdirreply *> (reply))
    {
      reply->deleteLater();
      return;
    }

  /*
  ** Let favicon replies be. Please see dwebpage.cc.
  */

  if(reply->property("dooble-favicon").toBool())
    return;

  if(reply->error() == QNetworkReply::NoError &&
     dooble::s_settings.value("settingsWindow/suppressHttpRedirect1",
			      false).toBool())
    {
      QUrl url(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).
	       toUrl());

      /*
      ** The URL may be relative.
      */

      url = reply->url().resolved(url);

      if(url.isEmpty() || !url.isValid() ||
	 url.toString(QUrl::StripTrailingSlash) ==
	 reply->url().toString(QUrl::StripTrailingSlash))
	{
	  /*
	  ** What's up?
	  */
	}
      else
	{
	  /*
	  ** Alright. Here we are. We have a redirect.
	  ** We can either completely stop the current load or
	  ** continue with this redirect if it intends
	  ** to retrieve secondary data for the originating object.
	  */

	  /*
	  ** Is WebKit aware of the redirect before this method is
	  ** reached? How so? The reply includes information containing
	  ** redirection information. WebKit must act upon that information.
	  ** But when? Does it matter? I wish createRequest()'s request
	  ** contained some information that would suggest that it's a
	  ** redirect. If it does, where is it?
	  */

	  /*
	  ** You may have a URL that redirects to some other URL
	  ** just as the originating object is about to fully load.
	  ** Stopping the load then would seem strange.
	  */

	  /*
	  ** Let's keep it simple and abort the current load
	  ** if any prohibited redirect occurs.
	  */

	  if(!dooble::s_httpRedirectWindow->allowed(url.host()))
	    {
	      QWebFrame *frame = qobject_cast<QWebFrame *>
		 (reply->request().originatingObject());

	      if(frame && frame->page() &&
		 frame == frame->page()->mainFrame())
		{
		  frame->page()->triggerAction
		    (QWebPage::StopScheduledPageRefresh);
		  frame->page()->triggerAction(QWebPage::Stop);
		  emit loadErrorPage(reply->url());
		}

	      emit exceptionRaised(dooble::s_httpRedirectWindow, url);
	      emit urlRedirectionRequest(url.host(),
					 reply->url(),
					 QDateTime::currentDateTime());
	    }
	}
    }

  /*
  ** Deleting the reply object may cause Dooble to terminate if
  ** a new event loop is initiated.
  */

  if(this != reply->parent())
    reply->setParent(this);
}

void dnetworkaccessmanager::setLinkClicked(const QUrl &url)
{
  m_linkClicked = url;
}
