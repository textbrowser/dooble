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

#include "dooble.h"
#include "derrorlog.h"
#include "dnetworkcache.h"
#include "dbookmarkswindow.h"
#include "dclearcontainers.h"

dclearcontainers::dclearcontainers(void):QMainWindow()
{
  m_parent = 0;
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
  statusBar()->setSizeGripEnabled(false);
#endif
  connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)),
	  this, SLOT(slotClicked(QAbstractButton *)));

  foreach(QCheckBox *checkBox, findChildren<QCheckBox *> ())
    connect(checkBox,
	    SIGNAL(toggled(bool)),
	    this,
	    SLOT(slotChecked(bool)));

  slotSetIcons();
}

void dclearcontainers::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dclearcontainers::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
    }

  QMainWindow::keyPressEvent(event);
}

void dclearcontainers::show(dooble *parent)
{
  m_parent = parent;
  disconnect(this, SIGNAL(clearHistory(void)));

  if(parent)
    connect(this,
	    SIGNAL(clearHistory(void)),
	    parent,
	    SLOT(slotClearHistory(void)));

  QSettings settings;

  ui.bookmarksCheckBox->setChecked
    (settings.value("clearContainersWindow/bookmarks", false).toBool());
  ui.cookiesCheckBox->setChecked
    (settings.value("clearContainersWindow/cookies", false).toBool());
  ui.downloadCheckBox->setChecked
    (settings.value("clearContainersWindow/downloadInformation",
		    false).toBool());
  ui.errorLogCheckBox->setChecked
    (settings.value("clearContainersWindow/errorLog", false).toBool());
  ui.exceptionsCheckBox->setChecked
    (settings.value("clearContainersWindow/exceptions", false).toBool());
  ui.faviconsCheckBox->setChecked
    (settings.value("clearContainersWindow/favicons", false).toBool());
  ui.historyCheckBox->setChecked
    (settings.value("clearContainersWindow/history", false).toBool());
  ui.offlineCacheCheckBox->setChecked
    (settings.value("clearContainersWindow/offlineCache", false).toBool());
  ui.spotonCheckBox->setChecked
    (settings.value("clearContainersWindow/spoton", false).toBool());

  if(parent)
    {
      if(parent->height() == height() &&
	 parent->width() == width())
	setGeometry(parent->geometry());
      else
	{
	  QPoint p(parent->pos());
	  int X = 0;
	  int Y = 0;

	  if(parent->width() >= width())
	    X = p.x() + (parent->width() - width()) / 2;
	  else
	    X = p.x() - (width() - parent->width()) / 2;

	  if(parent && parent->height() >= height())
	    Y = p.y() + (parent->height() - height()) / 2;
	  else
	    Y = p.y() - (height() - parent->height()) / 2;

	  move(X, Y);
	}
    }
  else
    move(100, 100);

  showNormal();
  activateWindow();
  raise();
}

void dclearcontainers::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon(QIcon(settings.value("clearContainers/windowIcon").
		      toString()));

  foreach(QPushButton *button,
	  ui.buttonBox->findChildren<QPushButton *>())
    {
      button->setIconSize(QSize(16, 16));

      if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole ||
	 ui.buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole ||
	 ui.buttonBox->buttonRole(button) == QDialogButtonBox::YesRole)
	button->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
      else
	button->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
    }
}

void dclearcontainers::slotClicked(QAbstractButton *button)
{
  if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole ||
     ui.buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole ||
     ui.buttonBox->buttonRole(button) == QDialogButtonBox::YesRole)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      if(ui.bookmarksCheckBox->isChecked())
	dooble::s_bookmarksWindow->clear();

      if(ui.cookiesCheckBox->isChecked())
	emit clearCookies();

      if(ui.downloadCheckBox->isChecked())
	dooble::s_downloadWindow->clear();

      if(ui.errorLogCheckBox->isChecked())
	dooble::s_errorLog->clear();

      if(ui.exceptionsCheckBox->isChecked())
	{
	  dooble::s_adBlockWindow->clear();
	  dooble::s_alwaysHttpsExceptionsWindow->clear();
	  dooble::s_cacheExceptionsWindow->clear();
	  dooble::s_cookiesBlockWindow->clear();
	  dooble::s_dntWindow->clear();
	  dooble::s_httpOnlyExceptionsWindow->clear();
	  dooble::s_httpRedirectWindow->clear();
	  dooble::s_httpReferrerWindow->clear();
	  dooble::s_imageBlockWindow->clear();
	  dooble::s_javaScriptExceptionsWindow->clear();
	  dooble::s_popupsWindow->clear();
	  dooble::s_sslExceptionsWindow->clear();
	}

      if(ui.faviconsCheckBox->isChecked())
	dmisc::clearFavicons();

      if(ui.historyCheckBox->isChecked())
	emit clearHistory();

      if(ui.offlineCacheCheckBox->isChecked())
	dooble::s_networkCache->clear();

      if(ui.spotonCheckBox->isChecked())
	if(dooble::s_spoton)
	  dooble::s_spoton->clear();

      QApplication::restoreOverrideCursor();
    }

  close();
}

void dclearcontainers::slotChecked(bool state)
{
  QCheckBox *checkBox = qobject_cast<QCheckBox *> (sender());

  if(!checkBox)
    return;

  QSettings settings;

  if(checkBox == ui.bookmarksCheckBox)
    settings.setValue("clearContainersWindow/bookmarks", state);
  else if(checkBox == ui.cookiesCheckBox)
    settings.setValue("clearContainersWindow/cookies", state);
  else if(checkBox == ui.downloadCheckBox)
    settings.setValue("clearContainersWindow/downloadInformation", state);
  else if(checkBox == ui.errorLogCheckBox)
    settings.setValue("clearContainersWindow/errorLog", state);
  else if(checkBox == ui.exceptionsCheckBox)
    settings.setValue("clearContainersWindow/exceptions", state);
  else if(checkBox == ui.faviconsCheckBox)
    settings.setValue("clearContainersWindow/favicons", state);
  else if(checkBox == ui.historyCheckBox)
    settings.setValue("clearContainersWindow/history", state);
  else if(checkBox == ui.offlineCacheCheckBox)
    settings.setValue("clearContainersWindow/offlineCache", state);
  else if(checkBox == ui.spotonCheckBox)
    settings.setValue("clearContainersWindow/spoton", state);
}

bool dclearcontainers::event(QEvent *event)
{
  return QMainWindow::event(event);
}
