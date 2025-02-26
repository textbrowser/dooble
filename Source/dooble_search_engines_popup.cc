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

#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSqlQuery>
#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_favicons.h"
#include "dooble_search_engines_popup.h"
#include "dooble_settings.h"
#include "dooble_ui_utilities.h"

dooble_search_engines_popup::dooble_search_engines_popup(QWidget *parent):
  QDialog(parent)
{
  m_model = new QStandardItemModel(this);
  m_model->setHorizontalHeaderLabels
    (QStringList() << tr("Title") << tr("Search Engine") << tr("Syntax"));
  m_predefined_urls["DuckDuckGo"] = QUrl::fromUserInput
    ("https://duckduckgo.com/?q=");
  m_predefined_urls["Ecosia"] =
    QUrl::fromUserInput("https://www.ecosia.org/search?q=");
  m_predefined_urls["Google"] =
    QUrl::fromUserInput("https://www.google.com/search?q=");
  m_predefined_urls["MetaGer"] =
    QUrl::fromUserInput("https://metager.org/meta/meta.ger3?eingabe=");
  m_predefined_urls["Swisscows"] =
    QUrl::fromUserInput("https://swisscows.com/web?query=");
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.splitter->setStretchFactor(0, 0);
  m_ui.splitter->setStretchFactor(1, 1);
  m_ui.splitter->restoreState
    (QByteArray::
     fromBase64(dooble_settings::
		setting("search_engines_window_splitter_state").toByteArray()));
  m_ui.view->setModel(m_model);
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add_search_engine(void)));
  connect(m_ui.add_checked,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add_predefined(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.splitter,
	  SIGNAL(splitterMoved(int, int)),
	  this,
	  SLOT(slot_splitter_moved(int, int)));
  connect(m_ui.view,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slot_double_clicked(const QModelIndex &)));
#ifdef Q_OS_MACOS
  m_ui.delete_selected->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}");
#endif
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
  prepare_icons();
  setWindowFlags(Qt::WindowStaysOnTopHint | windowFlags());
}

QList<QAction *> dooble_search_engines_popup::actions(void) const
{
  return m_actions.values();
}

QUrl dooble_search_engines_popup::default_address_bar_engine_url(void) const
{
  if(m_default_address_bar_engine_url.isEmpty())
    return m_predefined_urls.value("DuckDuckGo");
  else
    return m_default_address_bar_engine_url;
}

QUrl dooble_search_engines_popup::search_url(const QString &t) const
{
  auto const text(t.trimmed());

  if(text.isEmpty())
    return QUrl();

  for(int i = 0; i < m_model->rowCount(); i++)
    {
      auto item1 = m_model->item(i, 1);
      auto item2 = m_model->item(i, 2);

      if(!item1 || !item2 || item2->text().trimmed().isEmpty())
	continue;

      if(text.startsWith(item2->text()))
	{
	  auto url(item1->data().toUrl());

	  if(url.hasQuery())
	    url.setQuery
	      (url.query().
	       append(QString("%1").arg(text.mid(item2->text().length()))));
	  else
	    url = QUrl::fromUserInput
	      (url.toString().append(text.mid(item2->text().length())));

	  return url;
	}
    }

  return QUrl();
}

void dooble_search_engines_popup::add_search_engine
(const QByteArray &syntax, const QByteArray &title, const QUrl &url)
{
  if(!dooble::s_cryptography)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < m_model->rowCount(); i++)
    if(m_model->item(i, 0) &&
       m_model->item(i, 0)->text().trimmed() == title.trimmed())
      {
	QApplication::restoreOverrideCursor();
	return;
      }

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_search_engines.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_search_engines "
	   "(syntax, title, url, url_digest) VALUES (?, ?, ?, ?)");

	QByteArray bytes;

	if(syntax.trimmed().isEmpty())
	  bytes = bytes.toBase64();
	else
	  {
	    bytes = dooble::s_cryptography->encrypt_then_mac(syntax);

	    if(bytes.isEmpty())
	      goto done_label;
	    else
	      bytes = bytes.toBase64();
	  }

	query.addBindValue(bytes);
	bytes = dooble::s_cryptography->encrypt_then_mac(title);

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(bytes.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(bytes.toBase64());

	bytes = dooble::s_cryptography->hmac(url.toEncoded());

	if(bytes.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(bytes.toBase64());

	if(query.exec())
	  {
	    auto action = m_actions.value(title);

	    if(action)
	      {
		auto const list
		  (m_model->findItems(url.toEncoded(),
				      Qt::MatchFixedString,
				      1));

		if(!list.isEmpty())
		  {
		    list.at(0)->setData(url);
		    list.at(0)->setText(url.toEncoded());
		  }

		action->setProperty("url", url);
		goto done_label;
	      }

	    disconnect(m_model,
		       &QStandardItemModel::itemChanged,
		       this,
		       &dooble_search_engines_popup::slot_item_changed);

	    QList<QStandardItem *> list;
	    auto item = new QStandardItem();

	    action = new QAction
	      (dooble_favicons::icon_from_host(url), title, this);
	    action->setProperty("url", url);
	    item->setCheckState(Qt::Unchecked);
	    item->setData(url);
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    item->setText(title);
	    item->setToolTip(item->text());
	    list << item;
	    item = new QStandardItem();
	    item->setData(url);
	    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	    item->setText(url.toEncoded());
	    item->setToolTip(item->text());
	    list << item;
	    item = new QStandardItem();
	    item->setData(url);
	    item->setFlags
	      (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	    item->setText(syntax);
	    item->setToolTip(item->text());
	    list << item;
	    m_actions.insert(title, action);
	    m_model->appendRow(list);
	    m_model->sort(0);
	    m_ui.search_engine->clear();
	    m_ui.syntax->clear();
	    m_ui.title->clear();
	    connect(m_model,
		    &QStandardItemModel::itemChanged,
		    this,
		    &dooble_search_engines_popup::slot_item_changed);
	  }
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  prepare_viewport_icons();
  QApplication::restoreOverrideCursor();
  slot_search_timer_timeout();
}

void dooble_search_engines_popup::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_search_engines ("
	     "default_address_bar_engine TEXT, "
	     "syntax TEXT, "
	     "title TEXT NOT NULL, "
	     "url TEXT NOT NULL, "
	     "url_digest TEXT PRIMARY KEY NOT NULL)");
  query.exec
    ("ALTER TABLE dooble_search_engines ADD default_address_bar_engine TEXT");
  query.exec
    ("ALTER TABLE dooble_search_engines ADD syntax TEXT");
}

void dooble_search_engines_popup::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    accept();

  QDialog::keyPressEvent(event);
}

void dooble_search_engines_popup::prepare_icons(void)
{
  auto const icon_set(dooble_settings::setting("icon_set").toString());
  auto const use_material_icons(dooble_settings::use_material_icons());

  m_ui.delete_selected->setIcon
    (QIcon::fromTheme(use_material_icons + "edit-delete",
		      QIcon(QString(":/%1/36/delete.png").arg(icon_set))));
}

void dooble_search_engines_popup::prepare_viewport_icons(void)
{
  disconnect(m_model,
	     &QStandardItemModel::itemChanged,
	     this,
	     &dooble_search_engines_popup::slot_item_changed);
  m_ui.view->prepare_viewport_icons();
  connect(m_model,
	  &QStandardItemModel::itemChanged,
	  this,
	  &dooble_search_engines_popup::slot_item_changed);
}

void dooble_search_engines_popup::purge(void)
{
  m_model->removeRows(0, m_model->rowCount());

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_search_engines.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("DELETE FROM dooble_search_engines");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_search_engines_popup::resizeEvent(QResizeEvent *event)
{
  QDialog::resizeEvent(event);

  if(!parent())
    save_settings();
}

void dooble_search_engines_popup::save_settings(void)
{
  dooble_settings::set_setting
    ("search_engines_window_splitter_state",
     m_ui.splitter->saveState().toBase64());

  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("search_engines_window_geometry", saveGeometry().toBase64());
}

void dooble_search_engines_popup::set_icon(const QIcon &icon, const QUrl &url)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_model,
	     &QStandardItemModel::itemChanged,
	     this,
	     &dooble_search_engines_popup::slot_item_changed);

  auto const list
    (m_model->findItems(dooble_ui_utilities::simplified_url(url).toEncoded(),
			Qt::MatchFixedString | Qt::MatchStartsWith,
			1));

  foreach(auto i, list)
    {
      auto item = i ? m_model->item(i->row(), 0) : nullptr;

      if(item)
	item->setIcon(dooble_favicons::icon(icon));
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QMapIterator<QString, QAction *> it(m_actions);
#else
  QMultiMapIterator<QString, QAction *> it(m_actions);
#endif

  while(it.hasNext())
    {
      it.next();

      if(it.value())
	{
	  auto const str1
	    (dooble_ui_utilities::
	     simplified_url(it.value()->property("url").toUrl()).toEncoded());
	  auto const str2
	    (dooble_ui_utilities::simplified_url(url).toEncoded());

	  if(str1.startsWith(str2))
	    {
	      it.value()->setIcon(dooble_favicons::icon(icon));
	      break;
	    }
	}
    }

  connect(m_model,
	  &QStandardItemModel::itemChanged,
	  this,
	  &dooble_search_engines_popup::slot_item_changed);
  QApplication::restoreOverrideCursor();
}

void dooble_search_engines_popup::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool() && !parent())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("search_engines_window_geometry").
			      toByteArray()));

  QDialog::show();
}

void dooble_search_engines_popup::show_normal(QWidget *parent)
{
  if(dooble_settings::setting("save_geometry").toBool() && !this->parent())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("search_engines_window_geometry").
			      toByteArray()));

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(parent, this);

  QDialog::showNormal();
}

void dooble_search_engines_popup::slot_add_predefined(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < m_ui.predefined->count(); i++)
    {
      auto item = m_ui.predefined->item(i);

      if(item && item->checkState() == Qt::Checked)
	add_search_engine
	  (QByteArray(),
	   item->text().toUtf8(),
	   m_predefined_urls.value(item->text()));
    }

  QApplication::restoreOverrideCursor();
  slot_populate();
}

void dooble_search_engines_popup::slot_add_search_engine(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto const url(QUrl::fromUserInput(m_ui.search_engine->text()));

  if(url.isEmpty() || !url.isValid())
    return;

  add_search_engine(m_ui.syntax->text().trimmed().toUtf8(),
		    m_ui.title->text().trimmed().toUtf8(),
		    url);
}

void dooble_search_engines_popup::slot_delete_selected(void)
{
  auto list(m_ui.view->selectionModel()->selectedRows(1));

  if(list.isEmpty())
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to delete the selected entries?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_search_engines.db");

	if(db.open())
	  {
	    std::sort(list.begin(), list.end());

	    QSqlQuery query(db);

	    for(int i = list.size() - 1; i >= 0; i--)
	      if(!m_ui.view->isRowHidden(list.at(i).row()))
		{
	          QUrl const url(list.at(i).data().toString());

		  query.prepare
		    ("DELETE FROM dooble_search_engines WHERE url_digest = ?");
		  query.addBindValue
		    (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());

		  if(query.exec())
		    {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
		      QMutableMapIterator<QString, QAction *> it(m_actions);
#else
		      QMutableMultiMapIterator<QString, QAction *>
			it(m_actions);
#endif

		      while(it.hasNext())
			{
			  it.next();

			  if(it.value() &&
			     it.value()->property("url").toUrl() == url)
			    {
			      it.value()->deleteLater();
			      it.remove();
			      break;
			    }
			}

		      m_model->removeRow(list.at(i).row());
		    }
		}

	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
      QApplication::restoreOverrideCursor();
    }

  prepare_viewport_icons();
  slot_search_timer_timeout();
}

void dooble_search_engines_popup::slot_double_clicked(const QModelIndex &index)
{
  if(index.isValid() && index.column() == 2) // Syntax?
    return;

  if(QApplication::keyboardModifiers() & Qt::ControlModifier)
    emit open_link_in_new_tab(index.sibling(index.row(), 1).data().toString());
  else
    emit open_link(index.sibling(index.row(), 1).data().toString());
}

void dooble_search_engines_popup::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_search_engines_popup::slot_item_changed(QStandardItem *item)
{
  if(!dooble::s_cryptography ||
     !dooble::s_cryptography->authenticated() ||
     !item)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_model,
	     &QStandardItemModel::itemChanged,
	     this,
	     &dooble_search_engines_popup::slot_item_changed);

  for(int i = 0; i < m_model->rowCount(); i++)
    if(item != m_model->item(i) && m_model->item(i))
      m_model->item(i)->setCheckState(Qt::Unchecked);

  m_default_address_bar_engine_url = item->data().toUrl();

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_search_engines.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	if(item->column() == 0) // Title
	  {
	    query.setForwardOnly(true);

	    if(query.exec("SELECT OID FROM dooble_search_engines"))
	      while(query.next())
		{
		  QSqlQuery update(db);

		  update.prepare("UPDATE dooble_search_engines "
				 "SET default_address_bar_engine = ? "
				 "WHERE OID = ?");
		  update.addBindValue
		    (dooble::s_cryptography->encrypt_then_mac("false").
		     toBase64());
		  update.addBindValue(query.value(0));
		  update.exec();
		}

	    QSqlQuery update(db);

	    update.prepare("UPDATE dooble_search_engines "
			   "SET default_address_bar_engine = ? "
			   "WHERE url_digest = ?");

	    if(item->checkState() == Qt::Checked)
	      update.addBindValue
		(dooble::s_cryptography->encrypt_then_mac("true").toBase64());
	    else
	      update.addBindValue
		(dooble::s_cryptography->encrypt_then_mac("false").toBase64());

	    update.addBindValue
	      (dooble::s_cryptography->hmac(item->data().toUrl().toEncoded()).
	       toBase64());
	    update.exec();
	  }
	else if(item->column() == 2) // Syntax
	  {
	    QSqlQuery update(db);

	    update.prepare("UPDATE dooble_search_engines "
			   "SET syntax = ? "
			   "WHERE url_digest = ?");
	    update.addBindValue
	      (dooble::s_cryptography->
	       encrypt_then_mac(item->text().trimmed().toUtf8()).toBase64());
	    update.addBindValue
	      (dooble::s_cryptography->hmac(item->data().toUrl().toEncoded()).
	       toBase64());

	    if(update.exec())
	      {
		item->setText(item->text().trimmed());
		item->setToolTip(item->text());
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  connect(m_model,
	  &QStandardItemModel::itemChanged,
	  this,
	  &dooble_search_engines_popup::slot_item_changed);
  QApplication::restoreOverrideCursor();
}

void dooble_search_engines_popup::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      emit populated();
      m_ui.search->clear();
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.search->clear();

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QMutableMapIterator<QString, QAction *> it(m_actions);
#else
  QMutableMultiMapIterator<QString, QAction *> it(m_actions);
#endif

  while(it.hasNext())
    {
      it.next();

      if(it.value())
	it.value()->deleteLater();

      it.remove();
    }

  m_model->removeRows(0, m_model->rowCount());

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_search_engines.db");

    if(db.open())
      {
	create_tables(db);
	disconnect(m_model,
		   &QStandardItemModel::itemChanged,
		   this,
		   &dooble_search_engines_popup::slot_item_changed);

	QSqlQuery query(db);
	auto single_checked = true;

	query.setForwardOnly(true);

	if(query.exec("SELECT default_address_bar_engine, "
		      "syntax, "
		      "title, "
		      "url, "
		      "OID "
		      "FROM dooble_search_engines"))
	  while(query.next())
	    {
	      auto check_state
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      check_state = dooble::s_cryptography->mac_then_decrypt
		(check_state);

	      auto syntax
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      syntax = dooble::s_cryptography->mac_then_decrypt(syntax);

	      auto title
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      title = dooble::s_cryptography->mac_then_decrypt(title);

	      if(title.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_history",
		     query.value(4).toLongLong());
		  continue;
		}

	      auto url(QByteArray::fromBase64(query.value(3).toByteArray()));

	      url = dooble::s_cryptography->mac_then_decrypt(url);

	      if(url.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_search_engines",
		     query.value(4).toLongLong());
		  continue;
		}

	      QAction *action = nullptr;
	      QList<QStandardItem *> list;
	      auto item = new QStandardItem();

	      action = new QAction
		(dooble_favicons::icon_from_host(QUrl::fromEncoded(url)),
		 title,
		 this);
	      action->setProperty("url", QUrl::fromEncoded(url));

	      if(check_state == "true" && single_checked)
		{
		  item->setCheckState(Qt::Checked);
		  m_default_address_bar_engine_url = url;
		  single_checked = false;
		}
	      else
		item->setCheckState(Qt::Unchecked);

	      item->setData(QUrl::fromEncoded(url));
	      item->setFlags(Qt::ItemIsEnabled |
			     Qt::ItemIsSelectable |
			     Qt::ItemIsUserCheckable);
	      item->setIcon(dooble_favicons::icon(action->icon()));
	      item->setText(title);
	      item->setToolTip(item->text());
	      list << item;
	      item = new QStandardItem();
	      item->setData(QUrl::fromEncoded(url));
	      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	      item->setText(url);
	      item->setToolTip(item->text());
	      list << item;
	      item = new QStandardItem();
	      item->setData(QUrl::fromEncoded(url));
	      item->setFlags
		(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	      item->setText(syntax);
	      item->setToolTip(item->text());
	      list << item;
	      m_actions.insert(title, action);
	      m_model->appendRow(list);
	    }

	connect(m_model,
		&QStandardItemModel::itemChanged,
		this,
		&dooble_search_engines_popup::slot_item_changed);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  m_model->sort(0);
  QApplication::restoreOverrideCursor();
  slot_search_timer_timeout();
}

void dooble_search_engines_popup::slot_search_timer_timeout(void)
{
  auto model = qobject_cast<QStandardItemModel *> (m_ui.view->model());

  if(!model)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const text(m_ui.search->text().trimmed());
  auto count = model->rowCount();

  for(int i = 0; i < model->rowCount(); i++)
    if(text.isEmpty())
      m_ui.view->setRowHidden(i, false);
    else
      {
	auto item1 = model->item(i, 0);
	auto item2 = model->item(i, 1);
	auto item3 = model->item(i, 2);

	if(!item1 || !item2 || !item3)
	  m_ui.view->setRowHidden(i, false);
	else if(item1->text().contains(text, Qt::CaseInsensitive) ||
		item2->text().contains(text, Qt::CaseInsensitive) ||
		item3->text().contains(text, Qt::CaseInsensitive))
	  m_ui.view->setRowHidden(i, false);
	else
	  {
	    count -= 1;
	    m_ui.view->setRowHidden(i, true);
	  }
      }

  m_ui.entries->setText(tr("%1 Row(s)").arg(count));
  QApplication::restoreOverrideCursor();
  prepare_viewport_icons();
}

void dooble_search_engines_popup::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_search_engines_popup::slot_splitter_moved(int pos, int index)
{
  Q_UNUSED(index);
  Q_UNUSED(pos);
  dooble_settings::set_setting
    ("search_engines_window_splitter_state",
     m_ui.splitter->saveState().toBase64());
}
