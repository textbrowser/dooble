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
#include <QStyle>
#include <QToolButton>

#include "dooble.h"
#include "dooble_address_widget.h"
#include "dooble_settings.h"

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
  m_information = new QToolButton(this);
  m_information->setCursor(Qt::ArrowCursor);
  m_information->setIconSize(QSize(16, 16));
  m_information->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
  m_information->setToolTip(tr("Site Information"));
  m_line = new QFrame(this);
  m_line->setFrameShadow(QFrame::Sunken);
  m_line->setFrameShape(QFrame::VLine);
  m_line->setLineWidth(1);
  m_line->setMaximumHeight(sizeHint().height() - 10);
  m_line->setMaximumWidth(5);
  m_line->setMinimumWidth(5);
  m_menu = new QMenu(this);
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
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_information,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_show_site_information_menu(void)));
  connect(m_pull_down,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(pull_down_clicked(void)));
  prepare_icons();
  setMinimumHeight(sizeHint().height());
  setStyleSheet
    (QString("QLineEdit {padding-left: %1px; padding-right: %2px;}").
     arg(m_bookmark->sizeHint().width() +
	 m_information->sizeHint().width() +
	 frame_width + 10).
     arg(m_pull_down->sizeHint().width() + frame_width + 5));
}

QMenu *dooble_address_widget::menu(void) const
{
  return m_menu;
}

void dooble_address_widget::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_bookmark->setIcon(QIcon(QString(":/%1/16/bookmark.png").arg(icon_set)));
  m_information->setIcon
    (QIcon(QString(":/%1/16/information.png").arg(icon_set)));
  m_pull_down->setIcon(QIcon(QString(":/%1/16/pulldown.png").arg(icon_set)));
}

void dooble_address_widget::resizeEvent(QResizeEvent *event)
{
  int d = 0;
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = m_bookmark->sizeHint();
  QSize size2 = m_information->sizeHint();
  QSize size3 = m_pull_down->sizeHint();

  d = (rect().height() - size1.height()) / 2;
  m_bookmark->move(frame_width - rect().left() + size2.width() + 5,
		   rect().top() + d);
  d = (rect().height() - size2.height()) / 2;
  m_information->move(frame_width - rect().left() + 5, rect().top() + d);
  m_line->move(frame_width - rect().left() + size1.width() + size2.width() + 5,
	       rect().top() + 5);
  d = (rect().height() - size3.height()) / 2;
  m_pull_down->move
    (rect().right() - frame_width - size3.width() - 5, rect().top() + d);

  if(selectedText().isEmpty())
    setCursorPosition(0);

  QLineEdit::resizeEvent(event);
}

void dooble_address_widget::setText(const QString &text)
{
  QLineEdit::setText(text.trimmed());
  setCursorPosition(0);
  setToolTip(QLineEdit::text());
}

void dooble_address_widget::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_address_widget::slot_show_site_information_menu(void)
{
  QMenu menu(this);

  menu.addAction(tr("Show Site &Cookies..."), this, SIGNAL(show_cookies(void)));
  menu.exec(QCursor::pos());
}
