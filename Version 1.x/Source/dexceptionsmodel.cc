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

#include <QSqlQuery>
#include <QProgressBar>
#include <QSqlDatabase>

#include "dmisc.h"
#include "dooble.h"
#include "dexceptionsmodel.h"

dexceptionsmodel::dexceptionsmodel(const QString &tableName):
  QStandardItemModel()
{
  m_tableName = tableName;
  setObjectName(m_tableName);
  setSortRole(Qt::UserRole);
  connect(this,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slotCheckBoxItemChanged(QStandardItem *)));
  createExceptionsDatabase();
}

dexceptionsmodel::~dexceptionsmodel()
{
  removeRows(0, rowCount());
}

bool dexceptionsmodel::allowed(const QString &h) const
{
  QList<QStandardItem *> list;
  QString host(h.trimmed());

  if(host.startsWith("."))
    host.remove(0, 1);

  list = findItems(host);

  if(!list.isEmpty())
    {
      if(item(list.at(0)->row(), 3)->checkState() == Qt::Checked)
	return true;
      else
	return false;
    }
  else
    return false;
}

bool dexceptionsmodel::allow(const QString &h)
{
  QString host(h.trimmed());

  if(host.startsWith("."))
    host.remove(0, 1);

  if(host.trimmed().isEmpty())
    return false;

  bool state = false;
  QList<QStandardItem *> list(findItems(host.trimmed()));

  disconnect(this,
	     SIGNAL(itemChanged(QStandardItem *)),
	     this,
	     SLOT(slotCheckBoxItemChanged(QStandardItem *)));

  if(list.isEmpty())
    {
      list.clear();

      QStandardItem *item = 0;

      item = new QStandardItem(host.trimmed());
      item->setData(item->text(), Qt::UserRole);
      item->setEditable(false);
      list << item;
      item = new QStandardItem();
      item->setData("", Qt::UserRole);
      item->setEditable(false);
      list << item;

      QDateTime now(QDateTime::currentDateTime());

      item = new QStandardItem(now.toString("MM/dd/yyyy hh:mm:ss AP"));
      item->setData(now, Qt::UserRole);
      item->setEditable(false);
      list << item;
      item = new QStandardItem();
      item->setData(true, Qt::UserRole);
      item->setEditable(false);
      item->setCheckable(true);
      item->setCheckState(Qt::Checked);
      list << item;
      appendRow(list);
    }
  else
    this->item(list.at(0)->row(), 3)->setCheckState(Qt::Checked);

  connect(this,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slotCheckBoxItemChanged(QStandardItem *)));
  createExceptionsDatabase();

  if(!list.isEmpty() && dmisc::passphraseWasAuthenticated())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						    m_tableName);

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() +
			   QString("%1.db").arg(m_tableName));

	if(db.open())
	  {
	    if(!dooble::s_settings.
	       value("settingsWindow/"
		     "disableAllEncryptedDatabaseWrites", false).
	       toBool())
	      {
		QSqlQuery query(db);

		query.exec("PRAGMA synchronous = OFF");
		query.prepare
		  (QString("INSERT OR REPLACE INTO %1 "
			   "(host, originating_url, datetime, host_hash)"
			   "VALUES (?, ?, ?, ?)").arg(m_tableName));

		bool ok = true;

		query.bindValue
		  (0, dmisc::etm(host.trimmed().toUtf8(),
				 true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (1, dmisc::etm(this->item(list.at(0)->row(),
					      1)->text().toUtf8(),
				   true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (2,
		     dmisc::etm(this->item(list.at(0)->row(),
					   2)->text().toUtf8(),
				true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (3, dmisc::hashedString(host.trimmed().toUtf8(),
					    &ok).toBase64());

		if(ok)
		  state = query.exec();
	      }
	    else
	      state = true;
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(m_tableName);
    }
  else if(!list.isEmpty())
    state = true;

  return state;
}

void dexceptionsmodel::add(const QString &host,
			   const QUrl &url,
			   const QDateTime &dateTime)
{
  if(host.trimmed().isEmpty() || !findItems(host.trimmed()).isEmpty())
    return;

  disconnect(this,
	     SIGNAL(itemChanged(QStandardItem *)),
	     this,
	     SLOT(slotCheckBoxItemChanged(QStandardItem *)));

  QList<QStandardItem *> list;

  QStandardItem *item = 0;

  item = new QStandardItem(host.trimmed());
  item->setData(item->text(), Qt::UserRole);
  item->setEditable(false);
  list << item;
  item = new QStandardItem(url.toString(QUrl::StripTrailingSlash));
  item->setData(item->text(), Qt::UserRole);
  item->setEditable(false);
  list << item;
  item = new QStandardItem(dateTime.toString("MM/dd/yyyy hh:mm:ss AP"));
  item->setData(dateTime, Qt::UserRole);
  item->setEditable(false);
  list << item;
  item = new QStandardItem();
  item->setData(false, Qt::UserRole);
  item->setEditable(false);
  item->setCheckable(true);
  item->setCheckState(Qt::Unchecked);
  list << item;
  appendRow(list);
  connect(this,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slotCheckBoxItemChanged(QStandardItem *)));
}

void dexceptionsmodel::populate(void)
{
  removeRows(0, rowCount());

  if(!dmisc::passphraseWasAuthenticated())
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_tableName);

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() +
		       QString("%1.db").arg(m_tableName));

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec(QString("SELECT host, originating_url, "
			      "datetime FROM %1").
		      arg(m_tableName)))
	  {
	    while(query.next())
	      {
		bool ok = true;
		QString url
		  (QString::fromUtf8
		   (dmisc::daa
		    (QByteArray::fromBase64(query.value(1).toByteArray()),
		     &ok)));

		if(!ok)
		  continue;

		QString host
		  (QString::fromUtf8
		   (dmisc::daa
		    (QByteArray::fromBase64(query.value(0).toByteArray()),
		     &ok)));

		if(!ok)
		  continue;

		QString dateTime
		  (QString::fromUtf8
		   (dmisc::daa
		    (QByteArray::fromBase64(query.value(2).toByteArray()),
		     &ok)));

		if(!ok)
		  continue;

		if(!host.trimmed().isEmpty())
		  {
		    disconnect(this,
			       SIGNAL(itemChanged(QStandardItem *)),
			       this,
			       SLOT(slotCheckBoxItemChanged(QStandardItem *)));

		    QList<QStandardItem *> list;

		    QStandardItem *item = 0;

		    item = new QStandardItem(host.trimmed());
		    item->setData(item->text(), Qt::UserRole);
		    item->setEditable(false);
		    list << item;
		    item = new QStandardItem(url);
		    item->setData(item->text(), Qt::UserRole);
		    item->setEditable(false);
		    list << item;
		    item = new QStandardItem(dateTime);
		    item->setData
		      (QDateTime::fromString(dateTime,
					     "MM/dd/yyyy hh:mm:ss AP"),
		       Qt::UserRole);
		    item->setEditable(false);
		    list << item;
		    item = new QStandardItem();
		    item->setData(true, Qt::UserRole);
		    item->setEditable(false);
		    item->setCheckable(true);
		    item->setCheckState(Qt::Checked);
		    list << item;
		    appendRow(list);
		    connect(this,
			    SIGNAL(itemChanged(QStandardItem *)),
			    this,
			    SLOT(slotCheckBoxItemChanged(QStandardItem *)));
		  }
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(m_tableName);
}

void dexceptionsmodel::deleteList(const QModelIndexList &list)
{
  if(dmisc::passphraseWasAuthenticated())
    {
      int removedRows = 0;
      QStringList items;
      QModelIndexList l_list(list);

      /*
      ** Sort the list by row number. If the list is not sorted,
      ** the following while-loop misbehaves.
      */

      qSort(l_list);

      while(!l_list.isEmpty())
	{
	  QStandardItem *item = this->item
	    (l_list.takeFirst().row() - removedRows, 0);

	  if(item)
	    {
	      items.append(item->text());

	      if(removeRow(item->row()))
		removedRows += 1;
	    }
	}

      if(!items.isEmpty())
	{
	  {
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							m_tableName);

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() +
			       QString("%1.db").arg(m_tableName));

	    if(db.open())
	      while(!items.isEmpty())
		{
		  QSqlQuery query(db);
		  QString host(items.takeFirst());
		  bool ok = true;

		  query.prepare(QString("DELETE FROM %1 "
					"WHERE host_hash = ?").
				arg(m_tableName));
		  query.bindValue
		    (0, dmisc::hashedString(host.trimmed().toUtf8(),
					    &ok).toBase64());

		  if(ok)
		    query.exec();
		}

	    db.close();
	  }

	  QSqlDatabase::removeDatabase(m_tableName);
	}
    }
  else
    {
      int removedRows = 0;
      QModelIndexList l_list(list);

      /*
      ** Sort the list by row number. If the list is not sorted,
      ** the following while-loop misbehaves.
      */

      qSort(l_list);

      while(!l_list.isEmpty())
	{
	  QStandardItem *item = this->item
	    (l_list.takeFirst().row() - removedRows, 0);

	  if(item)
	    if(removeRow(item->row()))
	      removedRows += 1;
	}
    }
}

void dexceptionsmodel::slotCheckBoxItemChanged(QStandardItem *item)
{
  if(dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    return;

  if(dmisc::passphraseWasAuthenticated() &&
     item && item->isCheckable())
    {
      createExceptionsDatabase();

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_tableName);

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() +
			   QString("%1.db").arg(m_tableName));

	if(db.open())
	  {
	    if(item->checkState() == Qt::Checked)
	      {
		QSqlQuery query(db);

		query.prepare
		  (QString("INSERT OR REPLACE INTO %1 "
			   "(host, originating_url, datetime, host_hash)"
			   "VALUES (?, ?, ?, ?)").arg(m_tableName));

		bool ok = true;

		query.bindValue
		  (0, dmisc::etm(this->item(item->row(), 0)->text().
				 toUtf8(),
				 true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (1, dmisc::etm(this->item(item->row(), 1)->text().
				   toUtf8(),
				   true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (2, dmisc::etm(this->item(item->row(), 2)->text().
				   toUtf8(),
				   true, &ok).toBase64());

		if(ok)
		  query.bindValue
		    (3, dmisc::hashedString(this->item(item->row(), 0)->text().
					    toUtf8(), &ok).toBase64());

		if(ok)
		  query.exec();
	      }
	    else
	      {
		QSqlQuery query(db);
		bool ok = true;

		query.prepare(QString("DELETE FROM %1 WHERE "
				      "host_hash = ?").arg(m_tableName));
		query.bindValue
		  (0, dmisc::hashedString(this->item(item->row(), 0)->text().
					  toUtf8(), &ok).toBase64());

		if(ok)
		  query.exec();
	      }
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(m_tableName);
    }
}

void dexceptionsmodel::createExceptionsDatabase(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_tableName);

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + QString("%1.db").arg(m_tableName));

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec(QString("CREATE TABLE IF NOT EXISTS %1 ("
			   "host TEXT NOT NULL, "
			   "host_hash TEXT PRIMARY KEY NOT NULL, "
			   "originating_url TEXT, "
			   "datetime TEXT NOT NULL)").
		   arg(m_tableName));
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(m_tableName);
}

void dexceptionsmodel::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;

  /*
  ** This method assumes that the respective container
  ** is as current as possible. It will use the container's
  ** contents to create new database entries using the newly-encoded
  ** data. Obsolete data will be deleted via the purge method.
  */

  purge();

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  QList<int> rows;

  for(int i = 0; i < rowCount(); i++)
    {
      QStandardItem *item = this->item(i, 3);

      if(item && item->checkState() == Qt::Checked)
	rows.append(i);
    }

  if(progress)
    {
      progress->setMaximum(rows.size());
      progress->update();
    }

  for(int i = 0; i < rows.size(); i++)
    {
      QStandardItem *item = this->item(rows.at(i), 0);

      if(item)
	allow(item->text());

      if(progress)
	progress->setValue(i + 1);
    }

  if(progress)
    progress->setVisible(false);
}

void dexceptionsmodel::purge(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						m_tableName);

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() +
		       QString("%1.db").arg(m_tableName));

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec(QString("DELETE FROM %1").arg(m_tableName));
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(m_tableName);
}

bool dexceptionsmodel::contains(const QString &host) const
{
  return !findItems(host.trimmed()).isEmpty();
}

QStringList dexceptionsmodel::allowedHosts(void) const
{
  QStringList list;

  for(int i = 0; i < rowCount(); i++)
    if(item(i, 3)->checkState() == Qt::Checked)
      list.append(item(i, 0)->text().trimmed());

  return list;
}
