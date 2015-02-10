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
#include <QSettings>
#include <QResizeEvent>

#include "dooble.h"
#include "dsearchwidget.h"

dsearchwidget::dsearchwidget(QWidget *parent):QLineEdit(parent)
{
  setMaxLength(2500);
  findToolButton = new QToolButton(this);
  findToolButton->setToolTip(tr("Initiate Search"));
  findToolButton->setIconSize(QSize(16, 16));
  findToolButton->setCursor(Qt::ArrowCursor);
  findToolButton->setStyleSheet("QToolButton {"
				"border: none; "
				"padding-top: 0px; "
				"padding-bottom: 0px; "
				"}");
  pulldownToolButton = new QToolButton(this);
  pulldownToolButton->setCursor(Qt::ArrowCursor);
  pulldownToolButton->setIcon(QIcon("Icons/16x16/dooble.png"));
  pulldownToolButton->setIconSize(QSize(16, 16));
  pulldownToolButton->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
#ifdef Q_OS_MAC
     "padding-right: 10px; "
#else
     "padding-right: 5px; "
#endif
     "}"
#ifdef Q_OS_MAC
     "QToolButton::menu-button {border: none;}"
#else
     "QToolButton::menu-arrow {image: none;}"
     "QToolButton::menu-button {border: none;}"
#endif
     );

  QMenu *menu = new QMenu(this);

  m_type = "history";
  menu->setActiveAction(menu->addAction(QIcon("Icons/16x16/dooble.png"),
					"History"));
  menu->addSeparator();
  menu->addAction(QIcon("Icons/16x16/blekko.png"), "Blekko");
  menu->addAction(QIcon("Icons/16x16/dogpile.png"), "Dogpile");
  menu->addAction(QIcon("Icons/16x16/duckduckgo.png"), "DuckDuckGo");
  menu->addAction(QIcon("Icons/16x16/google.png"), "Google");
  menu->addAction(QIcon("Icons/16x16/ixquick.png"), "Ixquick");
  menu->addAction(QIcon("Icons/16x16/metager.png"), "MetaGer");
  menu->addAction(QIcon("Icons/16x16/startpage.png"), "Startpage");
  menu->addAction(QIcon("Icons/16x16/wikinews.png"), "Wikinews");
  menu->addAction(QIcon("Icons/16x16/wikipedia.png"), "Wikipedia");
  menu->addAction(QIcon("Icons/16x16/wolframalpha.png"), "WolframAlpha");
  menu->addAction(QIcon("Icons/16x16/yacy.png"), "YaCy");
  setPlaceholderText(menu->activeAction()->text());
  connect(menu, SIGNAL(triggered(QAction *)), this,
	  SLOT(slotSearchTypeChanged(QAction *)));
  pulldownToolButton->setMenu(menu);
  pulldownToolButton->setPopupMode(QToolButton::MenuButtonPopup);
  connect(pulldownToolButton, SIGNAL(clicked(void)),
	  pulldownToolButton, SLOT(showMenu(void)));
  slotSetIcons();

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet
    (QString("QLineEdit {padding-right: %1px; padding-left: %2px; "
	     "selection-background-color: darkgray;}").arg
     (findToolButton->sizeHint().width() + frameWidth + 5).arg
     (pulldownToolButton->sizeHint().width() + frameWidth + 5));
  setMinimumHeight(sizeHint().height() + 10);

  if(dooble::s_settings.contains("mainWindow/searchName"))
    {
      QString searchName(dooble::s_settings.value("mainWindow/searchName").
			 toString());

      for(int i = 0; i < menu->actions().size(); i++)
	if(menu->actions().at(i)->text() == searchName)
	  {
	    /*
	    ** Please note that "mainWindow/searchName" will be set
	    ** again in slotSearchTypeChanged().
	    */

	    slotSearchTypeChanged(menu->actions().at(i));
	    break;
	  }
    }
}

void dsearchwidget::resizeEvent(QResizeEvent *event)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = findToolButton->sizeHint();
  QSize size2 = pulldownToolButton->sizeHint();

  findToolButton->move
    (rect().right() - frameWidth - size1.width() - 5,
     (rect().bottom() + 2 - size1.height()) / 2);
  pulldownToolButton->move
    (frameWidth - rect().left() + 6,
     (rect().bottom() + 2 - size2.height()) / 2);
  QLineEdit::resizeEvent(event);
}

QString dsearchwidget::type(void) const
{
  if(m_type.isEmpty())
    return "history";
  else
    return m_type;
}

QToolButton *dsearchwidget::findButton(void) const
{
  return findToolButton;
}

void dsearchwidget::slotSearchTypeChanged(QAction *action)
{
  if(!action)
    return;

  setPlaceholderText(action->text());
  m_type = action->text().toLower();
  pulldownToolButton->setIcon(action->icon());
  update();

  QSettings settings;

  settings.setValue("mainWindow/searchName", action->text());
  dooble::s_settings["mainWindow/searchName"] = action->text();
}

void dsearchwidget::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("searchWidget");
  findToolButton->setIcon
    (QIcon(settings.value("findToolButton").toString()));
}

void dsearchwidget::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      /*
      ** Delegate the event to the parent. If the Bookmarks menu has
      ** not been made visible, the shortcut will not be available.
      */

      if(QKeySequence(event->modifiers() + event->key()) ==
	 QKeySequence(Qt::ControlModifier + Qt::Key_B))
	{
	  event->ignore();
	  return;
	}
      else if(QKeySequence(event->modifiers() + event->key()) ==
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
