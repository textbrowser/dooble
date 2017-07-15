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

#include <QLineEdit>
#include <QMouseEvent>

#include "dooble_label_widget.h"

static QLineEdit *line_edit = 0;

dooble_label_widget::dooble_label_widget
(const QString &text, QWidget *parent):QLabel(text, parent)
{
  if(!line_edit)
    line_edit = new QLineEdit(0);

  m_original_stylesheet = styleSheet();
}

dooble_label_widget::~dooble_label_widget()
{
}

void dooble_label_widget::enterEvent(QEvent *event)
{
  QPalette palette(line_edit->palette());

  setStyleSheet
    (QString("QLabel {background-color: %1; color: white}").
     arg(palette.color(QPalette::Highlight).name()));
  QLabel::enterEvent(event);
}

void dooble_label_widget::leaveEvent(QEvent *event)
{
  setStyleSheet(m_original_stylesheet);
  QLabel::leaveEvent(event);
}

void dooble_label_widget::mouseReleaseEvent(QMouseEvent *event)
{
  if(event)
    if(rect().contains(event->pos()))
      {
	emit clicked();
	hide();
      }

  QLabel::mouseReleaseEvent(event);
}
