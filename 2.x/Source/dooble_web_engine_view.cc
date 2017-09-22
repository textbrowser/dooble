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

#include <QContextMenuEvent>
#include <QWebEngineContextMenuData>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_gopher.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_page.h"
#include "dooble_web_engine_url_request_interceptor.h"
#include "dooble_web_engine_view.h"

dooble_web_engine_view::dooble_web_engine_view(bool is_private,
					       QWidget *parent):
  QWebEngineView(parent)
{
  m_cookies = 0;
  m_is_private = is_private;

  if(m_is_private)
    m_page = new dooble_web_engine_page
      (new QWebEngineProfile(this), m_is_private, this);
  else
    m_page = new dooble_web_engine_page(this);

  connect(m_page,
	  SIGNAL(certificate_exception_accepted(const QUrl &)),
	  this,
	  SLOT(slot_certificate_exception_accepted(const QUrl &)));

  if(QWebEngineProfile::defaultProfile() != m_page->profile())
    connect(m_page->profile(),
	    SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	    this,
	    SIGNAL(downloadRequested(QWebEngineDownloadItem *)));

  if(!m_page->profile()->urlSchemeHandler("gopher"))
    m_page->profile()->installUrlSchemeHandler
      ("gopher", new dooble_gopher(this));

  if(m_is_private)
    {
      m_cookies = new dooble_cookies(m_is_private, this);
      m_cookies_window = new dooble_cookies_window(m_is_private, this);
      m_cookies_window->setCookieStore(m_page->profile()->cookieStore());
      m_cookies_window->setCookies(m_cookies);
      m_page->profile()->setRequestInterceptor
	(dooble::s_url_request_interceptor);
      m_page->profile()->setSpellCheckEnabled(true);
      m_page->profile()->setSpellCheckLanguages({"en_US"});
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::FullScreenSupportEnabled, true);
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::JavascriptCanAccessClipboard,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard));
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::JavascriptCanOpenWindows,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptCanOpenWindows));
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::JavascriptEnabled,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::JavascriptEnabled));
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::LocalStorageEnabled, false);
      m_page->profile()->settings()->setAttribute
	(QWebEngineSettings::PluginsEnabled,
	 QWebEngineSettings::defaultSettings()->
	 testAttribute(QWebEngineSettings::PluginsEnabled));
      connect(dooble::s_settings,
	      SIGNAL(applied(void)),
	      this,
	      SLOT(slot_settings_applied(void)));
      connect(m_cookies,
	      SIGNAL(cookie_added(const QNetworkCookie &, bool)),
	      m_cookies_window,
	      SLOT(slot_cookie_added(const QNetworkCookie &, bool)));
      connect(m_cookies,
	      SIGNAL(cookie_removed(const QNetworkCookie &)),
	      m_cookies_window,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
      connect(m_page->profile()->cookieStore(),
	      SIGNAL(cookieAdded(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_added(const QNetworkCookie &)));
      connect(m_page->profile()->cookieStore(),
	      SIGNAL(cookieRemoved(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
      copy_default_profile_settings();
    }

  setPage(m_page);
}

bool dooble_web_engine_view::is_private(void) const
{
  return m_is_private;
}

dooble_web_engine_view *dooble_web_engine_view::createWindow
(QWebEnginePage::WebWindowType type)
{
  if(dooble_settings::setting("javascript_block_popups").toBool())
    if(type == QWebEnginePage::WebDialog)
      return 0;

  dooble_web_engine_view *view = new dooble_web_engine_view(m_is_private, 0);

  switch(type)
    {
    case QWebEnginePage::WebBrowserWindow:
    case QWebEnginePage::WebDialog:
      {
	emit create_window(view);
	break;
      }
    default:
      emit create_tab(view);
    }

  return view;
}

void dooble_web_engine_view::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = m_page->createStandardContextMenu();

  if(!menu)
    return;

  QAction *action = 0;
  QWebEngineContextMenuData context_menu_data = m_page->contextMenuData();

  if(!menu->actions().isEmpty() && !menu->actions().last()->isSeparator())
    menu->addSeparator();

  if(dooble_settings::
     setting("accepted_or_blocked_domains_mode").toString() == "accept")
    action = menu->addAction(tr("Accept Link's Domain(s)"),
			     this,
			     SLOT(slot_accept_or_block_domain(void)));
  else
    action = menu->addAction(tr("Block Link's Domain(s)"),
			     this,
			     SLOT(slot_accept_or_block_domain(void)));

  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("link_url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("link_url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);

  menu->exec(mapToGlobal(event->pos()));
  menu->deleteLater();
}

void dooble_web_engine_view::copy_default_profile_settings(void)
{
  if(!m_is_private)
    return;

  /*
  ** PluginsEnabled is not copied.
  */

  m_page->profile()->setHttpCacheMaximumSize
    (QWebEngineProfile::defaultProfile()->httpCacheMaximumSize());
  m_page->profile()->setHttpCacheType
    (QWebEngineProfile::defaultProfile()->httpCacheType());
  m_page->profile()->setHttpUserAgent
    (QWebEngineProfile::defaultProfile()->httpUserAgent());
  m_page->profile()->settings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     QWebEngineSettings::defaultSettings()->
     testAttribute(QWebEngineSettings::ScrollAnimatorEnabled));
  m_page->profile()->settings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled,
     QWebEngineSettings::defaultSettings()->
     testAttribute(QWebEngineSettings::XSSAuditingEnabled));
}

void dooble_web_engine_view::slot_accept_or_block_domain(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QString host(action->property("link_url").toUrl().host());
  int index = -1;

  while(!host.isEmpty())
    {
      dooble::s_accepted_or_blocked_domains->accept_or_block_domain(host);

      if((index = host.indexOf('.')) > 0)
	host.remove(0, index + 1);

      if(host.indexOf('.') < 0)
	break;
    }
}

void dooble_web_engine_view::slot_certificate_exception_accepted
(const QUrl &url)
{
  if(!url.isEmpty() && url.isValid())
    load(url);
}

void dooble_web_engine_view::slot_settings_applied(void)
{
  copy_default_profile_settings();
}

void dooble_web_engine_view::show_private_cookies(void)
{
  if(!m_cookies_window)
    return;

  m_cookies_window->filter(url().host());

  if(m_cookies_window->isVisible())
    {
      m_cookies_window->activateWindow();
      m_cookies_window->raise();
      return;
    }

  m_cookies_window->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, m_cookies_window);

  m_cookies_window->activateWindow();
  m_cookies_window->raise();
}
