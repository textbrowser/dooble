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

#include <QBoxLayout>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QMenu>
#include <QNetworkProxy>
#include <QPushButton>
#include <QSslError>
#include <QStack>
#include <QStyle>
#include <QUrl>
#include <QWebEngineHistory>
#include <QWebEngineView>

#include "dbookmarkswindow.h"
#include "ddownloadprompt.h"
#include "dmisc.h"
#include "dnetworkaccessmanager.h"
#include "dnetworkcache.h"
#include "dooble.h"
#include "dsettings.h"
#include "durlwidget.h"
#include "dview.h"
#include "dwebpage.h"
#include "dwebview.h"

dview::dview(QWidget *parent, const QByteArray &history, dcookies *cookies,
	     const QHash<QWebEngineSettings::WebAttribute,
	     bool> &webAttributes):
  QStackedWidget(parent)
{
  m_action = 0;
  m_cookieWindow = 0;
  m_cookies = 0;
  m_hasSslError = false;
  m_history = history;
  m_lastInfoLookupId = 0;
  webView = new dwebview(this);
  webView->setPage(new dwebpage(webView));
  setWebAttributes(webAttributes);

  QDataStream in(&m_history, QIODevice::ReadOnly);

  if(in.status() == QDataStream::Ok)
    {
      in >> *webView->page()->history();

      if(in.status() != QDataStream::Ok)
	webView->page()->history()->clear();
    }

  /*
  ** Please set the text size multiplier after creating the dwebpage
  ** object.
  */

  qreal multiplier = dooble::s_settings.value
    ("settingsWindow/textSizeMultiplier").toReal();

  if(multiplier < 1.0 || multiplier > 99.99)
    multiplier = 1.0;

  webView->setZoomFactor(multiplier);
  addWidget(webView);
  setCurrentWidget(webView);
  m_pageLoaded = false;
  m_percentLoaded = 0;
  m_selectedUrl = QUrl();
  selectedImageUrl = QUrl();
  connect(webView, SIGNAL(urlChanged(const QUrl &)), this,
	  SLOT(slotUrlChanged(const QUrl &)));
  connect(webView->page(), SIGNAL(iconChanged(void)), this,
	  SLOT(slotIconChanged(void)));
  connect(webView, SIGNAL(titleChanged(const QString &)), this,
	  SLOT(slotTitleChanged(const QString &)));
  connect(webView, SIGNAL(loadStarted(void)), this,
	  SLOT(slotLoadStarted(void)));
  connect(webView, SIGNAL(loadFinished(bool)), this,
	  SLOT(slotLoadFinished(bool)));
  connect(webView, SIGNAL(loadProgress(int)), this,
	  SLOT(slotLoadProgress(int)));
  connect(webView, SIGNAL(customContextMenuRequested(const QPoint &)),
	  this, SLOT(slotCustomContextMenuRequested(const QPoint &)));
  connect(dooble::s_settingsWindow,
	  SIGNAL(textSizeMultiplierChanged(const qreal)),
	  this,
	  SLOT(slotSetTextSizeMultiplier(const qreal)));
  connect(dooble::s_settingsWindow,
	  SIGNAL(reencodeRestorationFile(void)),
	  this,
	  SLOT(slotReencodeRestorationFile(void)));
  connect(webView->page(),
	  SIGNAL(selectionChanged(void)),
	  this,
	  SLOT(slotSelectionChanged(void)));

  /*
  ** The prompt may be displayed when the load completes. Therefore,
  ** a queued connection is necessary.
  */

  connect(webView->page(),
	  SIGNAL(loadErrorPage(const QUrl &)),
	  this,
	  SLOT(slotLoadErrorPage(const QUrl &)),
	  Qt::QueuedConnection);
  connect(webView->page(),
	  SIGNAL(loadErrorPage(const QUrl &)),
	  this,
	  SLOT(slotLoadErrorPage(const QUrl &)),
	  Qt::QueuedConnection);
  connect(this,
	  SIGNAL(sslError(const QString &,
			  const QUrl &,
			  const QDateTime &)),
	  dooble::s_sslExceptionsWindow,
	  SLOT(slotAdd(const QString &,
		       const QUrl &,
		       const QDateTime &)));

  if(cookies)
    if(cookies->arePrivate())
      {
	setPrivateCookies(true);
	m_cookies->setAllCookies(cookies->allCookies());
	m_cookies->setFavorites(cookies->favorites());
      }
}

dview::~dview()
{
  if(m_action)
    m_action->deleteLater();

  removeRestorationFiles();
}

void dview::slotLoadFinished(bool ok)
{
  m_pageLoaded = true;
  webView->page()->toHtml
    ([this](const QString &html)
     {
       m_html = html;
     });

  if(webView == sender())
    webView->update();

  emit loadFinished(ok);

  if(ok)
    recordRestorationHistory();
}

void dview::slotInitialLayoutCompleted(void)
{
  setCurrentWidget(webView);
}

void dview::slotLoadStarted(void)
{
  m_ipAddress = "";
  m_pageLoaded = false;
  m_percentLoaded = 0;

  if(m_action)
    m_action->setIcon(dmisc::iconForUrl(QUrl()));

  if(webView == sender())
    webView->update();

  emit loadStarted();
}

void dview::slotLoadProgress(int progress)
{
  if(progress >= 0)
    m_percentLoaded = progress;

  emit loadProgress(progress);
}

int dview::progress(void) const
{
  return m_percentLoaded;
}

bool dview::isLoaded(void) const
{
  return m_pageLoaded;
}

void dview::slotCustomContextMenuRequested(const QPoint &pos)
{
  Q_UNUSED(pos);
}

void dview::slotViewImage(void)
{
  emit viewImage(selectedImageUrl);
}

void dview::slotOpenLinkInNewTab(void)
{
  emit openLinkInNewTab(m_selectedUrl, m_cookies, webAttributes());
}

void dview::slotOpenLinkInNewTab(const QUrl &url)
{
  emit openLinkInNewTab(url, m_cookies, webAttributes());
}

void dview::slotOpenLinkInNewWindow(void)
{
  emit openLinkInNewWindow(m_selectedUrl, m_cookies, webAttributes());
}

void dview::slotOpenLinkInNewWindow(const QUrl &url)
{
  emit openLinkInNewWindow(url, m_cookies, webAttributes());
}

void dview::slotOpenImageInNewTab(void)
{
  emit openLinkInNewTab(selectedImageUrl, m_cookies, webAttributes());
}

void dview::slotOpenImageInNewWindow(void)
{
  emit openLinkInNewWindow(selectedImageUrl, m_cookies, webAttributes());
}

void dview::slotCopyLinkLocation(void)
{
  if(m_selectedUrl.scheme().toLower().trimmed() == "mailto")
    {
      QString str("");

      str = m_selectedUrl.toString
	(QUrl::RemoveScheme |
	 QUrl::RemoveUserInfo |
	 QUrl::RemovePort |
	 QUrl::RemoveQuery |
	 QUrl::RemoveFragment |
	 QUrl::StripTrailingSlash);
      emit copyLink(QUrl(str));
    }
  else
    emit copyLink(m_selectedUrl);
}

void dview::slotCopyLinkLocation(const QUrl &url)
{
  emit copyLink(url);
}

void dview::slotCopyImageLocation(void)
{
  emit copyLink(selectedImageUrl);
}

void dview::slotSaveLink(void)
{
  emit saveUrl(m_selectedUrl, 0);
}

void dview::slotSaveLink(const QUrl &url)
{
  emit saveUrl(url, 0);
}

void dview::slotSaveImage(void)
{
  emit saveUrl(selectedImageUrl, 0);
}

void dview::slotClearCookies(void)
{
  if(m_cookies)
    m_cookies->clear();
}

void dview::slotClearHistory(void)
{
  m_history.clear();
  removeRestorationFiles();
  webView->history()->clear();
}

void dview::load(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    {
      m_pageLoaded = true;
      return;
    }

  m_hasSslError = false;
  m_html.clear();
  m_url = url;
  stop();

  QString scheme(m_url.scheme().toLower().trimmed());

  if(scheme.startsWith("dooble-ssl-"))
    {
      scheme = scheme.mid(static_cast<int> (qstrlen("dooble-ssl-")));
      m_url.setScheme(scheme);
    }
  else if(scheme.startsWith("dooble-"))
    {
      scheme = scheme.mid(static_cast<int> (qstrlen("dooble-")));
      m_url.setScheme(scheme);
    }

  if(scheme == "http" || scheme == "https")
    {
      if(dooble::s_javaScriptExceptionsWindow->allowed(m_url.host()))
	/*
	** Exempt hosts must be prevented from executing JavaScript.
	*/

	webView->settings()->setAttribute
	  (QWebEngineSettings::JavascriptEnabled, false);
      else
	webView->settings()->setAttribute
	  (QWebEngineSettings::JavascriptEnabled, isJavaScriptEnabled());

      webView->load(m_url);
    }
  else if("file:" == url.toString(QUrl::StripTrailingSlash))
    {
      /*
      ** Root directory?
      */

      QUrl url(QUrl::fromLocalFile(QDir::rootPath()));

      webView->load(url);
    }
  else
    webView->load(m_url);
}

QIcon dview::icon(void) const
{
  return dmisc::iconForUrl(url());
}

void dview::slotCopySelectedText(void)
{
  webView->triggerPageAction(QWebEnginePage::Copy);
}

dwebpage *dview::page(void) const
{
  return qobject_cast<dwebpage *> (webView->page());
}

QUrl dview::url(void) const
{
  return m_url;
}

void dview::stop(void)
{
  /*
  ** The page has been loaded, even if partially.
  */

  m_pageLoaded = true;
  webView->stop();
}

void dview::reload(void)
{
  if(!webView->url().isEmpty())
    webView->page()->triggerAction(QWebEnginePage::Reload);
  else
    load(url());
}

void dview::back(void)
{
  stop();
  webView->back();
}

void dview::forward(void)
{
  stop();
  webView->forward();
}

void dview::goToItem(const QWebEngineHistoryItem &item)
{
  stop();
  webView->page()->history()->goToItem(item);
}

QString dview::title(void) const
{
  return m_title;
}

void dview::slotUrlChanged(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  m_hasSslError = false;
  m_html.clear();
  m_url = url;

  if(dooble::s_javaScriptExceptionsWindow->allowed(m_url.host()))
    /*
    ** Exempt hosts must be prevented from executing JavaScript.
    */

    webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled,
				      false);
  else
    webView->settings()->setAttribute
      (QWebEngineSettings::JavascriptEnabled, isJavaScriptEnabled());

  QString scheme(m_url.scheme().toLower().trimmed());

  if(scheme.startsWith("dooble-ssl-"))
    {
      scheme = scheme.mid(static_cast<int> (qstrlen("dooble-ssl-")));
      m_url.setScheme(scheme);
    }
  else if(scheme.startsWith("dooble-"))
    {
      scheme = scheme.mid(static_cast<int> (qstrlen("dooble-")));
      m_url.setScheme(scheme);
    }
  else if(scheme == "http" || scheme == "https")
    setCurrentWidget(webView);

  if(m_action)
    m_action->setData(m_url);

  findIpAddress(m_url);
  recordRestorationHistory();
  emit urlChanged(m_url);
}

void dview::slotTitleChanged(const QString &title)
{
  m_title = title.trimmed();

  QString l_title(m_title);

  if(l_title.isEmpty())
    l_title = url().toString(QUrl::StripTrailingSlash);

  l_title = dmisc::elidedTitleText(l_title);

  if(l_title.isEmpty())
    l_title = tr("(Untitled)");

  if(m_action)
    m_action->setText(l_title);

  emit titleChanged(l_title);
}

void dview::slotIconChanged(void)
{
  if(m_action)
    m_action->setIcon(icon());

  emit iconChanged();
}

bool dview::canGoBack(void) const
{
  return webView->page()->history()->canGoBack();
}

bool dview::canGoForward(void) const
{
  return webView->page()->history()->canGoForward();
}

QList<QWebEngineHistoryItem> dview::backItems(const int n) const
{
  return webView->page()->history()->backItems(n);
}

QList<QWebEngineHistoryItem> dview::forwardItems(const int n) const
{
  return webView->page()->history()->forwardItems(n);
}

void dview::print(QPrinter *printer)
{
  Q_UNUSED(printer);
}

QWebEnginePage *dview::currentFrame(void)
{
  if(webView == currentWidget())
    return webView->page();

  return 0;
}

QString dview::html(void)
{
  if(webView == currentWidget())
    return m_html;

  return QString("");
}

bool dview::isDir(void) const
{
  if(QFileInfo(url().toLocalFile()).isDir())
    return true;
  else
    return false;
}

bool dview::isFtp(void) const
{
  if(url().scheme().toLower().trimmed() == "ftp")
    return true;
  else
    return false;
}

void dview::slotSslErrors(QNetworkReply *reply,
			  const QList<QSslError> &errors)
{
  if(reply)
    {
      QWebEnginePage *frame = qobject_cast<QWebEnginePage *>
	(reply->request().originatingObject());

      if(!frame || reply->url().host() != url().host())
	{
	  /*
	  ** The method slotSslErrors() may be issued many times.
	  ** We only wish to display an error of the originating page.
	  */

	  if(dooble::s_settings.value("settingsWindow/sightSslErrors",
				      true).toBool())
	    {
	      if(dooble::s_sslExceptionsWindow->allowed(reply->url().host()))
		reply->ignoreSslErrors();
	      else
		{
		  if(dooble::s_settings.value("settingsWindow/sslLevel",
					      1).toInt() == 0)
		    /*
		    ** Relaxed. Ignore SSL errors.
		    */

		    reply->ignoreSslErrors();

		  emit exceptionRaised
		    (dooble::s_sslExceptionsWindow, reply->url());
		  emit sslError(reply->url().host(),
				reply->url(),
				QDateTime::currentDateTime());

		  if(frame &&
		     dooble::s_settings.value("settingsWindow/sslLevel",
					      1).toInt() == 1) // Strict.
		    {
		      QNetworkRequest request;
		      QUrl l_url(reply->url());

		      for(int i = 0; i < errors.size(); i++)
			request.setRawHeader(QString("SSL Error %1").
					     arg(i + 1).toLatin1(),
					     errors.at(i).errorString().
					     toLatin1());

		      l_url.setScheme(QString("dooble-ssl-%1").
				      arg(reply->url().scheme()));
		      request.setUrl(l_url);
		      webView->load(l_url);
		    }
		}
	    }
	  else
	    reply->ignoreSslErrors();

	  return;
	}

      if(dooble::s_settings.value("settingsWindow/sightSslErrors",
				  true).toBool())
	{
	  if(dooble::s_sslExceptionsWindow->allowed(reply->url().host()))
	    {
	      reply->ignoreSslErrors();
	      return;
	    }
	}
      else
	{
	  reply->ignoreSslErrors();
	  return;
	}

      QNetworkRequest request;
      QUrl l_url(reply->url());

      for(int i = 0; i < errors.size(); i++)
	request.setRawHeader(QString("SSL Error %1").arg(i + 1).toLatin1(),
			     errors.at(i).errorString().toLatin1());

      l_url.setScheme(QString("dooble-ssl-%1").arg(reply->url().scheme()));
      request.setUrl(l_url);
      webView->load(l_url);
      emit exceptionRaised(dooble::s_sslExceptionsWindow, reply->url());
      emit sslError(reply->url().host(),
		    reply->url(),
		    QDateTime::currentDateTime());
    }
}

void dview::setFocus(void)
{
  if(currentWidget())
    currentWidget()->setFocus();
}

void dview::slotHandleUnsupportedContent(const QUrl &url)
{
  if(dooble::s_networkCache)
    dooble::s_networkCache->remove(url);

  setUrlForUnsupportedContent(url);

  int choice = downloadPrompt(dmisc::fileNameFromUrl(url));

  if(choice != QDialog::Rejected)
    emit saveUrl(url, choice);
}

void dview::slotFinished(dnetworkdirreply *reply)
{
  if(reply)
    {
      webView->stop(); /*
		       ** The QWebView object must be stopped. Remember,
		       ** we may have nagivated to a directory via a link.
		       */
    }
}

void dview::slotFinished(dnetworkftpreply *reply)
{
  if(reply)
    {
      if(reply->error() != QNetworkReply::NoError)
	{
	  if(reply->error() == QNetworkReply::UnknownContentError)
	    slotHandleUnsupportedContent(reply->url());
	  else
	    slotLoadErrorPage(reply->url());
	}
      else
	{
	  webView->stop(); /*
			   ** The QWebView object must be stopped. Remember,
			   ** we may have nagivated to an FTP site via a link.
			   */

	  if(dooble::s_settings.value("mainWindow/offlineMode",
				      false).toBool())
	    slotLoadErrorPage(reply->url());
	  else
	    {
	    }
	}
    }
}

void dview::slotFinished(dnetworkblockreply *reply)
{
  if(reply)
    {
    }
}

void dview::slotFinished(dnetworkerrorreply *reply)
{
  if(reply)
    {
      QWebEnginePage *frame = qobject_cast<QWebEnginePage *>
	(reply->request().originatingObject());

      if(frame)
	frame->setHtml(reply->html(), reply->url());
      else
	webView->setHtml(reply->html(), reply->url());
    }
}

void dview::slotFinished(dnetworksslerrorreply *reply)
{
  if(reply)
    {
      QWebEnginePage *frame = qobject_cast<QWebEnginePage *>
	(reply->request().originatingObject());

      if(frame)
	frame->setHtml(reply->html(), reply->url());
      else
	webView->setHtml(reply->html(), reply->url());

      m_hasSslError = true;
    }
}

void dview::slotHandleUnsupportedContent(QNetworkReply *reply)
{
  if(qobject_cast<dnetworkdirreply *> (reply) ||
     qobject_cast<dnetworkftpreply *> (reply) ||
     qobject_cast<dnetworkblockreply *> (reply) ||
     qobject_cast<dnetworkerrorreply *> (reply) ||
     qobject_cast<dnetworksslerrorreply *> (reply))
    return;

  if(reply && reply->error() == QNetworkReply::NoError)
    {
      if(dooble::s_networkCache)
	dooble::s_networkCache->remove(reply->url());

      setUrlForUnsupportedContent(reply->url());

      if(reply->rawHeader("Content-Disposition").contains("inline") ||
	 reply->rawHeader("Content-Disposition").contains("attachment"))
	{
	  int choice = 0;
	  QString fileName(reply->rawHeader("Content-Disposition"));

	  if(fileName.contains("filename="))
	    {
	      fileName = fileName.mid
		(fileName.indexOf("filename=") +
		 static_cast<int> (qstrlen("filename=")));

	      if(fileName.contains(";"))
		fileName = fileName.remove(0, fileName.indexOf(';'));
	    }
	  else
	    fileName.clear();

	  if(fileName.isEmpty())
	    fileName = dmisc::fileNameFromUrl(reply->url());

	  fileName = fileName.remove('"');
	  reply->abort();

	  QUrl url(reply->url());

	  choice = downloadPrompt(fileName);

	  if(choice != QDialog::Rejected)
	    emit saveFile(fileName, url, choice);
	}
      else
	{
	  if(reply->rawHeader("Content-Type").trimmed().
	     toLower() == "application/octet-stream" &&
	     reply->rawHeader("Content-Length").toLongLong() > 0)
	    {
	      int choice = 0;
	      QString fileName(dmisc::fileNameFromUrl(reply->url()));

	      reply->abort();

	      QUrl url(reply->url());

	      choice = downloadPrompt(fileName);

	      if(choice != QDialog::Rejected)
		emit saveUrl(url, choice);
	    }
	  else if(reply->rawHeader("Content-Type").trimmed().contains("text"))
	    {
	      int choice = 0;
	      QString fileName(dmisc::fileNameFromUrl(reply->url()));

	      reply->abort();

	      QUrl url(reply->url());

	      choice = downloadPrompt(fileName);

	      if(choice != QDialog::Rejected)
		emit saveUrl(url, choice);
	    }
	  else if(reply->rawHeader("Content-Type").trimmed().
		  contains("application"))
	    {
	      int choice = 0;
	      QString fileName(dmisc::fileNameFromUrl(reply->url()));

	      reply->abort();

	      QUrl url(reply->url());

	      choice = downloadPrompt(fileName);

	      if(choice != QDialog::Rejected)
		emit saveUrl(url, choice);
	    }
	  else if(reply->rawHeader("Content-Length").trimmed().toInt() > 0)
	    {
	      int choice = 0;
	      QString fileName(dmisc::fileNameFromUrl(reply->url()));

	      reply->abort();

	      QUrl url(reply->url());

	      choice = downloadPrompt(fileName);

	      if(choice != QDialog::Rejected)
		emit saveUrl(url, choice);
	    }
	  else
	    reply->abort();
	}
    }
  else if(reply)
    reply->abort();
}

void dview::enterEvent(QEvent *event)
{
  QStackedWidget::enterEvent(event);
}

void dview::slotSelectionChanged(void)
{
  emit selectionChanged(webView->page()->selectedText());
}

void dview::slotPaste(void)
{
  webView->triggerPageAction(QWebEnginePage::Paste);
}

void dview::setTabAction(QAction *action)
{
  if(m_action)
    {
      removeAction(m_action);
      m_action->deleteLater();
    }

  m_action = action;

  if(m_action)
    addAction(m_action);
}

QAction *dview::tabAction(void) const
{
  return m_action;
}

void dview::slotLinkClicked(const QUrl &url)
{
  /*
  ** Because of the current link delegation policy, this method should
  ** never be reached.
  */

  load(url);
}

qreal dview::zoomFactor(void) const
{
  return webView->zoomFactor();
}

void dview::setZoomFactor(const qreal factor)
{
  webView->setZoomFactor(factor);
  webView->update();
}

void dview::zoomTextOnly(const bool state)
{
  Q_UNUSED(state);
  webView->update();
}

void dview::slotStop(void)
{
  stop();
}

void dview::slotViewPageSource(void)
{
  emit viewPageSource();
}

void dview::slotSetTextSizeMultiplier(const qreal multiplier)
{
  webView->setZoomFactor(multiplier);
}

bool dview::hasSecureConnection(void) const
{
  /*
  ** Simple, for now.
  */

  return !m_hasSslError &&
    url().scheme().toLower().trimmed() == "https";
}

void dview::findIpAddress(const QUrl &url)
{
  QHostInfo::abortHostLookup(m_lastInfoLookupId);

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    return;

  if(dooble::s_settings.value("settingsWindow/displayIpAddress",
			      false).toBool())
    {
      if(url.isValid())
	{
	  if(url.host().isEmpty())
	    m_lastInfoLookupId = QHostInfo::lookupHost
	      ("localhost", this,
	       SLOT(slotHostLookedUp(const QHostInfo &)));
	  else
	    m_lastInfoLookupId = QHostInfo::lookupHost
	      (url.host(), this,
	       SLOT(slotHostLookedUp(const QHostInfo &)));
	}
      else
	{
	  m_ipAddress = "";
	  emit ipAddressChanged(m_ipAddress);
	}
    }
  else
    {
      m_ipAddress = "";
      emit ipAddressChanged(m_ipAddress);
    }
}

void dview::slotHostLookedUp(const QHostInfo &hostInfo)
{
  if(dooble::s_settings.value("settingsWindow/displayIpAddress", false).
     toBool())
    foreach(const QHostAddress &address, hostInfo.addresses())
      if(!address.isNull())
	{
	  m_ipAddress = address.toString();
	  emit ipAddressChanged(m_ipAddress);
	  break;
	}
}

QString dview::ipAddress(void) const
{
  return m_ipAddress;
}

bool dview::isModified(void) const
{
  if(webView == currentWidget())
    return false;
  else
    return false;
}

void dview::post(const QUrl &url, const QString &text)
{
  if(url.isEmpty() || !url.isValid())
    {
      m_pageLoaded = true;
      return;
    }

  m_hasSslError = false;
  m_html.clear();
  m_url = url;
  stop();
  setCurrentWidget(webView);

  if(dooble::s_javaScriptExceptionsWindow->allowed(url.host()))
    /*
    ** Exempt hosts must be prevented from executing JavaScript.
    */

    webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled,
				      false);
  else
    webView->settings()->setAttribute
      (QWebEngineSettings::JavascriptEnabled, isJavaScriptEnabled());

  /*
  ** We don't need to set HTTP Pipelining here since
  ** dnetworkaccessmanager::createRequest() will set it.
  */

  QNetworkRequest request;

  request.setUrl(m_url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
		    "text/html; charset=UTF-8");
  request.setHeader(QNetworkRequest::ContentLengthHeader,
		    text.toUtf8().length());
  webView->load(m_url);
}

void dview::update(void)
{
  QStackedWidget::update();
}

QByteArray dview::history(void)
{
  QDataStream out(&m_history, QIODevice::WriteOnly);

  out << *webView->history();

  if(out.status() != QDataStream::Ok)
    m_history.clear();

  return m_history;
}

bool dview::isBookmarked(void) const
{
  if(dooble::s_bookmarksWindow &&
     dooble::s_bookmarksWindow->isBookmarked(url()))
    return true;
  else
    return false;
}

QString dview::description(void) const
{
  QString str("");

  if(webView == currentWidget())
    {
      if(webView->page())
	str = webviewUrl().toString(QUrl::StripTrailingSlash);
    }

  if(str.isEmpty())
    str = webviewUrl().toString(QUrl::StripTrailingSlash);

  return str;
}

void dview::slotLoadPage(const QUrl &url)
{
  load(url);
}

void dview::slotPrintCurrentFrame(void)
{
  emit printRequested(currentFrame());
}

void dview::recordRestorationHistory(void)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(!dmisc::isSchemeAcceptedByDooble(url().scheme()))
    return;
  else if(!dooble::s_settings.value("settingsWindow/sessionRestoration",
				    true).toBool())
    return;
  else if(webView->history()->count() == 0)
    return;

  prepareRestorationFileNames();

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);

  out << *webView->history();

  if(out.status() == QDataStream::Ok)
    {
      bool ok = true;

      bytes = dmisc::etm(bytes, true, &ok);

      if(ok)
	{
	  QDir().mkdir(dooble::s_homePath + QDir::separator() + "Histories");

	  QFile file(m_historyRestorationFileName);

	  if(file.open(QIODevice::WriteOnly))
	    file.write(bytes);

	  file.close();
	}
    }
}

int dview::tabIndex(void)
{
  int index = -1;
  QObject *prnt(this);
  dtabwidget *tab = 0;

  do
    {
      prnt = prnt->parent();
      tab = qobject_cast<dtabwidget *> (prnt);
    }
  while(prnt != 0 && tab == 0);

  if(tab)
    index = tab->indexOf(this);

  return index;
}

quint64 dview::parentId(void)
{
  quint64 id = 0;
  dooble *dbl = 0;
  QObject *prnt(this);

  do
    {
      prnt = prnt->parent();
      dbl = qobject_cast<dooble *> (prnt);
    }
  while(prnt != 0 && dbl == 0);

  if(dbl)
    id = dbl->id();

  return id;
}

void dview::prepareRestorationFileNames(void)
{
  /*
  ** QUuid.toString() +
  ** 00000000000000000000 + Parent ID +
  ** 0000000000 + Tab Index
  */

  m_historyRestorationFileName =
    dooble::s_homePath + QDir::separator() + "Histories" + QDir::separator() +
    dooble::s_id.toString().remove("{").remove("}").remove("-") +
    QString::number(parentId()).rightJustified(20, '0') +
    QString::number(tabIndex()).rightJustified(10, '0');
}

void dview::removeRestorationFiles(void)
{
  if(dmisc::passphraseWasAuthenticated())
    if(!m_historyRestorationFileName.isEmpty())
      {
	QFile::remove(m_historyRestorationFileName);
	m_historyRestorationFileName.clear();
      }
}

int dview::downloadPrompt(const QString &fileName)
{
  if(!findChildren<ddownloadprompt *> ().isEmpty())
    return QDialog::Rejected;

  return ddownloadprompt(this, fileName, ddownloadprompt::MultipleChoice).
    exec();
}

QString dview::statusMessage(void) const
{
  return QString("");
}

void dview::slotLoadErrorPage(const QUrl &url)
{
  QUrl l_url(url);

  if(l_url.scheme().toLower().trimmed() == "https")
    l_url.setScheme(QString("dooble-ssl-%1").arg(url.scheme()));
  else
    l_url.setScheme(QString("dooble-%1").arg(url.scheme()));

  webView->load(l_url);
}

QUrl dview::webviewUrl(void) const
{
  return webView->url();
}

QIcon dview::webviewIcon(void) const
{
  return dmisc::iconForUrl(webviewUrl());
}

bool dview::isBookmarkWorthy(void) const
{
  return url().toString(QUrl::StripTrailingSlash) ==
    webviewUrl().toString(QUrl::StripTrailingSlash);
}

void dview::slotReencodeRestorationFile(void)
{
  recordRestorationHistory();
}

void dview::slotDownloadRequested(const QNetworkRequest &request)
{
  int choice = downloadPrompt(dmisc::fileNameFromUrl(request.url()));

  if(choice != QDialog::Rejected)
    emit saveUrl(request.url(), choice);
}

bool dview::arePrivateCookiesEnabled(void) const
{
  return m_cookies != 0;
}

void dview::setPrivateCookies(const bool state)
{
  Q_UNUSED(state);
}

void dview::viewPrivateCookies(void)
{
  if(m_cookieWindow)
    m_cookieWindow->show(findDooble());
}

bool dview::areWebPluginsEnabled(void) const
{
  return page()->areWebPluginsEnabled();
}

bool dview::isJavaScriptEnabled(void) const
{
  return page()->isJavaScriptEnabled();
}

bool dview::isPrivateBrowsingEnabled(void) const
{
  return page()->isPrivateBrowsingEnabled();
}

void dview::setJavaScriptEnabled(const bool state)
{
  page()->setJavaScriptEnabled(state);
}

void dview::setPrivateBrowsingEnabled(const bool state)
{
  Q_UNUSED(state);
}

void dview::setWebPluginsEnabled(const bool state)
{
  Q_UNUSED(state);
}

QHash<QWebEngineSettings::WebAttribute, bool> dview::webAttributes(void) const
{
  return page()->webAttributes();
}

void dview::setWebAttributes
(const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes)
{
  QHashIterator<QWebEngineSettings::WebAttribute, bool> it(webAttributes);

  while(it.hasNext())
    {
      it.next();

      if(it.key() == QWebEngineSettings::JavascriptEnabled)
	page()->setJavaScriptEnabled(it.value());

      page()->settings()->setAttribute(it.key(), it.value());
    }
}

void dview::setUrlForUnsupportedContent(const QUrl &url)
{
  if(this->url().isEmpty() || !this->url().isValid() ||
     this->url() == QUrl::fromUserInput("about:blank"))
    {
      if(!url.isEmpty() && url.isValid())
	{
	  m_title = url.toString(QUrl::StripTrailingSlash);
	  m_url = url;
	  emit titleChanged(m_title);
	  emit urlChanged(m_url);
	}
    }
}

dooble *dview::findDooble(void)
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
