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
#include <QSqlDatabase>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

QAtomicInteger<qintptr> dooble_certificate_exceptions::s_db_id;

dooble_certificate_exceptions::dooble_certificate_exceptions(void):QMainWindow()
{
  m_ui.setupUi(this);
}

void dooble_certificate_exceptions::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
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

	    if(query.exec("SELECT error, exception_accepted, url "
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
			continue;

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
      QTableWidgetItem *item = 0;

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
}
