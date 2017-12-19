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

#include <QMouseEvent>

#include "dooble_address_widget.h"
#include "dooble_address_widget_completer_popup.h"

dooble_address_widget_completer_popup::dooble_address_widget_completer_popup
(QWidget *parent):QTableView(parent)
{
  m_line_edit = qobject_cast<dooble_address_widget *> (parent);
  setAlternatingRowColors(false);
  setMouseTracking(true);
  setShowGrid(false);
}

void dooble_address_widget_completer_popup::mouseMoveEvent(QMouseEvent *event)
{
  if(event && event->type() == QEvent::MouseMove)
    {
      QModelIndex index(indexAt(event->pos()));

      if(index.isValid())
	{
	  QString text("");

	  if(m_line_edit)
	    text = m_line_edit->text();

	  selectRow(index.row());

	  if(m_line_edit)
	    m_line_edit->setText(text);
	}
    }

  QTableView::mouseMoveEvent(event);
}
