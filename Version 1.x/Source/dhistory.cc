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

#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QProgressBar>
#include <QScrollBar>
#include <QSettings>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTimer>
#include <QUrl>

#include "dhistory.h"
#include "dmisc.h"
#include "dooble.h"

dhistory::dhistory(void):QMainWindow()
{
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  m_model = new QStandardItemModel(this);
  m_model->setSortRole(Qt::UserRole);

  QStringList list;

  list << tr("Title") << tr("Host")
       << tr("Location") << tr("Last Time Visited")
       << tr("Visits") << "Description";
  m_model->setHorizontalHeaderLabels(list);
  ui.history->setModel(m_model);
  ui.history->setColumnHidden(m_model->columnCount() - 1, true);
  ui.history->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui.history->setSortingEnabled(true);
  ui.history->horizontalHeader()->setDefaultAlignment
    (Qt::AlignLeft);
  ui.history->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
  ui.history->horizontalHeader()->setSortIndicatorShown(true);
  ui.history->horizontalHeader()->setStretchLastSection(true);

  for(int i = 0; i < ui.history->horizontalHeader()->count() - 1; i++)
    ui.history->resizeColumnToContents(i);

#if QT_VERSION >= 0x050000
  ui.history->horizontalHeader()->setSectionResizeMode
    (QHeaderView::Interactive);
#else
  ui.history->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#endif
  ui.searchLineEdit->setPlaceholderText(tr("Search History"));
  slotSetIcons();
  connect(ui.history->horizontalHeader(),
	  SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
	  this,
	  SLOT(slotSort(int, Qt::SortOrder)));
  connect(ui.deletePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeletePage(void)));
  connect(ui.deleteAllPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteAll(void)));
  connect(ui.treeWidget,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slotItemSelectionChanged(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.history,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slotItemDoubleClicked(const QModelIndex &)));
  connect(ui.closePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(close(void)));
  connect(ui.history->selectionModel(),
	  SIGNAL(selectionChanged(const QItemSelection &,
				  const QItemSelection &)),
	  this,
	  SLOT(slotItemsSelected(const QItemSelection &,
				 const QItemSelection &)));
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(ui.sharePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotShare(void)));
#endif
  ui.history->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.history,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotShowContextMenu(const QPoint &)));

  m_timer = new QTimer(this);
  m_timer->setInterval(2500);
  connect(m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotTimeout(void)));
  m_searchTimer = new QTimer(this);
  m_searchTimer->setInterval(750);
  m_searchTimer->setSingleShot(true);
  connect(m_searchTimer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotPopulate(void)));
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);

  QTreeWidgetItem *item1 = 0;
  QList<QTreeWidgetItem *> items;

  item1 = new QTreeWidgetItem((QTreeWidget *) 0,
			      QStringList(tr("All")));
  item1->setData(0, Qt::UserRole, "");
  items << item1;
  item1 = new QTreeWidgetItem((QTreeWidget *) 0,
			      QStringList(tr("Today")));
  item1->setData
    (0, Qt::UserRole, QDate::currentDate().toString(Qt::ISODate));
  items << item1;
  item1 = new QTreeWidgetItem((QTreeWidget *) 0,
			      QStringList(tr("Yesterday")));
  item1->setData
    (0, Qt::UserRole, QDate::currentDate().addDays(-1).
     toString(Qt::ISODate));
  items << item1;

  QDate currentMonth(QDate::currentDate());
  QString str("");
  QTreeWidgetItem *item2 = 0;
  int month = currentMonth.toString("M").toInt();

  if(month == 1)
    str = tr("January");
  else if(month == 2)
    str = tr("February");
  else if(month == 3)
    str = tr("March");
  else if(month == 4)
    str = tr("April");
  else if(month == 5)
    str = tr("May");
  else if(month == 6)
    str = tr("June");
  else if(month == 7)
    str = tr("July");
  else if(month == 8)
    str = tr("August");
  else if(month == 9)
    str = tr("September");
  else if(month == 10)
    str = tr("October");
  else if(month == 11)
    str = tr("November");
  else
    str = tr("December");

  item2 = new QTreeWidgetItem((QTreeWidget *) 0, QStringList(str));
  item2->setData
    (0, Qt::UserRole, currentMonth.toString(Qt::ISODate).mid(0, 7));
  items << item2;

  QDate lastMonth(QDate::currentDate().addMonths(-1));

  month = lastMonth.toString("M").toInt();

  if(month == 1)
    str = tr("January");
  else if(month == 2)
    str = tr("February");
  else if(month == 3)
    str = tr("March");
  else if(month == 4)
    str = tr("April");
  else if(month == 5)
    str = tr("May");
  else if(month == 6)
    str = tr("June");
  else if(month == 7)
    str = tr("July");
  else if(month == 8)
    str = tr("August");
  else if(month == 9)
    str = tr("September");
  else if(month == 10)
    str = tr("October");
  else if(month == 11)
    str = tr("November");
  else
    str = tr("December");

  QTreeWidgetItem *item3 = new QTreeWidgetItem
    ((QTreeWidget *) 0, QStringList(str));

  item3->setData
    (0, Qt::UserRole, lastMonth.toString(Qt::ISODate).mid(0, 7));
  items << item3;
  ui.treeWidget->insertTopLevelItems(0, items);

  for(int i = 0; i < currentMonth.daysInMonth(); i++)
    {
      QDate date;

      date.setDate(currentMonth.year(), currentMonth.month(), i + 1);
      item1 = new QTreeWidgetItem
	(item2, QStringList(tr(QString::number(i + 1).toLatin1().
			       constData())));
      item1->setData(0, Qt::UserRole, date.toString(Qt::ISODate));
    }

  for(int i = 0; i < lastMonth.daysInMonth(); i++)
    {
      QDate date;

      date.setDate(lastMonth.year(), lastMonth.month(), i + 1);
      item1 = new QTreeWidgetItem
	(item3, QStringList(tr(QString::number(i + 1).toLatin1().
			       constData())));
      item1->setData(0, Qt::UserRole, date.toString(Qt::ISODate));
    }

  if(items.size() > 1)
    items.at(1)->setSelected(true);

  ui.treeWidget->setFocus();

  if(dooble::s_settings.contains("historyWindow/splitterState"))
    ui.splitter->restoreState
      (dooble::s_settings.value("historyWindow/splitterState",
				"").toByteArray());

  if(dooble::s_settings.contains("historyWindow/tableColumnsState6"))
    {
      if(!ui.history->horizontalHeader()->restoreState
	 (dooble::s_settings.value("historyWindow/tableColumnsState6",
				   "").toByteArray()))
	{
	  ui.history->horizontalHeader()->setDefaultAlignment
	    (Qt::AlignLeft);
	  ui.history->horizontalHeader()->setSortIndicator(0,
							   Qt::AscendingOrder);
	  ui.history->horizontalHeader()->setSortIndicatorShown(true);
	  ui.history->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
	  ui.history->horizontalHeader()->setSectionResizeMode
	    (QHeaderView::Interactive);
#else
	  ui.history->horizontalHeader()->setResizeMode
	    (QHeaderView::Interactive);
#endif

	  for(int i = 0; i < ui.history->horizontalHeader()->count() - 1;
	      i++)
	    ui.history->resizeColumnToContents(i);
	}
    }
  else
    for(int i = 0; i < ui.history->horizontalHeader()->count() - 1; i++)
      ui.history->resizeColumnToContents(i);

#if QT_VERSION >= 0x050000
  ui.history->horizontalHeader()->setSectionsMovable(true);
#else
  ui.history->horizontalHeader()->setMovable(true);
#endif
  ui.sharePushButton->setEnabled(false);

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  if(dooble::s_spoton)
    dooble::s_spoton->registerWidget(ui.sharePushButton);
  else
    dmisc::logError("dhistory::dhistory(): dooble::s_spoton is 0.");
#else
  ui.sharePushButton->setEnabled(false);
#endif
}

dhistory::~dhistory()
{
  saveState();

  if(!dmisc::passphraseWasAuthenticated())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA secure_delete = ON");
	    query.exec("DELETE FROM history WHERE temporary = 1");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase("history");
    }
  else
    purge();
}

void dhistory::show(QWidget *parent)
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

  if(!isVisible())
    populate();

  if(!dooble::s_settings.value("settingsWindow/centerChildWindows",
			       false).toBool())
    {
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

	  if(dooble::s_settings.contains("historyWindow/geometry"))
	    {
	      if(dmisc::isGnome())
		setGeometry(dooble::s_settings.
			    value("historyWindow/geometry",
				  rect).toRect());
	      else
		{
		  QByteArray g(dooble::s_settings.
			       value("historyWindow/geometry").
			       toByteArray());

		  if(!restoreGeometry(g))
		    setGeometry(rect);
		}
	    }
	  else
	    setGeometry(rect);
	}
    }
  else
    dmisc::centerChildWithParent(this, m_parent);

  ui.history->resizeColumnToContents(3);
  showNormal();
  raise();

  if(!m_timer->isActive())
    m_timer->start();
}

void dhistory::purge(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "history_window_purge_all");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "history.db");

    if(db.open())
      {
	/*
	** Delete expired history items and items having invalid
	** URLs.
	*/

	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT url, last_visited FROM history "
		      "WHERE temporary = ?");
	query.bindValue(0, temporary);

	if(query.exec())
	  {
	    QDate date(QDate::currentDate());
	    QSqlQuery deleteQuery(db);

	    deleteQuery.exec("PRAGMA secure_delete = ON");
	    date = date.addDays
	      (-qBound(1,
		       dooble::s_settings.value("settingsWindow/historyDays",
						8).toInt(),
		       366));

	    while(query.next())
	      {
		bool ok = true;
	        QUrl url
		  (QUrl::fromEncoded
		   (dmisc::daa
		    (QByteArray::fromBase64
		     (query.value(0).toByteArray()), &ok),
		    QUrl::StrictMode));

		if(ok && url.isValid())
		  {
		    QDate urlDate
		      (QDate::fromString
		       (QString::fromUtf8
			(dmisc::daa
			 (QByteArray::fromBase64
			  (query.value(1).toByteArray()), &ok)),
			Qt::ISODate));

		    if(!ok || urlDate <= date)
		      {
			deleteQuery.prepare
			  ("DELETE FROM history WHERE "
			   "url = ? AND "
			   "temporary = ?");
			deleteQuery.bindValue(0, query.value(0));
			deleteQuery.bindValue(1, temporary);
			deleteQuery.exec();
		      }
		  }
		else
		  {
		    deleteQuery.prepare("DELETE FROM history WHERE "
					"url = ? AND temporary = ?");
		    deleteQuery.bindValue(0, query.value(0));
		    deleteQuery.bindValue(1, temporary);
		    deleteQuery.exec();
		  }
	      }
	  }

	if(dmisc::passphraseWasAuthenticated())
	  {
	    /*
	    ** Delete session-based history items.
	    */

	    QSqlQuery deleteQuery(db);

	    deleteQuery.exec("PRAGMA secure_delete = ON");
	    deleteQuery.exec
	      ("DELETE FROM history WHERE temporary = 1");
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history_window_purge_all");
  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(m_model->rowCount()).
			   arg(ui.history->selectionModel()->
			       selectedRows(0).size()));
}

void dhistory::slotSort(int column, Qt::SortOrder order)
{
  m_model->sort(column, order);
}

void dhistory::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("historyWindow/windowIcon").toString()));
  ui.closePushButton->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  ui.deletePushButton->setIcon
    (QIcon(settings.value("historyWindow/deleteButtonIcon").toString()));
  ui.deleteAllPushButton->setIcon
    (QIcon(settings.value("historyWindow/deleteAllButtonIcon").toString()));
  ui.sharePushButton->setIcon
    (QIcon(settings.value("spotonIcon").toString()));
  emit iconsChanged();
}

void dhistory::slotTimeout(void)
{
  QFileInfo fileInfo(dooble::s_homePath + QDir::separator() + "history.db");
  static QDateTime lastModificationTime;

  if(fileInfo.exists())
    {
      if(fileInfo.lastModified() <= lastModificationTime)
	return;
      else
	lastModificationTime = fileInfo.lastModified();
    }
  else
    lastModificationTime = QDateTime();

  populate();
}

void dhistory::slotDeletePage(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Delete an entry, or more, from the history table.
  */

  m_timer->stop();

  int removedRows = 0;
  QStringList items;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  /*
  ** Sort the list by row number. If the list is not sorted,
  ** the following while-loop misbehaves.
  */

  qSort(list);

  for(int i = 0; i < list.size(); i++)
    {
      QStandardItem *item = m_model->item
	(list.at(i).row() - removedRows, 2);

      if(item)
	{
	  items.append(item->text());

	  if(m_model->removeRow(item->row()))
	    removedRows += 1;
	}
    }

  if(!items.isEmpty())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase
	  ("QSQLITE", "history_window_delete");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  while(!items.isEmpty())
	    {
	      QSqlQuery query(db);
	      QString text(items.takeFirst());
	      QUrl url(QUrl::fromUserInput(text));
	      bool ok = true;
	      int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	      query.exec("PRAGMA secure_delete = ON");
	      query.prepare("DELETE FROM history WHERE "
			    "url_hash = ? AND "
			    "temporary = ?");
	      query.bindValue
		(0,
		 dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash),
				     &ok).toBase64());
	      query.bindValue(1, temporary);

	      if(ok)
		query.exec();
	    }

	db.close();
      }

      QSqlDatabase::removeDatabase("history_window_delete");
    }

  if(!list.isEmpty())
    {
      if(list.first().row() >= m_model->rowCount())
	ui.history->selectRow(m_model->rowCount() - 1);
      else
	ui.history->selectRow(list.first().row());
    }

  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(m_model->rowCount()).
			   arg(ui.history->selectionModel()->
			       selectedRows(0).size()));
  m_timer->start();
  QApplication::restoreOverrideCursor();
}

void dhistory::slotOpen(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = m_model->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit open(url);
    }
}

void dhistory::slotCopyUrl(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = m_model->itemFromIndex(list.takeFirst());

  if(item && QApplication::clipboard())
    QApplication::clipboard()->setText(item->text());
}

void dhistory::slotOpenInNewTab(void)
{
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  while(!list.isEmpty())
    {
      QStandardItem *item = m_model->itemFromIndex(list.takeFirst());

      if(item)
	{
	  QUrl url(QUrl::fromUserInput(item->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit createTab(url);
	}
    }
}

void dhistory::slotOpenInNewWindow(void)
{
  QStandardItem *item = 0;
  QModelIndexList list(ui.history->selectionModel()->selectedRows(2));

  if(!list.isEmpty())
    item = m_model->itemFromIndex(list.takeFirst());

  if(item)
    {
      QUrl url(QUrl::fromUserInput(item->text()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit openInNewWindow(url);
    }
}

void dhistory::slotShowContextMenu(const QPoint &point)
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
	}
      else
	{
	  menu.addAction(tr("&Bookmark"),
			 this, SLOT(slotBookmark(void)));
	  menu.addSeparator();
	  menu.addAction(tr("&Delete Pages"),
			 this, SLOT(slotDeletePage(void)));
	  menu.addSeparator();
	  menu.addAction(tr("Open in &New Tabs"),
			 this, SLOT(slotOpenInNewTab(void)));
	}

      menu.exec(ui.history->mapToGlobal(point));
    }
}

void dhistory::slotItemDoubleClicked(const QModelIndex &index)
{
  QStandardItem *item1 = 0;

  item1 = m_model->itemFromIndex(index);

  if(item1)
    {
      QStandardItem *item2 = m_model->item(item1->row(), 2);

      if(item2)
	{
	  QUrl url(QUrl::fromUserInput(item2->text()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit open(url);
	}
    }
}

void dhistory::slotParentDestroyed(void)
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

void dhistory::slotDeleteAll(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_timer->stop();

  QList<QTreeWidgetItem *> list(ui.treeWidget->selectedItems());

  if(!list.isEmpty())
    {
      QString dateStr("");
      QTreeWidgetItem *item = list.at(0);

      dateStr = item->data(0, Qt::UserRole).toString();

      if(dateStr.isEmpty())
	{
	  // All.

	  {
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history");
	    int temporary = !dmisc::passphraseWasAuthenticated();

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() + "history.db");

	    if(db.open())
	      {
		QSqlQuery query(db);

		query.exec("PRAGMA secure_delete = ON");

		if(temporary)
		  query.exec("DELETE FROM history WHERE temporary = 1");
		else
		  query.exec("DELETE FROM history");
	      }

	    db.close();
	  }

	  QSqlDatabase::removeDatabase("history");
	  statusBar()->showMessage
	    (QString(tr("%1 Item(s) / %2 Item(s) Selected")).
	     arg(m_model->rowCount()).
	     arg(ui.history->selectionModel()->
		 selectedRows(0).size()));
	  m_timer->start();
	  QApplication::restoreOverrideCursor();
	  return;
	}
    }

  QStringList items;

  for(int i = m_model->rowCount() - 1; i >= 0; i--)
    {
      QStandardItem *item = m_model->item(i, 2);

      if(item)
	{
	  items.append(item->text());
	  m_model->removeRow(item->row());
	}
    }

  if(!items.isEmpty())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase
	  ("QSQLITE", "history_window_delete_all");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  while(!items.isEmpty())
	    {
	      QSqlQuery query(db);
	      QString text(items.takeFirst());
	      QUrl url(QUrl::fromUserInput(text));
	      bool ok = true;
	      int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	      query.exec("PRAGMA secure_delete = ON");
	      query.prepare("DELETE FROM history WHERE "
			    "url_hash = ? AND "
			    "temporary = ?");
	      query.bindValue
		(0,
		 dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash),
				     &ok).toBase64());
	      query.bindValue(1, temporary);

	      if(ok)
		query.exec();
	    }

	db.close();
      }

      QSqlDatabase::removeDatabase("history_window_delete_all");
    }

  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(m_model->rowCount()).
			   arg(ui.history->selectionModel()->
			       selectedRows(0).size()));
  m_timer->start();
  QApplication::restoreOverrideCursor();
}

void dhistory::populate(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Remember the user's current selection. After the table
  ** is populated, highlight approximate selections.
  */

  int hPos = ui.history->horizontalScrollBar()->value();
  int vPos = ui.history->verticalScrollBar()->value();
  QModelIndexList list(ui.history->selectionModel()->selectedRows());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history_window");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "history.db");

    if(db.open())
      {
	m_model->removeRows(0, m_model->rowCount());

	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QString dateStr("");
	QString searchStr
	  (ui.searchLineEdit->text().toLower().trimmed());
	QSqlQuery query(db);
	QList<QTreeWidgetItem *> list(ui.treeWidget->selectedItems());

	if(!list.isEmpty())
	  {
	    QTreeWidgetItem *item = list.at(0);

	    dateStr = item->data(0, Qt::UserRole).toString();
	  }

	query.setForwardOnly(true);
	query.prepare("SELECT title, url, last_visited, "
		      "visits, icon, description "
		      "FROM history WHERE "
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

	      if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare("DELETE FROM history WHERE "
				      "url = ? AND "
				      "temporary = ?");
		  deleteQuery.bindValue(0, query.value(1));
		  deleteQuery.bindValue(1, temporary);
		  deleteQuery.exec();
		  continue;
		}

	      QString host(url.host());
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
		  (query.value(3).toByteArray()), &ok));

	      if(!ok)
		continue;

	      QString description
		(QString::fromUtf8
		 (dmisc::daa
		  (QByteArray::fromBase64
		   (query.value(5).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QDateTime dateTime
		(QDateTime::fromString
		 (QString::fromUtf8
		  (dmisc::daa
		   (QByteArray::fromBase64
		    (query.value(2).toByteArray()), &ok)),
		  Qt::ISODate));

	      if(!ok)
		continue;

	      QByteArray bytes
		(dmisc::daa
		 (query.value(4).toByteArray(), &ok));

	      if(!ok)
		continue;

	      if(!dateStr.isEmpty())
		if(!dateTime.date().toString(Qt::ISODate).
		   contains(dateStr))
		  continue;

	      if(host.isEmpty())
		host = "localhost";

	      if(!searchStr.isEmpty())
		if(!(host.toLower().contains(searchStr) ||
		     title.toLower().contains(searchStr) ||
		     url.toString(QUrl::StripTrailingSlash).
		     toLower().contains(searchStr)))
		  continue;

	      QIcon icon;
	      QBuffer buffer;
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
	      m_model->setRowCount(m_model->rowCount() + 1);
	      m_model->setItem(m_model->rowCount() - 1, 0, item);
	      item = new QStandardItem(host);
	      item->setData(host, Qt::UserRole);
	      item->setEditable(false);
	      m_model->setItem(m_model->rowCount() - 1, 1, item);
	      item = new QStandardItem(url.toString(QUrl::StripTrailingSlash));
	      item->setData(url.toString(QUrl::StripTrailingSlash),
			    Qt::UserRole);
	      item->setEditable(false);
	      m_model->setItem(m_model->rowCount() - 1, 2, item);
	      item = new QStandardItem
		(dateTime.toString("MM/dd/yyyy hh:mm:ss AP"));

	      /*
	      ** Sorting dates is complicated enough. By
	      ** inserting dates in the below format,
	      ** we'll guarantee that the date sort will behave.
	      */

	      item->setData(dateTime.toString("yyyy/MM/dd hh:mm:ss"),
			    Qt::UserRole);
	      item->setEditable(false);
	      m_model->setItem(m_model->rowCount() - 1, 3, item);
	      item = new QStandardItem(visits);
	      item->setData(visits.toLongLong(), Qt::UserRole);
	      item->setEditable(false);
	      m_model->setItem(m_model->rowCount() - 1, 4, item);
	      item = new QStandardItem(description);
	      m_model->setItem(m_model->rowCount() - 1, 5, item);
	    }

	slotSort(ui.history->horizontalHeader()->
		 sortIndicatorSection(),
		 ui.history->horizontalHeader()->
		 sortIndicatorOrder());
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history_window");
  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(m_model->rowCount()).
			   arg(ui.history->selectionModel()->
			       selectedRows(0).size()));
  ui.history->horizontalHeader()->setStretchLastSection(true);
  ui.history->setSelectionMode(QAbstractItemView::MultiSelection);

  while(!list.isEmpty())
    ui.history->selectRow(list.takeFirst().row());

  ui.history->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui.history->horizontalScrollBar()->setValue(hPos);
  ui.history->verticalScrollBar()->setValue(vPos);
  QApplication::restoreOverrideCursor();
}

void dhistory::slotItemSelectionChanged(void)
{
  if(isVisible())
    populate();
}

void dhistory::slotTextChanged(const QString &text)
{
  Q_UNUSED(text);
  m_searchTimer->start();
}

void dhistory::closeEvent(QCloseEvent *event)
{
  m_timer->stop();
  m_searchTimer->stop();
  m_model->removeRows(0, m_model->rowCount());
  saveState();
  QMainWindow::closeEvent(event);
}

void dhistory::saveState(void)
{
  if(isVisible())
    {
      if(dmisc::isGnome())
	dooble::s_settings["historyWindow/geometry"] = geometry();
      else
	dooble::s_settings["historyWindow/geometry"] = saveGeometry();
    }

  dooble::s_settings["historyWindow/splitterState"] =
    ui.splitter->saveState();
  dooble::s_settings["historyWindow/tableColumnsState6"] =
    ui.history->horizontalHeader()->saveState();

  QSettings settings;

  if(isVisible())
    {
      if(dmisc::isGnome())
	settings.setValue("historyWindow/geometry", geometry());
      else
	settings.setValue("historyWindow/geometry", saveGeometry());
    }

  settings.setValue("historyWindow/splitterState",
		    ui.splitter->saveState());
  settings.setValue("historyWindow/tableColumnsState6",
		    ui.history->horizontalHeader()->
		    saveState());
}

void dhistory::slotItemsSelected(const QItemSelection &selected,
				 const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(m_model->rowCount()).
			   arg(ui.history->selectionModel()->
			       selectedRows(0).size()));
}

void dhistory::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
      else if(event && (event->key() == Qt::Key_Backspace ||
			event->key() == Qt::Key_Delete))
	slotDeletePage();
      else if(event->key() == Qt::Key_F &&
	      event->modifiers() == Qt::ControlModifier)
	{
	  ui.searchLineEdit->setFocus();
	  ui.searchLineEdit->selectAll();
	}
    }

  QMainWindow::keyPressEvent(event);
}

void dhistory::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(dooble::s_settings.
	  value("settingsWindow/"
		"disableAllEncryptedDatabaseWrites", false).
	  toBool())
    return;

  m_timer->stop();

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						"history_reencode");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(progress)
	  {
	    progress->setMaximum(-1);
	    progress->update();
	  }

	int temporary = -1;

	if(query.exec("SELECT url, icon, last_visited, title, visits, "
		      "description FROM history WHERE "
		      "temporary = 0"))
	  while(query.next())
	    {
	      bool ok = true;
	      QUrl url
		(QUrl::fromEncoded
		 (dmisc::daa
		  (dmisc::s_reencodeCrypt,
		   QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok),
		  QUrl::StrictMode));

	      if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		continue;

	      QIcon icon;
	      QBuffer buffer;
	      QString title
		(QString::fromUtf8
		 (dmisc::daa
		  (dmisc::s_reencodeCrypt, QByteArray::fromBase64
		   (query.value(3).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QString visits
		(dmisc::daa
		 (dmisc::s_reencodeCrypt, QByteArray::fromBase64
		  (query.value(4).toByteArray()), &ok));

	      if(!ok)
		continue;

	      QString description
		(QString::fromUtf8
		 (dmisc::daa
		  (dmisc::s_reencodeCrypt, QByteArray::fromBase64
		   (query.value(5).toByteArray()), &ok)));

	      if(!ok)
		continue;

	      QDateTime dateTime
		(QDateTime::fromString
		 (QString::fromUtf8
		  (dmisc::daa
		   (dmisc::s_reencodeCrypt, QByteArray::fromBase64
		    (query.value(2).toByteArray()), &ok)),
		  Qt::ISODate));

	      if(!ok)
		continue;

	      QByteArray bytes
		(dmisc::daa
		 (dmisc::s_reencodeCrypt, query.value(1).toByteArray(),
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
		("INSERT OR REPLACE INTO history "
		 "(url, icon, last_visited, title, visits, "
		 "temporary, description, url_hash) "
		 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
	      insertQuery.bindValue
		(0,
		 dmisc::etm(url.toEncoded(QUrl::StripTrailingSlash),
			    true, &ok).toBase64());
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
		  (1,
		   dmisc::etm(bytes, true, &ok));

	      buffer.close();

	      if(ok)
		insertQuery.bindValue
		  (2,
		   dmisc::etm(dateTime.
			      toString(Qt::ISODate).
			      toUtf8(),
			      true, &ok).toBase64());

	      if(ok)
		{
		  if(title.trimmed().isEmpty())
		    insertQuery.bindValue
		      (3,
		       dmisc::etm(url.
				  toEncoded(QUrl::StripTrailingSlash),
				  true, &ok).toBase64());
		  else
		    insertQuery.bindValue
		      (3,
		       dmisc::etm(title.toUtf8(),
				  true, &ok).toBase64());
		}

	      if(ok)
		insertQuery.bindValue
		  (4,
		   dmisc::etm(visits.toLatin1(),
			      true, &ok).toBase64());

	      insertQuery.bindValue(5, temporary);

	      if(ok)
		insertQuery.bindValue
		  (6,
		   dmisc::etm(description.toUtf8(),
			      true, &ok).toBase64());

	      if(ok)
		insertQuery.bindValue
		  (7, dmisc::
		   hashedString(url.toEncoded(QUrl::StripTrailingSlash),
				&ok).toBase64());

	      if(ok)
		insertQuery.exec();
	    }

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM history WHERE temporary <> -1");
	query.exec("UPDATE history SET temporary = 0");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history_reencode");

  if(progress)
    progress->setVisible(false);

  m_timer->start();
}

void dhistory::slotBookmark(void)
{
  QDateTime now(QDateTime::currentDateTime());
  QModelIndexList list(ui.history->selectionModel()->selectedRows(0));

  while(!list.isEmpty())
    {
      QUrl url;
      QIcon icon;
      QString title("");
      QString description("");
      QStandardItem *item = m_model->item(list.first().row(), 0);

      if(item)
	{
	  icon = item->icon();
	  title = item->text();
	}

      item = m_model->item(list.first().row(), 2);

      if(item)
	{
	  url = QUrl::fromUserInput(item->text());
	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	}

      item = m_model->item(list.first().row(), 5);

      if(item)
	description = item->text();

      if(icon.isNull())
	icon = dmisc::iconForUrl(url);

      emit bookmark(url, icon, title, description, now, now);
      list.takeFirst();
    }
}

void dhistory::slotPopulate(void)
{
  populate();
}

qint64 dhistory::size(void) const
{
  return QFileInfo(dooble::s_homePath + QDir::separator() + "history.db").
    size();
}

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
void dhistory::slotShare(void)
{
  if(!dooble::s_spoton)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(ui.history->selectionModel()->selectedRows(0));

  while(!list.isEmpty())
    {
      QIcon icon;
      QStandardItem *item = m_model->item(list.first().row(), 0);
      QString content("");
      QString description("");
      QString title("");
      QUrl url;

      if(item)
	title = item->text();

      item = m_model->item(list.first().row(), 2);

      if(item)
	{
	  url = QUrl::fromUserInput(item->text());
	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	}

      item = m_model->item(list.first().row(), 5);

      if(item)
	description = item->text();

      dooble::s_spoton->share(url, title, description, content);
      list.takeFirst();
    }

  QApplication::restoreOverrideCursor();
}
#endif

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool dhistory::event(QEvent *event)
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
	  show(m_parent);
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool dhistory::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool dhistory::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
