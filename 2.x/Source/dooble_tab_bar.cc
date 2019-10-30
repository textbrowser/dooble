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

#include <QWebEngineSettings>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_page.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"
#include "dooble_ui_utilities.h"

dooble_tab_bar::dooble_tab_bar(QWidget *parent):QTabBar(parent)
{
  prepare_style_sheets();
  setContextMenuPolicy(Qt::CustomContextMenu);
  setDocumentMode(true);

  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());

      if(theme_color == "default")
	setDrawBase(true);
      else
	setDrawBase(false);
    }

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
#endif
  setUsesScrollButtons(true);
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(this,
	  SIGNAL(application_locked(bool, dooble *)),
	  dooble::s_application,
	  SLOT(slot_application_locked(bool, dooble *)));
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  slot_settings_applied();
}

QSize dooble_tab_bar::tabSizeHint(int index) const
{
  QFontMetrics font_metrics(font());
  QSize size(QTabBar::tabSizeHint(index));
  int preferred_tab_width = 225;
  static int maximum_tab_width = preferred_tab_width;
  static int minimum_tab_width = 125;
  static int preferred_tab_height = 15 + font_metrics.height();

  if(count() > 1)
    {
      preferred_tab_width = qMax(minimum_tab_width, rect().width() / count());
      preferred_tab_width = qMin(maximum_tab_width, preferred_tab_width);

      if(index == 0)
	preferred_tab_width += rect().width() % count();
    }
  else
    preferred_tab_width = qMin(preferred_tab_width, rect().width());

  size.setHeight(preferred_tab_height);
  size.setWidth(preferred_tab_width);
  return size;
}

bool dooble_tab_bar::is_private(void) const
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QWidget *parent = parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	{
	  QApplication::restoreOverrideCursor();
	  return qobject_cast<dooble *> (parent)->is_private();
	}
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  QApplication::restoreOverrideCursor();
  return false;
}

void dooble_tab_bar::hideEvent(QHideEvent *event)
{
  QTabBar::hideEvent(event);
  emit set_visible_corner_button(false);
}

void dooble_tab_bar::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());
  int i = 0;

  foreach(QToolButton *tool_button, findChildren <QToolButton *> ())
    {
      tool_button->setArrowType(Qt::NoArrow);

      if(i++ == 0)
	tool_button->setIcon
	  (QIcon(QString(":/%1/20/previous.png").arg(icon_set)));
      else
	tool_button->setIcon
	  (QIcon(QString(":/%1/20/next.png").arg(icon_set)));

      tool_button->setIconSize(QSize(18, 18));
    }
}

void dooble_tab_bar::prepare_style_sheets(void)
{
  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());
      static QColor s_background_color
	(QWidget::palette().color(QWidget::backgroundRole()));

      if(theme_color == "default")
	{
	  foreach(QToolButton *tool_button, findChildren <QToolButton *> ())
	    tool_button->setStyleSheet
	    (QString("QToolButton {background-color: %1;"
		     "border: none;"
		     "margin-bottom: 0px;"
		     "margin-top: 0px;"
		     "}"
		     "QToolButton::menu-button {border: none;}").
	     arg(s_background_color.name()));

	  setDrawBase(true);
	  setStyleSheet("QTabBar::tear {"
			"border: none; image: none; width: 0px;}");
	}
      else
	{
	  foreach(QToolButton *tool_button, findChildren <QToolButton *> ())
	    tool_button->setStyleSheet
	    (QString("QToolButton {background-color: %1;"
		     "border: none;"
		     "margin-bottom: 0px;"
		     "margin-top: 1px;"
		     "}"
		     "QToolButton::menu-button {border: none;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()));

	  setDrawBase(false);
	  setStyleSheet
	    (QString("QTabBar::close-button {"
		     "height: 16px;"
		     "image: url(:/Mac/16/tab_close.png);"
		     "subcontrol-origin: padding;"
		     "subcontrol-position: right;"
		     "width: 16px;}"
		     "QTabBar::tab {"
		     "background-color: %1;"
		     "border-left: 1px solid %2;"
		     "border-right: 1px solid %2;"
		     "color: %3;"
		     "margin-bottom: 0px;"
		     "margin-top: 1px;}"
		     "QTabBar::tab::hover:!selected {"
		     "background-color: %4;"
		     "color: %5;}"
		     "QTabBar::tab::selected {"
		     "background-color: %6;}"
		     "QTabBar::tear {"
		     "border: none; image: none; width: 0px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()).
	     arg(s_background_color.name()).
	     arg(dooble_settings::setting("denote_private_widgets").toBool() &&
		 is_private() ? "white" : "white").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-hovered-tab-color").
		       arg(theme_color)).name()).
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-not-selected-tab-text-color").
		       arg(theme_color)).name()).
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-selected-tab-color").
		       arg(theme_color)).name()));
	}
    }
  else if(dooble::s_application->style_name() == "macintosh")
    {
      foreach(QToolButton *tool_button, findChildren <QToolButton *> ())
	tool_button->setStyleSheet
	(QString("QToolButton {background-color: %1;"
		 "border: none;"
		 "margin-bottom: 0px;"
		 "margin-top: 0px;"
		 "}"
		 "QToolButton::menu-button {border: none;}").
	 arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
    }
  else
    {
      foreach(QToolButton *tool_button, findChildren <QToolButton *> ())
	tool_button->setStyleSheet
	(QString("QToolButton {background-color: %1;"
		 "border: none;"
		 "margin-bottom: 0px;"
		 "margin-top: 0px;"
		 "}"
		 "QToolButton::menu-button {border: none;}").
	 arg(QWidget::palette().color(QWidget::backgroundRole()).name()));

      setStyleSheet("QTabBar::tear {"
		    "border: none; image: none; width: 0px;}");
    }
}

void dooble_tab_bar::showEvent(QShowEvent *event)
{
  QTabBar::showEvent(event);
  emit set_visible_corner_button(true);
}

void dooble_tab_bar::slot_application_locked(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit application_locked
      (action->isChecked(), dooble_ui_utilities::find_parent_dooble(this));
}

void dooble_tab_bar::slot_close_other_tabs(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      int index = tabAt(action->property("point").toPoint());

      for(int i = count() - 1; i >= 0; i--)
	if(i != index)
	  emit tabCloseRequested(i);

      QApplication::restoreOverrideCursor();
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

void dooble_tab_bar::slot_hide_location_frame(void)
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
	page->user_hide_location_frame(action->isChecked());
    }
}

void dooble_tab_bar::slot_javascript(void)
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
	  (QWebEngineSettings::JavascriptEnabled, action->isChecked());
    }
}

void dooble_tab_bar::slot_open_tab_as_new_private_window(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit open_tab_as_new_private_window
      (tabAt(action->property("point").toPoint()));
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

void dooble_tab_bar::slot_reload_periodically(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit reload_tab_periodically
      (tabAt(action->property("point").toPoint()),
       action->property("seconds").toInt());
}

void dooble_tab_bar::slot_settings_applied(void)
{
  prepare_icons();
  prepare_style_sheets();
}

void dooble_tab_bar::slot_show_context_menu(const QPoint &point)
{
  QAction *action = nullptr;
  QMenu menu(this);
  int tab_at = tabAt(point);

  action = menu.addAction(tr("&Close Tab"),
			  this,
			  SLOT(slot_close_tab(void)));

  if(count() == 1)
    action->setEnabled
      (dooble::s_settings->setting("allow_closing_of_single_tab").toBool() &&
       tab_at > -1);
  else
    action->setEnabled(count() > 0 && tab_at > -1);

  action->setProperty("point", point);
  action = menu.addAction(tr("Close &Other Tabs"),
			  this,
			  SLOT(slot_close_other_tabs(void)));
  action->setEnabled(count() > 1 && tab_at > -1);
  action->setProperty("point", point);
  menu.addSeparator();

  QAction *open_as_new_private_window_action = menu.addAction
    (tr("Open as New P&rivate Window..."),
     this,
     SLOT(slot_open_tab_as_new_private_window(void)));

  open_as_new_private_window_action->setEnabled(false);
  open_as_new_private_window_action->setProperty("point", point);

  QAction *open_as_new_window_action = menu.addAction
    (tr("Open as &New Window..."),
     this,
     SLOT(slot_open_tab_as_new_window(void)));

  open_as_new_window_action->setEnabled(false);
  open_as_new_window_action->setProperty("point", point);
  menu.addAction(tr("New &Tab"),
		 this,
		 SIGNAL(new_tab(void)));
  menu.addSeparator();

  QAction *reload_action = menu.addAction(tr("&Reload"),
					  this,
					  SLOT(slot_reload(void)));
  reload_action->setProperty("point", point);

  QActionGroup *action_group = new QActionGroup(&menu);
  QMenu *sub_menu = menu.addMenu(tr("Reload Periodically"));

  action = sub_menu->addAction(tr("&15 Seconds"),
			       this,
			       SLOT(slot_reload_periodically(void)));
  action->setCheckable(true);
  action->setProperty("point", point);
  action->setProperty("seconds", 15);
  action_group->addAction(action);
  action = sub_menu->addAction(tr("&30 Seconds"),
			       this,
			       SLOT(slot_reload_periodically(void)));
  action->setCheckable(true);
  action->setProperty("point", point);
  action->setProperty("seconds", 30);
  action_group->addAction(action);
  action = sub_menu->addAction(tr("&45 Seconds"),
			       this,
			       SLOT(slot_reload_periodically(void)));
  action->setCheckable(true);
  action->setProperty("point", point);
  action->setProperty("seconds", 45);
  action_group->addAction(action);
  action = sub_menu->addAction(tr("&60 Seconds"),
			       this,
			       SLOT(slot_reload_periodically(void)));
  action->setCheckable(true);
  action->setProperty("point", point);
  action->setProperty("seconds", 60);
  action_group->addAction(action);
  sub_menu->addSeparator();
  action = sub_menu->addAction(tr("&None"),
			       this,
			       SLOT(slot_reload_periodically(void)));
  action->setCheckable(true);
  action->setChecked(true);
  action->setProperty("point", point);
  action->setProperty("seconds", 0);
  action_group->addAction(action);
  menu.addSeparator();

  QAction *back_action = menu.addAction(tr("&Back"));

  back_action->setProperty("point", point);

  QAction *forward_action = menu.addAction(tr("&Forward"));

  forward_action->setProperty("point", point);
  menu.addSeparator();

  QAction *javascript_action = menu.addAction(tr("&JavaScript"),
					      this,
					      SLOT(slot_javascript(void)));

  javascript_action->setEnabled(false);
  javascript_action->setProperty("point", point);

  QAction *web_plugins_action = menu.addAction(tr("Web &Plugins"),
					       this,
					       SLOT(slot_web_plugins(void)));

  web_plugins_action->setEnabled(false);
  web_plugins_action->setProperty("point", point);

  QAction *webgl_action = menu.addAction(tr("Web&GL"),
					 this,
					 SLOT(slot_webgl(void)));

  webgl_action->setEnabled(false);
  webgl_action->setProperty("point", point);

  dooble_page *page = nullptr;
  dooble_tab_widget *tab_widget = qobject_cast<dooble_tab_widget *>
    (parentWidget());

  if(tab_widget)
    {
      page = qobject_cast<dooble_page *> (tab_widget->widget(tabAt(point)));

      if(page)
	{
	  connect(back_action,
		  SIGNAL(triggered(void)),
		  page,
		  SLOT(slot_go_backward(void)));
	  connect(forward_action,
		  SIGNAL(triggered(void)),
		  page,
		  SLOT(slot_go_forward(void)));
	  javascript_action->setCheckable(true);
	  javascript_action->setEnabled(tab_at > -1);
	  open_as_new_private_window_action->setEnabled
	    (count() > 1 && tab_at > -1);
	  open_as_new_window_action->setEnabled
	    (count() > 1 && !page->is_private() && tab_at > -1);

	  switch(page->reload_periodically_seconds())
	    {
	    case 15:
	      {
		sub_menu->actions().at(0)->setChecked(true);
		break;
	      }
	    case 30:
	      {
		sub_menu->actions().at(1)->setChecked(true);
		break;
	      }
	    case 45:
	      {
		sub_menu->actions().at(2)->setChecked(true);
		break;
	      }
	    case 60:
	      {
		sub_menu->actions().at(3)->setChecked(true);
		break;
	      }
	    default:
	      {
		sub_menu->actions().at(4)->setChecked(true);
		break;
	      }
	    }

	  web_plugins_action->setCheckable(true);
	  web_plugins_action->setEnabled(tab_at > -1);
	  webgl_action->setCheckable(true);
	  webgl_action->setEnabled(tab_at > -1);

	  QWebEngineSettings *web_engine_settings = page->web_engine_settings();

	  if(web_engine_settings)
	    {
	      javascript_action->setChecked
		(web_engine_settings->testAttribute(QWebEngineSettings::
						    JavascriptEnabled));
	      web_plugins_action->setChecked
		(web_engine_settings->testAttribute(QWebEngineSettings::
						    PluginsEnabled));
	      webgl_action->setChecked
		(web_engine_settings->testAttribute(QWebEngineSettings::
						    WebGLEnabled));
	    }
	}
    }

  menu.addSeparator();
  action = menu.addAction(tr("&Decouple..."),
			  this,
			  SLOT(slot_decouple_tab(void)));
  action->setEnabled(count() > 1 && !page && tab_at > -1);
  action->setProperty("point", point);
  menu.addSeparator();
  action = menu.addAction(tr("&Hide Location Frame"),
			  this,
			  SLOT(slot_hide_location_frame(void)));
  action->setCheckable(true);
  action->setEnabled(page && tab_at > -1);
  action->setProperty("point", point);

  if(page)
    action->setChecked(page->is_location_frame_user_hidden());

  action->setEnabled(page && tab_at > -1);
  menu.addSeparator();

  QAction *lock_action = menu.addAction(tr("Lock Application"),
					this,
					SLOT(slot_application_locked(void)));

  lock_action->setCheckable(true);
  lock_action->setChecked(dooble::s_application->application_locked());
  lock_action->setEnabled(dooble_settings::has_dooble_credentials());
  back_action->setEnabled(page && page->can_go_back() && tab_at > -1);
  forward_action->setEnabled(page && page->can_go_forward() && tab_at > -1);
  reload_action->setEnabled(page && tab_at > -1);
  sub_menu->setEnabled(page && tab_at > -1);

  foreach(QAction *action, menu.actions())
    if(action != lock_action)
      if(dooble::s_application->application_locked())
	action->setEnabled(false);

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

void dooble_tab_bar::slot_webgl(void)
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
	  (QWebEngineSettings::WebGLEnabled, action->isChecked());
    }
}

void dooble_tab_bar::tabLayoutChange(void)
{
  /*
  ** The scrolling buttons can he hidden here.
  */

  QTabBar::tabLayoutChange();
  prepare_icons();
}
