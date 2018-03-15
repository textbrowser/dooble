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

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QSettings>
#include <QSizeGrip>
#include <QWidget>
#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
#if QT_VERSION < 0x050000 && defined(Q_WS_X11)
#include <QX11EmbedContainer>
#endif
#endif

#include "ddesktopwidget.h"
#include "dmisc.h"
#include "dooble.h"
#include "ui_dapplicationPropertiesWindow.h"

ddesktopwidget::ddesktopwidget(QWidget *parent):QMdiArea(parent)
{
  m_action = 0;

  if(parent)
    connect(parent,
	    SIGNAL(updateDesktopBackground(void)),
	    this,
	    SLOT(slotBackgroundImageChanged(void)));

  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  QFileInfo fileInfo
    (dooble::s_settings.value("desktopBackground", "").toString());

  if(fileInfo.isReadable())
    setBackground(QImage(dooble::s_settings.value("desktopBackground", "").
			 toString()));
  else
    {
      QDir currentDir(QDir::currentPath());
      QString fileName
	(QString("%1/Images/doobledesktop_background_rainbow.png").
	 arg(currentDir.path()));
      QSettings settings;

      settings.setValue("desktopBackground", fileName);
      dooble::s_settings["desktopBackground"] = fileName;
      setBackground(QImage(fileName));
    }

  showFileManagerWindow();
  setVisible(false);
}

ddesktopwidget::~ddesktopwidget()
{
  if(m_action)
    m_action->deleteLater();
}

void ddesktopwidget::mousePressEvent(QMouseEvent *event)
{
  if(event && event->button() == Qt::RightButton)
    {
      QMenu menu(this);

      menu.addAction(tr("&Change Desktop Background..."),
		     this, SLOT(slotChangeDesktopBackground(void)));
#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
#if QT_VERSION < 0x050000
      QFileInfo fileInfo("/usr/bin/xterm");

      if(fileInfo.isExecutable() && fileInfo.isReadable())
	{
	  menu.addSeparator();
	  menu.addAction(tr("&Launch Terminal..."),
			 this, SLOT(slotLaunchTerminal(void)));
	}
#endif
#endif
      menu.exec(QCursor::pos());
    }

  QMdiArea::mousePressEvent(event);
}

void ddesktopwidget::slotChangeDesktopBackground(void)
{
  QFileDialog *fileDialog = new QFileDialog(this);

  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_OS_MAC
  fileDialog->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog->setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog->setWindowTitle(tr("Dooble Web Browser: Desktop "
				"Background Image Selection"));
  fileDialog->setFileMode(QFileDialog::ExistingFile);
  fileDialog->setLabelText(QFileDialog::Accept, tr("Select"));
  fileDialog->setDirectory(QDir::currentPath());

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  connect(fileDialog,
	  SIGNAL(finished(int)),
	  this,
	  SLOT(slotBackgroundDialogFinished(int)));
  fileDialog->show();
}

void ddesktopwidget::slotBackgroundImageChanged(void)
{
  QFileInfo fileInfo
    (dooble::s_settings.value("desktopBackground", "").toString());

  if(fileInfo.isReadable())
    {
      setBackground
	(QImage(dooble::s_settings.value("desktopBackground", "").
		toString()));
      setVisible(false);
      setVisible(true);
    }
}

void ddesktopwidget::slotAddApplicationIcon(void)
{
  QDialog *dialog = new QDialog(this);
  QMdiSubWindow *launchApplicationWindow = new QMdiSubWindow(this);
  Ui_applicationPropertiesWindow propertiesUI;

  dialog->setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_OS_MAC
  dialog->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  propertiesUI.setupUi(dialog);
  dialog->setSizeGripEnabled(true);
  launchApplicationWindow->setWidget(dialog);
  launchApplicationWindow->adjustSize();
  launchApplicationWindow->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog,
	  SIGNAL(finished(int)),
	  launchApplicationWindow,
	  SLOT(deleteLater(void)));
  connect(launchApplicationWindow,
	  SIGNAL(destroyed(void)),
	  dialog,
	  SLOT(deleteLater(void)));
  connect(launchApplicationWindow,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(update(void)));

  QPoint p(0, 0);

  if(width() >= launchApplicationWindow->width())
    p.setX(width() / 2 - launchApplicationWindow->width() / 2);
  else
    p.setX(launchApplicationWindow->width() / 2 - width() / 2);

  if(height() >= launchApplicationWindow->height())
    p.setY(height() / 2 - launchApplicationWindow->height() / 2);
  else
    p.setY(launchApplicationWindow->height() / 2 - height() / 2);

  launchApplicationWindow->move(p);
  launchApplicationWindow->showNormal();
}

void ddesktopwidget::slotOK(void)
{
}

void ddesktopwidget::showFileManagerWindow(const QUrl &url)
{
  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);
  QMdiSubWindow *fileManagerWindow = new QMdiSubWindow(this);

  Q_UNUSED(new QSizeGrip(fileManagerWindow));
  fileManagerWindow->setWindowTitle
    (tr("Dooble Web Browser: File Manager"));
  fileManagerWindow->setWindowIcon
    (QIcon(settings.value("windowIcon").toString()));

  dfilemanager *fileManager = new dfilemanager(fileManagerWindow);

  connect(fileManagerWindow,
	  SIGNAL(destroyed(void)),
	  fileManager,
	  SLOT(deleteLater(void)));
  connect(fileManagerWindow,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(update(void)));
  fileManagerWindow->setWidget(fileManager);
  fileManagerWindow->setAttribute(Qt::WA_DeleteOnClose);
  fileManagerWindow->adjustSize();

  if(url.isEmpty())
    {
      QUrl url;
      QString path(dooble::s_settings.value("settingsWindow/myRetrievedFiles",
					    "").toString());

      url = QUrl::fromLocalFile(path);
      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      fileManager->load(url);
    }
  else
    fileManager->load(url);

  fileManagerWindow->showNormal();
}

#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
#if QT_VERSION < 0x050000 && defined(Q_WS_X11)
void ddesktopwidget::slotLaunchTerminal(void)
{
  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);
  QMdiSubWindow *terminalWindow = new QMdiSubWindow(this);
  QX11EmbedWidget *container = new QX11EmbedWidget(this);

  connect(terminalWindow,
	  SIGNAL(destroyed(void)),
	  container,
	  SLOT(deleteLater(void)));
  connect(terminalWindow,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(update(void)));
  terminalWindow->setWindowTitle
    (tr("Dooble Web Browser: Terminal"));
  terminalWindow->setWindowIcon
    (QIcon(settings.value("windowIcon").toString()));
  terminalWindow->setWidget(container);
  terminalWindow->setAttribute(Qt::WA_DeleteOnClose);
  terminalWindow->setWindowFlags
    ((terminalWindow->windowFlags() | Qt::CustomizeWindowHint) &
     ~Qt::WindowMaximizeButtonHint);
  terminalWindow->show();

  QProcess *process = new QProcess(this);

  connect(process,
	  SIGNAL(error(QProcess::ProcessError)),
	  terminalWindow,
	  SLOT(deleteLater(void)));
  connect(process,
	  SIGNAL(finished(int, QProcess::ExitStatus)),
	  terminalWindow,
	  SLOT(deleteLater(void)));

  QStringList arguments;

  arguments << "-sb";
  arguments << "-into" << QString::number(container->winId());
  process->start("/usr/bin/xterm", arguments);
  connect(terminalWindow,
	  SIGNAL(destroyed(QObject *)),
	  process,
	  SLOT(kill(void)));
  connect(process,
	  SIGNAL(finished(int, QProcess::ExitStatus)),
	  process,
	  SLOT(deleteLater(void)));
  terminalWindow->resize(530, 370);

  /*
  ** Enable a simple focus to prevent premature exits.
  */

  terminalWindow->setFocusPolicy(Qt::ClickFocus);
}
#else
void ddesktopwidget::slotLaunchTerminal(void)
{
}
#endif
#else
void ddesktopwidget::slotLaunchTerminal(void)
{
}
#endif

void ddesktopwidget::enterEvent(QEvent *event)
{
  QMdiArea::enterEvent(event);
}

void ddesktopwidget::setTabAction(QAction *action)
{
  if(m_action)
    {
      removeAction(m_action);
      m_action->deleteLater();
    }

  m_action = action;

  if(m_action)
    addAction(m_action);
}

QAction *ddesktopwidget::tabAction(void) const
{
  return m_action;
}

void ddesktopwidget::slotBackgroundDialogFinished(int result)
{
  QFileDialog *fileDialog = qobject_cast<QFileDialog *> (sender());

  if(fileDialog && result == QDialog::Accepted)
    {
      QString fileName("");
      QStringList list(fileDialog->selectedFiles());

      /*
      ** list.size() is 0 under Mac OS X for Qt 4.6.3. Why?
      ** Here's a workaround.
      */

#ifdef Q_OS_MAC
      if(list.isEmpty())
	{
	  QString directoryName("");

	  foreach(QWidget *w, fileDialog->findChildren<QWidget *> ())
	    {
	      if(qobject_cast<QLineEdit *> (w))
		fileName = qobject_cast<QLineEdit *> (w)->text().trimmed();
	      else if(qobject_cast<QComboBox *> (w) &&
		      QFileInfo(qobject_cast<QComboBox *> (w)->
				currentText()).exists())
		directoryName = qobject_cast<QComboBox *> (w)->currentText();

	      if(!fileName.isEmpty() && !directoryName.isEmpty())
		break;
	    }

	  fileName = directoryName + QDir::separator() + fileName;
	}
      else
	fileName = list.at(0);
#else
      if(!list.isEmpty())
	fileName = list.at(0);
#endif

      if(!fileName.isEmpty())
	{
	  QSettings settings;

	  settings.setValue("desktopBackground", fileName);
	  dooble::s_settings["desktopBackground"] = fileName;
	  setBackground(QImage(fileName));
	  setVisible(false);
	  setVisible(true);
	  emit backgroundImageChanged();
	}
    }

  if(fileDialog)
    fileDialog->deleteLater();
}
