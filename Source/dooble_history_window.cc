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

#include <QClipboard>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_address_widget_completer.h"
#include "dooble_application.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_ui_utilities.h"

// Perhaps a model-view solution would have been splendid. A floating
// window is also nice.

class dooble_history_window_favorite_item: public QTableWidgetItem
{
public:
  dooble_history_window_favorite_item(void):QTableWidgetItem()
  {
  }

  bool operator <(const QTableWidgetItem &other) const
  {
    return !icon().isNull() < !other.icon().isNull();
  }
};

dooble_history_window::dooble_history_window(bool floating):dooble_main_window()
{
  m_floating = floating;
  m_parent = nullptr;
  m_save_settings_timer.setInterval(1500);
  m_save_settings_timer.setSingleShot(true);
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.period->setCurrentRow(0);
  m_ui.period->setStyleSheet("QListWidget {background-color: transparent;}");
  m_ui.period->setVisible(!m_floating);

  if(!m_floating)
    {
      m_ui.splitter->setStretchFactor(0, 0);
      m_ui.splitter->setStretchFactor(1, 1);
      m_ui.splitter->restoreState
	(QByteArray::fromBase64(dooble_settings::
				setting("history_window_splitter_state").
				toByteArray()));
    }

  m_ui.table->horizontalHeader()->setVisible(!m_floating);
  m_ui.table->setWordWrap(false);
  m_ui.table->sortItems(m_ui.table->columnCount() - 1, Qt::DescendingOrder);

  if(m_floating)
    {
      m_ui.table->setColumnHidden(TableColumns::FAVORITE, true);
      m_ui.table->setColumnHidden(TableColumns::LOCATION, true);
      m_ui.table->setColumnHidden(TableColumns::LAST_VISITED, true);
    }
  else
    for(int i = 0; i < m_ui.table->columnCount(); i++)
      m_ui.table->horizontalHeader()->resizeSection
	(i,
	 qMax(dooble_settings::
	      setting(QString("history_horizontal_header_section_size_%1").
		      arg(i)).toInt(),
	      m_ui.table->horizontalHeader()->minimumSectionSize()));

  connect(&m_save_settings_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_save_settings_timeout(void)));
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(dooble::s_application,
	  SIGNAL(favorites_cleared(void)),
	  this,
	  SLOT(slot_favorites_cleared(void)));
  connect(dooble::s_application,
	  SIGNAL(history_cleared(void)),
	  this,
	  SLOT(slot_history_cleared(void)));
  connect(dooble::s_history,
	  SIGNAL(icon_updated(const QIcon &, const QUrl &)),
	  this,
	  SLOT(slot_icon_updated(const QIcon &, const QUrl &)));
  connect(dooble::s_history,
	  SIGNAL(item_updated(const QIcon &, const QWebEngineHistoryItem &)),
	  this,
	  SLOT(slot_item_updated(const QIcon &,
				 const QWebEngineHistoryItem &)));
  connect(dooble::s_history,
	  SIGNAL(new_item(const QIcon &, const QWebEngineHistoryItem &)),
	  this,
	  SLOT(slot_new_item(const QIcon &, const QWebEngineHistoryItem &)));
  connect(dooble::s_history,
	  SIGNAL(populated(void)),
	  this,
	  SLOT(slot_populate(void)));
  connect(m_ui.period,
	  SIGNAL(currentRowChanged(int)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.splitter,
	  SIGNAL(splitterMoved(int, int)),
	  this,
	  SLOT(slot_splitter_moved(int, int)));
  connect(m_ui.table,
	  SIGNAL(delete_pressed(void)),
	  this,
	  SLOT(slot_delete_pages(void)));
  connect(m_ui.table,
	  SIGNAL(enter_pressed(void)),
	  this,
	  SLOT(slot_enter_pressed(void)));
  connect(m_ui.table,
	  SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_double_clicked(QTableWidgetItem *)));
  connect(m_ui.table->horizontalHeader(),
	  SIGNAL(sectionResized(int, int, int)),
	  this,
	  SLOT(slot_horizontal_header_section_resized(int, int, int)));
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void dooble_history_window::closeEvent(QCloseEvent *event)
{
  dooble_main_window::closeEvent(event);
}

void dooble_history_window::discover_m_parent(void)
{
  if(!m_parent)
    {
      /*
      ** Locate a Dooble window.
      */

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      QWidgetList list(QApplication::topLevelWidgets());

      for(auto i : list)
	if(qobject_cast<dooble *> (i))
	  {
	    m_parent = i;
	    connect(m_parent,
		    SIGNAL(destroyed(void)),
		    this,
		    SLOT(slot_parent_destroyed(void)),
		    Qt::UniqueConnection);
	    connect(this,
		    SIGNAL(open_link(const QUrl &)),
		    m_parent,
		    SLOT(slot_open_link(const QUrl &)),
		    Qt::UniqueConnection);
	    break;
	  }

      QApplication::restoreOverrideCursor();
    }
}

void dooble_history_window::keyPressEvent(QKeyEvent *event)
{
  if(!parent())
    {
      if(event)
	switch(event->key())
	  {
	  case Qt::Key_Escape:
	    {
	      close();
	      break;
	    }
	  default:
	    {
	      break;
	    }
	  }

      dooble_main_window::keyPressEvent(event);
    }
  else if(event)
    event->ignore();
}

void dooble_history_window::prepare_viewport_icons(void)
{
  m_ui.table->prepare_viewport_icons();
}

void dooble_history_window::resizeEvent(QResizeEvent *event)
{
  dooble_main_window::resizeEvent(event);
  save_settings();
}

void dooble_history_window::save_settings(void)
{
  if(m_floating)
    {
      if(dooble_settings::setting("save_geometry").toBool())
	dooble_settings::set_setting
	  ("history_popup_geometry", saveGeometry().toBase64());
    }
  else
    {
      if(dooble_settings::setting("save_geometry").toBool())
	dooble_settings::set_setting
	  ("history_window_geometry", saveGeometry().toBase64());

      dooble_settings::set_setting
	("history_window_splitter_state",
	 m_ui.splitter->saveState().toBase64());
    }
}

void dooble_history_window::set_row_hidden(int i)
{
  auto item1 = m_ui.table->item(i, TableColumns::TITLE);
  auto item2 = m_ui.table->item(i, TableColumns::LOCATION);
  auto item3 = m_ui.table->item(i, TableColumns::LAST_VISITED);

  if(!item1 || !item2 || !item3)
    return;

  QDateTime period(QDateTime::currentDateTime());
  QString text(m_ui.search->text().toLower().trimmed());

  switch(m_ui.period->currentRow())
    {
    case 0: // All
      {
	if(text.isEmpty())
	  m_ui.table->setRowHidden(i, false);
	else if(item1->text().toLower().contains(text) ||
		item2->text().toLower().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    case 1: // Today
    case 2: // Yesterday
      {
	if(m_ui.period->currentRow() == 2)
	  period = period.addDays(-1);

	QDateTime date_time
	  (QDateTime::fromString(item3->text(), Qt::ISODate));

	if(date_time.date() == period.date())
	  {
	    if(text.isEmpty())
	      m_ui.table->setRowHidden(i, false);
	    else if(item1->text().toLower().contains(text) ||
		    item2->text().toLower().contains(text))
	      m_ui.table->setRowHidden(i, false);
	    else
	      m_ui.table->setRowHidden(i, true);
	  }
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    default:
      {
	if(m_ui.period->currentRow() == 4)
	  period = period.addMonths(-1);

	QDateTime date_time
	  (QDateTime::fromString(item3->text(), Qt::ISODate));

	if(date_time.date().month() == period.date().month() &&
	   date_time.date().year() == period.date().year())
	  {
	    if(text.isEmpty())
	      m_ui.table->setRowHidden(i, false);
	    else if(item1->text().toLower().contains(text) ||
		    item2->text().toLower().contains(text))
	      m_ui.table->setRowHidden(i, false);
	    else
	      m_ui.table->setRowHidden(i, true);
	  }
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    }
}

void dooble_history_window::show(QWidget *parent)
{
  m_parent = parent;

  if(m_parent)
    {
      connect(m_parent,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(slot_parent_destroyed(void)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(open_link(const QUrl &)),
	      m_parent,
	      SLOT(slot_open_link(const QUrl &)),
	      Qt::UniqueConnection);
    }

  if(m_floating)
    {
      if(dooble_settings::setting("save_geometry").toBool())
	restoreGeometry
	  (QByteArray::fromBase64(dooble_settings::
				  setting("history_popup_geometry").
				  toByteArray()));
    }
  else
    {
      if(dooble_settings::setting("save_geometry").toBool())
	restoreGeometry
	  (QByteArray::fromBase64(dooble_settings::
				  setting("history_window_geometry").
				  toByteArray()));

      if(dooble_settings::setting("center_child_windows").toBool())
	dooble_ui_utilities::center_window_widget(parent, this);
    }

  bool was_visible = isVisible();

  dooble_main_window::show();

  if(!was_visible)
    m_ui.table->prepare_viewport_icons();
}

void dooble_history_window::show_normal(QWidget *parent)
{
  m_parent = parent;

  if(m_parent)
    {
      connect(m_parent,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(slot_parent_destroyed(void)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(open_link(const QUrl &)),
	      m_parent,
	      SLOT(slot_open_link(const QUrl &)),
	      Qt::UniqueConnection);
    }

  if(m_floating)
    {
      if(dooble_settings::setting("save_geometry").toBool())
	restoreGeometry
	  (QByteArray::fromBase64(dooble_settings::
				  setting("history_popup_geometry").
				  toByteArray()));
    }
  else
    {
      if(dooble_settings::setting("save_geometry").toBool())
	restoreGeometry
	  (QByteArray::fromBase64(dooble_settings::
				  setting("history_window_geometry").
				  toByteArray()));

      if(dooble_settings::setting("center_child_windows").toBool())
	dooble_ui_utilities::center_window_widget(parent, this);
    }

  bool was_visible = isVisible();

  dooble_main_window::showNormal();

  if(!was_visible)
    m_ui.table->prepare_viewport_icons();
}

void dooble_history_window::slot_copy_location(void)
{
  auto clipboard = QApplication::clipboard();

  if(!clipboard)
    return;

  auto item = m_ui.table->currentItem();

  if(!item)
    return;

  item = m_ui.table->item(item->row(), TableColumns::LOCATION);

  if(item)
    clipboard->setText(item->text());
}

void dooble_history_window::slot_delete_pages(void)
{
  auto list(m_ui.table->selectionModel()->selectedRows(TableColumns::FAVORITE));

  if(list.isEmpty())
    return;

  QMessageBox mb(this);
  auto action = qobject_cast<QAction *> (sender());
  bool favorites_included = false;

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);

  if(action && action->property("prompt").toBool())
    mb.setText(tr("Favorites may be deleted. Continue?"));
  else
    mb.setText
      (tr("Are you sure that you wish to remove the selected item(s)?"));

  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();

  if(action && action->property("prompt").toBool())
    favorites_included = true;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end());
  emit delete_rows(favorites_included, list);

  QList<QUrl> urls;

  for(int i = list.size() - 1; i >= 0; i--)
    {
      if(!favorites_included)
	if(list.at(i).data(Qt::CheckStateRole) == Qt::Checked)
	  continue;

      if(m_ui.table->isRowHidden(list.at(i).row()))
	continue;

      QUrl url(list.at(i).data(Qt::UserRole).toUrl());

      urls << url;
      dooble_address_widget_completer::remove_item(url);
      m_items.remove(url);
      m_ui.table->removeRow(list.at(i).row());
    }

  dooble::s_history->remove_items_list(urls);
  QApplication::restoreOverrideCursor();
  prepare_viewport_icons();
  slot_search_timer_timeout();
}

void dooble_history_window::slot_delete_rows
(bool favorites_included, const QModelIndexList &list)
{
  for(int i = list.size() - 1; i >= 0; i--)
    {
      if(!favorites_included)
	if(list.at(i).data(Qt::CheckStateRole) == Qt::Checked)
	  continue;

      if(m_ui.table->isRowHidden(list.at(i).row()))
	continue;

      QUrl url(list.at(i).data(Qt::UserRole).toUrl());

      m_items.remove(url);
      m_ui.table->removeRow(list.at(i).row());
    }
}

void dooble_history_window::slot_enter_pressed(void)
{
  auto list(m_ui.table->selectionModel()->selectedRows(TableColumns::FAVORITE));

  if(list.isEmpty())
    return;
  else if(list.size() >= 5)
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Open %1 pages?").arg(list.size()));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}
    }

  discover_m_parent();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < list.size(); i++)
    emit open_link(list.at(i).data(Qt::UserRole).toUrl());

  m_ui.table->setFocus();
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_favorite_changed(const QUrl &url, bool state)
{
  auto item = m_items.value(url, nullptr);

  if(!item)
    return;

  item = m_ui.table->item(item->row(), TableColumns::FAVORITE);

  if(!item)
    return;

  if(state)
    {
      QString icon_set(dooble_settings::setting("icon_set").toString());

      item->setCheckState(Qt::Checked);
      item->setIcon(QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
    }
  else
    {
      item->setCheckState(Qt::Unchecked);
      item->setIcon(QIcon());
    }
}

void dooble_history_window::slot_favorites_cleared(void)
{
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    {
      auto item = m_ui.table->item(i, TableColumns::FAVORITE);

      if(!item)
	continue;

      item->setCheckState(Qt::Unchecked);
      item->setIcon(QIcon());
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
}

void dooble_history_window::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_history_window::slot_history_cleared(void)
{
  m_ui.search->clear();

  for(int i = m_ui.table->rowCount(); i >= 0; i--)
    {
      auto item = m_ui.table->item(i, TableColumns::FAVORITE);

      if(!item)
	{
	  m_ui.table->removeRow(i);
	  continue;
	}

      if(item->checkState() == Qt::Checked)
	continue;

      QUrl url(item->data(Qt::UserRole).toUrl());

      m_items.remove(url);
      m_ui.table->removeRow(i);
    }

  slot_search_timer_timeout();
}

void dooble_history_window::slot_horizontal_header_section_resized
(int logicalIndex, int oldSize, int newSize)
{
  Q_UNUSED(oldSize);

  if(m_floating)
    return;

  if(logicalIndex >= 0)
    dooble_settings::set_setting
      (QString("history_horizontal_header_section_size_%1").arg(logicalIndex),
       newSize);
}

void dooble_history_window::slot_icon_updated(const QIcon &icon,
					      const QUrl &url)
{
  auto item = m_items.value(url);

  if(!item)
    return;

  if(icon.isNull())
    item->setIcon(dooble_favicons::icon(url));
  else
    item->setIcon(icon);
}

void dooble_history_window::slot_item_changed(QTableWidgetItem *item)
{
  if(!item)
    return;
  else if(item->column() != TableColumns::FAVORITE)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  if(item->checkState() == Qt::Checked)
    {
      QString icon_set(dooble_settings::setting("icon_set").toString());

      item->setIcon(QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
    }
  else
    item->setIcon(QIcon());

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  dooble::s_history->save_favorite
    (item->data(Qt::UserRole).toUrl(), item->checkState() == Qt::Checked);
  emit favorite_changed
    (item->data(Qt::UserRole).toUrl(), item->checkState() == Qt::Checked);
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_item_double_clicked(QTableWidgetItem *item)
{
  if(!item)
    return;

  discover_m_parent();
  emit open_link(item->data(Qt::UserRole).toUrl());
}

void dooble_history_window::slot_item_updated(const QIcon &icon,
					      const QWebEngineHistoryItem &item)
{
  if(!item.isValid())
    return;

  auto item1 = m_items.value(item.url());

  if(!item1)
    {
      slot_new_item(icon, item);
      return;
    }
  else
    {
      if(icon.isNull())
	item1->setIcon(dooble_favicons::icon(item.url()));
      else
	item1->setIcon(icon);
    }

  auto item2 = m_ui.table->item(item1->row(), TableColumns::TITLE);

  if(item2)
    {
      auto title
	(item.title().trimmed().mid(0, dooble::Limits::MAXIMUM_TITLE_LENGTH));

      if(title.isEmpty())
	title = item.url().toString().mid
	  (0, dooble::Limits::MAXIMUM_URL_LENGTH);

      if(title.isEmpty())
	title = tr("Dooble");

      if(item2->text().isEmpty())
	item2->setText(title);
      else
	{
	  if(!dooble::s_history->is_favorite(item.url()))
	    item2->setText(title);
	}

      item2->setToolTip(dooble_ui_utilities::pretty_tool_tip(item2->text()));
    }

  auto item3 = m_ui.table->item(item1->row(), TableColumns::LAST_VISITED);

  if(item3)
    {
      item3->setText(item.lastVisited().toString(Qt::ISODate));
      set_row_hidden(item3->row());
    }
}

void dooble_history_window::slot_new_item(const QIcon &icon,
					  const QWebEngineHistoryItem &item)
{
  if(!item.isValid())
    return;

  if(m_items.contains(item.url()))
    {
      slot_item_updated(icon, item);
      return;
    }

  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  auto title
    (item.title().trimmed().mid(0, dooble::Limits::MAXIMUM_TITLE_LENGTH));

  if(title.isEmpty())
    title = item.url().toString().mid(0, dooble::Limits::MAXIMUM_URL_LENGTH);

  if(title.isEmpty())
    title = tr("Dooble");

  auto item1 = new dooble_history_window_favorite_item();

  item1->setData(Qt::UserRole, item.url());
  item1->setFlags(Qt::ItemIsEnabled |
		  Qt::ItemIsSelectable |
		  Qt::ItemIsUserCheckable);

  if(dooble::s_history->is_favorite(item.url()))
    {
      auto icon_set(dooble_settings::setting("icon_set").toString());

      item1->setCheckState(Qt::Checked);
      item1->setIcon(QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
    }
  else
    item1->setCheckState(Qt::Unchecked);

  QTableWidgetItem *item2 = nullptr;

  if(icon.isNull())
    item2 = new QTableWidgetItem(dooble_favicons::icon(item.url()), title);
  else
    item2 = new QTableWidgetItem(icon, title);

  item2->setData(Qt::UserRole, item.url());
  item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item2->setToolTip(dooble_ui_utilities::pretty_tool_tip(item2->text()));
  m_items[item.url()] = item2;

  auto item3 = new QTableWidgetItem(item.url().toString());

  item3->setData(Qt::UserRole, item.url());
  item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item3->setToolTip(item3->text());

  auto item4 = new QTableWidgetItem(item.lastVisited().toString(Qt::ISODate));

  item4->setData(Qt::UserRole, item.url());
  item4->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.table->setSortingEnabled(false);
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setItem
    (m_ui.table->rowCount() - 1, TableColumns::FAVORITE, item1);
  m_ui.table->setItem
    (m_ui.table->rowCount() - 1, TableColumns::TITLE, item2);
  m_ui.table->setItem
    (m_ui.table->rowCount() - 1, TableColumns::LOCATION, item3);
  m_ui.table->setItem
    (m_ui.table->rowCount() - 1, TableColumns::LAST_VISITED, item4);

  /*
  ** Hide or show the new row.
  */

  set_row_hidden(m_ui.table->rowCount() - 1);
  m_ui.table->setSortingEnabled(true);
  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  slot_search_timer_timeout();
}

void dooble_history_window::slot_parent_destroyed(void)
{
  m_parent = nullptr;
}

void dooble_history_window::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      m_items.clear();
      m_ui.entries->setText(tr("0 Row(s)"));
      m_ui.search->clear();
      m_ui.table->setRowCount(0);
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));
  m_items.clear();
  m_ui.search->clear();
  m_ui.table->setSortingEnabled(false);

  QHash<QUrl, QHash<dooble_history::HistoryItem, QVariant> > hash
    (dooble::s_history->history());
  QHashIterator<QUrl, QHash<dooble_history::HistoryItem, QVariant> > it(hash);
  QString icon_set(dooble_settings::setting("icon_set").toString());
  int i = 0;

  m_ui.entries->setText(tr("%1 Row(s)").arg(hash.size()));
  m_ui.table->setRowCount(0);
  m_ui.table->setRowCount(hash.size());

  while(it.hasNext())
    {
      it.next();

      QDateTime last_visited
	(it.value().value(dooble_history::HistoryItem::LAST_VISITED).
	 toDateTime());
      QString title
	(it.value().value(dooble_history::HistoryItem::TITLE).toString());
      QTableWidgetItem *item2 = nullptr;
      QTableWidgetItem *item3 = nullptr;
      QTableWidgetItem *item4 = nullptr;
      QUrl url(it.value().value(dooble_history::HistoryItem::URL).toUrl());
      dooble_history_window_favorite_item *item1 = nullptr;

      if(title.isEmpty())
	title = url.toString().mid(0, dooble::Limits::MAXIMUM_URL_LENGTH);

      if(title.isEmpty())
	title = tr("Dooble");

      item1 = new dooble_history_window_favorite_item();
      item1->setData(Qt::UserRole, url);
      item1->setFlags(Qt::ItemIsEnabled |
		      Qt::ItemIsSelectable |
		      Qt::ItemIsUserCheckable);

      if(it.value().value(dooble_history::HistoryItem::FAVORITE).toBool())
	{
	  item1->setCheckState(Qt::Checked);
	  item1->setIcon
	    (QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
	}
      else
	item1->setCheckState(Qt::Unchecked);

      item2 = new QTableWidgetItem(title);
      item2->setData(Qt::UserRole, url);
      item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item2->setToolTip(dooble_ui_utilities::pretty_tool_tip(item2->text()));
      item3 = new QTableWidgetItem(url.toString());
      item3->setData(Qt::UserRole, url);
      item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item3->setToolTip(item3->text());
      item4 = new QTableWidgetItem(last_visited.toString(Qt::ISODate));
      item4->setData(Qt::UserRole, url);
      item4->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_items[url] = item2;
      m_ui.table->setItem(i, TableColumns::FAVORITE, item1);
      m_ui.table->setItem(i, TableColumns::TITLE, item2);
      m_ui.table->setItem(i, TableColumns::LOCATION, item3);
      m_ui.table->setItem(i, TableColumns::LAST_VISITED, item4);
      i += 1;
    }

  m_ui.table->setSortingEnabled(true);
  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  prepare_viewport_icons();
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_save_settings_timeout(void)
{
  save_settings();
}

void dooble_history_window::slot_search_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QDateTime period(QDateTime::currentDateTime());
  QString text(m_ui.search->text().toLower().trimmed());

  switch(m_ui.period->currentRow())
    {
    case 0: // All
      {
	break;
      }
    case 1: // Today
      {
	break;
      }
    case 2: // Yesterday
      {
	period = period.addDays(-1);
	break;
      }
    case 3: // This Month
      {
	break;
      }
    case 4: // Previous Month
      {
	period = period.addMonths(-1);
	break;
      }
    default:
      {
	break;
      }
    }

  auto count = m_ui.table->rowCount();

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(m_ui.period->currentRow() <= 0 && text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	auto item1 = m_ui.table->item(i, TableColumns::TITLE);
	auto item2 = m_ui.table->item(i, TableColumns::LOCATION);
	auto item3 = m_ui.table->item(i, TableColumns::LAST_VISITED);

	if(!item1 || !item2 || !item3)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	switch(m_ui.period->currentRow())
	  {
	  case 0: // All
	    {
	      if(text.isEmpty())
		m_ui.table->setRowHidden(i, false);
	      else if(item1->text().toLower().contains(text) ||
		      item2->text().toLower().contains(text))
		m_ui.table->setRowHidden(i, false);
	      else
		{
		  count -= 1;
		  m_ui.table->setRowHidden(i, true);
		}

	      break;
	    }
	  case 1: // Today
	  case 2: // Yesterday
	    {
	      QDateTime date_time
		(QDateTime::fromString(item3->text(), Qt::ISODate));

	      if(date_time.date() == period.date())
		{
		  if(text.isEmpty())
		    m_ui.table->setRowHidden(i, false);
		  else if(item1->text().toLower().contains(text) ||
			  item2->text().toLower().contains(text))
		    m_ui.table->setRowHidden(i, false);
		  else
		    {
		      count -= 1;
		      m_ui.table->setRowHidden(i, true);
		    }
		}
	      else
		{
		  count -= 1;
		  m_ui.table->setRowHidden(i, true);
		}

	      break;
	    }
	  default:
	    {
	      QDateTime date_time
		(QDateTime::fromString(item3->text(), Qt::ISODate));

	      if(date_time.date().month() == period.date().month() &&
		 date_time.date().year() == period.date().year())
		{
		  if(text.isEmpty())
		    m_ui.table->setRowHidden(i, false);
		  else if(item1->text().toLower().contains(text) ||
			  item2->text().toLower().contains(text))
		    m_ui.table->setRowHidden(i, false);
		  else
		    {
		      count -= 1;
		      m_ui.table->setRowHidden(i, true);
		    }
		}
	      else
		{
		  count -= 1;
		  m_ui.table->setRowHidden(i, true);
		}

	      break;
	    }
	  }
      }

  m_ui.entries->setText(tr("%1 Row(s)").arg(count));
  QApplication::restoreOverrideCursor();
  m_ui.table->prepare_viewport_icons();
}

void dooble_history_window::slot_show_context_menu(const QPoint &point)
{
  QMenu menu(this);

  menu.addAction(tr("&Copy Location"), this, SLOT(slot_copy_location(void)));
  menu.addSeparator();
  menu.addAction(tr("&Delete Page(s) (Non-Favorites)"),
		 this,
		 SLOT(slot_delete_pages(void)));
  menu.addAction(tr("&Delete Page(s)"),
		 this,
		 SLOT(slot_delete_pages(void)))->setProperty("prompt", true);
  menu.exec(mapToGlobal(point));
}

void dooble_history_window::slot_splitter_moved(int pos, int index)
{
  if(m_floating)
    return;

  Q_UNUSED(index);
  Q_UNUSED(pos);
  m_save_settings_timer.start();
}
