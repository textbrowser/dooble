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
#include <QDesktopWidget>
#include <QWidget>

#include "dooble_ui_utilities.h"

void dooble_ui_utilities::center_window_widget(QWidget *parent, QWidget *widget)
{
  /*
  ** Adapted from qdialog.cpp.
  */

  if(!widget)
    return;

  QPoint p(0, 0);
  QWidget *w = parent;
  int extraw = 0, extrah = 0, scrn = 0;

  if(w)
    w = w->window();

  QRect desk;

  if(w)
    scrn = QApplication::desktop()->screenNumber(w);
  else if(QApplication::desktop()->isVirtualDesktop())
    scrn = QApplication::desktop()->screenNumber(QCursor::pos());
  else
    scrn = QApplication::desktop()->screenNumber(widget);

  desk = QApplication::desktop()->availableGeometry(scrn);

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); i++)
    {
      QWidget *current = list.at(i);

      if(current->isVisible())
	{
	  int framew = current->geometry().x() - current->x();
	  int frameh = current->geometry().y() - current->y();

	  extraw = qMax(extraw, framew);
	  extrah = qMax(extrah, frameh);
        }
    }

  if(extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40)
    {
      extrah = 40;
      extraw = 10;
    }

  if(w)
    {
      QPoint pp(w->mapToGlobal(QPoint(0, 0)));

      p = QPoint(pp.x() + w->width() / 2, pp.y() + w->height() / 2);
    }
  else
    p = QPoint(desk.x() + desk.width() / 2, desk.y() + desk.height() / 2);

  p = QPoint(p.x() - widget->width() / 2 - extraw,
	     p.y() - widget->height() / 2 - extrah);

  if(p.x() + extraw + widget->width() > desk.x() + desk.width())
    p.setX(desk.x() + desk.width() - widget->width() - extraw);

  if(p.x() < desk.x())
    p.setX(desk.x());

  if(p.y() + extrah + widget->height() > desk.y() + desk.height())
    p.setY(desk.y() + desk.height() - widget->height() - extrah);

  if(p.y() < desk.y())
    p.setY(desk.y());

  widget->move(p);
}
