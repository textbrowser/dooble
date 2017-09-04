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
#include "dooble_popup_menu.h"
#include "dooble_settings.h"

dooble_popup_menu::dooble_popup_menu(void):QDialog()
{
  m_ui.setupUi(this);
  m_ui.authenticate->setEnabled(dooble_settings::has_dooble_credentials());
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(dooble::s_settings,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  this,
	  SLOT(slot_dooble_credentials_authenticated(bool)));

#ifdef Q_OS_MACOS
  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    tool_button->setStyleSheet("QToolButton {border: none;}");
#endif

  prepare_icons();
  setWindowFlag(Qt::WindowStaysOnTopHint, true);
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
  m_ui.exit_dooble->setIcon
    (QIcon(QString(":/%1/48/exit_dooble.png").arg(icon_set)));
  m_ui.history->setIcon(QIcon(QString(":/%1/48/history.png").arg(icon_set)));
  m_ui.new_private_tab->setIcon
    (QIcon(QString(":/%1/48/new_private_tab.png").arg(icon_set)));
  m_ui.new_tab->setIcon(QIcon(QString(":/%1/48/new_tab.png").arg(icon_set)));
  m_ui.new_window->setIcon
    (QIcon(QString(":/%1/48/new_window.png").arg(icon_set)));
  m_ui.settings->setIcon(QIcon(QString(":/%1/48/settings.png").arg(icon_set)));
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
