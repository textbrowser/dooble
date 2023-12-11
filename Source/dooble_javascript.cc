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

#include <QPushButton>
#include <QShortcut>
#include <QWebEnginePage>

#include "dooble_javascript.h"

dooble_javascript::dooble_javascript(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  m_ui.buttons->button(QDialogButtonBox::Ok)->setText(tr("&Execute!"));
  connect(m_ui.buttons->button(QDialogButtonBox::Ok),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_execute(void)));
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
}

void dooble_javascript::set_page(QWebEnginePage *page)
{
  if(m_page)
    return;
  else if(page)
    {
      connect(page,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(deleteLater(void)));
      connect(page,
	      SIGNAL(titleChanged(const QString &)),
	      this,
	      SLOT(slot_title_changed(const QString &)));
      connect(page,
	      SIGNAL(urlChanged(const QUrl &)),
	      this,
	      SLOT(slot_url_changed(const QUrl &)));
      m_page = page;
      setWindowTitle
	(m_page->title().trimmed().isEmpty() ?
	 tr("Dooble: JavaScript Console") :
	 tr("%1 - Dooble: JavaScript Console").arg(m_page->title().trimmed()));
      slot_url_changed(m_page->url());
    }
}

void dooble_javascript::slot_execute(void)
{
  if(!m_page)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_page->runJavaScript(m_ui.text->toPlainText().trimmed());
  QApplication::restoreOverrideCursor();
}

void dooble_javascript::slot_title_changed(const QString &title)
{
  setWindowTitle
    (title.trimmed().isEmpty() ?
     tr("Dooble: JavaScript Console") :
     tr("%1 - Dooble: JavaScript Console").arg(title.trimmed()));
}

void dooble_javascript::slot_url_changed(const QUrl &url)
{
  m_ui.url->setText(url.toString());
  m_ui.url->setCursorPosition(0);
}
