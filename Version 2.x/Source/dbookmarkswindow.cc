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

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QProgressBar>
#include <QScrollBar>
#include <QSettings>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUrl>
#include <QWebEnginePage>

#include "dbookmarkspopup.h"
#include "dbookmarkswindow.h"
#include "dmisc.h"
#include "dooble.h"

dbookmarkswindow::dbookmarkswindow(void):QMainWindow()
{
  ui.setupUi(this);
  ui.bookmarks->setObjectName("dooble_bookmarks_table");
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
  statusBar()->setSizeGripEnabled(false);
#endif
  m_urlModel = new QStandardItemModel(this);
  m_urlModel->setSortRole(Qt::UserRole);

  QStringList list;

  list << tr("Title") << tr("Location")
       << tr("Date Created") << tr("Last Time Visited")
       << tr("Visits") << "Description";
  m_urlModel->setHorizontalHeaderLabels(list);
  ui.folders->setModel(dooble::s_bookmarksFolderModel);
  ui.bookmarks->setModel(m_urlModel);
  ui.bookmarks->setColumnHidden(m_urlModel->columnCount() - 1, true);
  ui.bookmarks->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.bookmarks->setSortingEnabled(true);
  ui.bookmarks->horizontalHeader()->setDefaultAlignment
    (Qt::AlignLeft);
  ui.bookmarks->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
  ui.bookmarks->horizontalHeader()->setSortIndicatorShown(true);
  ui.bookmarks->horizontalHeader()->setStretchLastSection(true);

  for(int i = 0; i < ui.bookmarks->horizontalHeader()->count() - 1; i++)
    ui.bookmarks->resizeColumnToContents(i);

  ui.bookmarks->horizontalHeader()->setSectionResizeMode
    (QHeaderView::Interactive);
  ui.searchLineEdit->setPlaceholderText(tr("Search Bookmarks"));
  slotSetIcons();
  connect(dooble::s_bookmarksFolderModel,
	  SIGNAL(dataChanged(const QModelIndex &,
			     const QModelIndex &)),
	  this,
	  SLOT(slotFolderDataChanged(const QModelIndex &,
				     const QModelIndex &)));
  connect(ui.bookmarks->horizontalHeader(),
	  SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
	  this,
	  SLOT(slotSort(int, Qt::SortOrder)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.bookmarks,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slotItemDoubleClicked(const QModelIndex &)));
  connect(ui.action_Close,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(close(void)));
  connect(ui.folders,
	  SIGNAL(itemSelected(const QModelIndex &)),
	  this,
	  SLOT(slotFolderSelected(const QModelIndex &)));
  connect(ui.bookmarks->selectionModel(),
	  SIGNAL(selectionChanged(const QItemSelection &,
				  const QItemSelection &)),
	  this,
	  SLOT(slotUrlSelected(const QItemSelection &,
			       const QItemSelection &)));
  connect(ui.addFolderPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAddFolder(void)));
  connect(ui.removeFolderPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteFolder(void)));
  connect(ui.titleLineEdit,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slotTitleChanged(void)));
  connect(ui.locationLineEdit,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slotLocationChanged(void)));
  connect(ui.folders,
	  SIGNAL(bookmarkReceived(const QModelIndex &)),
	  this,
	  SLOT(slotBookmarkReceived(const QModelIndex &)));
  connect(ui.descriptionTextEdit,
	  SIGNAL(textChanged(void)),
	  this,
	  SLOT(slotDescriptionChanged(void)));
  connect(ui.action_Export,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotExport(void)));
  connect(ui.action_Import,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotImport(void)));
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(ui.sharePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotShare(void)));
#endif
  ui.action_Import->setEnabled(false);
  ui.action_Import->setToolTip(tr("WebEngine does not yet support "
				  "Web elements. Web elements are required "
				  "for importing bookmarks."));
  ui.folders->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.folders, SIGNAL(customContextMenuRequested(const QPoint &)),
	  SLOT(slotShowContextMenu(const QPoint &)));
  ui.bookmarks->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.bookmarks, SIGNAL(customContextMenuRequested(const QPoint &)),
	  SLOT(slotShowContextMenu(const QPoint &)));
  ui.splitter1->setStretchFactor(0, 0);
  ui.splitter1->setStretchFactor(1, 1);
  ui.splitter2->setStretchFactor(0, 1);
  ui.splitter2->setStretchFactor(1, 0);
  ui.folders->setFocus();

  if(dooble::s_settings.contains("bookmarksWindow/splitter1State"))
    ui.splitter1->restoreState
      (dooble::s_settings.value("bookmarksWindow/splitter1State",
				"").toByteArray());

  if(dooble::s_settings.contains("bookmarksWindow/splitter2State"))
    ui.splitter2->restoreState
      (dooble::s_settings.value("bookmarksWindow/splitter2State",
				"").toByteArray());

  if(dooble::s_settings.contains("bookmarksWindow/tableColumnsState1"))
    {
      if(!ui.bookmarks->horizontalHeader()->
	 restoreState(dooble::s_settings.value("bookmarksWindow/"
					       "tableColumnsState1",
					       "").toByteArray()))
	{
	  ui.bookmarks->horizontalHeader()->setDefaultAlignment
	    (Qt::AlignLeft);
	  ui.bookmarks->horizontalHeader()->setSortIndicator
	    (0, Qt::AscendingOrder);
	  ui.bookmarks->horizontalHeader()->setSortIndicatorShown(true);
	  ui.bookmarks->horizontalHeader()->setStretchLastSection(true);
	  ui.bookmarks->horizontalHeader()->setSectionResizeMode
	    (QHeaderView::Interactive);
	  for(int i = 0; i < ui.bookmarks->horizontalHeader()->count() - 1;
	      i++)
	    ui.bookmarks->resizeColumnToContents(i);
	}
    }
  else
    for(int i = 0; i < ui.bookmarks->horizontalHeader()->count() - 1; i++)
      ui.bookmarks->resizeColumnToContents(i);

  ui.bookmarks->horizontalHeader()->setSectionsMovable(true);
  createBookmarksDatabase();

  /*
  ** Most other containers do not call populate until the user's passphrase
  ** has been validated. We do this here so as to prepare the folders model.
  */

  populate();

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  ui.sharePushButton->setEnabled(true);
#else
  ui.sharePushButton->setEnabled(false);
  ui.sharePushButton->setToolTip(tr("Spot-On support is not available."));
#endif
}

dbookmarkswindow::~dbookmarkswindow()
{
  saveState();
  purge();
}

void dbookmarkswindow::show(QWidget *parent)
{
  m_parent = parent;
  disconnect(m_parent,
	     SIGNAL(destroyed(void)),
	     this,
	     SLOT(slotParentDestroyed(void)));
  connect(m_parent,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(slotParentDestroyed(void)));
  disconnect(this, SIGNAL(open(const QUrl &)), 0, 0);
  disconnect(this, SIGNAL(createTab(const QUrl &)), 0, 0);
  disconnect(this, SIGNAL(openInNewWindow(const QUrl &)), 0, 0);
  connect(this, SIGNAL(open(const QUrl &)),
	  m_parent, SLOT(slotLoadPage(const QUrl &)));
  connect(this, SIGNAL(createTab(const QUrl &)),
	  m_parent, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(this, SIGNAL(openInNewWindow(const QUrl &)),
	  m_parent, SLOT(slotOpenLinkInNewWindow(const QUrl &)));

  QRect rect;

  if(m_parent)
    {
      rect = m_parent->geometry();
      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
    }
  else
    {
      rect.setX(100);
      rect.setY(100);
    }

  rect.setHeight(600);
  rect.setWidth(800);

  if(!isVisible())
    {
      /*
      ** Don't annoy the user.
      */

      if(dooble::s_settings.contains("bookmarksWindow/geometry"))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.
			value("bookmarksWindow/geometry",
			      rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.
			   value("bookmarksWindow/geometry").
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
    dmisc::centerChildWithParent(this, m_parent);

  showNormal();
  raise();
}

void dbookmarkswindow::purge(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM bookmarks WHERE temporary = 1");
	query.exec("DELETE FROM folders WHERE temporary = 1");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
}

void dbookmarkswindow::slotSort(int column, Qt::SortOrder order)
{
  m_urlModel->sort(column, order);
}

void dbookmarkswindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("bookmarksWindow/windowIcon").toString()));
  ui.action_Export->setIcon
    (QIcon(settings.value("bookmarksWindow/actionExport").toString()));
  ui.action_Import->setIcon
    (QIcon(settings.value("bookmarksWindow/actionImport").toString()));
  ui.action_Close->setIcon
    (QIcon(settings.value("bookmarksWindow/actionClose").toString()));
  ui.addFolderPushButton->setIcon
    (QIcon(settings.value("bookmarksWindow/addButtonIcon").toString()));
  ui.removeFolderPushButton->setIcon
    (QIcon(settings.value("bookmarksWindow/removeButtonIcon").toString()));
  ui.sharePushButton->setIcon
    (QIcon(settings.value("spotonIcon").toString()));
  emit iconsChanged();
}

void dbookmarkswindow::slotDeleteBookmark(void)
{
  /*
  ** Reached via a context menu or the Delete key.
  */

  QStandardItem *item1 = 0;
  QStandardItem *item2 = 0;
  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    item1 = dooble::s_bookmarksFolderModel->itemFromIndex(list.takeFirst());

  list = ui.bookmarks->selectionModel()->selectedRows(0);

  if(!list.isEmpty())
    item2 = m_urlModel->itemFromIndex(list.takeFirst());

  if(item1 && item2)
    {
      QUrl url;
      QStandardItem *item3 = m_urlModel->item(item2->row(), 1);

      if(item3)
	{
	  url = QUrl::fromUserInput(item3->text());
	  url = QUrl::fromEncoded
	    (url.toEncoded(QUrl::StripTrailingSlash));
	}

      if(!url.isEmpty() && url.isValid())
	{
	  {
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							"bookmarks");

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() + "bookmarks.db");

	    if(db.open())
	      {
		QSqlQuery query(db);
		bool ok = true;
		int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

		query.exec("PRAGMA secure_delete = ON");
		query.prepare("DELETE FROM bookmarks WHERE "
			      "folder_oid = ? AND "
			      "url_hash = ? AND "
			      "temporary = ?");
		query.bindValue
		  (0, item1->data(Qt::UserRole + 2).toLongLong());
		query.bindValue
		  (1,
		   dmisc::hashedString(url.
				       toEncoded(QUrl::
						 StripTrailingSlash),
				       &ok).toBase64());
		query.bindValue(2, temporary);

		if(ok)
		  query.exec();
	      }

	    db.close();
	  }

	  QSqlDatabase::removeDatabase("bookmarks");
	  emit changed();
	}

      m_urlModel->removeRow(item2->row());
    }
}

void dbookmarkswindow::slotBookmarkDeleted(const int folderOid,
					   const QUrl &url)
{
  /*
  ** The user requested that the specified bookmark be deleted via
  ** the Bookmarks popup. Find the appropriate bookmark and remove
  ** it from the model.
  */

  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    {
      QStandardItem *item = dooble::s_bookmarksFolderModel->itemFromIndex
	(list.takeFirst());

      if(item && folderOid == item->data(Qt::UserRole + 2).toLongLong())
	for(int i = 0; i < m_urlModel->rowCount(); i++)
	  if(m_urlModel->item(i, 1) &&
	     m_urlModel->item(i, 1)->text() ==
	     url.toString(QUrl::StripTrailingSlash))
	    {
	      m_urlModel->removeRow(i);
	      break;
	    }
    }

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.exec("PRAGMA secure_delete = ON");
	query.prepare("DELETE FROM bookmarks WHERE "
		      "folder_oid = ? AND "
		      "url_hash = ? AND "
		      "temporary = ?");
	query.bindValue(0, folderOid);
	query.bindValue
	  (1,
	   dmisc::hashedString(url.
			       toEncoded(QUrl::
					 StripTrailingSlash),
			       &ok).toBase64());
	query.bindValue(2, temporary);

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
}

void dbookmarkswindow::slotOpen(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(1));

  if(!list.isEmpty())
    item = m_urlModel->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit open(url);
    }
}

void dbookmarkswindow::slotCopyUrl(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(1));

  if(!list.isEmpty())
    item = m_urlModel->itemFromIndex(list.takeFirst());

  if(item && QApplication::clipboard())
    QApplication::clipboard()->setText(item->text());
}

void dbookmarkswindow::slotOpenInNewTab(void)
{
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(1));

  while(!list.isEmpty())
    {
      QStandardItem *item = m_urlModel->itemFromIndex(list.takeFirst());

      if(item)
	{
	  QUrl url(QUrl::fromUserInput(item->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit createTab(url);
	}
    }
}

void dbookmarkswindow::slotOpenInNewWindow(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(1));

  if(!list.isEmpty())
    item = m_urlModel->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit openInNewWindow(url);
    }
}

void dbookmarkswindow::slotShowContextMenu(const QPoint &point)
{
  if(sender() == ui.folders)
    {
      if(!ui.folders->selectionModel()->selectedIndexes().isEmpty())
	{
	  slotFolderSelected(ui.folders->indexAt(point));

	  QStandardItem *item = dooble::s_bookmarksFolderModel->itemFromIndex
	    (ui.folders->indexAt(point));

	  if(item && item->data(Qt::UserRole + 2).toLongLong() > -1)
	    {
	      QMenu menu(this);

	      if(ui.folders->indexAt(point).isValid())
		menu.addAction(tr("&Create Sub-Folder"),
			       this, SLOT(slotAddFolderOffParent(void)));

	      menu.exec(ui.folders->mapToGlobal(point));
	    }
	}
    }
  else if(sender() == ui.bookmarks)
    {
      if(!ui.bookmarks->selectionModel()->selectedRows().isEmpty())
	{
	  QMenu menu(this);

	  if(ui.bookmarks->indexAt(point).isValid())
	    {
	      menu.addAction(tr("&Copy URL"),
			     this, SLOT(slotCopyUrl(void)));
	      menu.addSeparator();
	      menu.addAction(tr("&Delete Bookmark"),
			     this, SLOT(slotDeleteBookmark(void)));
	      menu.addSeparator();
	      menu.addAction(tr("Open in &Current Tab"),
			     this, SLOT(slotOpen(void)));
	      menu.addAction(tr("Open in New &Tab"),
			     this, SLOT(slotOpenInNewTab(void)));
	      menu.addAction(tr("Open in &New Window..."),
			     this, SLOT(slotOpenInNewWindow(void)));
	      menu.addSeparator();

	      QAction *action = menu.addAction
		(tr("&Spot-On Share"), this, SLOT(slotShare(void)));

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
	      action->setEnabled(true);
#else
	      action->setEnabled(false);
	      action->setToolTip(tr("Spot-On support is not available."));
#endif
	    }

	  menu.exec(ui.bookmarks->mapToGlobal(point));
	}
    }
}

void dbookmarkswindow::slotItemDoubleClicked(const QModelIndex &index)
{
  QStandardItem *item1 = 0;

  item1 = m_urlModel->itemFromIndex(index);

  if(item1)
    {
      QStandardItem *item2 = m_urlModel->item(item1->row(), 1);

      if(item2)
	{
	  QUrl url(QUrl::fromUserInput(item2->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit open(url);
	}
    }
}

void dbookmarkswindow::slotParentDestroyed(void)
{
  /*
  ** Locate another Dooble window (parent).
  */

  foreach(QWidget *widget, QApplication::topLevelWidgets())
    if(qobject_cast<dooble *> (widget))
      if(widget != m_parent)
	{
	  m_parent = widget;
	  connect(m_parent,
		  SIGNAL(destroyed(void)),
		  this,
		  SLOT(slotParentDestroyed(void)));
	  disconnect(this, SIGNAL(open(const QUrl &)),
		     m_parent, SLOT(slotLoadPage(const QUrl &)));
	  disconnect(this, SIGNAL(createTab(const QUrl &)),
		     m_parent, SLOT(slotOpenLinkInNewTab(const QUrl &)));
	  disconnect(this, SIGNAL(openInNewWindow(const QUrl &)),
		     m_parent, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
	  connect(this, SIGNAL(open(const QUrl &)),
		  m_parent, SLOT(slotLoadPage(const QUrl &)));
	  connect(this, SIGNAL(createTab(const QUrl &)),
		  m_parent, SLOT(slotOpenLinkInNewTab(const QUrl &)));
	  connect(this, SIGNAL(openInNewWindow(const QUrl &)),
		  m_parent, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
	  break;
	}
}

void dbookmarkswindow::populate(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_urlModel->removeRows(0, m_urlModel->rowCount());
  dooble::s_bookmarksFolderModel->removeRows
    (0, dooble::s_bookmarksFolderModel->rowCount());

  QFileIconProvider iconProvider;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	/*
	** Resolve missing links! Link and Zelda.
	*/

	query.setForwardOnly(true);
	query.prepare("UPDATE bookmarks SET folder_oid = -1 "
		      "WHERE temporary = ? AND "
		      "folder_oid NOT IN (SELECT oid FROM "
		      "folders WHERE temporary = ?)");
	query.bindValue(0, temporary);
	query.bindValue(1, temporary);
	query.exec();
	query.exec("PRAGMA synchronous = FULL");

	QHash<qint64, QStandardItem *> parents;

	query.prepare("SELECT name, oid, parent_oid "
		      "FROM folders "
		      "WHERE temporary = ? "
		      "ORDER BY parent_oid");
	query.bindValue(0, temporary);

	if(query.exec())
	  while(query.next())
	    {
	      bool ok = true;
	      qint64 oid = query.value(1).toLongLong();
	      qint64 parentOid = -1;
	      QString name
		(QString::fromUtf8
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QStandardItem *item = new QStandardItem(name);

	      if(!query.isNull(2))
		{
		  parentOid = query.value(2).toLongLong();
		  item->setData(parentOid, Qt::UserRole + 3);
		}

	      /*
	      ** Set the folder's current name and oid.
	      */

	      item->setData(name, Qt::UserRole + 1);
	      item->setData(oid, Qt::UserRole + 2);
	      item->setIcon(iconProvider.icon(QFileIconProvider::Folder));
	      item->setEditable(true);
	      parents.insert(oid, item);

	      if(parentOid < 0)
		/*
		** Top-level folder.
		*/

		dooble::s_bookmarksFolderModel->appendRow(item);
	      else
		{
		  /*
		  ** This folder has a parent. Find the parent.
		  */

		  if(parents.contains(parentOid))
		    parents.value(parentOid)->appendRow(item);
		}
	    }

	parents.clear();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  dooble::s_bookmarksFolderModel->sort(0);

  QStandardItem *item = new QStandardItem(tr("Uncategorized"));

  item->setData(-1, Qt::UserRole + 2);
  item->setIcon(iconProvider.icon(QFileIconProvider::Folder));
  item->setEditable(false);
  dooble::s_bookmarksFolderModel->insertRow(0, item);
  QApplication::restoreOverrideCursor();
  ui.folders->selectionModel()->select
    (dooble::s_bookmarksFolderModel->index(0, 0),
     QItemSelectionModel::ClearAndSelect);
  populateTable(-1);
  emit changed();
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void dbookmarkswindow::slotTextChanged(const QString &text)
{
  /*
  ** Search text changed.
  */

  for(int i = 0; i < m_urlModel->rowCount(); i++)
    if((m_urlModel->item(i, 0) &&
	m_urlModel->item(i, 0)->text().toLower().
	contains(text.toLower().trimmed())) ||
       (m_urlModel->item(i, 1) &&
	m_urlModel->item(i, 1)->text().toLower().
	contains(text.toLower().trimmed())))
      ui.bookmarks->setRowHidden(i, false);
    else
      ui.bookmarks->setRowHidden(i, true);
}

void dbookmarkswindow::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void dbookmarkswindow::saveState(void)
{
  if(isVisible())
    {
      if(dmisc::isGnome())
	dooble::s_settings["bookmarksWindow/geometry"] = geometry();
      else
	dooble::s_settings["bookmarksWindow/geometry"] = saveGeometry();
    }

  dooble::s_settings["bookmarksWindow/splitter1State"] =
    ui.splitter1->saveState();
  dooble::s_settings["bookmarksWindow/splitter2State"] =
    ui.splitter2->saveState();
  dooble::s_settings["bookmarksWindow/tableColumnsState1"] =
    ui.bookmarks->horizontalHeader()->saveState();

  QSettings settings;

  if(isVisible())
    {
      if(dmisc::isGnome())
	settings.setValue("bookmarksWindow/geometry", geometry());
      else
	settings.setValue("bookmarksWindow/geometry", saveGeometry());
    }

  settings.setValue("bookmarksWindow/splitter1State",
		    ui.splitter1->saveState());
  settings.setValue("bookmarksWindow/splitter2State",
		    ui.splitter2->saveState());
  settings.setValue("bookmarksWindow/tableColumnsState1",
		    ui.bookmarks->horizontalHeader()->
		    saveState());
}

void dbookmarkswindow::slotUrlSelected(const QItemSelection &selected,
				       const QItemSelection &deselected)
{
  Q_UNUSED(deselected);

  if(!selected.indexes().isEmpty())
    {
      int row = selected.indexes().at(0).row();

      QStandardItem *item = 0;

      if((item = m_urlModel->item(row, 0)))
	{
	  ui.titleLineEdit->setText(item->text());
	  ui.titleLineEdit->setCursorPosition(0);
	}

      if((item = m_urlModel->item(row, 1)))
	{
	  ui.locationLineEdit->setText(item->text());
	  ui.locationLineEdit->setCursorPosition(0);
	}

      if((item = m_urlModel->item(row, 5)))
	{
	  disconnect(ui.descriptionTextEdit,
		     SIGNAL(textChanged(void)),
		     this,
		     SLOT(slotDescriptionChanged(void)));
	  ui.descriptionTextEdit->setPlainText(item->text());
	  connect(ui.descriptionTextEdit,
		  SIGNAL(textChanged(void)),
		  this,
		  SLOT(slotDescriptionChanged(void)));
	}
    }
  else
    {
      ui.titleLineEdit->clear();
      ui.locationLineEdit->clear();
      ui.descriptionTextEdit->clear();
    }
}

void dbookmarkswindow::slotFolderSelected(const QModelIndex &index)
{
  if(index.isValid())
    {
      QStandardItem *item = dooble::s_bookmarksFolderModel->itemFromIndex
	(index);

      if(item)
	populateTable(item->data(Qt::UserRole + 2).toLongLong());
    }
}

void dbookmarkswindow::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
      else if(event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Delete)
	{
	  if(ui.bookmarks->hasFocus())
	    slotDeleteBookmark();
	}
      else if(event->key() == Qt::Key_F &&
	      event->modifiers() == Qt::ControlModifier)
	{
	  ui.searchLineEdit->setFocus();
	  ui.searchLineEdit->selectAll();
	}
    }

  QMainWindow::keyPressEvent(event);
}

void dbookmarkswindow::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(dooble::s_settings.value("settingsWindow/"
				   "disableAllEncryptedDatabaseWrites",
				   false).toBool())
    return;

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	if(progress)
	  {
	    progress->setMaximum(dooble::s_bookmarksFolderModel->rowCount());
	    progress->update();
	  }

	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM folders WHERE temporary = 1");

	QModelIndexList list(dooble::s_bookmarksFolderModel->
			     match(dooble::s_bookmarksFolderModel->index(0, 0),
				   Qt::DisplayRole,
				   "*",
				   -1,
				   Qt::MatchRecursive | Qt::MatchWildcard));

	for(int i = 0; i < list.size(); i++)
	  {
	    QStandardItem *item = dooble::s_bookmarksFolderModel->
	      itemFromIndex(list.at(i));

	    if(item)
	      {
		QString name(item->text());
		bool ok = true;
		qint64 oid = item->data(Qt::UserRole + 2).toLongLong();

		/*
		** We're guaranteed that the oid is absolutely unique.
		*/

		query.prepare
		  ("UPDATE folders SET name = ? "
		   "WHERE oid = ? AND temporary = 0");
		query.bindValue
		  (0,
		   dmisc::etm(name.toUtf8(), true, &ok).toBase64());
		query.bindValue(1, oid);

		if(ok)
		  query.exec();
	      }

	    if(progress)
	      progress->setValue(i + 1);
	  }

	if(progress)
	  progress->update();

	{
	  int temporary = -1;
	  QSqlQuery query(db);

	  query.setForwardOnly(true);

	  if(query.exec("SELECT title, url, date_created, "
			"last_visited, "
			"visits, icon, description, folder_oid "
			"FROM bookmarks WHERE "
			"temporary = 0"))
	    while(query.next())
	      {
		bool ok = true;
		QUrl url
		  (QUrl::fromEncoded
		   (dmisc::daa
		    (dmisc::s_reencodeCrypt,
		     QByteArray::fromBase64
		     (query.value(1).toByteArray()), &ok),
		    QUrl::StrictMode));

		if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		  continue;

		qint64 folderOid = query.value(7).toLongLong();
		QIcon icon;
		QBuffer buffer;
		QString title
		  (QString::fromUtf8
		   (dmisc::daa
		    (dmisc::s_reencodeCrypt,
		     QByteArray::fromBase64
		     (query.value(0).toByteArray()), &ok)));

		if(!ok)
		  continue;

		QString visits
		  (dmisc::daa
		   (dmisc::s_reencodeCrypt,
		    QByteArray::fromBase64
		    (query.value(4).toByteArray()), &ok));

		if(!ok)
		  continue;

		QString description
		  (dmisc::daa
		   (dmisc::s_reencodeCrypt,
		    QByteArray::fromBase64
		    (query.value(6).toByteArray()), &ok));

		if(!ok)
		  continue;

		QDateTime dateTime1
		  (QDateTime::fromString
		   (QString::fromUtf8
		    (dmisc::daa
		     (dmisc::s_reencodeCrypt,
		      QByteArray::fromBase64
		      (query.value(2).toByteArray()), &ok)),
		    Qt::ISODate));

		if(!ok)
		  continue;

		QDateTime dateTime2
		  (QDateTime::fromString
		   (QString::fromUtf8
		    (dmisc::daa
		     (dmisc::s_reencodeCrypt,
		      QByteArray::fromBase64
		      (query.value(3).toByteArray()), &ok)),
		    Qt::ISODate));

		if(!ok)
		  continue;

		QByteArray bytes
		  (dmisc::daa
		   (dmisc::s_reencodeCrypt, query.value(5).toByteArray(),
		    &ok));

		if(!ok)
		  continue;

		buffer.setBuffer(&bytes);

		if(buffer.open(QIODevice::ReadOnly))
		  {
		    QDataStream in(&buffer);

		    in >> icon;

		    if(in.status() != QDataStream::Ok)
		      icon = QIcon();

		    buffer.close();
		  }
		else
		  icon = QIcon();

		if(icon.isNull())
		  icon = dmisc::iconForUrl(url);

		QSqlQuery insertQuery(db);

		insertQuery.prepare
		  ("INSERT OR REPLACE INTO bookmarks "
		   "(title, url, date_created, last_visited, icon, visits, "
		   "temporary, description, folder_oid, url_hash) "
		   "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
		insertQuery.bindValue
		  (0,
		   dmisc::etm(title.toUtf8(), true, &ok).
		   toBase64());

		if(ok)
		  insertQuery.bindValue
		    (1,
		     dmisc::etm(url.
				toEncoded(QUrl::StripTrailingSlash),
				true, &ok).toBase64());

		if(ok)
		  insertQuery.bindValue
		    (2,
		     dmisc::etm(dateTime1.toString(Qt::ISODate).
				toUtf8(), true, &ok).toBase64());

		if(ok)
		  insertQuery.bindValue
		    (3,
		     dmisc::etm(dateTime2.toString(Qt::ISODate).
				toUtf8(), true, &ok).toBase64());

		bytes.clear();
		buffer.setBuffer(&bytes);

		if(buffer.open(QIODevice::WriteOnly))
		  {
		    QDataStream out(&buffer);

		    if(icon.isNull())
		      out << dmisc::iconForUrl(url);
		    else
		      out << icon;

		    if(out.status() != QDataStream::Ok)
		      bytes.clear();
		  }
		else
		  bytes.clear();

		if(ok)
		  insertQuery.bindValue
		    (4,
		     dmisc::etm(bytes, true, &ok));

		buffer.close();

		if(ok)
		  insertQuery.bindValue
		    (5,
		     dmisc::etm(visits.toLatin1(), true, &ok).
		     toBase64());

		insertQuery.bindValue(6, temporary);

		if(ok)
		  insertQuery.bindValue
		    (7,
		     dmisc::etm(description.toUtf8(), true, &ok).
		     toBase64());

		insertQuery.bindValue
		  (8, folderOid);

		if(ok)
		  insertQuery.bindValue
		    (9,
		     dmisc::hashedString(url.
					 toEncoded(QUrl::
						   StripTrailingSlash),
					 &ok).toBase64());

		if(ok)
		  insertQuery.exec();
	      }

	  query.exec("PRAGMA secure_delete = ON");
	  query.exec("DELETE FROM bookmarks WHERE temporary <> -1");
	  query.exec("UPDATE bookmarks SET temporary = 0");
	}
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");

  if(progress)
    progress->setVisible(false);
}

void dbookmarkswindow::addFolder(void)
{
  QStandardItem *item = new QStandardItem(tr("New Folder"));
  QFileIconProvider iconProvider;

  item->setData(item->text(), Qt::UserRole + 1);
  item->setData(-1, Qt::UserRole + 3); /*
				       ** The folder does not have a parent.
				       */
  item->setIcon(iconProvider.icon(QFileIconProvider::Folder));
  item->setEditable(true);
  dooble::s_bookmarksFolderModel->setRowCount
    (dooble::s_bookmarksFolderModel->rowCount() + 1);
  dooble::s_bookmarksFolderModel->setItem
    (dooble::s_bookmarksFolderModel->rowCount() - 1, 0, item);
  saveFolder(item);
  emit changed();
  item = dooble::s_bookmarksFolderModel->takeRow(0).value(0);
  dooble::s_bookmarksFolderModel->sort(0);

  if(item)
    dooble::s_bookmarksFolderModel->insertRow(0, item);
}

void dbookmarkswindow::slotAddFolder(void)
{
  QItemSelection selection(ui.folders->selectionModel()->selection());

  addFolder();

  if(selection.indexes().value(0).isValid())
    ui.folders->selectionModel()->select
      (selection, QItemSelectionModel::ClearAndSelect);
  else
    ui.folders->selectionModel()->select
      (dooble::s_bookmarksFolderModel->index(0, 0),
       QItemSelectionModel::ClearAndSelect);
}

void dbookmarkswindow::slotAddFolderOffParent(void)
{
  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    {
      QStandardItem *item1 = dooble::s_bookmarksFolderModel->itemFromIndex
	(list.first());

      if(item1)
	{
	  QStandardItem *item2 = new QStandardItem(tr("New Folder"));
	  QFileIconProvider iconProvider;

	  item2->setData(item2->text(), Qt::UserRole + 1);
	  item2->setIcon(iconProvider.icon(QFileIconProvider::Folder));
	  item2->setEditable(true);
	  item2->setData(item1->data(Qt::UserRole + 2), Qt::UserRole + 3);
	  item1->appendRow(item2);
	  saveFolder(item2);
	  emit changed();
	  item1->sortChildren(0);
	  ui.folders->setExpanded(list.first(), true);
	}
    }
}

void dbookmarkswindow::createBookmarksDatabase(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS bookmarks ("
		   "folder_oid INTEGER NOT NULL DEFAULT -1, "
		   "title TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_hash TEXT NOT NULL, "
		   "date_created TEXT NOT NULL, "
		   "last_visited TEXT NOT NULL, "
		   "icon BLOB DEFAULT NULL, "
		   "visits TEXT NOT NULL, "
		   "description TEXT, "
		   "temporary INTEGER NOT NULL, "
		   "PRIMARY KEY(folder_oid, url_hash, temporary))");

	/*
	** Assigning each folder a unique identifier will provide
	** us with all sorts of benefits. For instance, deleting
	** a folder will also delete all of its bookmarks automatically.
	** Another benefit, moving a bookmark from one folder to
	** another is as simple as modifying the bookmark's folder_oid.
	*/

	query.exec("CREATE TABLE IF NOT EXISTS folders ("
		   "name TEXT NOT NULL, "
		   "oid INTEGER PRIMARY KEY NOT NULL, "
		   "parent_oid INTEGER, "
		   "temporary INTEGER NOT NULL)");
	query.exec("CREATE TRIGGER bookmarks_purge AFTER DELETE ON folders "
		   "FOR EACH row "
		   "BEGIN "
		   "DELETE FROM bookmarks WHERE folder_oid = old.oid "
		   "AND temporary = old.temporary; "
		   "DELETE FROM folders WHERE parent_oid = old.oid "
		   "AND temporary = old.temporary; "
		   "END;");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
}

void dbookmarkswindow::renameFolder(const QModelIndex &index)
{
  QStandardItem *item = 0;

  if((item = dooble::s_bookmarksFolderModel->itemFromIndex(index)))
    {
      /*
      ** Disconnect the dataChanged() signal before modifying the item.
      */

      disconnect(dooble::s_bookmarksFolderModel,
		 SIGNAL(dataChanged(const QModelIndex &,
				    const QModelIndex &)),
		 this,
		 SLOT(slotFolderDataChanged(const QModelIndex &,
					    const QModelIndex &)));
      disconnect(dooble::s_bookmarksFolderModel,
		 SIGNAL(dataChanged(const QModelIndex &,
				    const QModelIndex &)),
		 dooble::s_bookmarksPopup,
		 SLOT(slotFolderDataChanged(const QModelIndex &,
					    const QModelIndex &)));

      QString name(item->text().trimmed());

      if(!name.isEmpty())
	{
	  updateFolder(name, item->data(Qt::UserRole + 2).toLongLong());
	  item->setData(name, Qt::UserRole + 1);
	  item->setText(name);
	  emit changed();

	  QStandardItem *item =
	    dooble::s_bookmarksFolderModel->takeRow(0).value(0);

	  dooble::s_bookmarksFolderModel->sort(0);

	  if(item)
	    dooble::s_bookmarksFolderModel->insertRow(0, item);
	}
      else
	item->setText(item->data(Qt::UserRole + 1).toString());

      connect(dooble::s_bookmarksFolderModel,
	      SIGNAL(dataChanged(const QModelIndex &,
				 const QModelIndex &)),
	      this,
	      SLOT(slotFolderDataChanged(const QModelIndex &,
					 const QModelIndex &)));
      connect(dooble::s_bookmarksFolderModel,
	      SIGNAL(dataChanged(const QModelIndex &,
				 const QModelIndex &)),
	      dooble::s_bookmarksPopup,
	      SLOT(slotFolderDataChanged(const QModelIndex &,
					 const QModelIndex &)));

    }
}

void dbookmarkswindow::saveFolder(QStandardItem *item)
{
  if(dooble::s_settings.value("settingsWindow/"
			      "disableAllEncryptedDatabaseWrites", false).
     toBool())
    return;
  else if(!item)
    return;

  createBookmarksDatabase();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.setForwardOnly(true);
	query.exec("PRAGMA locking_mode = EXCLUSIVE");
	query.prepare
	  ("INSERT OR REPLACE INTO folders "
	   "(name, parent_oid, temporary) "
	   "VALUES (?, ?, ?)");
	query.bindValue
	  (0,
	   dmisc::etm(item->text().toUtf8(), true, &ok).toBase64());

	if(item->data(Qt::UserRole + 3).toLongLong() < 0)
	  query.bindValue(1, QVariant(QVariant::Int));
	else
	  query.bindValue(1, item->data(Qt::UserRole + 3).toLongLong());

	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.bindValue(2, temporary);

	if(ok)
	  query.exec();

	/*
	** We use max() because the folder name may not be unique.
	** The database lock should guarantee that the retrieved oid
	** is the correct one.
	*/

	if(query.exec("SELECT MAX(oid) FROM folders"))
	  if(query.next())
	    item->setData(query.value(0).toLongLong(), Qt::UserRole + 2);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
}

void dbookmarkswindow::updateFolder(const QString &name,
				    const qint64 oid)
{
  if(dooble::s_settings.value("settingsWindow/"
			      "disableAllEncryptedDatabaseWrites",
			      false).
     toBool())
    return;

  createBookmarksDatabase();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	bool ok = true;
	QSqlQuery query(db);

	query.prepare
	  ("UPDATE folders SET name = ? "
	   "WHERE oid = ? AND "
	   "temporary = ?");
	query.bindValue
	  (0,
	   dmisc::etm(name.toUtf8(), true, &ok).toBase64());
	query.bindValue(1, oid);
	query.bindValue(2, temporary);

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
}

void dbookmarkswindow::updateBookmark(const QUrl &originalUrl)
{
  Q_UNUSED(originalUrl);

  /*
  ** Updates the currently-selected bookmark.
  */

  qint64 folderOid = -2;
  QStandardItem *item1 = 0;
  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    item1 = dooble::s_bookmarksFolderModel->itemFromIndex(list.takeFirst());
  else
    return;

  if(item1)
    folderOid = item1->data(Qt::UserRole + 2).toLongLong();

  if(folderOid < -1)
    return;

  /*
  ** OK, so we have the folder's oid. Now we need the bookmark's
  ** icon, title, location, and the description.
  */

  list = ui.bookmarks->selectionModel()->selectedRows(0);

  if(list.isEmpty())
    return;
  else
    item1 = m_urlModel->itemFromIndex(list.takeFirst());

  if(item1 &&
     m_urlModel->item(item1->row(), 1) &&
     m_urlModel->item(item1->row(), 5))
    {
      createBookmarksDatabase();

      QUrl url;
      QIcon icon(item1->icon());
      QString title(item1->text());
      QString description(m_urlModel->item(item1->row(), 5)->text());

      url = QUrl::fromUserInput(m_urlModel->item(item1->row(), 1)->text());
      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

      if(icon.isNull())
	icon = dmisc::iconForUrl(url);

      if(title.isEmpty())
	title = url.toString(QUrl::StripTrailingSlash);

      if(dooble::s_settings.value("settingsWindow/"
				  "disableAllEncryptedDatabaseWrites",
				  false).
	 toBool())
	goto done_label;

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "bookmarks.db");

	if(db.open())
	  {
	    QSqlQuery query(db);
	    bool ok = true;
	    int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	    query.prepare
	      ("UPDATE bookmarks SET "
	       "icon = ?, "
	       "title = ?, "
	       "url = ?, "
	       "description = ? "
	       "WHERE url_hash = ? AND "
	       "folder_oid = ? AND "
	       "temporary = ?");

	    QBuffer buffer;
	    QByteArray bytes;

	    buffer.setBuffer(&bytes);

	    if(buffer.open(QIODevice::WriteOnly))
	      {
		QDataStream out(&buffer);

		out << icon;

		if(out.status() != QDataStream::Ok)
		  bytes.clear();
	      }
	    else
	      bytes.clear();

	    query.bindValue
	      (0,
	       dmisc::etm(bytes, true, &ok));
	    buffer.close();

	    if(ok)
	      {
		if(title.isEmpty())
		  query.bindValue
		    (1,
		     dmisc::etm
		     (url.toEncoded(QUrl::StripTrailingSlash),
		      true, &ok).toBase64());
		else
		  query.bindValue
		    (1,
		     dmisc::etm(title.toUtf8(), true, &ok).
		     toBase64());
	      }

	    if(ok)
	      query.bindValue
		(2,
		 dmisc::etm(url.
			    toEncoded(QUrl::StripTrailingSlash),
			    true, &ok).toBase64());

	    if(ok)
	      query.bindValue
		(3,
		 dmisc::etm(description.toUtf8(), true, &ok).
		 toBase64());

	    if(ok)
	      query.bindValue
		(4,
		 dmisc::hashedString(url.
				     toEncoded(QUrl::
					       StripTrailingSlash),
				     &ok).toBase64());

	    query.bindValue(5, folderOid);
	    query.bindValue(6, temporary);

	    if(ok)
	      query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase("bookmarks");
    done_label:
      emit changed();
      slotSort(ui.bookmarks->horizontalHeader()->
	       sortIndicatorSection(),
	       ui.bookmarks->horizontalHeader()->
	       sortIndicatorOrder());
      slotTextChanged(ui.searchLineEdit->text().trimmed());
    }
}

void dbookmarkswindow::slotAddBookmark(const QUrl &url,
				       const QIcon &icon,
				       const QString &title,
				       const QString &description,
				       const QDateTime &addDate,
				       const QDateTime &lastModified)
{
  if(url.isEmpty() || !url.isValid())
    return;

  if(dooble::s_settings.value("settingsWindow/"
			      "disableAllEncryptedDatabaseWrites", false).
     toBool())
    goto done_label;

  if(isBookmarked(url))
    return;

  createBookmarksDatabase();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.prepare
	  ("INSERT OR REPLACE INTO bookmarks "
	   "(title, url, date_created, last_visited, icon, visits, "
	   "temporary, description, url_hash) "
	   "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

	if(title.isEmpty())
	  query.bindValue
	    (0,
	     dmisc::etm(url.toEncoded(QUrl::StripTrailingSlash),
			true, &ok).toBase64());
	else
	  query.bindValue
	    (0,
	     dmisc::etm
	     (title.toUtf8(), true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (1,
	     dmisc::etm(url.toEncoded(QUrl::StripTrailingSlash),
			true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (2,
	     dmisc::etm(addDate.toString(Qt::ISODate).
			toUtf8(), true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (3,
	     dmisc::etm(lastModified.toString(Qt::ISODate).
			toUtf8(), true, &ok).toBase64());

	QBuffer buffer;
	QByteArray bytes;

	buffer.setBuffer(&bytes);

	if(buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    if(icon.isNull())
	      out << dmisc::iconForUrl(url);
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	if(ok)
	  query.bindValue
	    (4,
	     dmisc::etm(bytes, true, &ok));

	buffer.close();

	if(ok)
	  query.bindValue
	    (5,
	     dmisc::etm(QString::number(1).toLatin1(),
			true, &ok).toBase64());

	query.bindValue(6, temporary);

	if(ok)
	  query.bindValue
	    (7,
	     dmisc::etm(description.toUtf8(), true, &ok).
	     toBase64());

	if(ok)
	  query.bindValue
	    (8,
	     dmisc::hashedString(url.
				 toEncoded(QUrl::
					   StripTrailingSlash),
				 &ok).toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  populateTable();
 done_label:
  emit changed();
}

void dbookmarkswindow::populateTable(const qint64 folderOid)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Remember the user's current selection. After the table
  ** is populated, highlight an approximate selection.
  */

  int hPos = ui.bookmarks->horizontalScrollBar()->value();
  int vPos = ui.bookmarks->verticalScrollBar()->value();
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	m_urlModel->removeRows(0, m_urlModel->rowCount());

	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(folderOid == -1)
	  {
	    /*
	    ** The Uncategorized folder should also contain bookmarks
	    ** that have been abandoned.
	    */

	    query.prepare("SELECT title, url, date_created, "
			  "last_visited, "
			  "visits, icon, description "
			  "FROM bookmarks WHERE "
			  "temporary = ? "
			  "AND "
			  "(folder_oid = -1 OR "
			  "folder_oid NOT IN (SELECT oid FROM "
			  "folders WHERE temporary = ?))");
	    query.bindValue(0, temporary);
	    query.bindValue(1, temporary);
	  }
	else
	  {
	    query.prepare("SELECT title, url, date_created, "
			  "last_visited, "
			  "visits, icon, description "
			  "FROM bookmarks WHERE "
			  "temporary = ? AND "
			  "folder_oid = ?");
	    query.bindValue(0, temporary);
	    query.bindValue(1, folderOid);
	  }

	if(query.exec())
	  while(query.next())
	    {
	      bool ok = true;
	      QUrl url
		(QUrl::fromEncoded
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(1).toByteArray()), &ok),
		  QUrl::StrictMode));

	      if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		continue;

	      QIcon icon;
	      QBuffer buffer;
	      QString title
		(QString::fromUtf8
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QString visits
		(dmisc::daa
		 (QByteArray::fromBase64
		  (query.value(4).toByteArray()), &ok));

	      if(!ok)
		continue;

	      QString description
		(dmisc::daa
		 (QByteArray::fromBase64
		  (query.value(6).toByteArray()), &ok));

	      if(!ok)
		continue;

	      QDateTime dateTime1
		(QDateTime::fromString
		 (QString::fromUtf8
		  (dmisc::daa
		   (QByteArray::fromBase64
		    (query.value(2).toByteArray()), &ok)),
		  Qt::ISODate));

	      if(!ok)
		continue;

	      QDateTime dateTime2
		(QDateTime::fromString
		 (QString::fromUtf8
		  (dmisc::daa
		   (QByteArray::fromBase64
		    (query.value(3).toByteArray()), &ok)),
		  Qt::ISODate));

	      if(!ok)
		continue;

	      QByteArray bytes
		(dmisc::daa
		 (query.value(5).toByteArray(), &ok));

	      if(!ok)
		continue;

	      QStandardItem *item = new QStandardItem(title);

	      item->setData(title, Qt::UserRole);
	      buffer.setBuffer(&bytes);

	      if(buffer.open(QIODevice::ReadOnly))
		{
		  QDataStream in(&buffer);

		  in >> icon;

		  if(in.status() != QDataStream::Ok)
		    icon = QIcon();

		  buffer.close();
		}
	      else
		icon = QIcon();

	      if(icon.isNull())
		icon = dmisc::iconForUrl(url);

	      item->setIcon(icon);
	      item->setEditable(false);
	      m_urlModel->setRowCount(m_urlModel->rowCount() + 1);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 0, item);
	      item = new QStandardItem(url.toString(QUrl::StripTrailingSlash));
	      item->setData(url.toString(QUrl::StripTrailingSlash),
			    Qt::UserRole);
	      item->setEditable(false);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 1, item);
	      item = new QStandardItem
		(dateTime1.toString("MM/dd/yyyy hh:mm:ss AP"));
	      item->setData(dateTime1.toString("yyyy/MM/dd hh:mm:ss"),
			    Qt::UserRole);
	      item->setEditable(false);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 2, item);
	      item = new QStandardItem
		(dateTime2.toString("MM/dd/yyyy hh:mm:ss AP"));
	      item->setData(dateTime2.toString("yyyy/MM/dd hh:mm:ss"),
			    Qt::UserRole);
	      item->setEditable(false);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 3, item);
	      item = new QStandardItem(visits);
	      item->setData(visits.toLongLong(), Qt::UserRole);
	      item->setEditable(false);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 4, item);
	      item = new QStandardItem(description);
	      m_urlModel->setItem(m_urlModel->rowCount() - 1, 5, item);
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  slotSort(ui.bookmarks->horizontalHeader()->
	   sortIndicatorSection(),
	   ui.bookmarks->horizontalHeader()->
	   sortIndicatorOrder());

  while(!list.isEmpty())
    ui.bookmarks->selectRow(list.takeFirst().row());

  ui.bookmarks->horizontalScrollBar()->setValue(hPos);
  ui.bookmarks->verticalScrollBar()->setValue(vPos);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
  QApplication::restoreOverrideCursor();
}

void dbookmarkswindow::slotDeleteFolder(void)
{
  qint64 oid = -1;
  QStandardItem *item = 0;
  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    item = dooble::s_bookmarksFolderModel->itemFromIndex(list.takeFirst());

  if(item)
    oid = item->data(Qt::UserRole + 2).toLongLong();

  if(!item || oid < 0)
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.exec("PRAGMA recursive_triggers = ON");
	query.exec("PRAGMA secure_delete = ON");
	query.prepare("DELETE FROM folders WHERE oid = ? "
		      "AND temporary = ?");
	query.bindValue(0, oid);
	query.bindValue(1, temporary);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");

  if(item->parent())
    item->parent()->removeRow(item->row());
  else
    dooble::s_bookmarksFolderModel->removeRow(item->row());

  populateTable();
  emit changed();
}

void dbookmarkswindow::populateTable(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    if((item = dooble::
	s_bookmarksFolderModel->itemFromIndex(list.takeFirst())))
      populateTable(item->data(Qt::UserRole + 2).toLongLong());
}

QList<QAction *> dbookmarkswindow::actions(QWidget *parent) const
{
  /*
  ** Retrieve top-tier folders.
  */

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QAction *action = 0;
  QList<QAction *> actions;

  action = new QAction(tr("Bookmark &Page"), parent);
  actions.append(action);
  action = new QAction(parent);
  action->setSeparator(true);
  actions.append(action);
  action = new QAction(tr("Show &Bookmarks..."), parent);
  action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_B));
  actions.append(action);

  /*
  ** Use the folders model to populate the actions container.
  */

  for(int i = 0; i < dooble::s_bookmarksFolderModel->rowCount(); i++)
    {
      QStandardItem *item = dooble::s_bookmarksFolderModel->item(i, 0);

      if(!item)
	continue;

      if(actions.size() == 3)
	{
	  action = new QAction(parent);
	  action->setSeparator(true);
	  actions.append(action);
	}

      QMenu *menu = new QMenu(parent);

      action = new QAction(item->icon(), item->text(), parent);
      action->setMenu(menu);
      menu->menuAction()->setData(item->data(Qt::UserRole + 2));
      connect(menu,
	      SIGNAL(aboutToHide(void)),
	      this,
	      SLOT(slotAboutToHideMenu(void)));
      connect(menu,
	      SIGNAL(aboutToShow(void)),
	      this,
	      SLOT(slotAboutToShowMenu(void)));
      actions.append(action);
    }

  QApplication::restoreOverrideCursor();
  return actions;
}

QUrl dbookmarkswindow::titleChanged(void) const
{
  QUrl url;
  QStandardItem *item1 = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(5));

  if(!list.isEmpty())
    if((item1 = m_urlModel->itemFromIndex(list.takeFirst())))
      {
	QStandardItem *item2 = 0;

	item2 = m_urlModel->item(item1->row(), 0);

	if(item2 && m_urlModel->item(item1->row(), 1))
	  {
	    QString title(ui.titleLineEdit->text().trimmed());

	    if(title.isEmpty())
	      ui.titleLineEdit->setText(item2->text());
	    else
	      {
		item2->setData(title, Qt::UserRole);
		item2->setText(title);
		url = QUrl::fromUserInput
		  (m_urlModel->item(item2->row(), 1)->text());
		url = QUrl::fromEncoded
		  (url.toEncoded(QUrl::StripTrailingSlash));
	      }
	  }
      }

  return url;
}

void dbookmarkswindow::slotTitleChanged(void)
{
  if(!dmisc::isSchemeAcceptedByDooble(QUrl::
				      fromUserInput(ui.locationLineEdit->
						    text()).scheme()))
    return;

  QUrl url;

  url = titleChanged();
  ui.titleLineEdit->selectAll();

  if(!url.isEmpty() && url.isValid())
    url = locationChanged();

  if(!url.isEmpty() && url.isValid())
    updateBookmark(url);
}

void dbookmarkswindow::slotTitleChanged(const int folderOid,
					const QUrl &url,
					const QString &title)
{
  /*
  ** The Bookmarks popup emitted a titleChanged() signal. Find the
  ** item in the Bookmarks table and update it's contents, but only
  ** if the selected folder's oid matches the provided oid.
  */

  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    {
      QStandardItem *item = dooble::s_bookmarksFolderModel->itemFromIndex
	(list.takeFirst());

      if(item && folderOid == item->data(Qt::UserRole + 2).toLongLong())
	for(int i = 0; i < m_urlModel->rowCount(); i++)
	  if(m_urlModel->item(i, 0) && m_urlModel->item(i, 1) &&
	     m_urlModel->item(i, 1)->text() == url.
	     toString(QUrl::StripTrailingSlash))
	    {
	      QString l_title(title);

	      if(l_title.isEmpty())
		l_title = url.toString(QUrl::StripTrailingSlash);

	      if(!l_title.isEmpty())
		{
		  m_urlModel->item(i, 0)->setText(title);

		  /*
		  ** Now update the title textfield if the bookmark is also
		  ** selected.
		  */

		  if(ui.bookmarks->selectionModel()->
		     isRowSelected(i, ui.bookmarks->rootIndex()))
		    {
		      ui.titleLineEdit->setText(l_title);
		      ui.titleLineEdit->selectAll();
		    }
		}

	      break;
	    }
    }
}

QUrl dbookmarkswindow::locationChanged(void) const
{
  QUrl originalUrl;
  QStandardItem *item1 = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(5));

  if(!list.isEmpty())
    if((item1 = m_urlModel->itemFromIndex(list.takeFirst())))
      {
	QStandardItem *item2 = 0;

	item2 = m_urlModel->item(item1->row(), 1);

	if(item2)
	  {
	    QUrl url
	      (dmisc::correctedUrlPath(QUrl::fromUserInput(ui.
							   locationLineEdit->
							   text().trimmed())));

	    if(url.isValid())
	      {
		originalUrl = QUrl::fromUserInput(item2->text());
		originalUrl = QUrl::fromEncoded
		  (originalUrl.toEncoded(QUrl::StripTrailingSlash));
		item2->setData(url.toString(QUrl::StripTrailingSlash),
			       Qt::UserRole);
		item2->setText(url.toString(QUrl::StripTrailingSlash));
		ui.locationLineEdit->setText(item2->text());

		QStandardItem *item3 = m_urlModel->item(item1->row(), 0);

		if(item3)
		  {
		    url = QUrl::fromEncoded
		      (url.toEncoded(QUrl::StripTrailingSlash));
		    item3->setIcon(dmisc::iconForUrl(url));
		  }
	      }
	    else
	      ui.locationLineEdit->setText(item2->text());
	  }
      }

  return originalUrl;
}

void dbookmarkswindow::slotLocationChanged(void)
{
  if(!dmisc::isSchemeAcceptedByDooble(QUrl::
				      fromUserInput(ui.locationLineEdit->
						    text()).scheme()))
    return;

  QUrl url;

  url = titleChanged();

  if(!url.isEmpty() && url.isValid())
    {
      url = locationChanged();
      ui.locationLineEdit->selectAll();
    }

  if(!url.isEmpty() && url.isValid())
    updateBookmark(url);
}

void dbookmarkswindow::slotBookmarkReceived(const QModelIndex &index)
{
  /*
  ** This method is reached when a bookmark is dropped into
  ** a folder.
  */

  if(index.isValid() && index != ui.folders->currentIndex())
    {
      QStandardItem *item1 = dooble::s_bookmarksFolderModel->itemFromIndex
	(index);

      if(item1)
	{
	  qint64 oid = item1->data(Qt::UserRole + 2).toLongLong();
	  QStandardItem *item2 = 0;
	  QModelIndexList list
	    (ui.bookmarks->selectionModel()->selectedRows(1));

	  if(!list.isEmpty())
	    if((item2 = m_urlModel->itemFromIndex(list.takeFirst())))
	      {
		if(dooble::s_settings.
		   value("settingsWindow/"
			 "disableAllEncryptedDatabaseWrites",
			 false).
		   toBool())
		  goto done_label;

		createBookmarksDatabase();

		{
		  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							      "bookmarks");

		  db.setDatabaseName(dooble::s_homePath +
				     QDir::separator() + "bookmarks.db");

		  if(db.open())
		    {
		      QSqlQuery query(db);
		      QUrl url(QUrl::fromUserInput(item2->text()));
		      bool ok = true;
		      int temporary = dmisc::passphraseWasAuthenticated() ?
			0 : 1;

		      query.prepare
			("UPDATE bookmarks SET folder_oid = ? "
			 "WHERE url_hash = ? AND "
			 "temporary = ?");
		      query.bindValue(0, oid);
		      query.bindValue
			(1,
			 dmisc::hashedString(url.
					     toEncoded(QUrl::
						       StripTrailingSlash),
					     &ok).toBase64());
		      query.bindValue(2, temporary);

		      if(ok)
			query.exec();
		    }

		  db.close();
		}

		QSqlDatabase::removeDatabase("bookmarks");
	      done_label:
		emit changed();
		m_urlModel->removeRow(item2->row());
	      }
	}
    }
}

bool dbookmarkswindow::isBookmarked(const QUrl &url) const
{
  qint64 count = 0;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.setForwardOnly(true);
	query.prepare("SELECT COUNT(*) FROM bookmarks WHERE "
		      "url_hash = ? AND "
		      "temporary = ?");
	query.bindValue
	  (0,
	   dmisc::hashedString(url.
			       toEncoded(QUrl::
					 StripTrailingSlash),
			       &ok).toBase64());
	query.bindValue(1, temporary);

	if(ok && query.exec())
	  if(query.next())
	    count = query.value(0).toLongLong();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  return count > 0;
}

void dbookmarkswindow::slotRefresh(void)
{
  populateTable();
}

void dbookmarkswindow::slotDescriptionChanged(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(5));

  if(!list.isEmpty())
    if((item = m_urlModel->itemFromIndex(list.takeFirst())))
      {
	QUrl url;

	if(m_urlModel->item(item->row(), 1))
	  {
	    url = QUrl::fromUserInput
	      (m_urlModel->item(item->row(), 1)->text());
	    url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	    item->setText(ui.descriptionTextEdit->toPlainText().trimmed());
	    updateBookmark(url);
	  }
      }
}

void dbookmarkswindow::slotFolderDataChanged
(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  Q_UNUSED(bottomRight);

  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    if(topLeft == list.first())
      renameFolder(topLeft);
}

void dbookmarkswindow::slotAboutToHideMenu(void)
{
  /*
  ** It's not wise to clear the menu here. If you do, the action
  ** that is triggered will cause the opening of the Bookmarks window.
  ** Why? I don't know. I suspect that this slot is reached before
  ** the action's triggered() signal is emitted although I cannot
  ** confirm that. The issue is specific to OS X.
  */

#ifndef Q_OS_MAC
  QMenu *menu = qobject_cast<QMenu *> (sender());

  if(menu)
    menu->clear();
#endif
}

void dbookmarkswindow::slotAboutToShowMenu(void)
{
  QMenu *parentMenu = qobject_cast<QMenu *> (sender());

  if(!parentMenu)
    return;

  qint64 oid = parentMenu->menuAction()->data().toLongLong();

  parentMenu->clear();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	/*
	** Retrieve the bookmarks that are associated with this folder.
	*/

	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);
	QMultiMap<QString, QVariantList> titles;

	query.setForwardOnly(true);

	if(oid == -1)
	  {
	    query.prepare("SELECT title, url, icon "
			  "FROM bookmarks WHERE "
			  "temporary = ? AND "
			  "(folder_oid = -1 OR "
			  "folder_oid NOT IN (SELECT oid FROM "
			  "folders WHERE temporary = ?))");
	    query.bindValue(0, temporary);
	    query.bindValue(1, temporary);
	  }
	else
	  {
	    query.prepare("SELECT title, url, icon "
			  "FROM bookmarks WHERE "
			  "temporary = ? AND "
			  "folder_oid = ?");
	    query.bindValue(0, temporary);
	    query.bindValue(1, oid);
	  }

	if(query.exec())
	  while(query.next())
	    {
	      bool ok = true;
	      QUrl url
		(QUrl::fromEncoded
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(1).toByteArray()), &ok),
		  QUrl::StrictMode));

	      if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		continue;

	      QIcon icon;
	      QBuffer buffer;
	      QString title
		(QString::fromUtf8
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QByteArray bytes
		(dmisc::daa
		 (query.value(2).toByteArray(), &ok));

	      if(!ok)
		continue;

	      QVariantList list;

	      buffer.setBuffer(&bytes);

	      if(buffer.open(QIODevice::ReadOnly))
		{
		  QDataStream in(&buffer);

		  in >> icon;

		  if(in.status() != QDataStream::Ok)
		    icon = QIcon();

		  buffer.close();
		}
	      else
		icon = QIcon();

	      if(icon.isNull())
		icon = dmisc::iconForUrl(url);

	      if(title.isEmpty())
		title = url.toString(QUrl::StripTrailingSlash);

	      if(title.length() > dooble::MAX_NUMBER_OF_MENU_TITLE_CHARACTERS)
		title = title.mid
		  (0,
		   dooble::MAX_NUMBER_OF_MENU_TITLE_CHARACTERS - 3).
		  trimmed() + "...";

	      if(title.contains("&"))
		title = title.replace("&", "&&");

	      list.append(url);
	      list.append(icon);
	      titles.insert(title, list);
	    }

	QMapIterator<QString, QVariantList> i(titles);

	while(i.hasNext())
	  {
	    i.next();

	    if(i.value().size() < 2)
	      continue;

	    QUrl url;
	    QIcon icon;
	    QAction *action = 0;
	    QString title(i.key());

	    url = i.value().at(0).toUrl();
	    icon = i.value().at(1).value<QIcon> ();

	    if(icon.isNull())
	      icon = dmisc::iconForUrl(url);

	    action = new QAction(icon, title, parentMenu->parent());
	    action->setData(url);
	    connect(action,
		    SIGNAL(triggered(void)),
		    parentMenu->parent(),
		    SLOT(slotLinkActionTriggered(void)));

	    /*
	    ** I don't know of a smooth method to sort the bookmarks
	    ** by their URLs after they've been sorted (via the QMultiMap)
	    ** by their titles.
	    */

	    QAction *before = 0;

	    if(titles.count(title) > 1)
	      for(int j = 0; j < parentMenu->actions().size(); j++)
		{
		  before = parentMenu->actions().at(j);

		  if(url.toString(QUrl::StripTrailingSlash) <=
		     before->data().toUrl().
		     toString(QUrl::StripTrailingSlash))
		    break;
		}

	    parentMenu->insertAction(before, action);
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");

  /*
  ** Retrieve the sub-folders that are associated with this folder.
  */

  QModelIndexList list(dooble::s_bookmarksFolderModel->
		       match(dooble::s_bookmarksFolderModel->index(0, 0),
			     Qt::UserRole + 2,
			     oid,
			     1,
			     Qt::MatchRecursive));

  if(!list.isEmpty())
    {
      QStandardItem *item1 = dooble::s_bookmarksFolderModel->itemFromIndex
	(list.first());

      for(int i = 0; i < item1->rowCount(); i++)
	{
	  QStandardItem *item2 = item1->child(i, 0);

	  if(!item2)
	    continue;

	  QMenu *menu = new QMenu(parentMenu->parentWidget());
	  QAction *action = new QAction(item2->icon(), item2->text(),
					parentMenu->parentWidget());

	  action->setMenu(menu);
	  menu->menuAction()->setData(item2->data(Qt::UserRole + 2));
	  connect(menu,
		  SIGNAL(aboutToHide(void)),
		  this,
		  SLOT(slotAboutToHideMenu(void)));
	  connect(menu,
		  SIGNAL(aboutToShow(void)),
		  this,
		  SLOT(slotAboutToShowMenu(void)));
	  parentMenu->addAction(action);
	}
    }

  /*
  ** Add an "empty" action if the menu doesn't have actions.
  */

  if(parentMenu->actions().isEmpty())
    {
      QAction *action = new QAction(dmisc::iconForUrl(QUrl()), tr("Empty"),
				    parentMenu->parentWidget());

      action->setEnabled(false);
      parentMenu->addAction(action);
    }

  parentMenu->update();
}

void dbookmarkswindow::slotExport(void)
{
  QString path
    (dooble::s_settings.value("settingsWindow/myRetrievedFiles", "").
     toString());
  QFileInfo fileInfo(path);
  QFileDialog fileDialog(this);

  if(!fileInfo.isReadable() || !fileInfo.isWritable())
    path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);
  fileDialog.setDefaultSuffix("html");
  fileDialog.setDirectory(path);
  fileDialog.setFileMode(QFileDialog::AnyFile);
  fileDialog.setLabelText(QFileDialog::Accept, tr("Save"));
  fileDialog.setNameFilter("HTML (*.html)");
  fileDialog.selectFile("bookmarks.html");
  fileDialog.setWindowModality(Qt::WindowModal);
  fileDialog.setWindowTitle(tr("Dooble Web Browser: Export Bookmarks As"));

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      QFile file(fileDialog.selectedFiles().value(0));

      if(file.open(QIODevice::WriteOnly | QIODevice::Truncate |
		   QIODevice::Text))
	{
	  file.write("<!DOCTYPE Dooble Bookmarks>\n");
	  file.write("<META HTTP-EQUIV=\"Content-Type\" "
		     "CONTENT=\"text/html; charset=UTF-8\">\n"
		     "<TITLE>Bookmarks</TITLE>\n");
	  file.write("<DL>\n");

	  {
	    int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							"bookmarks");

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() + "bookmarks.db");

	    if(db.open())
	      {
		QSqlQuery query(db);

		query.setForwardOnly(true);
		query.prepare("SELECT title, url, date_created, "
			      "last_visited, icon "
			      "FROM bookmarks WHERE "
			      "temporary = ?");
		query.bindValue(0, temporary);

		if(query.exec())
		  while(query.next())
		    {
		      bool ok = true;
		      QUrl url
			(QUrl::fromEncoded
			 (dmisc::daa
			  (QByteArray::fromBase64
			   (query.value(1).toByteArray()), &ok),
			  QUrl::StrictMode));

		      if(!ok ||
			 !dmisc::isSchemeAcceptedByDooble(url.scheme()))
			continue;

		      QString title
			(QString::fromUtf8
			 (dmisc::daa
			  (QByteArray::fromBase64
			   (query.value(0).toByteArray()), &ok)));

		      if(!ok)
			continue;

		      QDateTime dateTime1
			(QDateTime::fromString
			 (QString::fromUtf8
			  (dmisc::daa
			   (QByteArray::fromBase64
			    (query.value(2).toByteArray()), &ok)),
			  Qt::ISODate));

		      if(!ok)
			continue;

		      QDateTime dateTime2
			(QDateTime::fromString
			 (QString::fromUtf8
			  (dmisc::daa
			   (QByteArray::fromBase64
			    (query.value(3).toByteArray()), &ok)),
			  Qt::ISODate));

		      if(!ok)
			continue;

		      QByteArray bytes
			(dmisc::daa
			 (query.value(4).toByteArray(), &ok));

		      if(!ok)
			continue;

		      file.write("<DT>\n");

		      QString ahref("");

		      ahref.append
			(QString("<A HREF=\"%1\" "
				 "ADD_DATE=\"%2\" "
				 "LAST_MODIFIED=\"%3\" "
				 "ICON=base64,\"%4\">"
				 "%5"
				 "</A>\n").
			 arg(url.toString(QUrl::StripTrailingSlash)).
			 arg(dateTime1.toMSecsSinceEpoch()).
			 arg(dateTime2.toMSecsSinceEpoch()).
			 arg(bytes.toBase64().constData()).
			 arg(title));
		      file.write(ahref.toUtf8());
		      file.write("</DT>\n");
		    }
	      }

	    db.close();
	  }

	  QSqlDatabase::removeDatabase("bookmarks");
	  file.write("</DL>");
	  file.flush();
	  file.close();
	}

      QApplication::restoreOverrideCursor();
    }
}

void dbookmarkswindow::slotImport(void)
{
  QString path
    (dooble::s_settings.value("settingsWindow/myRetrievedFiles", "").
     toString());
  QFileInfo fileInfo(path);
  QFileDialog fileDialog(this);

  if(!fileInfo.isReadable() || !fileInfo.isWritable())
    path = QStandardPaths::writableLocation
      (QStandardPaths::DesktopLocation);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog.setDefaultSuffix("html");
  fileDialog.setDirectory(path);
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setLabelText(QFileDialog::Accept, tr("Open"));
  fileDialog.setNameFilter("HTML (*.html)");
  fileDialog.selectFile("bookmarks.html");
  fileDialog.setWindowModality(Qt::WindowModal);
  fileDialog.setWindowTitle(tr("Dooble Web Browser: Import Bookmarks"));

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      QFile file(fileDialog.selectedFiles().value(0));

      if(file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  QWebEnginePage page;

	  page.setHtml(QString::fromUtf8(file.readAll()));
	  file.close();
	}

      QApplication::restoreOverrideCursor();
    }
}

qint64 dbookmarkswindow::size(void) const
{
  return QFileInfo(dooble::s_homePath + QDir::separator() + "bookmarks.db").
    size();
}

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
void dbookmarkswindow::slotShare(void)
{
  if(!dooble::s_spoton)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QStandardItem *item = 0;
  QModelIndexList list(ui.bookmarks->selectionModel()->selectedRows(1));

  if(!list.isEmpty())
    item = m_urlModel->itemFromIndex(list.takeFirst());

  if(item)
    {
      QString content("");
      QString description("");
      QString title("");
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

      if((item = m_urlModel->item(item->row(), 0)))
	title = item->text();

      if((item = m_urlModel->item(item->row(), 5)))
	description = item->text();

      dooble::s_spoton->share(url, title, description, content);
    }

  QApplication::restoreOverrideCursor();
}
#endif

void dbookmarkswindow::clear(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");

	if(dmisc::passphraseWasAuthenticated())
	  {
	    query.exec("DELETE FROM bookmarks");
	    query.exec("DELETE FROM folders");
	  }
	else
	  {
	    query.exec("DELETE FROM bookmarks WHERE temporary = 1");
	    query.exec("DELETE FROM folders WHERE temporary = 1");
	  }

	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  populate();
}

bool dbookmarkswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
