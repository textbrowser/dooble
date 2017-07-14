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
#include <QStackedWidget>

#include "dooble.h"
#include "dooble_page.h"
#include "dooble_web_engine_view.h"

dooble_page::dooble_page(QWidget *parent):QWidget(parent)
{
  m_ui.setupUi(this);
  m_ui.menus->setMenu(new QMenu(this));
  m_view = new dooble_web_engine_view(this);
  m_ui.frame->layout()->addWidget(m_view);
  connect(m_ui.menus,
	  SIGNAL(clicked(void)),
	  m_ui.menus,
	  SLOT(showMenu(void)));
  connect(m_ui.menus->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_standard_menus(void)));
  connect(m_view,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SIGNAL(titleChanged(const QString &)));
  prepare_icons();
  slot_prepare_standard_menus(); // Enables shortcuts.
}

void dooble_page::prepare_icons(void)
{
  QString icon_set(dooble::setting("icon_set").toString());

  m_ui.backward->setIcon(QIcon(QString(":/%1/backward.png").arg(icon_set)));
  m_ui.forward->setIcon(QIcon(QString(":/%1/forward.png").arg(icon_set)));
  m_ui.menus->setIcon(QIcon(QString(":/%1/menu.png").arg(icon_set)));
  m_ui.reload->setIcon(QIcon(QString(":/%1/reload.png").arg(icon_set)));
}

void dooble_page::slot_prepare_standard_menus(void)
{
  m_ui.menus->menu()->clear();

  QAction *action = 0;
  QMenu *menu = 0;

  menu = m_ui.menus->menu()->addMenu("&File");
  menu->addAction("New &Tab",
		  this,
		  SIGNAL(new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction("&New Window",
		  this,
		  SIGNAL(new_window(void)),
		  QKeySequence(tr("Ctrl+N")));
  menu->addSeparator();
  action = menu->addAction("&Close Tab",
			   this,
			   SIGNAL(close_tab(void)),
			   QKeySequence(tr("Ctrl+W")));

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    action->setEnabled
      (qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  menu->addSeparator();
  menu->addAction("E&xit Dooble",
		  this,
		  SIGNAL(quit_dooble(void)),
		  QKeySequence(tr("Ctrl+Q")));
}
