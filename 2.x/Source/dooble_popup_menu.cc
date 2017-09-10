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

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_clear_items.h"
#include "dooble_cookies_window.h"
#include "dooble_history_window.h"
#include "dooble_page.h"
#include "dooble_popup_menu.h"
#include "dooble_settings.h"

dooble_popup_menu::dooble_popup_menu(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  m_ui.authenticate->setEnabled(dooble_settings::has_dooble_credentials());
  connect(dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  this,
	  SLOT(slot_dooble_credentials_authenticated(bool)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.authenticate,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_authenticate(void)));

  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    {
      connect(tool_button,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slot_tool_button_clicked(void)));

      if(m_ui.exit_dooble == tool_button)
#ifdef Q_OS_MACOS
	tool_button->setStyleSheet
	  ("QToolButton {border: none;}"
	   "QToolButton::hover {background-color: darkred;}");
#else
        tool_button->setStyleSheet
	  ("QToolButton::hover {background-color: darkred;}");
#endif
      else
#ifdef Q_OS_MACOS
	tool_button->setStyleSheet
	  ("QToolButton {border: none;}"
	   "QToolButton::hover {background-color: darkorange;}");
#else
        {
	}
#endif
    }

  prepare_icons();
  setWindowFlag(Qt::WindowStaysOnTopHint, true);
}

dooble *dooble_popup_menu::find_parent_dooble(void) const
{
  QWidget *parent = parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	return qobject_cast<dooble *> (parent);
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  return 0;
}

void dooble_popup_menu::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.authenticate->setIcon
    (QIcon(QString(":/%1/48/authenticate.png").arg(icon_set)));
  m_ui.blocked_domains->setIcon
    (QIcon(QString(":/%1/48/blocked_domains.png").arg(icon_set)));
  m_ui.clear_items->setIcon
    (QIcon(QString(":/%1/48/clear_items.png").arg(icon_set)));
  m_ui.cookies->setIcon
    (QIcon(QString(":/%1/48/cookies.png").arg(icon_set)));
  m_ui.exit_dooble->setIcon
    (QIcon(QString(":/%1/48/exit_dooble.png").arg(icon_set)));
  m_ui.history->setIcon(QIcon(QString(":/%1/48/history.png").arg(icon_set)));
  m_ui.new_private_tab->setIcon
    (QIcon(QString(":/%1/48/new_private_tab.png").arg(icon_set)));
  m_ui.new_tab->setIcon(QIcon(QString(":/%1/48/new_tab.png").arg(icon_set)));
  m_ui.new_window->setIcon
    (QIcon(QString(":/%1/48/new_window.png").arg(icon_set)));
  m_ui.print->setIcon(QIcon(QString(":/%1/48/print.png").arg(icon_set)));
  m_ui.settings->setIcon(QIcon(QString(":/%1/48/settings.png").arg(icon_set)));
}

void dooble_popup_menu::slot_authenticate(void)
{
  if(m_dooble_page)
    disconnect(this,
	       SIGNAL(authenticate(void)),
	       m_dooble_page,
	       SLOT(slot_authenticate(void)));

  /*
  ** Locate the top-most dooble_page object.
  */

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(QWidget *widget, QApplication::topLevelWidgets())
    if(qobject_cast<dooble *> (widget) &&
       qobject_cast<dooble *> (widget)->isVisible())
      {
	m_dooble_page = qobject_cast<dooble *> (widget)->current_page();

	if(m_dooble_page)
	  widget->raise();

	break;
      }

  QApplication::restoreOverrideCursor();

  if(m_dooble_page)
    {
      connect(this,
	      SIGNAL(authenticate(void)),
	      m_dooble_page,
	      SLOT(slot_authenticate(void)),
	      Qt::UniqueConnection);
      emit authenticate();
    }
}

void dooble_popup_menu::slot_dooble_credentials_authenticated(bool state)
{
  if(state)
    m_ui.authenticate->setEnabled(false);
  else
    m_ui.authenticate->setEnabled(dooble_settings::has_dooble_credentials());
}

void dooble_popup_menu::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_popup_menu::slot_tool_button_clicked(void)
{
  if(m_ui.blocked_domains == sender())
    {
      emit show_accepted_or_blocked_domains();
      accept();
    }
  else if(m_ui.clear_items == sender())
    {
      dooble_clear_items clear_items(find_parent_dooble());

      connect(&clear_items,
	      SIGNAL(containers_cleared(void)),
	      dooble::s_application,
	      SIGNAL(containers_cleared(void)));
      clear_items.exec();
    }
  else if(m_ui.cookies == sender())
    {
      emit show_cookies();
      accept();
    }
  else if(m_ui.exit_dooble == sender())
    emit quit_dooble();
  else if(m_ui.history == sender())
    {
      emit show_history();
      accept();
    }
  else if(m_ui.new_private_tab == sender())
    {
      dooble *d = find_parent_dooble();

      if(d)
	d->new_page(true);
    }
  else if(m_ui.new_tab == sender())
    {
      dooble *d = find_parent_dooble();

      if(d)
	d->new_page(false);
    }
  else if(m_ui.new_window == sender())
    (new dooble())->show();
  else if(m_ui.print == sender())
    {
      dooble *d = find_parent_dooble();

      if(d)
	d->print_current_page();
    }
  else if(m_ui.settings == sender())
    {
      emit show_settings();
      accept();
    }
}
