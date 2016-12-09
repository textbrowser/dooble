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

#include "dmisc.h"
#include "dooble.h"
#include "dwebview.h"

dwebview::dwebview(QWidget *parent):QWebEngineView(parent)
{
  m_allowPopup = false;
  m_lastButtonPressed = Qt::NoButton;
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slotLoadStarted(void)));
}

bool dwebview::checkAndClearPopup(void)
{
  bool allowPopup = m_allowPopup;

  m_allowPopup = false;
  return allowPopup;
}

void dwebview::mousePressEvent(QMouseEvent *event)
{
  /*
  ** We use this method to determine valid popups.
  */

  m_allowPopup = true;
  m_lastButtonPressed = event ? event->button() : m_lastButtonPressed;
  QWebEngineView::mousePressEvent(event);
}

Qt::MouseButton dwebview::mouseButtonPressed(void) const
{
  return m_lastButtonPressed;
}

void dwebview::slotLoadStarted(void)
{
  m_lastButtonPressed = Qt::NoButton;
}
