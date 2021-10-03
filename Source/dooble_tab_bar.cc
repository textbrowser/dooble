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
#include <QtMath>

#include <algorithm>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_charts.h"
#include "dooble_downloads.h"
#include "dooble_page.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"
#include "dooble_ui_utilities.h"

dooble_tab_bar::dooble_tab_bar(QWidget *parent):QTabBar(parent)
{
  m_corner_widget = nullptr;
  m_next_tool_button = new QToolButton(nullptr);
  m_next_tool_button->setAutoRaise(true);
  m_next_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_next_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_next_tool_button->setStyleSheet
    ("QToolButton {margin-bottom: 1px; margin-top: 1px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_previous_tool_button = new QToolButton(nullptr);
  m_previous_tool_button->setAutoRaise(true);
  m_previous_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_previous_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_previous_tool_button->setStyleSheet
    ("QToolButton {margin-bottom: 1px; margin-top: 1px;}"
     "QToolButton::menu-button {border: none;}");
#endif

  int i = 0;

  foreach(auto tool_button, findChildren<QToolButton *> ())
    if(i++ == 0)
      connect(m_previous_tool_button,
	      SIGNAL(clicked(void)),
	      tool_button,
	      SIGNAL(clicked(void)),
	      Qt::QueuedConnection);
    else
      connect(m_next_tool_button,
	      SIGNAL(clicked(void)),
	      tool_button,
	      SIGNAL(clicked(void)),
	      Qt::QueuedConnection);

  prepare_style_sheets();
  setContextMenuPolicy(Qt::CustomContextMenu);
  setDocumentMode(true);

  if(dooble::s_application->style_name() == "fusion")
    {
      auto theme_color(dooble_settings::setting("theme_color").toString());

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
  connect(m_next_tool_button,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_next_tab(void)),
	  Qt::QueuedConnection);
  connect(m_previous_tool_button,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_previous_tab(void)),
	  Qt::QueuedConnection);
  connect(this,
	  SIGNAL(application_locked(bool, dooble *)),
	  dooble::s_application,
	  SLOT(slot_application_locked(bool, dooble *)));
  connect(this,
	  SIGNAL(currentChanged(int)),
	  this,
	  SLOT(slot_next_tab(void)),
	  Qt::QueuedConnection);
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  slot_settings_applied();
}

QSize dooble_tab_bar::tabSizeHint(int index) const
{
  auto size(QTabBar::tabSizeHint(index));
  auto tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    {
      auto f = qFloor(static_cast<double> (rect().height()) /
		      static_cast<double> (qMax(1, count())));
      static int maximum_tab_height = 225;
      static int minimum_tab_height = 125;

      size.setHeight(qMin(f, maximum_tab_height));

      if(count() - 1 == index)
	{
	  int d = rect().height() - count() * size.height();

	  if(d > 0)
	    size.setHeight(qMin(d + size.height(), maximum_tab_height));
	}

      size.setHeight(qMax(minimum_tab_height, size.height()));
    }
  else
    {
      auto f = qFloor(static_cast<double> (rect().width()) /
		      static_cast<double> (qMax(1, count())));
      static int maximum_tab_width = 225;
      static int minimum_tab_width = 125;
#ifdef Q_OS_MACOS
      QFontMetrics font_metrics(font());
      static auto tab_height = 15 + font_metrics.height();
#else
      static auto tab_height = qBound
	(0,
	 dooble_settings::getenv("DOOBLE_TAB_HEIGHT_OFFSET").toInt(),
	 50);
#endif

      size.setHeight(size.height() + tab_height);
      size.setWidth(qMin(f, maximum_tab_width));

      if(count() - 1 == index)
	{
	  int d = rect().width() - count() * size.width();

	  if(d > 0)
	    size.setWidth(qMin(d + size.width(), maximum_tab_width));
	}

      size.setWidth(qMax(minimum_tab_width, size.width()));
    }

  return size;
}

bool dooble_tab_bar::is_private(void) const
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto parent = parentWidget();

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
  QList<QToolButton *> list;
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());
  int i = 0;

  if(dooble::s_application->style_name() == "macintosh")
    {
      list = findChildren<QToolButton *> ();
      std::reverse(list.begin(), list.end());
    }
  else
    list << m_next_tool_button << m_previous_tool_button;

  foreach(auto tool_button, list)
    {
      tool_button->setArrowType(Qt::NoArrow);

      if(i++ == 0)
	tool_button->setIcon
	  (QIcon::fromTheme(use_material_icons + "go-next",
			    QIcon(QString(":/%1/20/next.png").arg(icon_set))));
      else
	tool_button->setIcon
	  (QIcon::fromTheme(use_material_icons + "go-previous",
			    QIcon(QString(":/%1/20/previous.png").
				  arg(icon_set))));

      tool_button->setIconSize(QSize(18, 18));
    }
}

void dooble_tab_bar::prepare_style_sheets(void)
{
  if(dooble::s_application->style_name() == "fusion")
    {
      QList<QToolButton *> list;
      auto theme_color(dooble_settings::setting("theme_color").toString());
      static auto s_background_color
	(QWidget::palette().color(QWidget::backgroundRole()));

      list << m_next_tool_button << m_previous_tool_button;

      if(theme_color == "default")
	{
	  foreach(auto tool_button, list)
	    tool_button->setStyleSheet
	    ("QToolButton {margin-bottom: 1px; margin-top: 1px;}"
	     "QToolButton::menu-button {border: none;}");

	  setDrawBase(true);
	  setStyleSheet
	    ("QTabBar::scroller {height: 0px; margin-left: 0.09em; width: 0px;}"
	     "QTabBar::tear {border: none; image: none; width: 0px;}");
	}
      else
	{
	  foreach(auto tool_button, list)
	    tool_button->setStyleSheet
	    (QString("QToolButton {background-color: %1;"
		     "margin-bottom: 1px;"
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
		     "QTabBar::scroller {height: 0px; "
		     "margin-left: 0.09em; width: 0px;}"
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
    foreach(auto tool_button, findChildren <QToolButton *> ())
      tool_button->setStyleSheet
      (QString("QToolButton {background-color: %1;"
	       "border: none;"
	       "margin-bottom: 1px;"
	       "margin-top: 1px;"
	       "}"
	       "QToolButton::menu-button {border: none;}").
       arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
  else
    setStyleSheet
      ("QTabBar::scroller {height: 0px; margin-left: 0.09em; width: 0px;}"
       "QTabBar::tear {border: none; image: none; width: 0px;}");
}

void dooble_tab_bar::set_corner_widget(QWidget *widget)
{
  if(m_corner_widget)
    return;
  else
    m_corner_widget = widget;

  m_corner_widget->layout()->addWidget(m_previous_tool_button);
  m_corner_widget->layout()->addWidget(m_next_tool_button);
  prepare_icons();
}

void dooble_tab_bar::showEvent(QShowEvent *event)
{
  QTabBar::showEvent(event);
  emit set_visible_corner_button(true);
}

void dooble_tab_bar::slot_anonymous_tab_headers(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit anonymous_tab_headers(action->isChecked());
}

void dooble_tab_bar::slot_application_locked(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit application_locked
      (action->isChecked(), dooble_ui_utilities::find_parent_dooble(this));
}

void dooble_tab_bar::slot_close_other_tabs(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      auto index = tabAt(action->property("point").toPoint());

      for(int i = count() - 1; i >= 0; i--)
	if(i != index)
	  emit tabCloseRequested(i);

      QApplication::restoreOverrideCursor();
    }
}

void dooble_tab_bar::slot_close_tab(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit tabCloseRequested(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_decouple_tab(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit decouple_tab(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_hide_location_frame(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto tab_widget = qobject_cast<dooble_tab_widget *> (parentWidget());

  if(tab_widget)
    {
      auto page = qobject_cast<dooble_page *>
	(tab_widget->widget(tabAt(action->property("point").toPoint())));

      if(page)
	page->user_hide_location_frame(action->isChecked());
    }
}

void dooble_tab_bar::slot_javascript(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto tab_widget = qobject_cast<dooble_tab_widget *> (parentWidget());

  if(tab_widget)
    {
      auto page = qobject_cast<dooble_page *>
	(tab_widget->widget(tabAt(action->property("point").toPoint())));

      if(page)
	page->enable_web_setting
	  (QWebEngineSettings::JavascriptEnabled, action->isChecked());
    }
}

void dooble_tab_bar::slot_next_tab(void)
{
  int i = 0;

  foreach(auto tool_button, findChildren<QToolButton *> ())
    if(i++ == 0)
      m_previous_tool_button->setEnabled(tool_button->isEnabled());
    else
      m_next_tool_button->setEnabled(tool_button->isEnabled());
}

void dooble_tab_bar::slot_open_tab_as_new_private_window(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit open_tab_as_new_private_window
      (tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_open_tab_as_new_window(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit open_tab_as_new_window(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_previous_tab(void)
{
  int i = 0;

  foreach(auto tool_button, findChildren<QToolButton *> ())
    if(i++ == 0)
      m_previous_tool_button->setEnabled(tool_button->isEnabled());
    else
      m_next_tool_button->setEnabled(tool_button->isEnabled());
}

void dooble_tab_bar::slot_reload(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(action)
    emit reload_tab(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_reload_periodically(void)
{
  auto action = qobject_cast<QAction *> (sender());

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
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto tab_at = tabAt(point);
  auto use_material_icons(dooble_settings::use_material_icons());

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

  auto open_as_new_private_window_action = menu.addAction
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/48/new_private_window.png").
			    arg(icon_set))),
     tr("Open as New P&rivate Window..."),
     this,
     SLOT(slot_open_tab_as_new_private_window(void)));

  open_as_new_private_window_action->setEnabled(false);
  open_as_new_private_window_action->setProperty("point", point);

  auto open_as_new_window_action = menu.addAction
    (QIcon::fromTheme(use_material_icons + "window-new",
		      QIcon(QString(":/%1/48/new_window.png").arg(icon_set))),
     tr("Open as &New Window..."),
     this,
     SLOT(slot_open_tab_as_new_window(void)));

  open_as_new_window_action->setEnabled(false);
  open_as_new_window_action->setProperty("point", point);
  menu.addAction
    (QIcon::fromTheme(use_material_icons + "folder-new",
		      QIcon(QString(":/%1/48/new_tab.png").arg(icon_set))),
     tr("New &Tab"),
     this,
     SIGNAL(new_tab(void)));
  menu.addSeparator();

  auto reload_action = menu.addAction
    (QIcon::fromTheme(use_material_icons + "view-refresh",
		      QIcon(QString(":/%1/20/reload.png").arg(icon_set))),
     tr("&Reload"),
     this,
     SLOT(slot_reload(void)));
  reload_action->setProperty("point", point);

  auto action_group = new QActionGroup(&menu);
  auto sub_menu = menu.addMenu(tr("Reload Periodically"));

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

  auto back_action = menu.addAction
    (QIcon::fromTheme(use_material_icons + "go-previous",
		      QIcon(QString(":/%1/36/backward.png").arg(icon_set))),
     tr("&Back"));

  back_action->setProperty("point", point);

  auto forward_action = menu.addAction
    (QIcon::fromTheme(use_material_icons + "go-next",
		      QIcon(QString(":/%1/36/forward.png").arg(icon_set))),
     tr("&Forward"));

  forward_action->setProperty("point", point);
  menu.addSeparator();

  auto javascript_action = menu.addAction(tr("&JavaScript"),
					  this,
					  SLOT(slot_javascript(void)));

  javascript_action->setEnabled(false);
  javascript_action->setProperty("point", point);

  auto web_plugins_action = menu.addAction(tr("Web &Plugins"),
					   this,
					   SLOT(slot_web_plugins(void)));

  web_plugins_action->setEnabled(false);
  web_plugins_action->setProperty("point", point);

  auto webgl_action = menu.addAction(tr("Web&GL"),
				     this,
				     SLOT(slot_webgl(void)));

  webgl_action->setEnabled(false);
  webgl_action->setProperty("point", point);

  auto tab_widget = qobject_cast<dooble_tab_widget *> (parentWidget());
  dooble *d = nullptr;
  dooble_downloads *downloads = nullptr;
  dooble_page *page = nullptr;

  if(tab_widget)
    {
      d = dooble_ui_utilities::find_parent_dooble(this);
      downloads = qobject_cast<dooble_downloads *>
	(tab_widget->widget(tabAt(point)));
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

	  if(sub_menu->actions().size() >= 5)
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

	  auto web_engine_settings = page->web_engine_settings();

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
  action = menu.addAction(tr("Anonymous Tab Headers"),
			  this,
			  SLOT(slot_anonymous_tab_headers(void)));
  action->setCheckable(true);
  action->setChecked(dooble::s_application->application_locked() ? false :
		     d ? d->anonymous_tab_headers() : false);
  action->setEnabled(!dooble::s_application->application_locked());
  action = menu.addAction(tr("&Decouple..."),
			  this,
			  SLOT(slot_decouple_tab(void)));

  if(downloads)
    action->setEnabled(count() > 1 && !downloads->is_private() && tab_at > -1);
  else
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

  auto lock_action = menu.addAction(tr("Lock Application"),
				    this,
				    SLOT(slot_application_locked(void)));

  lock_action->setCheckable(true);
  lock_action->setChecked(dooble::s_application->application_locked());
  lock_action->setEnabled(dooble_settings::has_dooble_credentials());
  back_action->setEnabled(page && page->can_go_back() && tab_at > -1);
  forward_action->setEnabled(page && page->can_go_forward() && tab_at > -1);
  reload_action->setEnabled(page && tab_at > -1);
  sub_menu->setEnabled(page && tab_at > -1);

  foreach(auto action, menu.actions())
    if(action != lock_action)
      if(dooble::s_application->application_locked())
	action->setEnabled(false);

  menu.exec(mapToGlobal(point));
}

void dooble_tab_bar::slot_web_plugins(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto tab_widget = qobject_cast<dooble_tab_widget *> (parentWidget());

  if(tab_widget)
    {
      auto page = qobject_cast<dooble_page *>
	(tab_widget->widget(tabAt(action->property("point").toPoint())));

      if(page)
	page->enable_web_setting
	  (QWebEngineSettings::PluginsEnabled, action->isChecked());
    }
}

void dooble_tab_bar::slot_webgl(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  auto tab_widget = qobject_cast<dooble_tab_widget *> (parentWidget());

  if(tab_widget)
    {
      auto page = qobject_cast<dooble_page *>
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

  if(dooble::s_application->style_name() == "macintosh")
    {
      prepare_icons();
      return;
    }

  foreach(auto tool_button, findChildren<QToolButton *> ())
    {
      if(isVisible())
	emit show_corner_widget(tool_button->isVisible());

      tool_button->setArrowType(Qt::NoArrow);
      tool_button->setIcon(QIcon());
      tool_button->setStyleSheet
	("QToolButton {background-color: transparent;}");
    }
}
