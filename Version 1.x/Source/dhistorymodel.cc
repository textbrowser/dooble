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
#include <QUrl>
#include <QIcon>
#include <QTimer>
#include <QBuffer>
#include <QMimeData>
#include <QSqlQuery>

#include "dmisc.h"
#include "dooble.h"
#include "dhistorymodel.h"

dhistorymodel::dhistorymodel(void):QStandardItemModel()
{
  m_watchers = 0;
  setColumnCount(6); /* 0 - Host
		     ** 1 - Last Visited Date
		     ** 2 - URL
		     ** 3 - Title
		     ** 4 - Visits
		     ** 5 - Description
		     */
  m_timer = new QTimer(this);
  m_timer->setInterval(2500);
  connect(m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotTimeout(void)));
}

dhistorymodel::~dhistorymodel()
{
}

void dhistorymodel::slotTimeout(void)
{
  QFileInfo fileInfo(dooble::s_homePath +
		     QDir::separator() + "history.db");

  if(fileInfo.lastModified() <= m_lastModificationTime)
    return;
  else
    m_lastModificationTime = fileInfo.lastModified();

  if(m_watchers > 0)
    populate();
  else
    /*
    ** This will force an update whenever m_watchers is
    ** greater than zero.
    */

    m_lastModificationTime = QDateTime();
}

bool dhistorymodel::deletePage(const QString &text)
{
  bool deleted = false;
  QList<QStandardItem *> list
    (findItems(text, Qt::MatchExactly, 2)); // The URL.

  if(!list.isEmpty())
    {
      /*
      ** Delete an entry from the history table.
      */

      m_timer->stop();

      QStandardItem *item = list.at(0);

      if(item)
	deleted = removeRow(item->row());

      {
	QSqlDatabase db = QSqlDatabase::addDatabase
	  ("QSQLITE", "history_model_delete");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  {
	    QSqlQuery query(db);
	    QUrl url(QUrl::fromUserInput(text));
	    bool ok = true;
	    int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	    query.exec("PRAGMA secure_delete = ON");
	    query.prepare("DELETE FROM history WHERE "
			  "url_hash = ? AND "
			  "temporary = ?");
	    query.bindValue
	      (0,
	       dmisc::hashedString(url.
				   toEncoded(QUrl::
					     StripTrailingSlash),
				   &ok).toBase64());
	    query.bindValue(1, temporary);

	    if(ok)
	      query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase("history_model_delete");
      m_timer->start();
    }

  return deleted;
}

void dhistorymodel::deleteAll(void)
{
  m_timer->stop();
  beginResetModel();

  QStringList items;

  for(int i = rowCount() - 1; i >= 0; i--)
    {
      QStandardItem *item = this->item(i, 2);

      if(item)
	{
	  items.append(item->text());
	  removeRow(item->row());
	}
    }

  if(!items.isEmpty())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase
	  ("QSQLITE", "history_model_delete_all");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  while(!items.isEmpty())
	    {
	      QSqlQuery query(db);
	      QUrl url(QUrl::fromUserInput(items.takeFirst()));
	      bool ok = true;
	      int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	      query.exec("PRAGMA secure_delete = ON");
	      query.prepare("DELETE FROM history WHERE "
			    "url_hash = ? AND "
			    "temporary = ?");
	      query.bindValue
		(0,
		 dmisc::hashedString(url.
				     toEncoded(QUrl::
					       StripTrailingSlash),
				     &ok).toBase64());
	      query.bindValue(1, temporary);

	      if(ok)
		query.exec();
	    }

	db.close();
      }

      QSqlDatabase::removeDatabase("history_model_delete_all");
    }

  endResetModel();
  m_timer->start();
}

void dhistorymodel::populate(void)
{
  /*
  ** If the model was normal, it would not require clearing
  ** and repopulating whenever the disk data changed.
  ** However, the data are encrypted. I suppose
  ** one could perform a difference between the disk data
  ** and the model's data and populate the model with the results.
  ** Such an approach would still require reading the entire history from the
  ** disk as we can't predict change (unless we compare
  ** encrypted values). It gets bothersome. Perhaps a new
  ** date field (the date of the inserted item) would ease our troubles?
  */

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "history_model");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "history.db");

    if(db.open())
      {
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT last_visited, url, "
		      "title, visits, icon, "
		      "description "
		      "FROM history WHERE "
		      "temporary = ?");
	query.bindValue(0, temporary);

	if(query.exec())
	  {
	    beginResetModel();
	    removeRows(0, rowCount());

	    while(query.next())
	      {
		bool ok = true;
		QUrl url(QUrl::fromEncoded
			 (dmisc::daa
			  (QByteArray::fromBase64
			   (query.value(1).toByteArray()), &ok),
			  QUrl::StrictMode));

		if(!ok || !dmisc::isSchemeAcceptedByDooble(url.scheme()))
		  continue;

		QString host(url.host());
		QString title
		  (QString::fromUtf8
		   (dmisc::daa
		    (QByteArray::fromBase64
		     (query.value(2).toByteArray()), &ok)));

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
		      (query.value(0).toByteArray()), &ok)),
		    Qt::ISODate));

		if(!ok)
		  continue;

		QByteArray bytes;

		bytes = query.value(4).toByteArray();
		bytes = dmisc::daa(bytes, &ok);

		if(!ok)
		  continue;

		QIcon icon;
		QBuffer buffer;

		buffer.setBuffer(&bytes);

		QStandardItem *item = 0;

		if(host.isEmpty())
		  host = "localhost";

		item = new QStandardItem(host);

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
		setRowCount(rowCount() + 1);
		setItem(rowCount() - 1, 0, item);
		item = new QStandardItem
		  (dateTime.toString("yyyy/MM/dd hh:mm:ss"));

		/*
		** Sorting dates is complicated enough. By
		** inserting dates in the below format,
		** we'll guarantee that the date sort will behave.
		*/

		item->setEditable(false);
		setItem(rowCount() - 1, 1, item);
		item = new QStandardItem
		  (url.toString(QUrl::StripTrailingSlash));
		item->setData(url);
		item->setIcon(icon);
		item->setEditable(false);
		setItem(rowCount() - 1, 2, item);

		if(title.isEmpty())
		  title = url.toString(QUrl::StripTrailingSlash);

		item = new QStandardItem(title);
		item->setIcon(icon);
		item->setEditable(false);
		setItem(rowCount() - 1, 3, item);

		/*
		** What are you doing? Remember, sorting
		** will be performed via the view. If we sort
		** the model, the views may become twisted.
		** Since visits is an integer, we'd like to
		** make it a sortable string. Are ten digits
		** enough for an integer?
		*/

		item = new QStandardItem
		  (QString::number(visits.toLongLong()).
		   rightJustified(10, '0'));
		item->setEditable(false);
		setItem(rowCount() - 1, 4, item);
		item = new QStandardItem(description);
		setItem(rowCount() - 1, 5, item);
	      }

	    endResetModel();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history_model");
}

QMimeData *dhistorymodel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = QStandardItemModel::mimeData(indexes);

  if(mimeData)
    {
      QList<QUrl> list;

      for(int i = 0; i < indexes.size(); i++)
	{
	  QStandardItem *item = this->item(indexes.at(i).row(), 2);

	  if(item)
	    list.append(item->data().toUrl());
	}

      mimeData->setUrls(list);
    }

  return mimeData;
}

void dhistorymodel::addWatchers(const int watcher)
{
  m_watchers += watcher;

  if(m_watchers > 0)
    m_timer->start();
  else
    {
      m_timer->stop();
      m_lastModificationTime = QDateTime();
      removeRows(0, rowCount());
    }
}
