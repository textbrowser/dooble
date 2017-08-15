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
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QUrl>

#include "dooble.h"
#include "dooble_blocked_domains.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_blocked_domains::dooble_blocked_domains(void):QMainWindow()
{
  m_ui.setupUi(this);
  m_ui.table->sortByColumn(1, Qt::AscendingOrder);
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
  connect(m_ui.delete_rows,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_rows(void)));
}

bool dooble_blocked_domains::contains(const QString &domain) const
{
  return m_blocked_domains.value(domain, 0);
}

void dooble_blocked_domains::closeEvent(QCloseEvent *event)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("blocked_domains_geometry", saveGeometry().toBase64());

  QMainWindow::closeEvent(event);
}

void dooble_blocked_domains::populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_blocked_domains.clear();
  m_ui.table->clearContents();

  QMap<QString, int> oids;
  QString database_name("dooble_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT blocked, domain, OID "
		      "FROM dooble_blocked_domains"))
	  while(query.next())
	    {
	      QByteArray data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		continue;

	      QByteArray data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		continue;

	      m_blocked_domains[data2.constData()] = QVariant(data1).toBool();
	      oids[data2.constData()] = query.value(2).toInt();
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  m_ui.table->setRowCount(m_blocked_domains.size());
  m_ui.table->setSortingEnabled(false);
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  QHashIterator<QString, char> it(m_blocked_domains);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      if(it.value())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, oids.value(it.key()));
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.table->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key());
      item->setData(Qt::UserRole, oids.value(it.key()));
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 1, item);
      i += 1;
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  QApplication::restoreOverrideCursor();
}

void dooble_blocked_domains::purge(void)
{
  QString database_name("dooble_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_blocked_domains");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_blocked_domains::save_blocked_domain(const QString &domain,
						 bool state)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.exec("CREATE TABLE IF NOT EXISTS dooble_blocked_domains ("
		   "blocked TEXT NOT NULL, "
		   "domain TEXT NOT NULL, "
		   "domain_digest TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_blocked_domains "
	   "(blocked, domain, domain_digest) VALUES (?, ?, ?)");

	QByteArray data
	  (dooble::s_cryptography->encrypt_then_mac(state ? "true" : "false"));

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(domain.toUtf8());

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(domain);
	ok &= !data.isEmpty();

	if(ok)
	  query.addBindValue(data.toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_blocked_domains::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("blocked_domains_geometry").
					   toByteArray()));

  QMainWindow::show();
  populate();
}

void dooble_blocked_domains::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("blocked_domains_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
  populate();
}

void dooble_blocked_domains::slot_add(void)
{
  QInputDialog dialog(this);

  dialog.setLabelText(tr("Domain / URL"));
  dialog.setTextEchoMode(QLineEdit::Normal);
  dialog.setWindowIcon(windowIcon());
  dialog.setWindowTitle(tr("Dooble: New Blocked Domain"));

  if(dialog.exec() != QDialog::Accepted)
    return;

  QString text = QUrl::fromUserInput
    (dialog.textValue().toLower().trimmed()).host();

  if(text.isEmpty())
    return;

  if(!m_ui.table->findItems(text, Qt::MatchFixedString).isEmpty())
    return;

  m_blocked_domains[text] = 1;
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  for(int i = 0; i < 2; i++)
    {
      QTableWidgetItem *item = new QTableWidgetItem();

      if(i == 0)
	{
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);
	  item->setCheckState(Qt::Checked);
	}
      else
	{
	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  item->setText(text);
	}

      m_ui.table->setItem(m_ui.table->rowCount() - 1, i, item);
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  save_blocked_domain(text, true);
}

void dooble_blocked_domains::slot_delete_rows(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedRows(1));

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
	return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	for(int i = list.size() - 1; i >= 0; i--)
	  {
	    query.prepare("DELETE FROM dooble_blocked_domains WHERE OID = ?");
	    query.addBindValue(list.at(i).data(Qt::UserRole));

	    if(query.exec())
	      {
		m_blocked_domains.remove(list.at(i).data().toString());
		m_ui.table->removeRow(list.at(i).row());
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_blocked_domains::slot_item_changed(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  bool state = item->checkState() == Qt::Checked;

  item = m_ui.table->item(item->row(), 1);

  if(!item)
    return;

  m_blocked_domains[item->text()] = state;
  save_blocked_domain(item->text(), state);
}

void dooble_blocked_domains::slot_populate(void)
{
  populate();
}
