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

#include <QMenu>
#include <QStackedWidget>
#include <QWebEngineHistoryItem>

#include "dooble.h"
#include "dooble_page.h"
#include "dooble_web_engine_view.h"

dooble_page::dooble_page(QWidget *parent):QWidget(parent)
{
  m_ui.setupUi(this);
  m_ui.backward->setEnabled(false);
  m_ui.backward->setMenu(new QMenu(this));
  m_ui.forward->setEnabled(false);
  m_ui.forward->setMenu(new QMenu(this));
  m_ui.menus->setMenu(new QMenu(this));
  m_ui.progress->setVisible(false);
  m_view = new dooble_web_engine_view(this);
  m_ui.frame->layout()->addWidget(m_view);
  connect(m_ui.address,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.backward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_backward(void)));
  connect(m_ui.backward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_backward_menu(void)));
  connect(m_ui.forward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_forward(void)));
  connect(m_ui.forward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_forward_menu(void)));
  connect(m_ui.menus,
	  SIGNAL(clicked(void)),
	  m_ui.menus,
	  SLOT(showMenu(void)));
  connect(m_ui.menus->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_standard_menus(void)));
  connect(m_ui.reload,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reload_or_stop(void)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SIGNAL(iconChanged(const QIcon &)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SIGNAL(loadFinished(bool)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slot_load_finished(bool)));
  connect(m_view,
	  SIGNAL(loadProgress(int)),
	  this,
	  SLOT(slot_load_progress(int)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SIGNAL(loadStarted(void)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
  connect(m_view,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SIGNAL(titleChanged(const QString &)));
  connect(m_view,
	  SIGNAL(urlChanged(const QUrl &)),
	  this,
	  SLOT(slot_url_changed(const QUrl &)));
  prepare_icons();
  slot_prepare_standard_menus(); // Enables shortcuts.
}

void dooble_page::load_page(const QUrl &url)
{
  m_view->load(url);
}

void dooble_page::prepare_icons(void)
{
  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.backward->setIcon(QIcon(QString(":/%1/backward.png").arg(icon_set)));
  m_ui.forward->setIcon(QIcon(QString(":/%1/forward.png").arg(icon_set)));
  m_ui.menus->setIcon(QIcon(QString(":/%1/menu.png").arg(icon_set)));
  m_ui.reload->setIcon(QIcon(QString(":/%1/reload.png").arg(icon_set)));
}

void dooble_page::slot_go_backward(void)
{
  m_view->history()->back();
}

void dooble_page::slot_go_forward(void)
{
  m_view->history()->forward();
}

void dooble_page::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.progress->setVisible(false);
  m_ui.reload->setIcon(QIcon(QString(":/%1/reload.png").arg(icon_set)));
}

void dooble_page::slot_load_page(void)
{
  load_page(QUrl::fromUserInput(m_ui.address->text().trimmed()));
}

void dooble_page::slot_load_progress(int progress)
{
  m_ui.backward->setEnabled(m_view->history()->canGoBack());
  m_ui.forward->setEnabled(m_view->history()->canGoForward());
  m_ui.progress->setValue(progress);
  m_ui.progress->setVisible(progress < 100);
}

void dooble_page::slot_load_started(void)
{
  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.progress->setVisible(true);
  m_ui.reload->setIcon(QIcon(QString(":/%1/stop.png").arg(icon_set)));
}

void dooble_page::slot_open_url(void)
{
  m_ui.address->selectAll();
  m_ui.address->setFocus();
}

void dooble_page::slot_prepare_backward_menu(void)
{
  m_ui.backward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.backward->setEnabled(items.size() > 0);

  for(int i = items.size() - 1; i >= 0; i--)
    {
      QAction *action = m_ui.backward->menu()->addAction
	(items.at(i).title());

      action->setProperty("url", items.at(i).url());
    }
}

void dooble_page::slot_prepare_forward_menu(void)
{
  m_ui.forward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.forward->setEnabled(items.size() > 0);

  for(int i = 0; i < items.size(); i++)
    {
      QAction *action = m_ui.forward->menu()->addAction
	(items.at(i).title());

      action->setProperty("url", items.at(i).url());
    }
}

void dooble_page::slot_prepare_standard_menus(void)
{
  m_ui.menus->menu()->clear();

  QAction *action = 0;
  QMenu *menu = 0;

  menu = m_ui.menus->menu()->addMenu("&File");
  menu->addAction(tr("New &Tab"),
		  this,
		  SIGNAL(new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction(tr("&New Window"),
		  this,
		  SIGNAL(new_window(void)),
		  QKeySequence(tr("Ctrl+N")));
  menu->addAction(tr("&Open URL"),
		  this,
		  SLOT(slot_open_url(void)),
		  QKeySequence(tr("Ctrl+L")));
  menu->addSeparator();
  action = menu->addAction(tr("&Close Tab"),
			   this,
			   SIGNAL(close_tab(void)),
			   QKeySequence(tr("Ctrl+W")));

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    action->setEnabled
      (qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  menu->addSeparator();
  menu->addAction(tr("E&xit Dooble"),
		  this,
		  SIGNAL(quit_dooble(void)),
		  QKeySequence(tr("Ctrl+Q")));
}

void dooble_page::slot_reload_or_stop(void)
{
  if(m_ui.progress->isVisible())
    m_view->stop();
  else
    m_view->reload();
}

void dooble_page::slot_url_changed(const QUrl &url)
{
  m_ui.address->setText(url.toString());
}
