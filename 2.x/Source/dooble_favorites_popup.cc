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

#include <QKeyEvent>
#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_favorites_popup.h"
#include "dooble_history.h"
#include "dooble_settings.h"

dooble_favorites_popup::dooble_favorites_popup(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);

  if(dooble::s_history && dooble::s_history->favorites_model())
    m_ui.view->setModel(dooble::s_history->favorites_model());
  else
    QTimer::singleShot(1500, this, SLOT(slot_set_favorites_model(void)));

  m_ui.view->setColumnHidden(2, true);
  m_ui.view->setColumnHidden(3, true);
  m_ui.sort_order->setCurrentIndex
    (qBound(0,
	    dooble_settings::setting("favorites_sort_index").toInt(),
	    m_ui.sort_order->count() - 1));
  slot_sort(m_ui.sort_order->currentIndex());
  connect(m_ui.sort_order,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slot_sort(int)));
}

void dooble_favorites_popup::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    accept();

  QDialog::keyPressEvent(event);
}

void dooble_favorites_popup::slot_set_favorites_model(void)
{
  if(m_ui.view->model())
    return;
  else if(dooble::s_history && dooble::s_history->favorites_model())
    {
      m_ui.view->setModel(dooble::s_history->favorites_model());
      slot_sort(m_ui.sort_order->currentIndex());
    }
  else
    QTimer::singleShot(1500, this, SLOT(slot_set_favorites_model(void)));
}

void dooble_favorites_popup::slot_sort(int index)
{
  if(index == 0) // Last Visited
    m_ui.view->sortByColumn(2, Qt::DescendingOrder);
  else if(index == 1) // Most Popular
    m_ui.view->sortByColumn(3, Qt::DescendingOrder);
  else // Title
    m_ui.view->sortByColumn(0, Qt::DescendingOrder);

  if(sender())
    dooble_settings::set_setting("favorites_sort_index", index);
}
