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

#include "dooble.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"

dooble_certificate_exceptions::dooble_certificate_exceptions(void):QMainWindow()
{
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.table->setWordWrap(false);
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.reset,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reset(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
}

void dooble_certificate_exceptions::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_certificate_exceptions::exception_accepted(const QString &error,
						       const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTableWidgetItem *> list
    (m_ui.table->
     findItems(url.toString(), Qt::MatchEndsWith | Qt::MatchStartsWith));

  QApplication::restoreOverrideCursor();

  if(!list.isEmpty())
    return;

  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);

  QTableWidgetItem *item = nullptr;

  item = new QTableWidgetItem(url.toString());
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 0, item);
  item = new QTableWidgetItem(error.trimmed());
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 1, item);
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortItems
    (0, m_ui.table->horizontalHeader()->sortIndicatorOrder());
}

void dooble_certificate_exceptions::keyPressEvent(QKeyEvent *event)
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

void dooble_certificate_exceptions::purge(void)
{
  m_ui.table->setRowCount(0);
}

void dooble_certificate_exceptions::remove_exception(const QUrl &url)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTableWidgetItem *> list
    (m_ui.table->
     findItems(url.toString(), Qt::MatchEndsWith | Qt::MatchStartsWith));

  QApplication::restoreOverrideCursor();

  if(list.isEmpty())
    return;

  QList<int> rows;

  for(int i = 0; i < list.size(); i++)
    {
      QTableWidgetItem *item = list.at(i);

      if(item)
	rows << item->row();
    }

  std::sort(rows.begin(), rows.end());

  for(int i = rows.size() - 1; i >= 0; i--)
    m_ui.table->removeRow(rows.at(i));
}

void dooble_certificate_exceptions::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_certificate_exceptions::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("certificate_exceptions_geometry", saveGeometry().toBase64());
}

void dooble_certificate_exceptions::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("certificate_exceptions_geometry").
			      toByteArray()));

  QMainWindow::show();
}

void dooble_certificate_exceptions::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("certificate_exceptions_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
}

void dooble_certificate_exceptions::slot_add(void)
{
  dooble_certificate_exceptions_menu_widget::exception_accepted
    (m_ui.error->currentText(), QUrl::fromUserInput(m_ui.url->text()));
}

void dooble_certificate_exceptions::slot_delete_selected(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedRows(0));

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
      mb.setText(tr("Are you sure that you wish to delete the selected "
		    "exceptions(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_certificate_exceptions");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_certificate_exceptions.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	for(int i = list.size() - 1; i >= 0; i--)
	  {
	    query.prepare
	      ("DELETE FROM dooble_certificate_exceptions "
	       "WHERE temporary = ? AND url_digest IN (?, ?)");
	    query.addBindValue
	      (dooble::s_cryptography->authenticated() ? 0 : 1);
	    query.addBindValue
	      (dooble::s_cryptography->
	       hmac(list.at(i).data().toString()).toBase64());
	    query.addBindValue
	      (dooble::s_cryptography->
	       hmac(list.at(i).data().toString() + "/").toBase64());

	    if(query.exec())
	      m_ui.table->removeRow(list.at(i).row());
	  }

	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_certificate_exceptions::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_certificate_exceptions::slot_populate(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.table->setRowCount(0);

  QList<QHash<QString, QVariant> > list;

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      QString database_name("dooble_certificate_exceptions");

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(dooble_settings::setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_certificate_exceptions.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT error, exception_accepted, url, OID "
			  "FROM dooble_certificate_exceptions WHERE "
			  "temporary = 0"))
	      while(query.next())
		{
		  QHash<QString, QVariant> hash;

		  for(int i = 0; i < 3; i++)
		    {
		      QByteArray data
			(QByteArray::fromBase64(query.value(i).toByteArray()));

		      data = dooble::s_cryptography->mac_then_decrypt(data);

		      if(data.isEmpty())
			{
			  dooble_database_utilities::remove_entry
			    (db,
			     "dooble_certificate_exceptions",
			     query.value(3).toLongLong());
			  continue;
			}

		      if(i == 0)
			hash["error"] = data;
		      else if(i == 1)
			hash["exception_accepted"] =
			  data == "true" ? true : false; // Not used.
		      else
			hash["url"] = QUrl(data);
		    }

		  if(hash.size() == 3)
		    list << hash;
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }

  m_ui.table->setRowCount(list.size());
  m_ui.table->setSortingEnabled(false);

  for(int i = 0; i < list.size(); i++)
    {
      QHash<QString, QVariant> hash(list.at(i));
      QTableWidgetItem *item = nullptr;

      item = new QTableWidgetItem(hash.value("url").toString());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 0, item);
      item = new QTableWidgetItem(hash.value("error").toString().trimmed());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 1, item);
    }

  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortItems
    (0, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  QApplication::restoreOverrideCursor();
  emit populated();
}

void dooble_certificate_exceptions::slot_reset(void)
{
  m_ui.error->setCurrentIndex(0);
  m_ui.url->clear();
  m_ui.url->setFocus();
}

void dooble_certificate_exceptions::slot_search_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	QTableWidgetItem *item1 = m_ui.table->item(i, 0);
	QTableWidgetItem *item2 = m_ui.table->item(i, 1);

	if(!item1 || !item2)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	if(item1->text().toLower().contains(text) ||
	   item2->text().toLower().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);
      }

  QApplication::restoreOverrideCursor();
}
