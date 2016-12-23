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

#include <QFileIconProvider>
#include <QIcon>
#include <QKeyEvent>
#include <QNetworkCookie>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include "dcookiewindow.h"
#include "dmisc.h"
#include "dooble.h"

dcookiewindow::dcookiewindow(dcookies *cookies, QWidget *parent):
  QMainWindow(parent)
{
  m_cookies = cookies;
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
  statusBar()->setSizeGripEnabled(false);
#endif
  ui.deleteAllPushButton->setMenu(new QMenu(this));
  ui.deleteAllPushButton->menu()->addAction(tr("Include Preserved Sites"));
  ui.deleteAllPushButton->menu()->actions().at(0)->setCheckable(true);
  ui.deleteAllPushButton->menu()->actions().at(0)->setChecked
    (dooble::s_settings.value("cookiesWindow/deleteAllPreservedChecked",
			      false).toBool());
  connect(ui.deleteAllPushButton->menu()->actions().at(0),
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slotActionToggled(bool)));
  m_model = new QStandardItemModel(this);
  m_model->setSortRole(Qt::UserRole);

  QStringList list;

  list << tr("Site") << tr("Name") << tr("Path")
       << tr("Expiration Date") << tr("HTTP")
       << tr("Secure") << tr("Session Cookie")
       << tr("Value") << tr("Preserve");
  m_model->setHorizontalHeaderLabels(list);
  ui.cookies->setModel(m_model);
  ui.cookies->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.cookies->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.cookies->setSortingEnabled(true);
  ui.cookies->header()->setDefaultAlignment(Qt::AlignLeft);
  ui.cookies->header()->setSortIndicator(0, Qt::AscendingOrder);
  ui.cookies->header()->setSortIndicatorShown(true);
  ui.cookies->header()->setStretchLastSection(true);

  for(int i = 0; i < ui.cookies->header()->count(); i++)
    ui.cookies->resizeColumnToContents(i);

  ui.cookies->header()->setSectionResizeMode(QHeaderView::Interactive);
  ui.searchLineEdit->setPlaceholderText(tr("Search Sites"));
  connect(ui.cookies,
	  SIGNAL(collapsed(const QModelIndex &)),
	  this,
	  SLOT(slotCollapsed(const QModelIndex &)));
  connect(ui.cookies->header(),
	  SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
	  this,
	  SLOT(slotSort(int, Qt::SortOrder)));
  connect(ui.deletePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteCookie(void)));
  connect(ui.deleteAllPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteAll(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.closePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(close(void)));
  connect(m_model,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slotCheckBoxItemChanged(QStandardItem *)));

  if(dooble::s_settings.contains("cookiesWindow/tableColumnsState1"))
    {
      if(!ui.cookies->header()->restoreState
	 (dooble::s_settings.value
	  ("cookiesWindow/tableColumnsState1", "").toByteArray()))
	{
	  ui.cookies->header()->setDefaultAlignment(Qt::AlignLeft);
	  ui.cookies->header()->setSortIndicator(0, Qt::AscendingOrder);
	  ui.cookies->header()->setSortIndicatorShown(true);
	  ui.cookies->header()->setStretchLastSection(true);

	  for(int i = 0; i < ui.cookies->header()->count(); i++)
	    ui.cookies->resizeColumnToContents(i);
	}
    }
  else
    for(int i = 0; i < ui.cookies->header()->count(); i++)
      ui.cookies->resizeColumnToContents(i);

  ui.cookies->header()->setSectionsMovable(true);
  slotSetIcons();
}

dcookiewindow::~dcookiewindow()
{
  saveState();
}

void dcookiewindow::show(QWidget *parent)
{
  if(!isVisible())
    populate();

  QRect rect;

  if(parent)
    {
      rect = parent->geometry();
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

      if(dooble::s_settings.contains("cookiesWindow/geometry"))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.
			value("cookiesWindow/geometry",
			      rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.
			   value("cookiesWindow/geometry").
			   toByteArray());

	      if(!restoreGeometry(g))
		setGeometry(rect);
	    }
	}
      else
	setGeometry(rect);
    }

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      true).toBool())
    dmisc::centerChildWithParent(this, parent);

  showNormal();
  activateWindow();
  raise();
  ui.cookies->setFocus();
}

void dcookiewindow::slotSort(int column, Qt::SortOrder order)
{
  m_model->sort(column, order);
}

void dcookiewindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(),
     QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("cookiesWindow/windowIcon").toString()));
  ui.closePushButton->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  ui.deletePushButton->setIcon
    (QIcon(settings.value("cookiesWindow/deleteButtonIcon").toString()));
  ui.deleteAllPushButton->setIcon
    (QIcon(settings.value("cookiesWindow/deleteAllButtonIcon").toString()));
  emit iconsChanged();
}

void dcookiewindow::slotDeleteCookie(void)
{
  QModelIndexList list(ui.cookies->selectionModel()->selectedRows(1));

  if(!list.isEmpty())
    {
      QModelIndex index1(list.takeFirst());

      if(index1.parent().isValid())
	{
	  if(!ui.cookies->isRowHidden(index1.parent().row(),
				      ui.cookies->rootIndex()))
	    {
	      QModelIndex index2(m_model->index(index1.row(), 2,
						index1.parent()));
	      QNetworkCookie cookie;

	      cookie.setName(index1.data().toByteArray());
	      cookie.setDomain(m_model->itemFromIndex(index1.parent())->
			       data(Qt::UserRole + 1).toString());
	      cookie.setPath(index2.data().toString());
	      m_cookies->removeCookie(cookie);
	      m_model->removeRows(index1.row(), 1, index1.parent());

	      if(m_model->rowCount(index1.parent()) == 0 &&
		 !m_cookies->isFavorite(m_model->
					itemFromIndex(index1.
						      parent())->
					data(Qt::UserRole + 1).
					toString()))
		{
		  /*
		  ** Remove the domain if it does not have
		  ** children and the domain is not considered
		  ** a favorite.
		  */

		  index1 = m_model->index(index1.parent().row(), 0);
		  m_cookies->removeDomains
		    (QStringList(m_model->itemFromIndex(index1)->
				 data(Qt::UserRole + 1).toString()));
		  m_model->removeRows(index1.row(), 1);
		}
	    }
	}
      else if(!ui.cookies->isRowHidden(index1.row(), ui.cookies->rootIndex()))
	{
	  /*
	  ** Remove all the cookies for the selected domain.
	  */

	  index1 = m_model->index(index1.row(), 0);
	  m_cookies->removeDomains
	    (QStringList(m_model->itemFromIndex(index1)->
			 data(Qt::UserRole + 1).toString()));
	  m_model->removeRows(index1.row(), 1);
	}
    }

  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void dcookiewindow::slotDeleteAll(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  bool deleteFavorites = dooble::s_settings.value
    ("cookiesWindow/deleteAllPreservedChecked", false).toBool();
  QStringList domains;

  /*
  ** Remove only the current view's cookies.
  */

  for(int i = m_model->rowCount() - 1; i >= 0; i--)
    if(!ui.cookies->isRowHidden(i, ui.cookies->rootIndex()))
      {
	if(m_model->item(i, 0) &&
	   m_cookies->
	   isFavorite(m_model->item(i, 0)->data(Qt::UserRole + 1).toString()))
	  {
	    if(deleteFavorites)
	      {
		domains.append
		  (m_model->item(i, 0)->data(Qt::UserRole + 1).toString());
		m_model->removeRow(i);
	      }
	  }
	else if(m_model->item(i, 0))
	  {
	    domains.append
	      (m_model->item(i, 0)->data(Qt::UserRole + 1).toString());
	    m_model->removeRow(i);
	  }
      }

  m_cookies->removeDomains(domains);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
  QApplication::restoreOverrideCursor();
}

void dcookiewindow::populate(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_model,
	     SIGNAL(itemChanged(QStandardItem *)),
	     this,
	     SLOT(slotCheckBoxItemChanged(QStandardItem *)));

  int hPos = ui.cookies->horizontalScrollBar()->value();
  int vPos = ui.cookies->verticalScrollBar()->value();
  int selectedRow = -1;
  int selectedColumn = -1;
  QList<QNetworkCookie> list(m_cookies->allCookiesAndFavorites());

  /*
  ** Remember the expanded rows.
  */

  QStringList expandedDomains;

  for(int i = 0; i < m_model->rowCount(); i++)
    if(m_model->item(i, 0) &&
       ui.cookies->isExpanded(m_model->indexFromItem(m_model->item(i, 0))))
      expandedDomains.append(m_model->item(i, 0)->text());

  /*
  ** Remember the selection.
  */

  if(ui.cookies->selectionModel()->currentIndex().isValid())
    {
      selectedRow = ui.cookies->selectionModel()->currentIndex().row();
      selectedColumn = ui.cookies->selectionModel()->currentIndex().column();
    }

  QString selectedName;
  QString selectedPath;
  QString selectedDomain;
  QModelIndexList selected(ui.cookies->selectionModel()->selectedIndexes());

  if(selected.size() >= 3)
    {
      if(selected.at(0).parent().isValid())
	selectedDomain = m_model->data(selected.at(0).parent()).toString();
      else
	selectedDomain = m_model->data(selected.at(0)).toString();

      selectedName = m_model->data(selected.at(1)).toString();
      selectedPath = m_model->data(selected.at(2)).toString();
    }

  m_model->removeRows(0, m_model->rowCount());

  QModelIndex leftIndex;
  QModelIndex rightIndex;
  QItemSelection selection;
  QFileIconProvider icon;

  /*
  ** If the domain, name, and path of the previously-selected
  ** cookie matches the cookie that was just added to the model,
  ** that cookie should be selected as long as the cookie's parent
  ** is expanded. If the cookie's parent is not expanded, the parent
  ** of the cookie should be selected.
  */

  for(int i = 0; i < list.size(); i++)
    {
      int row = 0;
      QStandardItem *item1 = 0;
      QStandardItem *item2 = 0;
      QNetworkCookie cookie(list.at(i));
      QList<QStandardItem *> items;

      if(cookie.domain().isEmpty())
	continue;

      items = m_model->findItems(cookie.domain());

      if(!items.isEmpty())
	{
	  if(cookie.name().isEmpty())
	    /*
	    ** This is only true if this cookie is a favorite.
	    */

	    continue;

	  /*
	  ** Prevent duplicate entries. Just like in dcookies,
	  ** a cookie's uniqueness is defined by its name,
	  ** domain, and path.
	  */

	  bool found = false;

	  item1 = items.at(0);

	  if(m_cookies->
	     isFavorite(item1->data(Qt::UserRole + 1).toString()))
	    {
	      m_model->item(item1->row(), 8)->setData(true, Qt::UserRole);
	      m_model->item(item1->row(), 8)->setCheckState(Qt::Checked);
	      m_model->item(item1->row(), 8)->setBackground
		(QColor(0, 153, 153));
	    }
	  else
	    {
	      m_model->item(item1->row(), 8)->setData(false, Qt::UserRole);
	      m_model->item(item1->row(), 8)->setCheckState(Qt::Unchecked);
	      m_model->item(item1->row(), 8)->setBackground(QBrush());
	    }

	  for(int j = 0; j < item1->rowCount(); j++)
	    if(item1->child(j, 1)->text() == cookie.name() &&
	       item1->child(j, 2)->text() == cookie.path())
	      {
		/*
		** Update fields.
		*/

		item1->child(j, 3)->setText
		  (cookie.expirationDate().toLocalTime().
		   toString("MM/dd/yyyy hh:mm:ss AP"));
		item1->child(j, 3)->setData
		  (cookie.expirationDate().toLocalTime().
		   toString("yyyy/MM/dd hh:mm:ss"),
		   Qt::UserRole);
		item1->child(j, 4)->setText
		  (QVariant(cookie.isHttpOnly()).toString());
		item1->child(j, 4)->setData
		  (item1->child(j, 4)->text(), Qt::UserRole);
		item1->child(j, 5)->setText
		  (QVariant(cookie.isSecure()).toString());
		item1->child(j, 5)->setData
		  (item1->child(j, 5)->text(), Qt::UserRole);
		item1->child(j, 6)->setText
		  (QVariant(cookie.isSessionCookie()).toString());
		item1->child(j, 6)->setData
		  (item1->child(j, 6)->text(), Qt::UserRole);
		item1->child(j, 7)->setText
		  (QVariant(cookie.value()).toString());
		item1->child(j, 7)->setData
		  (cookie.value(), Qt::UserRole);
		found = true;
		break;
	      }

	  if(found)
	    {
	      if(expandedDomains.contains(item1->text()))
		ui.cookies->setExpanded
		  (m_model->indexFromItem(item1), true);

	      continue;
	    }

	  item1->setRowCount(item1->rowCount() + 1);
	  row = item1->rowCount() - 1;
	}
      else
	{
	  /*
	  ** New domain being added.
	  */

	  m_model->setRowCount(m_model->rowCount() + 1);
	  row = m_model->rowCount() - 1;
	  item1 = new QStandardItem(icon.icon(QFileIconProvider::Folder),
				    cookie.domain());
	  item1->setData(cookie.domain(), Qt::UserRole + 1);
	  item1->setEditable(false);
	  item1->setData(item1->text(), Qt::UserRole);
	  m_model->setItem(row, 0, item1);
	  item2 = new QStandardItem();
	  item2->setData(item1->data(Qt::UserRole + 1), Qt::UserRole + 1);
	  item2->setEditable(false);
	  item2->setCheckable(true);
	  item2->setData(false, Qt::UserRole);
	  item2->setCheckState(Qt::Unchecked);

	  if(m_cookies->
	     isFavorite(item2->data(Qt::UserRole + 1).toString()))
	    {
	      item2->setData(true, Qt::UserRole);
	      item2->setCheckState(Qt::Checked);
	      item2->setBackground(QColor(0, 153, 153));
	    }
	  else
	    item2->setBackground(QBrush());

	  m_model->setItem(row, 8, item2);

	  if(cookie.name().isEmpty())
	    /*
	    ** Single domain, most likely a favorite.
	    */

	    continue;

	  row = 0;
	}

      if(expandedDomains.contains(item1->text()))
	ui.cookies->setExpanded(m_model->indexFromItem(item1), true);

      item1->setColumnCount(8);
      item2 = new QStandardItem();
      item2->setEditable(false);
      item1->setChild(row, 0, item2);
      item2 = new QStandardItem(QVariant(cookie.name()).toString());
      item2->setEditable(false);
      item2->setData(item2->text(), Qt::UserRole);
      item1->setChild(row, 1, item2);
      item2 = new QStandardItem(cookie.path());
      item2->setEditable(false);
      item2->setData(item2->text(), Qt::UserRole);
      item1->setChild(row, 2, item2);
      item2 = new QStandardItem
	(cookie.expirationDate().toLocalTime().
	 toString("MM/dd/yyyy hh:mm:ss AP"));
      item2->setEditable(false);
      item2->setData
	(cookie.expirationDate().toLocalTime().
	 toString("yyyy/MM/dd hh:mm:ss"),
	 Qt::UserRole);
      item1->setChild(row, 3, item2);
      item2 = new QStandardItem(QVariant(cookie.isHttpOnly()).toString());
      item2->setEditable(false);
      item2->setData(item2->text(), Qt::UserRole);
      item1->setChild(row, 4, item2);
      item2 = new QStandardItem(QVariant(cookie.isSecure()).toString());
      item2->setEditable(false);
      item2->setData(item2->text(), Qt::UserRole);
      item1->setChild(row, 5, item2);
      item2 = new QStandardItem
	(QVariant(cookie.isSessionCookie()).toString());
      item2->setEditable(false);
      item2->setData(item2->text(), Qt::UserRole);
      item1->setChild(row, 6, item2);
      item2 = new QStandardItem(QVariant(cookie.value()).toString());
      item2->setEditable(false);
      item2->setData(cookie.value(), Qt::UserRole);
      item1->setChild(row, 7, item2);
    }

  bool found = false;

  for(int i = 0; i < m_model->rowCount(); i++)
    {
      if(m_model->item(i, 0) &&
	 expandedDomains.contains(m_model->item(i, 0)->text()))
	{
	  if(selectedDomain == m_model->item(i, 0)->text())
	    {
	      for(int j = 0; j < m_model->item(i, 0)->rowCount(); j++)
		if(selectedName == m_model->item(i, 0)->child(j, 1)->text() &&
		   selectedPath == m_model->item(i, 0)->child(j, 2)->text())
		  {
		    leftIndex = m_model->index(j, 0,
					       m_model->index(i, 0));
		    rightIndex = m_model->index(j, 7,
						m_model->index(i, 0));
		    selection.select(leftIndex, rightIndex);
		    found = true;
		    break;
		  }

	      if(!found)
		{
		  leftIndex = m_model->index(i, 0);
		  rightIndex = m_model->index(i, m_model->columnCount() - 1);
		  selection.select(leftIndex, rightIndex);
		}

	      break;
	    }
	}
      else if(m_model->item(i, 0) &&
	      selectedDomain == m_model->item(i, 0)->text())
	{
	  leftIndex = m_model->index(i, 0);
	  rightIndex = m_model->index(i, m_model->columnCount() - 1);
	  selection.select(leftIndex, rightIndex);
	  break;
	}
    }

  ui.cookies->selectionModel()->select
    (selection, QItemSelectionModel::ClearAndSelect);

  if(leftIndex.isValid())
    {
      if(selectedColumn > -1)
	{
	  if(leftIndex.sibling(leftIndex.row(), selectedColumn).isValid())
	    leftIndex = leftIndex.sibling(leftIndex.row(), selectedColumn);
	  else
	    leftIndex = m_model->index(leftIndex.row(), selectedColumn);
	}

      ui.cookies->selectionModel()->setCurrentIndex
	(leftIndex, QItemSelectionModel::Current);
    }
  else
    {
      QModelIndex index;

      if(selectedRow > -1 && selectedColumn > -1)
	index = m_model->index(selectedRow, selectedColumn);
      else if(selectedRow > -1)
	index = m_model->index(selectedRow, 0);

      if(index.isValid())
	ui.cookies->selectionModel()->setCurrentIndex
	  (index, QItemSelectionModel::Current);
    }

  m_model->sort
    (ui.cookies->header()->sortIndicatorSection(),
     ui.cookies->header()->sortIndicatorOrder());

  if(selection.isEmpty())
    {
      leftIndex = m_model->index(0, 0);
      rightIndex = m_model->index(0, m_model->columnCount() - 1);
      selection.select(leftIndex, rightIndex);
      ui.cookies->selectionModel()->select
	(selection, QItemSelectionModel::ClearAndSelect);
      ui.cookies->selectionModel()->setCurrentIndex
	(leftIndex, QItemSelectionModel::Current);
    }

  ui.cookies->horizontalScrollBar()->setValue(hPos);
  ui.cookies->verticalScrollBar()->setValue(vPos);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
  connect(m_model,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slotCheckBoxItemChanged(QStandardItem *)));
  QApplication::restoreOverrideCursor();
}

void dcookiewindow::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void dcookiewindow::slotTextChanged(const QString &text)
{
  /*
  ** Search text changed.
  */

  int n = 0;
  int count = 0;
  int expiredCount = 0;
  int sessionCount = 0;
  QDateTime now(QDateTime::currentDateTime());

  for(int i = 0; i < m_model->rowCount(); i++)
    if(text.trimmed().isEmpty() ||
       (m_model->item(i, 0) &&
	m_model->item(i, 0)->text().toLower().contains(text.trimmed().
						       toLower())))
      {
	count += m_model->item(i, 0)->rowCount();
	ui.cookies->setRowHidden(i, ui.cookies->rootIndex(), false);

	for(int j = 0; j < m_model->item(i, 0)->rowCount(); j++)
	  if(m_model->item(i, 0)->child(j, 6) &&
		  QVariant(m_model->item(i, 0)->child(j, 6)->
			   data(Qt::UserRole)).toBool())
	    sessionCount += 1;
	  else if(m_model->item(i, 0)->child(j, 3) &&
		  QVariant(m_model->item(i, 0)->child(j, 3)->
			   data(Qt::UserRole)).toDateTime().
		  toLocalTime() <= now)
	    expiredCount += 1;
      }
    else
      {
	n += 1;
	ui.cookies->setRowHidden(i, ui.cookies->rootIndex(), true);
      }

  statusBar()->showMessage
    (QString(tr("%1 Cookie(s) / %2 Expired Cookie(s) / "
		"%3 Session Cookie(s) / %4 Site(s)")).
     arg(count).
     arg(expiredCount).
     arg(sessionCount).
     arg(m_model->rowCount() - n));
}

void dcookiewindow::saveState(void)
{
  dooble::s_settings["cookiesWindow/tableColumnsState1"] =
    ui.cookies->header()->saveState();

  if(isVisible())
    {
      if(dmisc::isGnome())
	dooble::s_settings["cookiesWindow/geometry"] = geometry();
      else
	dooble::s_settings["cookiesWindow/geometry"] = saveGeometry();
    }

  QSettings settings;

  settings.setValue("cookiesWindow/tableColumnsState1",
		    ui.cookies->header()->saveState());

  if(isVisible())
    {
      if(dmisc::isGnome())
	settings.setValue("cookiesWindow/geometry", geometry());
      else
	settings.setValue("cookiesWindow/geometry", saveGeometry());
    }
}

void dcookiewindow::slotCheckBoxItemChanged(QStandardItem *item)
{
  if(item && item->isCheckable())
    {
      if(item->checkState() == Qt::Checked)
	{
	  m_cookies->allowDomain
	    (item->data(Qt::UserRole + 1).toString(), true);
	  item->setBackground(QColor(0, 153, 153));
	}
      else
	{
	  m_cookies->allowDomain
	    (item->data(Qt::UserRole + 1).toString(), false);
	  item->setBackground(QBrush());
	}
    }
}

void dcookiewindow::slotDomainsRemoved(const QStringList &list)
{
  for(int i = m_model->rowCount() - 1; i >= 0; i--)
    {
      QStandardItem *item = m_model->item(i, 0);

      if(item && list.contains(item->data(Qt::UserRole + 1).toString()))
	m_model->removeRow(i);
    }

  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void dcookiewindow::slotActionToggled(bool checked)
{
  QSettings settings;

  settings.setValue("cookiesWindow/deleteAllPreservedChecked",
		    checked);
  dooble::s_settings["cookiesWindow/deleteAllPreservedChecked"] =
    checked;
}

void dcookiewindow::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
      else if(event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Delete)
	slotDeleteCookie();
      else if(event->key() == Qt::Key_F &&
	      event->modifiers() == Qt::ControlModifier)
	{
	  ui.searchLineEdit->setFocus();
	  ui.searchLineEdit->selectAll();
	}
    }

  QMainWindow::keyPressEvent(event);
}

void dcookiewindow::slotCollapsed(const QModelIndex &index)
{
  /*
  ** Select the parent if a child is selected and the parent
  ** has just been collapsed.
  */

  QModelIndexList selected(ui.cookies->selectionModel()->selectedIndexes());

  if(!selected.isEmpty())
    if(index == selected.at(0).parent())
      {
	ui.cookies->selectionModel()->clear();

	QModelIndex leftIndex(m_model->index(index.row(), 0));
	QModelIndex rightIndex
	  (m_model->index(index.row(), m_model->columnCount() - 1));
	QItemSelection selection(leftIndex, rightIndex);

	ui.cookies->selectionModel()->select
	  (selection, QItemSelectionModel::ClearAndSelect);

	if(leftIndex.isValid())
	  ui.cookies->selectionModel()->setCurrentIndex
	    (leftIndex, QItemSelectionModel::Current);
      }
}

void dcookiewindow::slotCookiesChanged(void)
{
  /*
  ** Schedule a populate.
  */

  if(isVisible())
    {
      QTimer *timer = findChild<QTimer *> ();

      if(!timer)
	{
	  timer = new QTimer(this);
	  timer->setInterval(2500);
	  timer->setSingleShot(true);
	  connect(timer,
		  SIGNAL(timeout(void)),
		  this,
		  SLOT(slotPopulate(void)));
         }

      if(timer->isActive())
	timer->stop();

      timer->start();
    }
}

void dcookiewindow::slotPopulate(void)
{
  if(isVisible())
    populate();
}

bool dcookiewindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}

void dcookiewindow::find(const QString &text)
{
  ui.searchLineEdit->setText(text);
}
