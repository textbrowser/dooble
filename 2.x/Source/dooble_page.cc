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
#include <QShortcut>
#include <QStackedWidget>
#include <QWebEngineHistoryItem>
#include <QWidgetAction>

#include "dooble.h"
#include "dooble_label_widget.h"
#include "dooble_page.h"
#include "dooble_web_engine_view.h"

dooble_page::dooble_page(dooble_web_engine_view *view, QWidget *parent):
  QWidget(parent)
{
  m_ui.setupUi(this);
  m_ui.backward->setEnabled(false);
  m_ui.backward->setMenu(new QMenu(this));
  m_ui.forward->setEnabled(false);
  m_ui.forward->setMenu(new QMenu(this));
  m_ui.menus->setMenu(new QMenu(this));
  m_ui.progress->setVisible(false);

  if(view)
    {
      m_view = view;
      m_view->setParent(this);
    }
  else
    m_view = new dooble_web_engine_view(this);

  m_ui.frame->layout()->addWidget(m_view);
  connect(m_ui.address,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.address,
	  SIGNAL(pull_down_clicked(void)),
	  this,
	  SLOT(slot_show_pull_down_menu(void)));
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
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_tab(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_window(dooble_web_engine_view *)));
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
  connect(m_view->page(),
	  SIGNAL(linkHovered(const QString &)),
	  this,
	  SLOT(slot_link_hovered(const QString &)));
  new QShortcut(QKeySequence(tr("Ctrl+R")), m_view, SLOT(reload(void)));
  new QShortcut(QKeySequence(tr("Esc")), this, SLOT(slot_escape(void)));
  prepare_icons();
  slot_prepare_standard_menus(); // Enables shortcuts.
}

QString dooble_page::title(void) const
{
  return m_view->title();
}

QUrl dooble_page::url(void) const
{
  return m_view->url();
}

void dooble_page::go_to_backward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::go_to_forward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::load_page(const QUrl &url)
{
  m_view->load(url);
}

void dooble_page::prepare_icons(void)
{
  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.backward->setIcon(QIcon(QString(":/%1/32/backward.png").arg(icon_set)));
  m_ui.forward->setIcon(QIcon(QString(":/%1/32/forward.png").arg(icon_set)));
  m_ui.menus->setIcon(QIcon(QString(":/%1/32/menu.png").arg(icon_set)));
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/reload.png").arg(icon_set)));
}

void dooble_page::slot_escape(void)
{
  m_ui.address->setText(m_view->url().toString());
  m_view->stop();
}

void dooble_page::slot_go_backward(void)
{
  m_view->history()->back();
}

void dooble_page::slot_go_forward(void)
{
  m_view->history()->forward();
}

void dooble_page::slot_go_to_backward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_backward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_forward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_forward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_item(void)
{
  m_ui.address->menu()->hide();

  QLabel *label = qobject_cast<QLabel *> (sender());

  if(!label)
    return;

  QList<QWebEngineHistoryItem> items(m_view->history()->items());
  int index = label->property("index").toInt();

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::slot_link_hovered(const QString &url)
{
  QFontMetrics fm(m_ui.link_hovered->fontMetrics());

  m_ui.link_hovered->setText
    (fm.elidedText(url.trimmed(),
		   Qt::ElideRight,
		   qAbs(width() - (m_ui.progress->isVisible() ?
				   m_ui.progress->width() : 0) - 15)));
}

void dooble_page::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.progress->setVisible(false);
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/reload.png").arg(icon_set)));
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
}

void dooble_page::slot_load_started(void)
{
  emit iconChanged(QIcon());
  emit titleChanged("");

  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.progress->setVisible(true);
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/stop.png").arg(icon_set)));
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
      QAction *action = 0;
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      action = m_ui.backward->menu()->addAction
	(title, this, SLOT(slot_go_to_backward_item(void)));
      action->setProperty("index", i);
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
      QAction *action = 0;
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      action = m_ui.forward->menu()->addAction
	(title, this, SLOT(slot_go_to_forward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_prepare_standard_menus(void)
{
  m_ui.menus->menu()->clear();

  QAction *action = 0;
  QMenu *menu = 0;

  /*
  ** File Menu
  */

  menu = m_ui.menus->menu()->addMenu(tr("&File"));
  menu->addAction(tr("New &Tab"),
		  this,
		  SIGNAL(new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction(tr("&New Window..."),
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

  /*
  ** Tools Menu
  */

  menu = m_ui.menus->menu()->addMenu(tr("&Tools"));
  menu->addAction(tr("&Blocked Domains..."),
		  this,
		  SIGNAL(show_blocked_domains(void)));
}

void dooble_page::slot_reload_or_stop(void)
{
  if(m_ui.progress->isVisible())
    m_view->stop();
  else
    m_view->reload();
}

void dooble_page::slot_show_pull_down_menu(void)
{
  m_ui.address->menu()->clear();

  QList<QWebEngineHistoryItem> items(m_view->history()->items());

  if(items.isEmpty())
    return;

  m_ui.address->menu()->setMaximumWidth(width());
  m_ui.address->menu()->setMinimumWidth(width());

  QFontMetrics fm(m_ui.address->menu()->fontMetrics());

  for(int i = 0; i < MAXIMUM_HISTORY_ITEMS && i < items.size(); i++)
    {
      QString title(items.at(i).title().trimmed());
      QString url(items.at(i).url().toString().trimmed());
      QWidgetAction *widget_action = new QWidgetAction(m_ui.address->menu());
      dooble_label_widget *label = 0;

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      QString text("<html>" +
		   title +
		   " - " +
		   QString("<font color='blue'><u>%1</u></font>").arg(url) +
		   "</html>");

      label = new dooble_label_widget(fm.elidedText(text,
						    Qt::ElideRight,
						    width() - 10),
				      m_ui.address->menu());
      label->setMargin(5);
      label->setMinimumHeight(fm.height() + 10);
      label->setProperty("index", i);
      connect(label,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slot_go_to_item(void)));
      widget_action->setDefaultWidget(label);
      m_ui.address->menu()->addAction(widget_action);
    }

  m_ui.address->menu()->popup(mapToGlobal(QPoint(0, m_ui.frame->pos().y())));
}

void dooble_page::slot_url_changed(const QUrl &url)
{
  m_ui.address->setText(url.toString());
}
