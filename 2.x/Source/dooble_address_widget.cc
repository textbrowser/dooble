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

#include <QStyle>
#include <QToolButton>

#include "dooble.h"
#include "dooble_address_widget.h"

dooble_address_widget::dooble_address_widget(QWidget *parent):QLineEdit(parent)
{
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  m_bookmark = new QToolButton(this);
  m_bookmark->setCursor(Qt::ArrowCursor);
  m_bookmark->setIconSize(QSize(16, 16));
  m_bookmark->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
  m_bookmark->setToolTip(tr("Bookmark"));
  m_pull_down = new QToolButton(this);
  m_pull_down->setCursor(Qt::ArrowCursor);
  m_pull_down->setIconSize(QSize(16, 16));
  m_pull_down->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
  m_pull_down->setToolTip(tr("Show History"));
  prepare_icons();
  setMinimumHeight(sizeHint().height() + 10);
  setStyleSheet
    (QString("QLineEdit {padding-left: %1px; padding-right: %2px;}").
     arg(m_bookmark->sizeHint().width() + frame_width + 5).
     arg(m_pull_down->sizeHint().width() + frame_width + 5));
}

void dooble_address_widget::prepare_icons(void)
{
  QString icon_set(dooble::setting("icon_set").toString());

  m_bookmark->setIcon(QIcon(QString(":/%1/16/bookmark.png").arg(icon_set)));
  m_pull_down->setIcon(QIcon(QString(":/%1/16/pulldown.png").arg(icon_set)));
}

void dooble_address_widget::resizeEvent(QResizeEvent *event)
{
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = m_bookmark->sizeHint();
  QSize size2 = m_pull_down->sizeHint();

  m_bookmark->move
    (frame_width - rect().left() + 6, (rect().bottom() + 2 -
				       size1.height()) / 2);
  m_pull_down->move
    (rect().right() - frame_width - size2.width() - 5, (rect().bottom() + 2 -
							size2.height()) / 2);

  if(selectedText().isEmpty())
    setCursorPosition(0);

  QLineEdit::resizeEvent(event);
}

void dooble_address_widget::setText(const QString &text)
{
  QLineEdit::setText(text);
  setCursorPosition(0);
}
