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

#include <QWebEngineProfile>

#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_clear_items.h"
#include "dooble_cookies.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_settings.h"

dooble_clear_items::dooble_clear_items(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  connect(m_ui.buttonBox->button(QDialogButtonBox::Apply),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_clear_items(void)));

  foreach(QCheckBox *check_box, findChildren<QCheckBox *> ())
    {
      check_box->setChecked
	(dooble_settings::setting(QString("dooble_clear_items_%1").
				  arg(check_box->objectName())).toBool());
      connect(check_box,
	      SIGNAL(toggled(bool)),
	      this,
	      SLOT(slot_check_box_toggled(bool)));
    }
}

void dooble_clear_items::slot_check_box_toggled(bool state)
{
  QCheckBox *check_box = qobject_cast<QCheckBox *> (sender());

  if(!check_box)
    return;

  dooble_settings::set_setting
    (QString("dooble_clear_items_%1").arg(check_box->objectName()), state);
}

void dooble_clear_items::slot_clear_items(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  bool state = false;

  if(m_ui.accepted_blocked_domains->isChecked())
    {
      dooble_accepted_or_blocked_domains::purge();
      state = true;
    }

  if(m_ui.certificate_error_exceptions->isChecked())
    {
      dooble_certificate_exceptions_menu_widget::purge();
      state = true;
    }

  if(m_ui.cookies->isChecked())
    {
      dooble_cookies::purge();
      state = true;
    }

  if(m_ui.download_history->isChecked())
    {
      dooble_downloads::purge();
      state = true;
    }

  if(m_ui.favicons->isChecked())
    {
      dooble_favicons::purge();
      state = true;
    }

  if(m_ui.history->isChecked())
    {
      dooble_history::purge();
      state = true;
    }

  if(m_ui.visited_links->isChecked())
    {
      QWebEngineProfile::defaultProfile()->clearAllVisitedLinks();
      state = true;
    }

  QApplication::restoreOverrideCursor();

  if(state)
    emit containers_cleared();
}
