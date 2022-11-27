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

#include <QApplication>
#include <QStandardItemModel>

#include "dooble_favicons.h"
#include "dooble_table_view.h"

dooble_table_view::dooble_table_view(QWidget *parent):QTableView(parent)
{
  setWordWrap(false);
}

void dooble_table_view::prepare_viewport_icons(void)
{
  auto model = qobject_cast<QStandardItemModel *> (this->model());

  if(!model)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto a = rowAt(viewport()->rect().topLeft().y());
  auto b = rowAt(viewport()->rect().bottomLeft().y());

  if(b == -1)
    /*
    ** Approximate the number of rows.
    */

    b = a + viewport()->rect().bottomLeft().y() / qMax(1, rowHeight(a));
  else if(b == 0)
    /*
    ** Approximate the number of rows.
    */

    b = a + parentWidget()->rect().bottomLeft().y() / qMax(1, rowHeight(a));

  for(int i = a; i <= b; i++)
    {
      auto item = model->item(i, static_cast<int> (Columns::Title));

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
	item->setIcon(dooble_favicons::icon(item->data().toUrl()));
    }

  QApplication::restoreOverrideCursor();
}

void dooble_table_view::resizeEvent(QResizeEvent *event)
{
  QTableView::resizeEvent(event);
  prepare_viewport_icons();
}

void dooble_table_view::scrollContentsBy(int dx, int dy)
{
  QTableView::scrollContentsBy(dx, dy);
  prepare_viewport_icons();
}
