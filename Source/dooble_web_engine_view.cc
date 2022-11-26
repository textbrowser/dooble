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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QDesktopWidget>
#include <QWebEngineContextMenuData>
#else
#include <QWebEngineContextMenuRequest>
#endif
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_gopher.h"
#include "dooble_search_engines_popup.h"
#include "dooble_web_engine_page.h"
#include "dooble_web_engine_view.h"

dooble_web_engine_view::dooble_web_engine_view
(QWebEngineProfile *web_engine_profile, QWidget *parent):QWebEngineView(parent)
{
  dooble::s_gopher->set_web_engine_view(this);
  m_dialog_requests_timer.setInterval(100);
  m_dialog_requests_timer.setSingleShot(true);
  m_is_private = QWebEngineProfile::defaultProfile() != web_engine_profile &&
    web_engine_profile;

  if(m_is_private)
    m_page = new dooble_web_engine_page(web_engine_profile, m_is_private, this);
  else
    m_page = new dooble_web_engine_page(this);

  connect(&m_dialog_requests_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_create_dialog_requests(void)));
  connect(m_page,
	  SIGNAL(certificate_exception_accepted(const QUrl &)),
	  this,
	  SLOT(slot_certificate_exception_accepted(const QUrl &)));
  connect(m_page,
	  SIGNAL(featurePermissionRequestCanceled(const QUrl &,
						  QWebEnginePage::Feature)),
	  this,
	  SIGNAL(featurePermissionRequestCanceled(const QUrl &,
						  QWebEnginePage::Feature)));
  connect(m_page,
	  SIGNAL(featurePermissionRequested(const QUrl &,
					    QWebEnginePage::Feature)),
	  this,
	  SIGNAL(featurePermissionRequested(const QUrl &,
					    QWebEnginePage::Feature)));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  connect(m_page,
	  SIGNAL(printRequested(void)),
	  this,
	  SIGNAL(printRequested(void)));
#endif
  connect(m_page,
	  SIGNAL(windowCloseRequested(void)),
	  this,
	  SIGNAL(windowCloseRequested(void)));
  connect(this,
	  SIGNAL(loadProgress(int)),
	  this,
	  SLOT(slot_load_progress(int)));
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));

  if(QWebEngineProfile::defaultProfile() != m_page->profile())
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    connect(m_page->profile(),
	    SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	    this,
	    SIGNAL(downloadRequested(QWebEngineDownloadItem *)));
#else
    connect(m_page->profile(),
	    SIGNAL(downloadRequested(QWebEngineDownloadRequest *)),
	    this,
	    SIGNAL(downloadRequested(QWebEngineDownloadRequest *)));
#endif

  if(!m_page->profile()->urlSchemeHandler("gopher"))
    m_page->profile()->installUrlSchemeHandler("gopher", dooble::s_gopher);

  setPage(m_page);
}

dooble_web_engine_view::~dooble_web_engine_view()
{
  for(int i = m_dialog_requests.size() - 1; i >= 0; i--)
    if(m_dialog_requests.at(i) && m_dialog_requests.at(i)->parent() == this)
      {
	m_dialog_requests.at(i)->deleteLater();
	m_dialog_requests.removeAt(i);
      }
}

QWebEngineProfile *dooble_web_engine_view::web_engine_profile(void) const
{
  return m_page->profile();
}

QSize dooble_web_engine_view::sizeHint(void) const
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  return QApplication::desktop()->screenGeometry(this).size();
#else
  auto screen = QGuiApplication::screenAt(pos());

  if(screen)
    return screen->geometry().size();
  else
    return QSize(250, 250);
#endif
}

bool dooble_web_engine_view::is_private(void) const
{
  return m_is_private;
}

dooble_web_engine_view *dooble_web_engine_view::createWindow
(QWebEnginePage::WebWindowType type)
{
  auto view = new dooble_web_engine_view(m_page->profile(), nullptr);

  view->setVisible(false);

  if(!m_page->last_clicked_link().isEmpty() &&
     m_page->last_clicked_link().isValid())
    view->setUrl(m_page->last_clicked_link());

  m_page->reset_last_clicked_link();

  if(type == QWebEnginePage::WebBrowserWindow ||
     type == QWebEnginePage::WebDialog)
    if(dooble_settings::setting("javascript").toBool() &&
       dooble_settings::setting("javascript_block_popups").toBool())
      {
	auto url(QUrl::fromUserInput(this->url().host()));

	url.setScheme(this->url().scheme());

	if(!(dooble_settings::
	     site_has_javascript_block_popup_exception(this->url()) ||
	     dooble_settings::site_has_javascript_block_popup_exception(url)))
	  {
	    m_dialog_requests << view;
	    m_dialog_requests_timer.start();
	    view->setParent(this);
	    return view;
	  }
      }

  switch(type)
    {
    case QWebEnginePage::WebBrowserWindow:
      {
	emit create_window(view);
	break;
      }
    case QWebEnginePage::WebDialog:
      {
	emit create_dialog(view);
	break;
      }
    default:
      {
	emit create_tab(view);
	break;
      }
    }

  return view;
}

void dooble_web_engine_view::contextMenuEvent(QContextMenuEvent *event)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto menu = m_page->createStandardContextMenu();
#else
  auto menu = createStandardContextMenu();
#endif

  if(!menu)
    menu = new QMenu(this);

  QAction *action = nullptr;

  /*
  ** Change some icons.
  */

  auto icon_set(dooble_settings::setting("icon_set").toString());

  if((action = m_page->action(QWebEnginePage::Back)))
    action->setIcon(QIcon(QString(":/%1/20/previous.png").arg(icon_set)));

  if((action = m_page->action(QWebEnginePage::Forward)))
    action->setIcon(QIcon(QString(":/%1/20/next.png").arg(icon_set)));

  if((action = m_page->action(QWebEnginePage::Reload)))
    action->setIcon(QIcon(QString(":/%1/20/reload.png").arg(icon_set)));

  /*
  ** Change some text.
  */

  if((action = m_page->action(QWebEnginePage::CopyImageToClipboard)))
    action->setText(tr("Copy Image"));

  if((action = m_page->action(QWebEnginePage::CopyImageUrlToClipboard)))
    action->setText(tr("Copy Image Address"));

  if((action = m_page->action(QWebEnginePage::CopyLinkToClipboard)))
    action->setText(tr("Copy Link Address"));

  if((action = m_page->action(QWebEnginePage::CopyMediaUrlToClipboard)))
    action->setText(tr("Copy Media Link"));

  if((action = m_page->action(QWebEnginePage::PasteAndMatchStyle)))
    action->setText(tr("Paste and Match Style"));

  if((action = m_page->action(QWebEnginePage::DownloadImageToDisk)))
    action->setText(tr("Save Image"));

  if((action = m_page->action(QWebEnginePage::DownloadLinkToDisk)))
    action->setText(tr("Save Link"));

  if((action = m_page->action(QWebEnginePage::SavePage)))
    action->setText(tr("Save Page"));

  if((action = m_page->action(QWebEnginePage::SelectAll)))
    action->setText(tr("Select All"));

  /*
  ** Hide some actions.
  */

  QList<QWebEnginePage::WebAction> list;

  list << QWebEnginePage::OpenLinkInNewTab
       << QWebEnginePage::OpenLinkInNewWindow;

  foreach(const auto i, list)
    if((action = m_page->action(i)))
      action->setVisible(false);

  action = m_page->action(QWebEnginePage::ViewSource);

  if(action)
    action->setText(tr("View Page Source"));

  if(!menu->actions().isEmpty() && !menu->actions().constLast()->isSeparator())
    menu->addSeparator();

  action = menu->addAction
    (tr("Open &Link"),
     this,
     SLOT(slot_open_link_in_current_page(void)));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto context_menu_data = m_page->contextMenuData();
#else
  auto context_menu_data = lastContextMenuRequest();
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#else
  if(context_menu_data)
    {
      if(context_menu_data->linkUrl().isValid())
	action->setProperty("url", context_menu_data->linkUrl());
      else if(context_menu_data->mediaUrl().isValid())
	action->setProperty("url", context_menu_data->mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#endif

  action = menu->addAction
    (tr("Open Link in a New P&rivate Window"),
     this,
     SLOT(slot_open_link_in_new_private_window(void)));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#else
  if(context_menu_data)
    {
      if(context_menu_data->linkUrl().isValid())
	action->setProperty("url", context_menu_data->linkUrl());
      else if(context_menu_data->mediaUrl().isValid())
	action->setProperty("url", context_menu_data->mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#endif

  action = menu->addAction
    (tr("Open Link in a New &Tab"),
     this,
     SLOT(slot_open_link_in_new_tab(void)));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#else
  if(context_menu_data)
    {
      if(context_menu_data->linkUrl().isValid())
	action->setProperty("url", context_menu_data->linkUrl());
      else if(context_menu_data->mediaUrl().isValid())
	action->setProperty("url", context_menu_data->mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#endif

  action = menu->addAction
    (tr("Open Link in a &New Window"),
     this,
     SLOT(slot_open_link_in_new_window(void)));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#else
  if(context_menu_data)
    {
      if(context_menu_data->linkUrl().isValid())
	action->setProperty("url", context_menu_data->linkUrl());
      else if(context_menu_data->mediaUrl().isValid())
	action->setProperty("url", context_menu_data->mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#endif

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

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(context_menu_data.isValid())
    {
      if(context_menu_data.linkUrl().isValid())
	action->setProperty("url", context_menu_data.linkUrl());
      else if(context_menu_data.mediaUrl().isValid())
	action->setProperty("url", context_menu_data.mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#else
  if(context_menu_data)
    {
      if(context_menu_data->linkUrl().isValid())
	action->setProperty("url", context_menu_data->linkUrl());
      else if(context_menu_data->mediaUrl().isValid())
	action->setProperty("url", context_menu_data->mediaUrl());
      else
	action->setEnabled(false);
    }
  else
    action->setEnabled(false);
#endif

  if(dooble::s_search_engines_window)
    {
      menu->addSeparator();

      auto actions(dooble::s_search_engines_window->actions());
      auto sub_menu = menu->addMenu("Search Selected Text");

      if(!actions.isEmpty() && !selectedText().isEmpty())
	{
	  sub_menu->setStyleSheet("QMenu {menu-scrollable: 1;}");

	  foreach(auto i, actions)
	    if(i)
	      {
		auto action = sub_menu->addAction(i->icon(),
						  i->text(),
						  this,
						  SLOT(slot_search(void)));

		action->setProperty("selected_text", selectedText());
		action->setProperty("url", i->property("url"));
	      }
	}
      else
	sub_menu->setEnabled(false);
    }

  menu->exec(mapToGlobal(event->pos()));
  menu->deleteLater();
}

void dooble_web_engine_view::download(const QString &file_name, const QUrl &url)
{
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  m_page->download(url, file_name);
#else
  Q_UNUSED(file_name);
  Q_UNUSED(url);
#endif
}

void dooble_web_engine_view::resizeEvent(QResizeEvent *event)
{
  QWebEngineView::resizeEvent(event);
  m_page->resize_certificate_error_widget();
}

void dooble_web_engine_view::save(const QString &file_name)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_page->save(file_name, QWebEngineDownloadItem::CompleteHtmlSaveFormat);
#else
  m_page->save(file_name, QWebEngineDownloadRequest::CompleteHtmlSaveFormat);
#endif
}

void dooble_web_engine_view::slot_accept_or_block_domain(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto host(action->property("url").toUrl().host());
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

void dooble_web_engine_view::set_feature_permission
(const QUrl &security_origin,
 QWebEnginePage::Feature feature,
 QWebEnginePage::PermissionPolicy policy)
{
  dooble::s_settings->set_site_feature_permission
    (security_origin,
     feature,
     policy == QWebEnginePage::PermissionGrantedByUser);
  m_page->setFeaturePermission(security_origin, feature, policy);
}

void dooble_web_engine_view::slot_certificate_exception_accepted
(const QUrl &url)
{
  if(!url.isEmpty() && url.isValid())
    load(url);
}

void dooble_web_engine_view::slot_create_dialog_requests(void)
{
  foreach(auto dialog_request, m_dialog_requests)
    emit create_dialog_request(dialog_request);

  m_dialog_requests.clear();
}

void dooble_web_engine_view::slot_load_progress(int progress)
{
  if(progress == 100)
    emit loadFinished(true);
}

void dooble_web_engine_view::slot_load_started(void)
{
  dooble::s_gopher->set_web_engine_view(this);
}

void dooble_web_engine_view::slot_open_link_in_current_page(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto url(action->property("url").toUrl());

  if(!url.isEmpty() && url.isValid())
    emit open_link_in_current_page(url);
}

void dooble_web_engine_view::slot_open_link_in_new_private_window(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto url(action->property("url").toUrl());

  if(!url.isEmpty() && url.isValid())
    emit open_link_in_new_private_window(url);
}

void dooble_web_engine_view::slot_open_link_in_new_window(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto url(action->property("url").toUrl());

  if(!url.isEmpty() && url.isValid())
    emit open_link_in_new_window(url);
}

void dooble_web_engine_view::slot_open_link_in_new_tab(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto url(action->property("url").toUrl());

  if(!url.isEmpty() && url.isValid())
    emit open_link_in_new_tab(url);
}

void dooble_web_engine_view::slot_search(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto url(action->property("url").toUrl());

  if(!url.isEmpty() && url.isValid())
    {
      auto str
	(url.query().
	 append(QString("\"%1\"").arg(action->property("selected_text").
				      toString())));

      url.setQuery(str);
      emit open_link_in_new_tab(url);
    }
}

void dooble_web_engine_view::slot_settings_applied(void)
{
}
