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
#include <QWebEngineSettings>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_page.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"

dooble_tab_bar::dooble_tab_bar(QWidget *parent):QTabBar(parent)
{
  foreach(QToolButton *toolButton, findChildren <QToolButton *> ())
    if(dooble::s_application->style_name() == "macintosh")
      toolButton->setStyleSheet
	(QString("QToolButton {background-color: %1;"
		 "border: none;"
		 "margin-bottom: 0px;"
		 "margin-top: 0px;"
		 "}"
		 "QToolButton::menu-button {border: none;}").
	 arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
    else
      toolButton->setStyleSheet
	(QString("QToolButton {background-color: %1;"
		 "border: none;"
		 "margin-bottom: 3px;"
		 "margin-top: 3px;"
		 "}"
		 "QToolButton::menu-button {border: none;}").
	 arg(QWidget::palette().color(QWidget::backgroundRole()).name()));

  setContextMenuPolicy(Qt::CustomContextMenu);
  setDocumentMode(true);
  setElideMode(Qt::ElideRight);
  setExpanding(true);
  setMovable(true);
  setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
#ifdef Q_OS_MACOS
  setStyleSheet("QTabBar::close-button {"
		"height: 16px;"
		"image: url(:/Mac/16/tab_close.png);"
		"subcontrol-origin: padding;"
		"subcontrol-position: right;"
		"width: 16px;}"
		"QTabBar::tear {"
		"border: none; image: none; width: 0px;}");
#else
  setStyleSheet("QTabBar::tear {"
		"border: none; image: none; width: 0px;}");
#endif
  setUsesScrollButtons(true);
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
}

QSize dooble_tab_bar::tabSizeHint(int index) const
{
  QFontMetrics font_metrics(font());
  QSize size(QTabBar::tabSizeHint(index));
  int preferred_tab_width = 200;
  static int preferred_tab_height = 15 + font_metrics.height();

  if(count() > 1)
    {
      if(rect().width() / count() > qMin(preferred_tab_width,
					 rect().width() / count()))
	preferred_tab_width = qMin(preferred_tab_width, rect().width());
      else
	preferred_tab_width = qMax
	  (preferred_tab_width,
	   rect().width() / count() + (index == 0 ?
				       rect().width() % count() : 0));
    }
  else
    preferred_tab_width = qMin(preferred_tab_width, rect().width());

  size.setHeight(preferred_tab_height);
  size.setWidth(preferred_tab_width);
  return size;
}

void dooble_tab_bar::hideEvent(QHideEvent *event)
{
  QTabBar::hideEvent(event);
  emit set_visible_corner_button(false);
}

void dooble_tab_bar::showEvent(QShowEvent *event)
{
  QTabBar::showEvent(event);
  emit set_visible_corner_button(true);
}

void dooble_tab_bar::slot_close_other_tabs(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      int index = tabAt(action->property("point").toPoint());

      for(int i = count() - 1; i >= 0; i--)
	if(i != index)
	  emit tabCloseRequested(i);
    }
}

void dooble_tab_bar::slot_close_tab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit tabCloseRequested(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_decouple_tab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit decouple_tab(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_open_tab_as_new_window(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit open_tab_as_new_window(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_reload(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit reload_tab(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_show_context_menu(const QPoint &point)
{
  QAction *action = 0;
  QMenu menu(this);
  int tab_at = tabAt(point);

  action = menu.addAction(tr("Close T&ab"),
			  this,
			  SLOT(slot_close_tab(void)));
  action->setEnabled(count() > 1 && tab_at > -1);
  action->setProperty("point", point);
  action = menu.addAction(tr("Close &Other Tabs"),
			  this,
			  SLOT(slot_close_other_tabs(void)));
  action->setEnabled(count() > 1 && tab_at > -1);
  action->setProperty("point", point);
  menu.addSeparator();
  action = menu.addAction(tr("Open as New &Window..."),
			  this,
			  SLOT(slot_open_tab_as_new_window(void)));
  action->setEnabled(count() > 1 && tab_at > -1);
  action->setProperty("point", point);
  menu.addAction(tr("New &Tab"),
		 this,
		 SIGNAL(new_tab(void)));
  menu.addSeparator();

  QAction *reload_action = menu.addAction(tr("&Reload"),
					  this,
					  SLOT(slot_reload(void)));
  reload_action->setProperty("point", point);
  menu.addSeparator();
  action = menu.addAction(tr("Web &Plugins"),
			  this,
			  SLOT(slot_web_plugins(void)));
  action->setEnabled(false);
  action->setProperty("point", point);

  dooble_page *page = 0;
  dooble_tab_widget *tab_widget = qobject_cast<dooble_tab_widget *>
    (parentWidget());

  if(tab_widget)
    {
      page = qobject_cast<dooble_page *> (tab_widget->widget(tabAt(point)));

      if(page)
	{
	  action->setCheckable(true);
	  action->setEnabled(tab_at > -1);

	  QWebEngineSettings *web_engine_settings = page->web_engine_settings();

	  if(web_engine_settings)
	    action->setChecked
	      (web_engine_settings->testAttribute(QWebEngineSettings::
						  PluginsEnabled));
	}
    }

  menu.addSeparator();
  action = menu.addAction(tr("&Decouple"),
			  this,
			  SLOT(slot_decouple_tab(void)));
  action->setEnabled(count() > 1 && !page && tab_at > -1);
  action->setProperty("point", point);
  reload_action->setEnabled(page && tab_at > -1);
  menu.exec(mapToGlobal(point));
}

void dooble_tab_bar::slot_web_plugins(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  dooble_tab_widget *tab_widget = qobject_cast<dooble_tab_widget *>
    (parentWidget());

  if(tab_widget)
    {
      dooble_page *page = qobject_cast<dooble_page *>
	(tab_widget->widget(tabAt(action->property("point").toPoint())));

      if(page)
	page->enable_web_setting
	  (QWebEngineSettings::PluginsEnabled, action->isChecked());
    }
}
