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

#include <QKeyEvent>
#include <QSettings>
#include <QScrollBar>

#include "dmisc.h"
#include "dooble.h"
#include "derrorlog.h"

derrorlog::derrorlog(void):QMainWindow()
{
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  m_findLineEditPalette = ui.findLineEdit->palette();
  connect(ui.action_Close,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotClose(void)));
  connect(ui.actionClear_Log,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotClear(void)));
  connect(ui.action_Find, SIGNAL(triggered(void)), this,
	  SLOT(slotShowFind(void)));
  connect(ui.hideFindToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotHideFind(void)));
  connect(ui.findLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotNextFind(void)));
  connect(ui.findLineEdit, SIGNAL(textEdited(const QString &)), this,
	  SLOT(slotNextFind(const QString &)));
  connect(ui.nextToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotNextFind(void)));
  connect(ui.previousToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotPreviousFind(void)));

  /*
  ** The logError() method may be called from multiple threads.
  */

  connect(this,
	  SIGNAL(appendToLog(const QString &)),
	  this,
	  SLOT(slotAppendToLog(const QString &)),
	  Qt::QueuedConnection);

  /*
  ** We can't call slotSetIcons() here because we may not be in the
  ** proper state.
  */

  slotHideFind();
}

void derrorlog::saveState(void)
{
  /*
  ** geometry() may return (0, 0) coordinates if the window is
  ** not visible.
  */

  if(!isVisible())
    return;

  if(dmisc::isGnome())
    dooble::s_settings["errorLog/geometry"] = geometry();
  else
    dooble::s_settings["errorLog/geometry"] = saveGeometry();

  QSettings settings;

  if(dmisc::isGnome())
    settings.setValue("errorLog/geometry", geometry());
  else
    settings.setValue("errorLog/geometry", saveGeometry());
}

void derrorlog::slotClose(void)
{
  close();
}

void derrorlog::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void derrorlog::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape &&
	 ui.findFrame->isVisible())
	ui.findFrame->setVisible(false);
      else if(event->key() == Qt::Key_Escape)
	close();
    }

  QMainWindow::keyPressEvent(event);
}

void derrorlog::slotShow(void)
{
  QAction *action = qobject_cast<QAction *> (sender());
  QRect rect(100, 100, 800, 600);
  QToolButton *toolButton = qobject_cast<QToolButton *> (sender());
  QWidget *parent = 0;

  if(action && action->parentWidget())
    {
      parent = action->parentWidget();
      rect = action->parentWidget()->geometry();
      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }
  else if(toolButton && toolButton->parentWidget())
    {
      parent = toolButton->parentWidget();
      rect = parent->geometry();

      while(parent)
	{
	  if(qobject_cast<dooble *> (parent))
	    {
	      rect = parent->geometry();
	      break;
	    }

	  parent = parent->parentWidget();
	}

      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }

  if(!isVisible())
    {
      /*
      ** Don't annoy the user.
      */

      if(dooble::s_settings.contains("errorLog/geometry"))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.value("errorLog/geometry",
						 rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.value("errorLog/geometry").
			   toByteArray());

	      if(!restoreGeometry(g))
		setGeometry(rect);
	    }
	}
      else
	setGeometry(rect);
    }

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, parent);

  showNormal();
  raise();
}

void derrorlog::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("errorLog");
  ui.actionClear_Log->setIcon
    (QIcon(settings.value("actionClearLog").toString()));
  ui.action_Close->setIcon(QIcon(settings.value("actionClose").toString()));
  ui.action_Find->setIcon(QIcon(settings.value("actionFind").toString()));
  ui.hideFindToolButton->setIcon
    (QIcon(settings.value("hideFindToolButton").toString()));
  ui.nextToolButton->setIcon
    (QIcon(settings.value("nextToolButton").toString()));
  ui.previousToolButton->setIcon
    (QIcon(settings.value("previousToolButton").toString()));
  setWindowIcon(QIcon(settings.value("windowIcon").toString()));
}

void derrorlog::logError(const QString &error)
{
  emit appendToLog(error);
}

bool derrorlog::hasErrors(void) const
{
  return !ui.textBrowser->document()->isEmpty();
}

void derrorlog::slotAppendToLog(const QString &error)
{
  /*
  ** Ignore duplicate errors.
  */

  if(error == m_lastError)
    return;

  m_lastError = error;

  QString text("");
  QDateTime now(QDateTime::currentDateTime());

  text.append(now.toString(Qt::ISODate));
  text.append("\r");
  text.append(error);
  text.append("\r");
  ui.textBrowser->append(text);

  if(dooble::s_settings.value("settingsWindow/notifyOfExceptions",
			      true).toBool())
    emit errorLogged();
}

void derrorlog::slotClear(void)
{
  m_lastError.clear();
  ui.textBrowser->clear();
}

void derrorlog::slotNextFind(void)
{
  slotNextFind(ui.findLineEdit->text());
}

void derrorlog::slotNextFind(const QString &text)
{
  QTextDocument::FindFlags findFlags = 0;

  if(ui.matchCaseCheckBox->isChecked())
    findFlags |= QTextDocument::FindCaseSensitively;

  if(ui.textBrowser->find(text, findFlags) || text.isEmpty())
    {
      ui.findLineEdit->setPalette(m_findLineEditPalette);

      if(text.isEmpty())
        ui.textBrowser->moveCursor(QTextCursor::PreviousCharacter);
    }
  else
    {
      if(ui.textBrowser->textCursor().anchor() ==
	 ui.textBrowser->textCursor().position())
	{
	  if(!ui.textBrowser->textCursor().atEnd())
	    {
	      QColor color(240, 128, 128); // Light Coral
	      QPalette palette(ui.findLineEdit->palette());

	      palette.setColor(ui.findLineEdit->backgroundRole(), color);
	      ui.findLineEdit->setPalette(palette);
	    }
	  else
	    ui.textBrowser->moveCursor(QTextCursor::Start);
	}
      else
	{
	  ui.textBrowser->moveCursor(QTextCursor::Start);
	  slotNextFind(text);
	}
    }
}

void derrorlog::slotPreviousFind(void)
{
  slotPreviousFind(ui.findLineEdit->text());
}

void derrorlog::slotPreviousFind(const QString &text)
{
  QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;

  if(ui.matchCaseCheckBox->isChecked())
    findFlags |= QTextDocument::FindCaseSensitively;

  if(ui.textBrowser->find(text, findFlags) || text.isEmpty())
    ui.findLineEdit->setPalette(m_findLineEditPalette);
  else
    {
      if(ui.textBrowser->textCursor().anchor() ==
	 ui.textBrowser->textCursor().position())
	{
	  if(!ui.textBrowser->textCursor().atEnd())
	    {
	      QColor color(240, 128, 128); // Light Coral
	      QPalette palette(ui.findLineEdit->palette());

	      palette.setColor(ui.findLineEdit->backgroundRole(), color);
	      ui.findLineEdit->setPalette(palette);
	    }
	  else
	    ui.textBrowser->moveCursor(QTextCursor::End);
	}
      else
	{
	  ui.textBrowser->moveCursor(QTextCursor::End);
	  slotPreviousFind(text);
	}
    }
}

void derrorlog::slotHideFind(void)
{
  ui.findFrame->setVisible(false);
}

void derrorlog::slotShowFind(void)
{
  ui.findFrame->setVisible(true);
  ui.findLineEdit->setFocus();
  ui.findLineEdit->selectAll();

#ifdef Q_OS_MAC
  static int fixed = 0;

  if(!fixed)
    {
      QColor color(255, 255, 255);
      QPalette palette(ui.findLineEdit->palette());

      palette.setColor(ui.findLineEdit->backgroundRole(), color);
      ui.findLineEdit->setPalette(palette);
      fixed = 1;
    }
#endif
}

void derrorlog::clear(void)
{
  slotClear();
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool derrorlog::event(QEvent *event)
{
  if(event)
    if(event->type() == QEvent::WindowStateChange)
      if(windowState() == Qt::WindowNoState)
	{
	  /*
	  ** Minimizing the window on OS 10.6.8 and Qt 5.x will cause
	  ** the window to become stale once it has resurfaced.
	  */

	  hide();
	  show();
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool derrorlog::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool derrorlog::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
