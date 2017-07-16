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

#include <QMenu>

#include "dooble_tab_bar.h"

dooble_tab_bar::dooble_tab_bar(QWidget *parent):QTabBar(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setDocumentMode(true);
  setElideMode(Qt::ElideRight);
  setExpanding(true);
  setMovable(true);
  setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
  setUsesScrollButtons(true);
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
}

QSize dooble_tab_bar::tabSizeHint(int index) const
{
  QSize size(QTabBar::tabSizeHint(index));
  int preferred_tab_width = 225;

  if(!(parentWidget() &&
       count() * rect().width() < parentWidget()->size().width()))
    preferred_tab_width = qBound
      (125,
       qMax(size.width(), rect().width() / qMax(1, count())),
       preferred_tab_width);

  size.setWidth(preferred_tab_width);
  return size;
}

void dooble_tab_bar::slot_close_other_tabs(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      int index = tabAt(action->property("point").toPoint());

      for(int i = count() - 1; i >= 0; i--)
	if(i != index)
	  emit tabCloseRequested(i);
    }
}

void dooble_tab_bar::slot_close_tab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    emit tabCloseRequested(tabAt(action->property("point").toPoint()));
}

void dooble_tab_bar::slot_show_context_menu(const QPoint &point)
{
  QAction *action = 0;
  QMenu menu(this);

  action = menu.addAction(tr("Close &Tab"),
			  this,
			  SLOT(slot_close_tab(void)));
  action->setEnabled(count() > 1);
  action->setProperty("point", point);
  action = menu.addAction(tr("Close &Other Tabs"),
			  this,
			  SLOT(slot_close_other_tabs(void)));
  action->setEnabled(count() > 1);
  action->setProperty("point", point);
  menu.exec(mapToGlobal(point));
}
