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
#include <QSettings>
#include <QClipboard>
#include <QHideEvent>
#include <QScrollBar>

#include "dmisc.h"
#include "dooble.h"
#include "dhistorymodel.h"
#include "dhistorysidebar.h"
#include "dbookmarkswindow.h"

/*
** The application's cursor is overridden whenever the model is about to be
** reset and restored after the model has been reset. The visibility
** of the sidebar is not considered because the sidebar may become
** hidden during the reset phase.
*/

dhistorysidebar::dhistorysidebar(QWidget *parent):QWidget(parent)
{
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  ui.history->setObjectName("dooble_history_sidebar");
  ui.history->setModel(dooble::s_historyModel);
  ui.history->setContextMenuPolicy(Qt::CustomContextMenu);
  ui.history->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(visibilityChanged(const bool)),
	  this,
	  SLOT(slotVisibilityChanged(const bool)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.history,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slotItemDoubleClicked(const QModelIndex &)));
  connect(ui.closeWidget,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(closed(void)));
  connect(ui.sortByWidget,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotSort(int)));
  connect(ui.history, SIGNAL(customContextMenuRequested(const QPoint &)),
	  SLOT(slotShowContextMenu(const QPoint &)));
  connect(dooble::s_historyModel,
	  SIGNAL(modelReset(void)),
	  this,
	  SLOT(slotModelReset(void)));
  connect(dooble::s_historyModel,
	  SIGNAL(modelAboutToBeReset(void)),
	  this,
	  SLOT(slotModelAboutToBeReset(void)));
  connect(this,
	  SIGNAL(bookmark(const QUrl &,
			  const QIcon &,
			  const QString &,
			  const QString &,
			  const QDateTime &,
			  const QDateTime &)),
	  dooble::s_bookmarksWindow,
	  SLOT(slotAddBookmark(const QUrl &,
			       const QIcon &,
			       const QString &,
			       const QString &,
			       const QDateTime &,
			       const QDateTime &)));
  slotSetIcons();
  ui.sortByWidget->setCurrentIndex
    (dooble::s_settings.value("historySideBar/sortBy", 0).toInt());
  m_searchTimer = new QTimer(this);
  m_searchTimer->setInterval(750);
  m_searchTimer->setSingleShot(true);
  connect(m_searchTimer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotPopulate(void)));
}

dhistorysidebar::~dhistorysidebar()
{
}

void dhistorysidebar::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  ui.closeWidget->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  emit iconsChanged();
}

void dhistorysidebar::slotDeletePage(void)
{
  /*
  ** Delete an entry, or more, from the history table.
  */

  int removedRows = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  /*
  ** Sort the list by row number. If the list is not sorted,
  ** the following while-loop misbehaves.
  */

  qSort(list);

  while(!list.isEmpty())
    {
      QStandardItem *item = dooble::s_historyModel->item
	(list.takeFirst().row() - removedRows, 2);

      if(item)
	if(dooble::s_historyModel->deletePage(item->text()))
	  removedRows += 1;
    }
}

void dhistorysidebar::slotOpen(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = dooble::s_historyModel->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit open(url);
    }

  list.clear();
}

void dhistorysidebar::slotCopyUrl(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = dooble::s_historyModel->itemFromIndex(list.takeFirst());

  if(item && QApplication::clipboard())
    QApplication::clipboard()->setText(item->text());

  list.clear();
}

void dhistorysidebar::slotOpenInNewTab(void)
{
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  while(!list.isEmpty())
    {
      QStandardItem *item = dooble::s_historyModel->
	itemFromIndex(list.takeFirst());

      if(item)
	{
	  QUrl url(QUrl::fromUserInput(item->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit createTab(url);
	}
    }
}

void dhistorysidebar::slotOpenInNewWindow(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = dooble::s_historyModel->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit openInNewWindow(url);
    }

  list.clear();
}

void dhistorysidebar::slotShowContextMenu(const QPoint &point)
{
  if(!ui.history->selectionModel()->selectedRows().isEmpty())
    {
      QMenu menu(this);

      if(ui.history->selectionModel()->selectedRows().size() == 1 &&
	 ui.history->indexAt(point).isValid())
	{
	  menu.addAction(tr("&Bookmark"),
			 this, SLOT(slotBookmark(void)));
	  menu.addSeparator();
	  menu.addAction(tr("&Copy URL"),
			 this, SLOT(slotCopyUrl(void)));
	  menu.addSeparator();
	  menu.addAction(tr("&Delete Page"),
			 this, SLOT(slotDeletePage(void)));
	  menu.addSeparator();
	  menu.addAction(tr("Open in &Current Tab"),
			 this, SLOT(slotOpen(void)));
	  menu.addAction(tr("Open in New &Tab"),
			 this, SLOT(slotOpenInNewTab(void)));
	  menu.addAction(tr("Open in &New Window"),
			 this, SLOT(slotOpenInNewWindow(void)));
	  menu.addSeparator();

	  QAction *action = menu.addAction(tr("&Spot-On Share"),
					   this, SLOT(slotShare(void)));

	  if(dooble::s_spoton)
	    action->setEnabled(dooble::s_spoton->isKernelRegistered());
	  else
	    action->setEnabled(false);
	}
      else
	{
	  menu.addAction(tr("&Bookmark"),
			 this, SLOT(slotBookmark(void)));
	  menu.addAction(tr("&Delete Pages"),
			 this, SLOT(slotDeletePage(void)));
	  menu.addSeparator();
	  menu.addAction(tr("Open in &New Tabs"),
			 this, SLOT(slotOpenInNewTab(void)));
	  menu.addSeparator();

	  QAction *action = menu.addAction(tr("&Spot-On Share"),
					   this, SLOT(slotShare(void)));

	  if(dooble::s_spoton)
	    action->setEnabled(dooble::s_spoton->isKernelRegistered());
	  else
	    action->setEnabled(false);
	}

      menu.exec(ui.history->mapToGlobal(point));
    }
}

void dhistorysidebar::slotItemDoubleClicked(const QModelIndex &index)
{
  QStandardItem *item1 = 0;

  item1 = dooble::s_historyModel->itemFromIndex(index);

  if(item1)
    {
      QStandardItem *item2 = dooble::s_historyModel->item(item1->row(), 2);

      if(item2)
	{
	  QUrl url(QUrl::fromUserInput(item2->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit open(url);
	}
    }
}

void dhistorysidebar::textChanged(const QString &text)
{
  /*
  ** Search text changed.
  */

  int index = 0;

  for(int i = 0; i < ui.history->horizontalHeader()->count(); i++)
    if(!ui.history->isColumnHidden(i))
      {
	index = i;
	break;
      }

  for(int i = 0; i < dooble::s_historyModel->rowCount(); i++)
    if(!text.trimmed().isEmpty() &&
       !dooble::s_historyModel->item(i, index)->text().toLower().
       contains(text.toLower().trimmed()))
      ui.history->setRowHidden(i, true);
    else
      ui.history->setRowHidden(i, false);
}

void dhistorysidebar::saveState(void)
{
  dooble::s_settings["historySideBar/sortBy"] = ui.sortByWidget->
    currentIndex();

  QSettings settings;

  settings.setValue("historySideBar/sortBy",
		    ui.sortByWidget->currentIndex());
}

void dhistorysidebar::slotVisibilityChanged(const bool state)
{
  /*
  ** We're only interested in a model if the sidebar
  ** is visible. Resources are precious.
  */

  if(state)
    {
      dooble::s_historyModel->addWatchers(1);
      slotSort(ui.sortByWidget->currentIndex());
    }
  else
    dooble::s_historyModel->addWatchers(-1);
}

void dhistorysidebar::slotModelReset(void)
{
  if(isVisible())
    {
      slotSort(ui.sortByWidget->currentIndex());
      textChanged(ui.searchLineEdit->text().trimmed());
    }

  QApplication::restoreOverrideCursor();
}

void dhistorysidebar::slotSort(int index)
{
  saveState();

  for(int i = 0; i < ui.history->horizontalHeader()->count(); i++)
    ui.history->setColumnHidden(i, true);

  Qt::SortOrder sortOrder;

  if(index == 0) // Host
    {
      sortOrder = Qt::AscendingOrder;
      ui.history->setColumnHidden(2, false); // Location
    }
  else if(index == 1) // Last Visited
    {
      sortOrder = Qt::DescendingOrder;
      ui.history->setColumnHidden(3, false); // Title
    }
  else if(index == 2) // Location
    {
      sortOrder = Qt::AscendingOrder;
      ui.history->setColumnHidden(2, false); // Location
    }
  else if(index == 3) // Title
    {
      sortOrder = Qt::AscendingOrder;
      ui.history->setColumnHidden(3, false); // Title
    }
  else if(index == 4) // Visits
    {
      sortOrder = Qt::DescendingOrder;
      ui.history->setColumnHidden(3, false); // Title
    }
  else
    {
      sortOrder = Qt::AscendingOrder;
      ui.history->setColumnHidden(2, false); // Location
    }

  if(index >= 0 && index < ui.history->horizontalHeader()->count())
    ui.history->sortByColumn(index, sortOrder);
  else
    ui.history->sortByColumn(0, sortOrder);
}

void dhistorysidebar::hideEvent(QHideEvent *event)
{
  m_searchTimer->stop();

  if(event && (event->type() == QEvent::Hide ||
	       event->type() == QEvent::HideToParent))
    emit visibilityChanged(false);

  QWidget::hideEvent(event);
}

void dhistorysidebar::showEvent(QShowEvent *event)
{
  if(event && (event->type() == QEvent::Show ||
	       event->type() == QEvent::ShowToParent))
    emit visibilityChanged(true);

  QWidget::showEvent(event);
}

void dhistorysidebar::search(const QString &text)
{
  ui.searchLineEdit->setText(text);
  textChanged(ui.searchLineEdit->text().trimmed());
}

void dhistorysidebar::keyPressEvent(QKeyEvent *event)
{
  if(event && (event->key() == Qt::Key_Backspace ||
	       event->key() == Qt::Key_Delete))
    slotDeletePage();

  QWidget::keyPressEvent(event);
}

void dhistorysidebar::slotModelAboutToBeReset(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void dhistorysidebar::slotBookmark(void)
{
  QDateTime now(QDateTime::currentDateTime());
  QModelIndexList list(ui.history->selectionModel()->selectedRows(0));

  while(!list.isEmpty())
    {
      QUrl url;
      QIcon icon;
      QString title("");
      QString description("");
      QStandardItem *item = dooble::s_historyModel->item
	(list.first().row(), 3);

      if(item)
	{
	  icon = item->icon();
	  title = item->text();
	}

      item = dooble::s_historyModel->item(list.first().row(), 2);

      if(item)
	{
	  url = QUrl::fromUserInput(item->text());
	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	}

      item = dooble::s_historyModel->item(list.first().row(), 5);

      if(item)
	description = item->text();

      if(icon.isNull())
	icon = dmisc::iconForUrl(url);

      emit bookmark(url, icon, title, description, now, now);
      list.takeFirst();
    }
}

void dhistorysidebar::slotPopulate(void)
{
  textChanged(ui.searchLineEdit->text().trimmed());
}

void dhistorysidebar::slotTextChanged(const QString &text)
{
  Q_UNUSED(text);
  m_searchTimer->start();
}

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
void dhistorysidebar::slotShare(void)
{
  if(!dooble::s_spoton)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(ui.history->selectionModel()->selectedRows(0));

  while(!list.isEmpty())
    {
      QStandardItem *item = dooble::s_historyModel->item
	(list.first().row(), 3);
      QString content("");
      QString description("");
      QString title("");
      QUrl url;

      if(item)
	title = item->text();

      item = dooble::s_historyModel->item(list.first().row(), 2);

      if(item)
	{
	  url = QUrl::fromUserInput(item->text());
	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	}

      item = dooble::s_historyModel->item(list.first().row(), 5);

      if(item)
	description = item->text();

      dooble::s_spoton->share(url, title, description, content);
      list.takeFirst();
    }

  QApplication::restoreOverrideCursor();
}
#endif
