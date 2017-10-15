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
#include "dooble_cryptography.h"
#include "dooble_history_window.h"
#include "dooble_popup_menu.h"
#include "dooble_settings.h"

dooble_popup_menu::dooble_popup_menu(qreal zoom_factor, QWidget *parent):
  QDialog(parent)
{
  m_ui.setupUi(this);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    m_ui.authenticate->setEnabled(false);
  else
    m_ui.authenticate->setEnabled(dooble_settings::has_dooble_credentials());

  if(!dooble_settings::has_dooble_credentials())
    m_ui.authenticate->setToolTip
      (tr("Permanent credentials have not been prepared."));

  m_ui.zoom_frame->setVisible
    (dooble_settings::
     zoom_frame_location_string(dooble_settings::
				setting("zoom_frame_location_index").
				toInt()) == "popup_menu");
  connect(m_ui.authenticate,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(authenticate(void)));

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
	   "QToolButton::hover {background-color: #b71c1c;}");
#else
        tool_button->setStyleSheet
	  ("QToolButton::hover {background-color: #b71c1c;}");
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
  slot_zoomed(zoom_factor);
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
  m_ui.new_private_window->setIcon
    (QIcon(QString(":/%1/48/new_private_window.png").arg(icon_set)));
  m_ui.new_tab->setIcon(QIcon(QString(":/%1/48/new_tab.png").arg(icon_set)));
  m_ui.new_window->setIcon
    (QIcon(QString(":/%1/48/new_window.png").arg(icon_set)));
  m_ui.print->setIcon(QIcon(QString(":/%1/48/print.png").arg(icon_set)));
  m_ui.save_page->setIcon(QIcon(QString(":/%1/48/save.png").arg(icon_set)));
  m_ui.settings->setIcon(QIcon(QString(":/%1/48/settings.png").arg(icon_set)));

  int preferred_height = 50;
  int preferred_width = 50;

  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    {
      tool_button->setIconSize(QSize(48, 48));
      preferred_height = qMax
	(preferred_height, tool_button->sizeHint().height());
      preferred_width = qMax
	(preferred_width, tool_button->sizeHint().width());
    }

  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    if(m_ui.zoom_in != tool_button &&
       m_ui.zoom_out != tool_button &&
       m_ui.zoom_reset != tool_button)
      {
	tool_button->setMaximumSize(QSize(preferred_width, preferred_height));
	tool_button->setMinimumSize(QSize(preferred_width, preferred_height));
      }
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
  else if(m_ui.new_private_window == sender())
    (new dooble(QUrl(), true))->show();
  else if(m_ui.new_tab == sender())
    {
      dooble *d = find_parent_dooble();

      if(d)
	d->new_page(QUrl(), d->is_private());

      accept();
    }
  else if(m_ui.new_window == sender())
    (new dooble(QUrl(), false))->show();
  else if(m_ui.print == sender())
    {
      dooble *d = find_parent_dooble();

      if(d)
	d->print_current_page();
    }
  else if(m_ui.save_page == sender())
    {
      emit save();
      accept();
    }
  else if(m_ui.settings == sender())
    {
      emit show_settings();
      accept();
    }
  else if(m_ui.zoom_in == sender())
    emit zoom_in();
  else if(m_ui.zoom_out == sender())
    emit zoom_out();
  else if(m_ui.zoom_reset == sender())
    emit zoom_reset();
}

void dooble_popup_menu::slot_zoomed(qreal zoom_factor)
{
  m_ui.zoom_reset->setText
    (tr("%1%").
     arg(QString::
	 number(static_cast<int> (100 * qBound(0.25, zoom_factor, 5.0)))));
}
