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
#include <QSettings>
#include <QResizeEvent>

#include "dmisc.h"
#include "dooble.h"
#include "dsettings.h"
#include "dsettingshomelinewidget.h"

dsettingshomelinewidget::dsettingshomelinewidget(QWidget *parent):
  QLineEdit(parent)
{
  setMaxLength(2500);
  copyAndPasteToolButton = new QToolButton(this);
  copyAndPasteToolButton->setMenu(new QMenu(this));
  copyAndPasteToolButton->setToolTip(tr("Select URL"));
  copyAndPasteToolButton->setIconSize(QSize(16, 16));
  copyAndPasteToolButton->setCursor(Qt::ArrowCursor);
  copyAndPasteToolButton->setPopupMode(QToolButton::MenuButtonPopup);
  copyAndPasteToolButton->setStyleSheet
    ("QToolButton {border: none; "
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
  slotSetIcons();
  connect(copyAndPasteToolButton,
	  SIGNAL(clicked(void)),
	  copyAndPasteToolButton,
	  SLOT(showMenu(void)));
  connect(copyAndPasteToolButton->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slotSetUrls(void)));

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet
    (QString("QLineEdit {padding-right: %1px; padding-left: %2px; "
	     "selection-background-color: darkgray;}").arg
     (copyAndPasteToolButton->sizeHint().width() + frameWidth + 5).arg
     (0));
  setMinimumHeight(sizeHint().height() + 10);
}

void dsettingshomelinewidget::resizeEvent(QResizeEvent *event)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = copyAndPasteToolButton->sizeHint();

  copyAndPasteToolButton->move
    (rect().right() - frameWidth - size1.width() - 5,
     (rect().bottom() + 2 - size1.height()) / 2);
  QLineEdit::resizeEvent(event);
}

void dsettingshomelinewidget::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("settingsWindow");
  copyAndPasteToolButton->setIcon
    (QIcon(settings.value("copyAndPasteToolButtonIcon").toString()));
}

void dsettingshomelinewidget::slotMenuActionTriggered(QAction *action)
{
  if(action)
    {
      setText(action->data().toUrl().toString(QUrl::StripTrailingSlash));
      setCursorPosition(0);
    }
}

void dsettingshomelinewidget::slotSetUrls(void)
{
  QList<QVariantList> urls;

  if(dooble::s_settingsWindow->parentDooble())
    urls = dooble::s_settingsWindow->parentDooble()->tabUrls();

  if(!urls.isEmpty())
    {
      while(!copyAndPasteToolButton->menu()->actions().isEmpty())
	{
	  QAction *action = copyAndPasteToolButton->menu()->actions().first();

	  copyAndPasteToolButton->menu()->removeAction(action);
	  action->deleteLater();
	}

      for(int i = 0; i < urls.size(); i++)
	{
	  if(urls.at(i).size() < 3)
	    continue;

	  QIcon icon;
	  QAction *action = 0;

	  action = new QAction(urls.at(i).at(0).toString(), this);
	  action->setData(urls.at(i).at(1).toUrl());
	  icon = urls.at(i).at(2).value<QIcon> ();

	  if(icon.isNull())
	    icon = dmisc::iconForUrl(action->data().toUrl());

	  action->setIcon(icon);
	  copyAndPasteToolButton->menu()->addAction(action);
	}

      connect(copyAndPasteToolButton->menu(),
	      SIGNAL(triggered(QAction *)), this,
	      SLOT(slotMenuActionTriggered(QAction *)));
      copyAndPasteToolButton->menu()->update();
    }
}
