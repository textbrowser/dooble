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

#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSqlQuery>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_ui_utilities.h"

dooble_accepted_or_blocked_domains::dooble_accepted_or_blocked_domains(void):
  QMainWindow()
{
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.table->sortItems(1, Qt::AscendingOrder);
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(m_ui.accept_mode,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slot_radio_button_toggled(bool)));
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
  connect(m_ui.add_exception,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_new_exception(void)));
  connect(m_ui.block_mode,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slot_radio_button_toggled(bool)));
  connect(m_ui.delete_all_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_all_exceptions(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.delete_selected_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected_exceptions(void)));
  connect(m_ui.exception,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_new_exception(void)));
  connect(m_ui.import,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_import(void)));
  connect(m_ui.save,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));

  if(dooble_settings::
     setting("accepted_or_blocked_domains_mode").toString() == "accept")
    m_ui.accept_mode->click();
  else
    m_ui.block_mode->click();

  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
}

dooble_accepted_or_blocked_domains::~dooble_accepted_or_blocked_domains()
{
  m_future.cancel();
  m_future.waitForFinished();
}

bool dooble_accepted_or_blocked_domains::contains(const QString &domain) const
{
  return m_domains.value(domain.toLower().trimmed(), 0) == 1;
}

bool dooble_accepted_or_blocked_domains::exception(const QUrl &url) const
{
  return m_exceptions.value(url.host(), 0) == 1 ||
    m_exceptions.value(url.toString(), 0) == 1;
}

void dooble_accepted_or_blocked_domains::abort(void)
{
  m_future.cancel();
  m_future.waitForFinished();
}

void dooble_accepted_or_blocked_domains::accept_or_block_domain
(const QString &domain, const bool replace)
{
  if(domain.trimmed().isEmpty())
    return;
  else if(m_domains.contains(domain.toLower().trimmed()))
    return;

  m_domains[domain.toLower().trimmed()] = 1;
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  for(int i = 0; i < 2; i++)
    {
      QTableWidgetItem *item = new QTableWidgetItem();

      item->setData(Qt::UserRole, domain);

      if(i == 0)
	{
	  item->setCheckState(Qt::Checked);
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);
	}
      else
	{
	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  item->setText(domain.toLower().trimmed());
	}

      m_ui.table->setItem(m_ui.table->rowCount() - 1, i, item);
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortItems
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  save_blocked_domain(domain.toLower().trimmed(), replace, true);
}

void dooble_accepted_or_blocked_domains::add_session_url(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  m_ui.session_rejections->setSortingEnabled(false);
  m_ui.session_rejections->setRowCount(m_ui.session_rejections->rowCount() + 1);

  QTableWidgetItem *item = new QTableWidgetItem(url.toString());

  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.session_rejections->setItem
    (m_ui.session_rejections->rowCount() - 1, 0, item);
  m_ui.session_rejections->setSortingEnabled(true);
  m_ui.session_rejections->sortItems
    (0, m_ui.session_rejections->horizontalHeader()->sortIndicatorOrder());
}

void dooble_accepted_or_blocked_domains::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_accepted_or_blocked_domains::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec
    ("CREATE TABLE IF NOT EXISTS dooble_accepted_or_blocked_domains ("
     "domain TEXT NOT NULL, "
     "domain_digest TEXT NOT NULL PRIMARY KEY, "
     "state TEXT NOT NULL)");
  query.exec
    ("CREATE TABLE IF NOT EXISTS "
     "dooble_accepted_or_blocked_domains_exceptions ("
     "state TEXT NOT NULL, "
     "url TEXT NOT NULL, "
     "url_digest TEXT NOT NULL PRIMARY KEY)");
}

void dooble_accepted_or_blocked_domains::keyPressEvent(QKeyEvent *event)
{
  if(!parent())
    {
      if(event && event->key() == Qt::Key_Escape)
	close();

      QMainWindow::keyPressEvent(event);
    }
  else if(event)
    event->ignore();
}

void dooble_accepted_or_blocked_domains::new_exception(const QString &url)
{
  if(m_exceptions.contains(url.trimmed()))
    return;
  else if(url.trimmed().isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_ui.exceptions,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_exceptions_item_changed(QTableWidgetItem *)));
  m_ui.exceptions->setRowCount
    (m_ui.exceptions->rowCount() + 1);
  m_ui.exception->clear();

  QTableWidgetItem *item = new QTableWidgetItem();

  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, url);
  item->setFlags(Qt::ItemIsEnabled |
		 Qt::ItemIsSelectable |
		 Qt::ItemIsUserCheckable);
  m_ui.exceptions->setItem
    (m_ui.exceptions->rowCount() - 1, 0, item);
  item = new QTableWidgetItem(url);
  item->setData(Qt::UserRole, url);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.exceptions->setItem
    (m_ui.exceptions->rowCount() - 1, 1, item);
  m_ui.exceptions->sortItems(1);
  connect(m_ui.exceptions,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_exceptions_item_changed(QTableWidgetItem *)));
  QApplication::restoreOverrideCursor();
  save_exception(url, true);
}

void dooble_accepted_or_blocked_domains::populate(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.table->setRowCount(0);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_accepted_or_blocked_domains.db");

	if(db.open())
	  {
	    create_tables(db);

	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT domain, state, OID "
			  "FROM dooble_accepted_or_blocked_domains"))
	      while(query.next())
		{
		  QByteArray data1
		    (QByteArray::fromBase64(query.value(0).toByteArray()));

		  data1 = dooble::s_cryptography->mac_then_decrypt(data1);

		  if(data1.isEmpty())
		    {
		      dooble_database_utilities::remove_entry
			(db,
			 "dooble_accepted_or_blocked_domains",
			 query.value(2).toLongLong());
		      continue;
		    }

		  QByteArray data2
		    (QByteArray::fromBase64(query.value(1).toByteArray()));

		  data2 = dooble::s_cryptography->mac_then_decrypt(data2);

		  if(data2.isEmpty())
		    {
		      dooble_database_utilities::remove_entry
			(db,
			 "dooble_accepted_or_blocked_domains",
			 query.value(2).toLongLong());
		      continue;
		    }

		  m_domains[data1.constData()] = QVariant
		    (data2).toBool() ? 1 : 0;
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }

  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setRowCount(m_domains.size());
  m_ui.table->setSortingEnabled(false);

  QHashIterator<QString, char> it(m_domains);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      if(it.value())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.table->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key());
      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 1, item);
      i += 1;
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortItems
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::populate_exceptions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_exceptions.clear();
  m_ui.exceptions->setRowCount(0);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_accepted_or_blocked_domains.db");

	if(db.open())
	  {
	    create_tables(db);

	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT state, url, OID "
			  "FROM dooble_accepted_or_blocked_domains_exceptions"))
	      while(query.next())
		{
		  QByteArray data1
		    (QByteArray::fromBase64(query.value(0).toByteArray()));

		  data1 = dooble::s_cryptography->mac_then_decrypt(data1);

		  if(data1.isEmpty())
		    {
		      dooble_database_utilities::remove_entry
			(db,
			 "dooble_accepted_or_blocked_domains_exceptions",
			 query.value(2).toLongLong());
		      continue;
		    }

		  QByteArray data2
		    (QByteArray::fromBase64(query.value(1).toByteArray()));

		  data2 = dooble::s_cryptography->mac_then_decrypt(data2);

		  if(data2.isEmpty())
		    {
		      dooble_database_utilities::remove_entry
			(db,
			 "dooble_accepted_or_blocked_domains_exceptions",
			 query.value(2).toLongLong());
		      continue;
		    }

		  m_exceptions[data2] = (data1 == "true") ? 1 : 0;
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }

  disconnect(m_ui.exceptions,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_exceptions_item_changed(QTableWidgetItem *)));
  m_ui.exceptions->setRowCount(m_exceptions.size());

  QHashIterator<QString, char> it(m_exceptions);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      if(it.value())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.exceptions->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key());
      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.exceptions->setItem(i, 1, item);
      i += 1;
    }

  connect(m_ui.exceptions,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_exceptions_item_changed(QTableWidgetItem *)));
  m_ui.exceptions->sortItems(1);
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::purge(void)
{
  m_domains.clear();
  m_exceptions.clear();
  m_future.cancel();
  m_future.waitForFinished();
  m_ui.table->setRowCount(0);

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_accepted_or_blocked_domains");
	query.exec("DELETE FROM dooble_accepted_or_blocked_domains_exceptions");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_accepted_or_blocked_domains::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_accepted_or_blocked_domains::save
(const QByteArray &authentication_key,
 const QByteArray &encryption_key,
 const QHash<QString, char> &hash)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	create_tables(db);

	QHashIterator<QString, char> it(hash);
	QSqlQuery query(db);
	dooble_cryptography cryptography
	  (authentication_key,
	   encryption_key,
	   dooble_settings::setting("block_cipher_type").toString(),
	   dooble_settings::setting("hash_type").toString());

	query.exec("PRAGMA synchronous = OFF");

	while(it.hasNext() && !m_future.isCanceled())
	  {
	    it.next();
	    query.prepare
	      ("INSERT INTO dooble_accepted_or_blocked_domains "
	       "(domain, domain_digest, state) VALUES (?, ?, ?)");

	    QByteArray data
	      (cryptography.
	       encrypt_then_mac(it.key().toLower().trimmed().toUtf8()));

	    if(data.isEmpty())
	      continue;
	    else
	      query.addBindValue(data.toBase64());

	    data = cryptography.hmac(it.key().toLower().trimmed());

	    if(data.isEmpty())
	      continue;

	    query.addBindValue(data.toBase64());
	    data = cryptography.encrypt_then_mac("true");

	    if(data.isEmpty())
	      continue;
	    else
	      query.addBindValue(data.toBase64());

	    query.exec();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_accepted_or_blocked_domains::save_blocked_domain
(const QString &domain, bool replace, bool state)
{
  if(domain.trimmed().isEmpty())
    return;
  else if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      m_domains[domain.toLower().trimmed()] = state ? 1 : 0;
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	if(replace)
	  query.prepare
	    ("INSERT OR REPLACE INTO dooble_accepted_or_blocked_domains "
	     "(domain, domain_digest, state) VALUES (?, ?, ?)");
	else
	  query.prepare
	    ("INSERT INTO dooble_accepted_or_blocked_domains "
	     "(domain, domain_digest, state) VALUES (?, ?, ?)");

	QByteArray data
	  (dooble::s_cryptography->
	   encrypt_then_mac(domain.toLower().trimmed().toUtf8()));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(domain.toLower().trimmed());

	if(data.isEmpty())
	  goto done_label;

	query.addBindValue(data.toBase64());
	data = dooble::s_cryptography->encrypt_then_mac
	  (state ? "true" : "false");

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::save_exception(const QString &url,
							bool state)
{
  if(url.trimmed().isEmpty())
    return;

  m_exceptions[url.trimmed()] = state ? 1 : 0;

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO "
	   "dooble_accepted_or_blocked_domains_exceptions "
	   "(state, url, url_digest) VALUES (?, ?, ?)");

	QByteArray data
	  (dooble::s_cryptography->
	   encrypt_then_mac(state ? QByteArray("true") : QByteArray("false")));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(url.toUtf8());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(url.toUtf8());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("accepted_or_blocked_domains_geometry", saveGeometry().toBase64());
}

void dooble_accepted_or_blocked_domains::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("accepted_or_blocked_domains_geometry").
			      toByteArray()));

  QMainWindow::show();
}

void dooble_accepted_or_blocked_domains::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("accepted_or_blocked_domains_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
}

void dooble_accepted_or_blocked_domains::slot_add(void)
{
  QInputDialog dialog(this);

  dialog.setLabelText(tr("Domain / URL"));
  dialog.setTextEchoMode(QLineEdit::Normal);
  dialog.setWindowIcon(windowIcon());
  dialog.setWindowTitle(tr("Dooble: New Domain"));

  if(dialog.exec() != QDialog::Accepted)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();

  QString text = QUrl::fromUserInput
    (dialog.textValue().toLower().trimmed()).host();

  if(text.isEmpty())
    return;

  accept_or_block_domain(text);
}

void dooble_accepted_or_blocked_domains::slot_delete_all_exceptions(void)
{
  if(m_ui.exceptions->rowCount() > 0)
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete all of the exceptions?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  m_exceptions.clear();
  m_ui.exceptions->setRowCount(0);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_accepted_or_blocked_domains_exceptions");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_accepted_or_blocked_domains::slot_delete_selected(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedRows(1));

  for(int i = list.size() - 1; i >= 0; i--)
    if(m_ui.table->isRowHidden(list.at(i).row()))
      list.removeAt(i);

  std::sort(list.begin(), list.end());

  QApplication::restoreOverrideCursor();

  if(list.size() > 0)
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected domain(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_accepted_or_blocked_domains.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");

	    for(int i = list.size() - 1; i >= 0; i--)
	      {
		query.prepare
		  ("DELETE FROM dooble_accepted_or_blocked_domains "
		   "WHERE domain_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data(Qt::UserRole).toString()).toBase64());

		if(query.exec())
		  {
		    m_domains.remove(list.at(i).data().toString());
		    m_ui.table->removeRow(list.at(i).row());
		  }
	      }

	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
  else
    for(int i = list.size() - 1; i >= 0; i--)
      {
	m_domains.remove(list.at(i).data().toString());
	m_ui.table->removeRow(list.at(i).row());
      }

  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::slot_delete_selected_exceptions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.exceptions->selectionModel()->selectedRows(1));

  QApplication::restoreOverrideCursor();

  if(list.size() > 0)
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected exception(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end());

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name(dooble_database_utilities::database_name());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_accepted_or_blocked_domains.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.exec("PRAGMA synchronous = OFF");

	    for(int i = list.size() - 1; i >= 0; i--)
	      {
		query.prepare
		  ("DELETE FROM dooble_accepted_or_blocked_domains_exceptions "
		   "WHERE url_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data().toString().toUtf8()).toBase64());

		if(query.exec())
		  {
		    m_exceptions.remove(list.at(i).data().toString());
		    m_ui.exceptions->removeRow(list.at(i).row());
		  }
	      }

	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
  else
    for(int i = list.size() - 1; i >= 0; i--)
      {
	m_exceptions.remove(list.at(i).data().toString());
	m_ui.exceptions->removeRow(list.at(i).row());
      }

  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::slot_exceptions_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  bool state = item->checkState() == Qt::Checked;

  item = m_ui.exceptions->item(item->row(), 1);

  if(!item)
    return;

  save_exception(item->text(), state);
}

void dooble_accepted_or_blocked_domains::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_accepted_or_blocked_domains::slot_import(void)
{
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setDirectory(QDir::currentPath() + QDir::separator() + "Data");
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setWindowTitle(tr("Dooble: Import Accepted / Blocked Domains"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();

      QFile file(dialog.selectedFiles().value(0));

      if(file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  dialog.close();
	  m_ui.search->clear();
	  m_ui.table->setRowCount(0);
	  repaint();
	  QApplication::processEvents();
	  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	  QByteArray data(2048, 0);
	  qint64 rc = 0;

	  while((rc = file.readLine(data.data(),
				    static_cast<qint64> (data.length()))) >= 0)
	    {
	      if(data.trimmed().endsWith(".invalid") ||
		 data.trimmed().startsWith("#"))
		continue;

	      QUrl url(QUrl::fromUserInput(data.mid(0, static_cast<int> (rc)).
					   trimmed()));

	      if(!url.isEmpty() && url.isValid())
		m_domains[url.host()] = 1;
	    }

	  file.close();
	  QApplication::restoreOverrideCursor();
	  populate();

	  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
	    {
	      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	      if(m_future.isRunning())
		{
		  m_future.cancel();
		  m_future.waitForFinished();
		}

	      m_future = QtConcurrent::run
		(this,
		 &dooble_accepted_or_blocked_domains::save,
		 dooble::s_cryptography->keys().first,
		 dooble::s_cryptography->keys().second,
		 m_domains);
	      QApplication::restoreOverrideCursor();
	    }
	}
    }

  QApplication::processEvents();
}

void dooble_accepted_or_blocked_domains::slot_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  bool state = item->checkState() == Qt::Checked;

  item = m_ui.table->item(item->row(), 1);

  if(!item)
    return;

  m_domains[item->text()] = state ? 1 : 0;
  save_blocked_domain(item->text(), true, state);
}

void dooble_accepted_or_blocked_domains::slot_new_exception(const QString &url)
{
  new_exception(url);
}

void dooble_accepted_or_blocked_domains::slot_new_exception(void)
{
  new_exception(m_ui.exception->text().trimmed());
}

void dooble_accepted_or_blocked_domains::slot_populate(void)
{
  m_domains.clear();
  m_exceptions.clear();
  populate();
  populate_exceptions();
  emit populated();
}

void dooble_accepted_or_blocked_domains::slot_radio_button_toggled(bool state)
{
  if(m_ui.accept_mode == sender() && state)
    {
      dooble_settings::set_setting
	("accepted_or_blocked_domains_mode", "accept");
      m_ui.exceptions->setHorizontalHeaderLabels
	(QStringList() << tr("Reject") << tr("Site"));
      m_ui.table->setHorizontalHeaderLabels
	(QStringList() << tr("Accepted") << tr("Domain"));
    }
  else if(m_ui.block_mode == sender())
    {
      dooble_settings::set_setting
	("accepted_or_blocked_domains_mode", "block");
      m_ui.exceptions->setHorizontalHeaderLabels
	(QStringList() << tr("Allow") << tr("Site"));
      m_ui.table->setHorizontalHeaderLabels
	(QStringList() << tr("Blocked") << tr("Domain"));
    }
}

void dooble_accepted_or_blocked_domains::slot_save(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < m_ui.session_rejections->rowCount(); i++)
    {
      QTableWidgetItem *item = m_ui.session_rejections->item(i, 0);

      if(item)
	{
	  QUrl url(QUrl::fromUserInput(item->text()));

	  save_blocked_domain(url.host(), false, true);
	}
    }

  m_ui.session_rejections->setRowCount(0);
  QApplication::restoreOverrideCursor();
  populate();
}

void dooble_accepted_or_blocked_domains::slot_search_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	QTableWidgetItem *item = m_ui.table->item(i, 1);

	if(!item)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	if(item->text().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);
      }

  QApplication::restoreOverrideCursor();
}
