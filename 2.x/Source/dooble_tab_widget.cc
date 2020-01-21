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
#include "dooble_ui_utilities.h"

dooble_tab_widget::dooble_tab_widget(QWidget *parent):QTabWidget(parent)
{
  m_add_tab_tool_button = new QToolButton(this);
  m_add_tab_tool_button->setAutoRaise(true);
  m_add_tab_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {margin-bottom: 1px; margin-top: 1px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_add_tab_tool_button->setToolTip(tr("New Tab"));
  m_private_tool_button = new QToolButton(this);
  m_private_tool_button->setAutoRaise(false);
  m_private_tool_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_private_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_private_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 1px; margin-top: 1px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_private_tool_button->setToolTip(tr("This is a private window."));
  m_private_tool_button->setVisible
    (dooble_settings::setting("denote_private_widgets").toBool() &&
     is_private());
  m_tabs_menu_button = new QToolButton(this);
  m_tabs_menu_button->setArrowType(Qt::NoArrow);
  m_tabs_menu_button->setAutoRaise(true);
  m_tabs_menu_button->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_tabs_menu_button->setStyleSheet
    ("QToolButton {border:none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}");
#else
  m_tabs_menu_button->setStyleSheet
    ("QToolButton {margin-bottom: 1px; margin-top: 1px;}"
     "QToolButton::menu-button {border: none;}");
#endif
  m_left_corner_widget = new QFrame(this);
  m_right_corner_widget = new QFrame(this);
  delete m_left_corner_widget->layout();
  delete m_right_corner_widget->layout();
  m_left_corner_widget->setLayout(new QHBoxLayout(this));
  m_left_corner_widget->layout()->addWidget(m_private_tool_button);
  m_left_corner_widget->layout()->addWidget(m_tabs_menu_button);
  m_left_corner_widget->layout()->addWidget(m_add_tab_tool_button);
#ifdef Q_OS_MACOS
  m_left_corner_widget->layout()->setContentsMargins(5, 5, 5, 5);
#else
  m_left_corner_widget->layout()->setContentsMargins(5, 0, 5, 0);
#endif
  m_left_corner_widget->layout()->setSpacing(0);
  m_left_corner_widget->setVisible(true);
  m_right_corner_widget->setLayout(new QHBoxLayout(this));
#ifdef Q_OS_MACOS
  m_right_corner_widget->layout()->setContentsMargins(5, 5, 5, 5);
#else
  m_right_corner_widget->layout()->setContentsMargins(5, 0, 5, 0);
#endif
  m_right_corner_widget->layout()->setSpacing(0);
  m_right_corner_widget->setVisible(true);

  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());

      if(theme_color == "default")
	{
	  m_left_corner_widget->setStyleSheet("QWidget {height: auto;}");
	  m_right_corner_widget->setStyleSheet
	    (m_left_corner_widget->styleSheet());
	  setStyleSheet("");
	}
      else
	{
	  m_left_corner_widget->setStyleSheet
	    (QString("QWidget {background-color: %1;"
		     "border-right: 0px solid %2;"
		     "height: auto;"
		     "margin-bottom: 0px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-corner-widget-background-color").
		       arg(theme_color)).name()).
	     arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
	  m_right_corner_widget->setStyleSheet
	    (m_left_corner_widget->styleSheet());
	  setStyleSheet
	    (QString("QTabBar {background-color: %1; margin-top: 1px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()));
	}
    }

  m_tab_bar = new dooble_tab_bar(this);
  m_tab_bar->set_corner_widget(m_right_corner_widget);
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
	  SIGNAL(open_tab_as_new_private_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_private_window(int)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_window(int)));
  connect(m_tab_bar,
	  SIGNAL(reload_tab(int)),
	  this,
	  SIGNAL(reload_tab(int)));
  connect(m_tab_bar,
	  SIGNAL(reload_tab_periodically(int, int)),
	  this,
	  SIGNAL(reload_tab_periodically(int, int)));
  connect(m_tab_bar,
	  SIGNAL(set_visible_corner_button(bool)),
	  this,
	  SLOT(slot_set_visible_corner_button(bool)));
  connect(m_tab_bar,
	  SIGNAL(show_corner_widget(bool)),
	  this,
	  SLOT(slot_show_right_corner_widget(bool)));
  connect(m_tabs_menu_button,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(tabs_menu_button_clicked(void)));
  prepare_icons();
  setCornerWidget(m_left_corner_widget, Qt::TopLeftCorner);
  setTabBar(m_tab_bar);
}

QIcon dooble_tab_widget::tabIcon(int index) const
{
  QIcon icon(QTabWidget::tabIcon(index));

  if(icon.isNull())
    {
      auto *label = qobject_cast<QLabel *>
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

dooble_page *dooble_tab_widget::page(int index) const
{
  return qobject_cast<dooble_page *> (widget(index));
}

void dooble_tab_widget::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_add_tab_tool_button->setIcon
    (QIcon::fromTheme("list-add",
		      QIcon(QString(":/%1/18/add.png").arg(icon_set))));
  m_private_tool_button->setIcon
    (QIcon::fromTheme("view-private",
		      QIcon(QString(":/%1/18/private.png").arg(icon_set))));
  m_tabs_menu_button->setIcon
    (QIcon::fromTheme("go-down",
		      QIcon(QString(":/%1/18/pulldown.png").arg(icon_set))));
}

void dooble_tab_widget::prepare_tab_label(int index, const QIcon &icon)
{
  if(index < 0 || index >= count())
    return;

#ifdef Q_OS_MACOS
  QTabBar::ButtonPosition side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar));

  side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;

  QLabel *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

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
#else
  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->
     styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, m_tab_bar));

  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;

  auto *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      label = new QLabel(this);
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      label->setFixedSize(QSize(16, 16));
      label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
      m_tab_bar->setTabButton(index, side, nullptr);
      m_tab_bar->setTabButton(index, side, label);
    }
  else if(!label->movie() || label->movie()->state() != QMovie::Running)
    label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

  label->setProperty("icon", icon);
#endif
}

void dooble_tab_widget::setTabIcon(int index, const QIcon &icon)
{
  prepare_tab_label(index, icon);
}

void dooble_tab_widget::setTabTextColor(int index, const QColor &color)
{
  m_tab_bar->setTabTextColor(index, color);
}

void dooble_tab_widget::setTabToolTip(int index, const QString &text)
{
  QTabWidget::setTabToolTip(index, dooble_ui_utilities::pretty_tool_tip(text));
}

void dooble_tab_widget::slot_load_finished(void)
{
  auto *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			nullptr,
			m_tab_bar));
  int index = indexOf(page);

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  auto *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(label)
    {
      QMovie *movie = label->movie();

      if(movie)
	{
	  movie->stop();
	  movie->deleteLater();
	}

      label->setMovie(nullptr);

      if(dooble::s_application->style_name() == "fusion")
	{
	  QIcon icon(label->property("icon").value<QIcon> ());

	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	}

      label->update();
      m_tab_bar->update();
    }
}

void dooble_tab_widget::slot_load_started(void)
{
  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			nullptr,
			m_tab_bar));
  int index = indexOf(qobject_cast<QWidget *> (sender()));

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  auto *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      label = new QLabel(this);
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      m_tab_bar->setTabButton(index, side, nullptr);
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
    setCornerWidget(m_left_corner_widget, Qt::TopLeftCorner);
  else
    setCornerWidget(nullptr, Qt::TopLeftCorner);

  m_left_corner_widget->setVisible(state);
}

void dooble_tab_widget::slot_settings_applied(void)
{
  if(dooble::s_application->style_name() == "fusion")
    {
      QString theme_color(dooble_settings::setting("theme_color").toString());

      if(theme_color == "default")
	{
	  m_left_corner_widget->setStyleSheet("QWidget {height: auto;}");
	  m_right_corner_widget->setStyleSheet
	    (m_left_corner_widget->styleSheet());
	  setStyleSheet("");
	}
      else
	{
	  m_left_corner_widget->setStyleSheet
	    (QString("QWidget {background-color: %1;"
		     "border-right: 0px solid %2;"
		     "height: auto;"
		     "margin-bottom: 0px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-corner-widget-background-color").
		       arg(theme_color)).name()).
	     arg(QWidget::palette().color(QWidget::backgroundRole()).name()));
	  m_right_corner_widget->setStyleSheet
	    (m_left_corner_widget->styleSheet());
	  setStyleSheet
	    (QString("QTabBar {background-color: %1; margin-top: 1px;}").
	     arg(dooble_application::s_theme_colors.
		 value(QString("%1-tabbar-background-color").
		       arg(theme_color)).name()));
	}
    }

  m_left_corner_widget->setVisible(true);
  m_private_tool_button->setVisible
    (dooble_settings::setting("denote_private_widgets").toBool() &&
     is_private());
  prepare_icons();
}

void dooble_tab_widget::slot_show_right_corner_widget(bool state)
{
  if(state)
    setCornerWidget(m_right_corner_widget, Qt::TopRightCorner);
  else
    setCornerWidget(nullptr, Qt::TopRightCorner);

  m_right_corner_widget->setVisible(state);
}

void dooble_tab_widget::tabRemoved(int index)
{
  QTabWidget::tabRemoved(index);

  if(count() == 1)
    setTabsClosable
      (dooble::s_settings->setting("allow_closing_of_single_tab").toBool());
  else
    setTabsClosable(count() > 0);

  if(count() == 0)
    /*
    ** A support window has discovered a new parent.
    */

    emit empty_tab();
}
