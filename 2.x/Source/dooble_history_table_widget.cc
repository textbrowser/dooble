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
#include <QHeaderView>

#include "dooble_application.h"
#include "dooble_favicons.h"
#include "dooble_history_table_widget.h"

dooble_history_table_widget::dooble_history_table_widget(QWidget *parent):
  QTableWidget(parent)
{
  connect(horizontalHeader(),
	  SIGNAL(sectionClicked(int)),
	  this,
	  SLOT(slot_section_clicked(int)));
}

void dooble_history_table_widget::keyPressEvent(QKeyEvent *event)
{
  if(event)
    switch(event->key())
      {
      case Qt::Key_Delete:
	{
	  emit delete_pressed();
	  break;
	}
      case Qt::Key_Enter:
      case Qt::Key_Return:
	{
	  emit enter_pressed();
	  break;
	}
      default:
	{
	  break;
	}
      }

  QTableWidget::keyPressEvent(event);
}

void dooble_history_table_widget::prepare_viewport_icons(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int a = rowAt(viewport()->rect().topLeft().y());
  int b = rowAt(viewport()->rect().bottomLeft().y());

  if(b == -1)
    /*
    ** Approximate the number of rows.
    */

    b = a + viewport()->rect().bottomLeft().y() / qMax(1, rowHeight(a));

  for(int i = a; i <= b; i++)
    {
      QTableWidgetItem *item = this->item(i, 1); // Title

      if(!item)
	continue;
      else if(isRowHidden(i))
	{
	  b += 1;
	  continue;
	}
      else if(!item->icon().isNull())
	continue;
      else
	item->setIcon(dooble_favicons::icon(item->data(Qt::UserRole).toUrl()));
    }

  QApplication::restoreOverrideCursor();
}

void dooble_history_table_widget::resizeEvent(QResizeEvent *event)
{
  QTableWidget::resizeEvent(event);
  prepare_viewport_icons();
}

void dooble_history_table_widget::scrollContentsBy(int dx, int dy)
{
  QTableWidget::scrollContentsBy(dx, dy);
  prepare_viewport_icons();
}

void dooble_history_table_widget::slot_section_clicked(int index)
{
  Q_UNUSED(index);
  prepare_viewport_icons();
}
