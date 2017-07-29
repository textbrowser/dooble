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
#include <QSqlQuery>

#include "dooble_blocked_domains.h"
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
  populate();
}

bool dooble_blocked_domains::contains(const QString &domain) const
{
  return m_blocked_domains.contains(domain);
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
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_blocked_domains.clear();
  m_ui.table->clearContents();

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

	if(query.exec("SELECT blocked, domain FROM dooble_blocked_domains"))
	  while(query.next())
	    m_blocked_domains[query.value(1).toString()] =
	      query.value(0).toBool();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  m_ui.table->setRowCount(m_blocked_domains.size());
  m_ui.table->setSortingEnabled(false);

  QHashIterator<QString, char> it(m_blocked_domains);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      item->setCheckState(Qt::Checked);
      m_ui.table->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 1, item);
      i += 1;
    }

  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  QApplication::restoreOverrideCursor();
}

void dooble_blocked_domains::save_blocked_domain(const QString &domain,
						 bool state)
{
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

	query.exec("CREATE TABLE IF NOT EXISTS dooble_blocked_domains ("
		   "blocked TEXT NOT NULL, "
		   "domain TEXT NOT NULL, "
		   "domain_digest TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_blocked_domains "
	   "(blocked, domain, domain_digest) VALUES (?, ?, ?)");
	query.addBindValue(state);
	query.addBindValue(domain);
	query.addBindValue(domain);
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
}

void dooble_blocked_domains::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("blocked_domains_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_blocked_domains::slot_add(void)
{
  QString text = QInputDialog::
    getText(this, tr("Dooble: New Blocked Domain"), tr("Blocked Domain")).
    trimmed();

  if(text.isEmpty())
    return;

  if(!m_ui.table->findItems(text, Qt::MatchFixedString).isEmpty())
    return;

  m_blocked_domains[text] = 1;
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);

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

  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  save_blocked_domain(text, true);
}

void dooble_blocked_domains::slot_delete_rows(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedRows(1));
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
	    query.prepare("DELETE FROM dooble_blocked_domains "
			  "WHERE domain_digest = ?");
	    query.addBindValue(list.at(i).data().toString());

	    if(query.exec())
	      m_ui.table->removeRow(list.at(i).row());
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}
