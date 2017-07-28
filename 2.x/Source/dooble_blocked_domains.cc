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

#include <QInputDialog>

#include "dooble_blocked_domains.h"
#include "dooble_settings.h"

dooble_blocked_domains::dooble_blocked_domains(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
}

void dooble_blocked_domains::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("blocked_domains_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble_blocked_domains::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("blocked_domains_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_blocked_domains::slot_add(void)
{
  QString text = QInputDialog::
    getText(this, tr("Dooble: New Blocked Domain"), tr("Blocked Domain")).
    trimmed();

  if(text.isEmpty())
    return;

  if(!m_ui.table->findItems(text, Qt::MatchFixedString).isEmpty())
    return;

  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);

  for(int i = 0; i < 2; i++)
    {
      QTableWidgetItem *item = new QTableWidgetItem();

      if(i == 0)
	{
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);
	  item->setCheckState(Qt::Checked);
	}
      else
	{
	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  item->setText(text);
	}

      m_ui.table->setItem(m_ui.table->rowCount() - 1, i, item);
    }

  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
}
