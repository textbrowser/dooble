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

#include <QDropEvent>
#include <QStandardItemModel>
#include <QUrl>
#include <QtDebug>

#include "dbookmarkstree.h"

dbookmarkstree::dbookmarkstree(QWidget *parent):QTreeView(parent)
{
}

void dbookmarkstree::dropEvent(QDropEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_bookmarks_table" &&
     event->mimeData())
    {
      QModelIndex index(indexAt(event->pos()));

      if(index.isValid())
	{
	  event->accept();
	  emit bookmarkReceived(index);
	}
      else
	{
	  event->ignore();
	  return;
	}
    }

  QTreeView::dropEvent(event);
}

void dbookmarkstree::dragMoveEvent(QDragMoveEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_bookmarks_tree" &&
     event->mimeData())
    {
      if(!indexAt(event->pos()).isValid())
	{
	  event->ignore();
	  return;
	}
    }

  QTreeView::dragMoveEvent(event);
}

void dbookmarkstree::dragEnterEvent(QDragEnterEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_bookmarks_table" &&
     event->mimeData())
    {
      if(!indexAt(event->pos()).isValid())
	{
	  event->ignore();
	  return;
	}
    }

  QTreeView::dragEnterEvent(event);
}

void dbookmarkstree::keyPressEvent(QKeyEvent *event)
{
  QTreeView::keyPressEvent(event);

  if(event)
    {
      int key = event->key();

      if(key == Qt::Key_Up ||
	 key == Qt::Key_Down ||
	 key == Qt::Key_PageUp ||
	 key == Qt::Key_PageDown)
	emit itemSelected(currentIndex());
    }
}

void dbookmarkstree::mouseReleaseEvent(QMouseEvent *event)
{
  /*
  ** The user may drag the mouse cursor over items without releasing
  ** the mouse button. If we don't listen on this event, we will
  ** not be able to capture the release point. If we don't capture
  ** the release point, the bookmarks for the current folder will
  ** not be displayed.
  */

  QTreeView::mouseReleaseEvent(event);

  if(event && indexAt(event->pos()).isValid())
    emit itemSelected(currentIndex());
}
