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

#include <QBuffer>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#endif

#include "dmisc.h"
#include "dooble.h"
#include "dfilesystemmodel.h"

dfilesystemmodel::dfilesystemmodel(QObject *parent):QFileSystemModel(parent)
{
  m_headers << tr("Name")
	    << tr("Size")
	    << tr("Type")
	    << tr("Date Modified")
	    << tr("Date Accessed")
	    << tr("Owner")
	    << tr("Group")
	    << tr("Permissions");

  if(dooble::s_settings.value("settingsWindow/record_file_suffixes",
			      true).toBool())
    connect(this,
	    SIGNAL(directoryLoaded(const QString &)),
	    this,
	    SLOT(slotDirectoryLoaded(const QString &)));
}

dfilesystemmodel::~dfilesystemmodel()
{
  m_future.cancel();
  m_future.waitForFinished();
}

QVariant dfilesystemmodel::headerData(int section, Qt::Orientation orientation,
				      int role) const
{
  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      if(section >= 0 && section < m_headers.size())
	return m_headers.at(section);
    }

  return QFileSystemModel::headerData(section, orientation, role);
}

int dfilesystemmodel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_headers.size();
}

QVariant dfilesystemmodel::data(const QModelIndex &index, int role) const
{
  if(index.isValid())
    {
      switch(index.column())
	{
	case 1:
	  if(role == Qt::DisplayRole)
	    return QVariant(dmisc::formattedSize(fileInfo(index).size()));
	  else
	    return QVariant();
	case 4:
	  if(role == Qt::DisplayRole)
	    return QVariant(fileInfo(index).lastRead());
	  else
	    return QVariant();
	case 5:
	  if(role == Qt::DisplayRole)
	    return QVariant(fileInfo(index).owner());
	  else
	    return QVariant();
	case 6:
	  if(role == Qt::DisplayRole)
	    return QVariant(fileInfo(index).group());
	  else
	    return QVariant();
	case 7:
	  if(role == Qt::DisplayRole)
	    {
	      QString p("");
	      QFile::Permissions permissions(fileInfo(index).permissions());

#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
	      static QString s_array[] = {"r", "w", "x",
					  "r", "w", "x",
					  "r", "w", "x"};
	      static QFile::Permission e_array[] =
		{QFile::ReadOwner, QFile::WriteOwner, QFile::ExeOwner,
		 QFile::ReadGroup, QFile::WriteGroup, QFile::ExeGroup,
		 QFile::ReadOther, QFile::WriteOther, QFile::ExeOther};

	      for(int i = 0; i < 9; i++)
		if(permissions & e_array[i])
		  p += s_array[i];
		else
		  p += "-";
#elif defined(Q_OS_WIN32)
	      static QString s_array[] = {"r", "w", "x",
					  "r", "w", "x",
					  "r", "w", "x"};
	      static QFile::Permission e_array[] =
		{QFile::ReadUser, QFile::WriteUser, QFile::ExeUser,
		 QFile::ReadGroup, QFile::WriteGroup, QFile::ExeGroup,
		 QFile::ReadOther, QFile::WriteOther, QFile::ExeOther};

	      for(int i = 0; i < 9; i++)
		if(permissions & e_array[i])
		  p += s_array[i];
		else
		  p += "-";
#endif

	      return p;
	    }
	  else
	    return QVariant();
	default:
	  break;
	}
    }

  return QFileSystemModel::data(index, role);
}

void dfilesystemmodel::slotDirectoryLoaded(const QString &path)
{
  if(m_future.isFinished())
    m_future = QtConcurrent::run
      (this, &dfilesystemmodel::processDirectory, path);
}

void dfilesystemmodel::processDirectory(const QString &path)
{
  QMap<QString, QString> suffixesA;
  QMap<QString, QString> suffixesU;

  {
    QSqlQuery *query = 0;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "applications");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "applications.db");

    if(db.open())
      {
	query = new QSqlQuery(db);
	query->exec("CREATE TABLE IF NOT EXISTS applications ("
		    "file_suffix TEXT PRIMARY KEY NOT NULL, "
		    "action TEXT DEFAULT NULL, "
		    "icon BLOB DEFAULT NULL)");
	query->exec("PRAGMA synchronous = OFF");
      }

    for(int i = 0; i < rowCount(index(path)); i++)
      {
	if(m_future.isCanceled())
	  break;

	QFileInfo info;
	QModelIndex idx(index(i, 0, index(path)));

	info = fileInfo(idx);

	if(info.isFile())
	  {
	    QString suffix(info.suffix().trimmed());

	    if(dmisc::
	       canDoobleOpenLocalFile(QUrl::
				      fromLocalFile(info.
						    absoluteFilePath())))
	      continue;

	    if(!suffix.isEmpty() && !info.completeBaseName().isEmpty())
	      {
		QString action("prompt");
		QReadLocker locker(&dooble::s_applicationsActionsLock);

		if(dooble::s_applicationsActions.contains(suffix))
		  action = dooble::s_applicationsActions[suffix];

		/*
		** Updating the applications.db database while
		** the user is watching the directory that houses
		** the database will devastate Dooble.
		*/

		if(query &&
		   !dooble::s_applicationsActions.contains(suffix))
		  {
		    locker.unlock();
		    query->prepare
		      ("INSERT INTO applications ("
		       "file_suffix, action, icon) "
		       "VALUES (?, ?, ?)");
		    query->bindValue(0, suffix);
		    query->bindValue(1, action);

		    QIcon icon(fileIcon(idx));
		    QBuffer buffer;
		    QByteArray bytes;

		    buffer.setBuffer(&bytes);

		    if(buffer.open(QIODevice::WriteOnly))
		      {
			QDataStream out(&buffer);

			out << icon.pixmap(16, QIcon::Normal, QIcon::On);

			if(out.status() != QDataStream::Ok)
			  bytes.clear();
		      }
		    else
		      bytes.clear();

		    query->bindValue(2, bytes);
		    buffer.close();
		    query->exec();
		  }
		else
		  locker.unlock();

		{
		  QWriteLocker locker(&dooble::s_applicationsActionsLock);

		  if(!dooble::s_applicationsActions.contains(suffix))
		    {
		      suffixesA[suffix] = "prompt";
		      dooble::s_applicationsActions[suffix] = "prompt";
		    }
		  else
		    suffixesU[suffix] = action;
		}
	      }
	  }
      }

    if(query)
      delete query;

    db.close();
  }

  QSqlDatabase::removeDatabase("applications");

  if(m_future.isCanceled())
    return;

  if(!suffixesA.isEmpty())
    emit suffixesAdded(suffixesA);

  for(int i = 0; i < suffixesU.keys().size(); i++)
    emit suffixUpdated(suffixesU.keys().at(i),
		       suffixesU.value(suffixesU.keys().at(i)));
}

void dfilesystemmodel::enable(const bool state)
{
  if(state)
    connect(this,
	    SIGNAL(directoryLoaded(const QString &)),
	    this,
	    SLOT(slotDirectoryLoaded(const QString &)),
	    Qt::UniqueConnection);
  else
    disconnect(this,
	       SIGNAL(directoryLoaded(const QString &)),
	       this,
	       SLOT(slotDirectoryLoaded(const QString &)));
}
