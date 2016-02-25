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

#include <QResizeEvent>
#include <QSettings>
#include <QStyle>

#include "dgenericsearchwidget.h"
#include "dooble.h"

dgenericsearchwidget::dgenericsearchwidget(QWidget *parent):QLineEdit(parent)
{
  setMaxLength(2500);
  findToolButton = new QToolButton(this);
  findToolButton->setToolTip(tr("Initiate Search"));
  findToolButton->setIconSize(QSize(16, 16));
  findToolButton->setCursor(Qt::ArrowCursor);
  findToolButton->setStyleSheet("QToolButton {"
				"border: none; "
				"padding-bottom: 0px; "
				"}");
  slotSetIcons();

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet
    (QString("QLineEdit {padding-right: %1px; padding-left: %2px; "
	     "selection-background-color: darkgray;}").arg
     (findToolButton->sizeHint().width() + frameWidth + 5).arg
     (frameWidth + 5));
  setMinimumHeight(sizeHint().height() + 10);
}

void dgenericsearchwidget::resizeEvent(QResizeEvent *event)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = findToolButton->sizeHint();

  findToolButton->move
    (rect().right() - frameWidth - size1.width() - 5,
     (rect().bottom() + 2 - size1.height()) / 2);
  QLineEdit::resizeEvent(event);
}

void dgenericsearchwidget::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("searchWidget");
  findToolButton->setIcon
    (QIcon(settings.value("findToolButton").toString()));
}

void dgenericsearchwidget::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(QKeySequence(event->modifiers() + event->key()) ==
	 QKeySequence(Qt::ControlModifier + Qt::Key_F))
	{
	  event->ignore();
	  setFocus();
	  selectAll();
	  return;
	}
    }

  QLineEdit::keyPressEvent(event);
}
