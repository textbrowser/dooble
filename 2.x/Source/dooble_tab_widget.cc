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

#include <QLabel>
#include <QMovie>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_page.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"

dooble_tab_widget::dooble_tab_widget(QWidget *parent):QTabWidget(parent)
{
  m_add_tab_tool_button = new QToolButton(this);
  m_add_tab_tool_button->setAutoRaise(true);
  m_add_tab_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_add_tab_tool_button->setToolTip(tr("New Tab"));
  m_private_tool_button = new QToolButton(this);
  m_private_tool_button->setAutoRaise(true);
  m_private_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_private_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_private_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_private_tool_button->setToolTip(tr("This is a private window."));
  m_private_tool_button->setVisible
    (dooble_settings::setting("denote_private_widgets").toBool() &&
     is_private());
  m_tabs_menu_button = new QToolButton(this);
  m_tabs_menu_button->setArrowType(Qt::NoArrow);
  m_tabs_menu_button->setAutoRaise(true);
  m_tabs_menu_button->setCheckable(true);
  m_tabs_menu_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_tabs_menu_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_tabs_menu_button->setStyleSheet
    ("QToolButton {margin-bottom: 3px; margin-top: 3px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_corner_widget = new QFrame(this);
  m_corner_widget->setLayout(new QHBoxLayout(this));
  m_corner_widget->layout()->setContentsMargins(5, 3, 5, 3);
  m_corner_widget->layout()->addWidget(m_private_tool_button);
  m_corner_widget->layout()->addWidget(m_tabs_menu_button);
  m_corner_widget->layout()->addWidget(m_add_tab_tool_button);
  m_corner_widget->layout()->setSpacing(0);
  m_corner_widget->setVisible
    (!dooble_settings::setting("auto_hide_tab_bar").toBool());

  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());

      if(theme_color == "default")
	{
	  m_corner_widget->setStyleSheet("");
	  setStyleSheet("");
	}
      else
	{
	  m_corner_widget->setStyleSheet
	    (QString("QFrame {background-color: %1;"
		     "border-right: 0px solid %2;"
		     "margin-bottom: 0px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-corner-widget-background-color").
		       arg(theme_color)).name()).
	     arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
	  setStyleSheet
	    (QString("QTabBar {background-color: %1; margin-top: 1px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()));
	}
    }

  m_tab_bar = new dooble_tab_bar(this);
  m_tab_bar->setAutoHide
    (dooble_settings::setting("auto_hide_tab_bar").toBool());
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_add_tab_tool_button,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(new_tab(void)));
  connect(m_tab_bar,
	  SIGNAL(decouple_tab(int)),
	  this,
	  SIGNAL(decouple_tab(int)));
  connect(m_tab_bar,
	  SIGNAL(new_tab(void)),
	  this,
	  SIGNAL(new_tab(void)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_window(int)));
  connect(m_tab_bar,
	  SIGNAL(reload_tab(int)),
	  this,
	  SIGNAL(reload_tab(int)));
  connect(m_tab_bar,
	  SIGNAL(set_visible_corner_button(bool)),
	  this,
	  SLOT(slot_set_visible_corner_button(bool)));
  connect(m_tabs_menu_button,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(tabs_menu_button_clicked(void)));
  prepare_icons();

  if(!dooble_settings::setting("auto_hide_tab_bar").toBool())
    setCornerWidget(m_corner_widget, Qt::TopLeftCorner);

  setTabBar(m_tab_bar);
}

QIcon dooble_tab_widget::tabIcon(int index) const
{
  QIcon icon(QTabWidget::tabIcon(index));

  if(icon.isNull())
    {
      QLabel *label = qobject_cast<QLabel *>
	(m_tab_bar->tabButton(index, QTabBar::LeftSide));

      if(!label)
	label = qobject_cast<QLabel *>
	  (m_tab_bar->tabButton(index, QTabBar::RightSide));

      if(label)
	icon = *label->pixmap();
    }

  return icon;
}

QToolButton *dooble_tab_widget::tabs_menu_button(void) const
{
  return m_tabs_menu_button;
}

bool dooble_tab_widget::is_private(void) const
{
  QWidget *parent = parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	return qobject_cast<dooble *> (parent)->is_private();
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  return false;
}

dooble_page *dooble_tab_widget::page(int index) const
{
  return qobject_cast<dooble_page *> (widget(index));
}

void dooble_tab_widget::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_add_tab_tool_button->setIcon
    (QIcon(QString(":/%1/36/add.png").arg(icon_set)));
  m_private_tool_button->setIcon
    (QIcon(QString(":/%1/18/private.png").arg(icon_set)));
  m_tabs_menu_button->setIcon
    (QIcon(QString(":/%1/18/pulldown.png").arg(icon_set)));
}

void dooble_tab_widget::prepare_tab_label(int index, const QIcon &icon)
{
  if(index < 0 || index >= count())
    return;

  if(dooble::s_application->style_name() == "fusion" ||
     dooble::s_application->style_name() == "windows")
    {
      QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->
	styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);

      side = (side == QTabBar::LeftSide) ?
	QTabBar::RightSide : QTabBar::LeftSide;

      QLabel *label = qobject_cast<QLabel *>
	(m_tab_bar->tabButton(index, side));

      if(!label)
	{
	  label = new QLabel(this);
	  label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	  label->setFixedSize(QSize(16, 16));
	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	  m_tab_bar->setTabButton(index, side, 0);
	  m_tab_bar->setTabButton(index, side, label);
	}
      else if(!label->movie() || label->movie()->state() != QMovie::Running)
	label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

      label->setProperty("icon", icon);
    }
  else
    {
#ifdef Q_OS_MACOS
      QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->
	styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);

      side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide :
	QTabBar::RightSide;

      QLabel *label = qobject_cast<QLabel *>
	(m_tab_bar->tabButton(index, side));

      if(!label)
	{
	  label = new QLabel(this);
	  label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	  m_tab_bar->setTabButton(index, side, 0);
	  m_tab_bar->setTabButton(index, side, label);
	}
      else if(!label->movie() || label->movie()->state() != QMovie::Running)
	label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

      label->setProperty("icon", icon);
#endif
    }
}

void dooble_tab_widget::setTabIcon(int index, const QIcon &icon)
{
  prepare_tab_label(index, icon);
}

void dooble_tab_widget::setTabTextColor(int index, const QColor &color)
{
  m_tab_bar->setTabTextColor(index, color);
}

void dooble_tab_widget::slot_load_finished(void)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);
  int index = indexOf(page);

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  QLabel *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(label)
    {
      QMovie *movie = label->movie();

      if(movie)
	{
	  movie->stop();
	  movie->deleteLater();
	}

      label->setMovie(0);

      if(dooble::s_application->style_name() == "fusion")
	{
	  QIcon icon(label->property("icon").value<QIcon> ());

	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	}

      m_tab_bar->setVisible(false);
      m_tab_bar->setVisible(true);
    }
}

void dooble_tab_widget::slot_load_started(void)
{
  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);
  int index = indexOf(qobject_cast<QWidget *> (sender()));

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  QLabel *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      label = new QLabel(this);
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      m_tab_bar->setTabButton(index, side, 0);
      m_tab_bar->setTabButton(index, side, label);
    }

  QMovie *movie = label->movie();

  if(!movie)
    {
      movie = new QMovie
	(":/Miscellaneous/spinning_wheel.gif", QByteArray(), label);
      label->setMovie(movie);
      movie->setScaledSize(QSize(16, 16));
      movie->start();
    }
  else
    {
      if(movie->state() != QMovie::Running)
	movie->start();
    }
}

void dooble_tab_widget::slot_set_visible_corner_button(bool state)
{
  if(state)
    setCornerWidget(m_corner_widget, Qt::TopLeftCorner);
  else
    setCornerWidget(0, Qt::TopLeftCorner);

  m_corner_widget->setVisible(state);
}

void dooble_tab_widget::slot_settings_applied(void)
{
  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());

      if(theme_color == "default")
	{
	  m_corner_widget->setStyleSheet("");
	  setStyleSheet("");
	}
      else
	{
	  m_corner_widget->setStyleSheet
	    (QString("QFrame {background-color: %1;"
		     "border-right: 0px solid %2;"
		     "margin-bottom: 0px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-corner-widget-background-color").
		       arg(theme_color)).name()).
	     arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
	  setStyleSheet
	    (QString("QTabBar {background-color: %1; margin-top: 1px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()));
	}
    }

  m_private_tool_button->setVisible
    (dooble_settings::setting("denote_private_widgets").toBool() &&
     is_private());
  prepare_icons();

  if(dooble_settings::setting("auto_hide_tab_bar").toBool())
    {
      if(count() > 1)
	{
	  setCornerWidget(m_corner_widget, Qt::TopLeftCorner);
	  m_corner_widget->setVisible(true);
	}
      else
	{
	  setCornerWidget(0, Qt::TopLeftCorner);
	  m_corner_widget->setVisible(false);
	}

      m_tab_bar->setAutoHide(true);
    }
  else
    {
      setCornerWidget(m_corner_widget, Qt::TopLeftCorner);
      m_corner_widget->setVisible(true);
      m_tab_bar->setAutoHide(false);
    }
}

void dooble_tab_widget::tabRemoved(int index)
{
  QTabWidget::tabRemoved(index);
  setTabsClosable(count() > 0);

  if(count() == 0)
    /*
    ** A support window has discovered a new parent.
    */

    emit empty_tab();
}
