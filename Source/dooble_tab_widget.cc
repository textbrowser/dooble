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
#include "dooble_favicons.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"
#include "dooble_ui_utilities.h"

dooble_tab_widget::dooble_tab_widget(QWidget *parent):QTabWidget(parent)
{
  m_add_tab_tool_button = new QToolButton(this);
  m_add_tab_tool_button->setArrowType(Qt::NoArrow);
  m_add_tab_tool_button->setAutoRaise(true);
  m_add_tab_tool_button->setIconSize(QSize(18, 18));
  m_add_tab_tool_button->setMenu(new QMenu(this));
  m_add_tab_tool_button->setPopupMode(QToolButton::DelayedPopup);
#ifdef Q_OS_MACOS
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
     "QToolButton::menu-button {border: none;}"
     "QToolButton::menu-indicator {image: none;}");
#else
  m_add_tab_tool_button->setStyleSheet
    ("QToolButton {margin-bottom: 10px; margin-top: 10px;}"
     "QToolButton::menu-button {border: none;}"
     "QToolButton::menu-indicator {image: none;}");
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
    ("QToolButton {border: none; margin-bottom: 10px; margin-top: 10px;}"
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
    ("QToolButton {border: none; margin-bottom: 0px; margin-top: 0px;}"
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
  m_right_corner_widget->setVisible(true);

  if(dooble::s_application->style_name() == "fusion" ||
     dooble::s_application->style_name().contains("windows"))
    {
      auto const theme_color
	(dooble_settings::setting("theme_color").toString());

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
  connect(m_add_tab_tool_button->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_history_menu(void)));
  connect(m_tab_bar,
	  SIGNAL(anonymous_tab_headers(bool)),
	  this,
	  SIGNAL(anonymous_tab_headers(bool)));
  connect(m_tab_bar,
	  SIGNAL(clone_tab(int)),
	  this,
	  SIGNAL(clone_tab(int)));
  connect(m_tab_bar,
	  SIGNAL(decouple_tab(int)),
	  this,
	  SIGNAL(decouple_tab(int)));
  connect(m_tab_bar,
	  SIGNAL(new_tab(void)),
	  this,
	  SIGNAL(new_tab(void)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_cute_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_cute_window(int)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_private_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_private_window(int)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_window(int)));
  connect(m_tab_bar,
	  SIGNAL(pin_tab(bool, int)),
	  this,
	  SIGNAL(pin_tab(bool, int)));
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
  setTabBar(m_tab_bar);
  set_tab_position();
}

QIcon dooble_tab_widget::tabIcon(int index) const
{
  auto icon(QTabWidget::tabIcon(index));

  if(icon.isNull())
    {
      auto label = qobject_cast<QLabel *>
	(m_tab_bar->tabButton(index, QTabBar::LeftSide));

      if(!label)
	label = qobject_cast<QLabel *>
	  (m_tab_bar->tabButton(index, QTabBar::RightSide));

      if(label)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	icon = label->pixmap(Qt::ReturnByValue);
#else
	icon = *label->pixmap();
#endif
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

dooble_page *dooble_tab_widget::page(int index) const
{
  return qobject_cast<dooble_page *> (widget(index));
}

dooble_tab_bar *dooble_tab_widget::tab_bar(void) const
{
  return m_tab_bar;
}

void dooble_tab_widget::prepare_icons(void)
{
  auto const icon_set(dooble_settings::setting("icon_set").toString());
  auto const use_material_icons(dooble_settings::use_material_icons());

  m_add_tab_tool_button->setIcon
    (QIcon::fromTheme(use_material_icons + "list-add",
		      QIcon(QString(":/%1/18/add.png").arg(icon_set))));
  m_private_tool_button->setIcon
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/18/private.png").arg(icon_set))));
  m_tabs_menu_button->setIcon
    (QIcon::fromTheme(use_material_icons + "go-down",
		      QIcon(QString(":/%1/18/pulldown.png").arg(icon_set))));
}

void dooble_tab_widget::prepare_tab_label(int index, const QIcon &i)
{
  auto icon(i);
  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			nullptr,
			m_tab_bar));

  if(icon.isNull()) // Qt 6
    {
      auto page = qobject_cast<dooble_page *> (widget(index));

      if(page)
	icon = dooble_favicons::icon(page->url());
      else
	icon = dooble_favicons::icon(QUrl());
    }

#ifdef Q_OS_MACOS
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::RightSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#endif
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    {
      auto label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

      if(label)
	label->deleteLater();

      QTabWidget::setTabIcon(index, icon);
      m_tab_bar->setTabButton(index, side, nullptr);
      return;
    }
  else // East -> North
    QTabWidget::setTabIcon(index, QIcon());
#endif

  if(index < 0 || index >= count())
    return;

  auto label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      label = new QLabel(this);
      label->setAlignment(Qt::AlignCenter);
#ifndef Q_OS_MACOS
      label->setMinimumSize(QSize(16, 16));
#endif
      label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
      m_tab_bar->setTabButton(index, side, nullptr);
      m_tab_bar->setTabButton(index, side, label);
    }
  else if(!label->movie() || label->movie()->state() != QMovie::Running)
    label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

  label->setProperty("icon", icon);
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

void dooble_tab_widget::set_is_cute(bool is_cute)
{
  if(is_cute)
    {
      m_left_corner_widget->setVisible(false);
      m_right_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
      setCornerWidget(nullptr, Qt::TopRightCorner);
      setTabPosition(QTabWidget::North);
    }
}

void dooble_tab_widget::set_tab_pinned(bool state, int index)
{
  auto const static list
    (QList<QTabBar::ButtonPosition> () << QTabBar::LeftSide
                                       << QTabBar::RightSide);

  if(state)
    {
      for(int i = 0; i < list.size(); i++)
	{
	  auto widget = tabBar()->tabButton(index, list.at(i));

	  if(!qobject_cast<QLabel *> (widget))
	    {
	      m_close_buttons[page(index)] = widget;
	      tabBar()->setTabButton(index, list.at(i), nullptr);
	      break;
	    }
	}
    }
  else
    {
      for(int i = 0; i < list.size(); i++)
	{
	  auto widget = tabBar()->tabButton(index, list.at(i));

	  if(!widget)
	    {
	      tabBar()->setTabButton
		(index, list.at(i), m_close_buttons.value(page(index)));
	      break;
	    }
	}
    }
}

void dooble_tab_widget::set_tab_position(void)
{
  auto const show_left_corner_widget = dooble_settings::setting
    ("show_left_corner_widget").toBool();

  if(!show_left_corner_widget)
    {
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
    }

  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east")
    {
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
      setTabPosition(QTabWidget::East);
    }
  else if(tab_position == "south")
    {
#ifdef Q_OS_MACOS
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
#else
      if(!cornerWidget(Qt::TopLeftCorner) && show_left_corner_widget)
	setCornerWidget(m_left_corner_widget, Qt::TopLeftCorner);
#endif

      setTabPosition(QTabWidget::South);
    }
  else if(tab_position == "west")
    {
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
      setTabPosition(QTabWidget::West);
    }
  else
    {
      if(!cornerWidget(Qt::TopLeftCorner) && show_left_corner_widget)
	setCornerWidget(m_left_corner_widget, Qt::TopLeftCorner);

      setTabPosition(QTabWidget::North);
    }

  m_tab_bar->resize(m_tab_bar->sizeHint());
}

void dooble_tab_widget::slot_about_to_show_history_menu(void)
{
  if(dooble::s_history->last_n_actions(1).isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_add_tab_tool_button->menu()->clear();

  QFontMetrics const font_metrics(m_add_tab_tool_button->menu()->font());
  auto const list
    (dooble::s_history->
     last_n_actions(5 + static_cast<int> (dooble_page::
					  ConstantsEnum::
					  MAXIMUM_HISTORY_ITEMS)));

  foreach(auto i, list)
    {
      connect(i,
	      SIGNAL(triggered(void)),
	      this,
	      SLOT(slot_history_action_triggered(void)));
      i->setText
	(font_metrics.elidedText(i->text(),
				 Qt::ElideRight,
				 dooble_ui_utilities::
				 context_menu_width(m_add_tab_tool_button->
						    menu())));
      m_add_tab_tool_button->menu()->addAction(i);
    }

  QApplication::restoreOverrideCursor();
}

void dooble_tab_widget::slot_history_action_triggered(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  emit new_tab(action->data().toUrl());
}

void dooble_tab_widget::slot_load_finished(void)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    return;
#endif

  auto page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  auto const index = indexOf(page);
  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			nullptr,
			m_tab_bar));

#ifdef Q_OS_MACOS
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::RightSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#endif
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  auto label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(label)
    {
      auto movie = label->movie();

      if(movie)
	{
	  movie->stop();
	  movie->deleteLater();
	}

      label->setMovie(nullptr);

      if(dooble::s_application->style_name() == "fusion")
	{
	  auto const icon(label->property("icon").value<QIcon> ());

	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	}

      label->update();
      m_tab_bar->update();
    }
}

void dooble_tab_widget::slot_load_started(void)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    return;
#endif

  auto const index = indexOf(qobject_cast<QWidget *> (sender()));
  auto side = static_cast<QTabBar::ButtonPosition>
    (style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			nullptr,
			m_tab_bar));

#ifdef Q_OS_MACOS
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::RightSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#endif
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  auto label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      label = new QLabel(this);
      label->setAlignment(Qt::AlignCenter);
#ifndef Q_OS_MACOS
      label->setMinimumSize(QSize(16, 16));
#endif
      m_tab_bar->setTabButton(index, side, nullptr);
      m_tab_bar->setTabButton(index, side, label);
    }

  auto movie = label->movie();

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
  if(!dooble_settings::setting("show_left_corner_widget").toBool())
    {
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
      return;
    }

  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    {
      m_left_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopLeftCorner);
    }
  else
    {
#ifdef Q_OS_MACOS
      if(tab_position == "south")
	{
	  m_left_corner_widget->setVisible(false);
	  setCornerWidget(nullptr, Qt::TopLeftCorner);
	  return;
	}
#endif

      if(state)
	setCornerWidget(m_left_corner_widget, Qt::TopLeftCorner);
      else
	setCornerWidget(nullptr, Qt::TopLeftCorner);

      m_left_corner_widget->setVisible(state); // Order is important.
    }
}

void dooble_tab_widget::slot_settings_applied(void)
{
  if(dooble::s_application->style_name() == "fusion" ||
     dooble::s_application->style_name().contains("windows"))
    {
      auto const theme_color
	(dooble_settings::setting("theme_color").toString());

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

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    {
      auto side = static_cast<QTabBar::ButtonPosition>
	(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
			    nullptr,
			    m_tab_bar));

#ifdef Q_OS_MACOS
      side = (side == QTabBar::LeftSide) ?
	QTabBar::LeftSide : QTabBar::RightSide;
#else
      side = (side == QTabBar::LeftSide) ?
	QTabBar::RightSide : QTabBar::LeftSide;
#endif

      for(int i = 0; i < count(); i++)
	{
	  auto label = qobject_cast<QLabel *> (m_tab_bar->tabButton(i, side));

	  if(label)
	    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	      QTabWidget::setTabIcon(i, label->pixmap(Qt::ReturnByValue));
#else
	      QTabWidget::setTabIcon(i, *label->pixmap());
#endif
	      label->deleteLater();
	    }

	  m_tab_bar->setTabButton(i, side, nullptr);
	}
    }
#endif

  m_left_corner_widget->setVisible(true);
  m_private_tool_button->setVisible
    (dooble_settings::setting("denote_private_widgets").toBool() &&
     is_private());
  m_tab_bar->setDocumentMode
    (dooble_settings::setting("tab_document_mode").toBool());
  prepare_icons();
}

void dooble_tab_widget::slot_show_right_corner_widget(bool state)
{
  auto const tab_position
    (dooble_settings::setting("tab_position").toString().trimmed());

  if(tab_position == "east" || tab_position == "west")
    {
      m_right_corner_widget->setVisible(false);
      setCornerWidget(nullptr, Qt::TopRightCorner);
    }
  else
    {
      if(state)
	setCornerWidget(m_right_corner_widget, Qt::TopRightCorner);
      else
	setCornerWidget(nullptr, Qt::TopRightCorner);

      m_right_corner_widget->setVisible(state); // Order is important.
    }
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
