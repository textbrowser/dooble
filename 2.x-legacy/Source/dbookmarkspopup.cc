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

#include <QSettings>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QFileIconProvider>

#include "dmisc.h"
#include "dooble.h"
#include "dbookmarkspopup.h"
#include "dbookmarkswindow.h"

dbookmarkspopup::dbookmarkspopup(void):QWidget()
{
  m_folderOid = -1;
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  ui.folders->setModel(dooble::s_bookmarksFolderModel);
  connect(ui.folders,
	  SIGNAL(itemSelected(const QModelIndex &)),
	  this,
	  SLOT(slotFolderSelected(const QModelIndex &)));
  connect(ui.titleLineEdit,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slotTitleChanged(void)));
  connect(ui.closePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(closed(void)));
  connect(ui.addFolderPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAddFolder(void)));
  connect(ui.descriptionTextEdit,
	  SIGNAL(textChanged(void)),
	  this,
	  SLOT(slotDescriptionChanged(void)));
  connect(ui.deleteBookmarkPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteBookmark(void)));
  connect(dooble::s_bookmarksFolderModel,
	  SIGNAL(dataChanged(const QModelIndex &,
			     const QModelIndex &)),
	  this,
	  SLOT(slotFolderDataChanged(const QModelIndex &,
				     const QModelIndex &)));
  slotSetIcons();
}

dbookmarkspopup::~dbookmarkspopup()
{
}

void dbookmarkspopup::populate(const QUrl &url)
{
  m_folderOid = -1;
  m_title.clear();
  m_url = url;
  ui.titleLineEdit->clear();
  disconnect(ui.descriptionTextEdit,
	     SIGNAL(textChanged(void)),
	     this,
	     SLOT(slotDescriptionChanged(void)));
  ui.descriptionTextEdit->clear();
  connect(ui.descriptionTextEdit,
	  SIGNAL(textChanged(void)),
	  this,
	  SLOT(slotDescriptionChanged(void)));

  /*
  ** Retrieve the bookmark's folder, title, and description.
  */

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT title, description, folder_oid, url "
		      "FROM bookmarks WHERE temporary = ? AND "
		      "url_hash = ?");
	query.bindValue(0, temporary);
	query.bindValue
	  (1,
	   dmisc::hashedString(url.
			       toEncoded(QUrl::StripTrailingSlash), &ok).
	   toBase64());

	if(ok && query.exec())
	  if(query.next())
	    {
	      m_folderOid = query.value(2).toLongLong();

	      if(ok)
		m_title =
		  QString::fromUtf8
		  (dmisc::daa
		   (QByteArray::fromBase64
		    (query.value(0).toByteArray()), &ok));

	      QString description;

	      if(ok)
		description = dmisc::daa
		  (QByteArray::fromBase64(query.value(1).toByteArray()),
		   &ok);

	      if(ok)
		{
		  ui.titleLineEdit->setText(m_title);
		  disconnect(ui.descriptionTextEdit,
			     SIGNAL(textChanged(void)),
			     this,
			     SLOT(slotDescriptionChanged(void)));
		  ui.descriptionTextEdit->setPlainText(description);
		  connect(ui.descriptionTextEdit,
			  SIGNAL(textChanged(void)),
			  this,
			  SLOT(slotDescriptionChanged(void)));
		}
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  ui.titleLineEdit->selectAll();
  ui.titleLineEdit->setFocus();

  if(dooble::s_bookmarksFolderModel->index(0, 0).isValid())
    {
      /*
      ** Recursively search for the bookmark's folder.
      ** If found, select the folder and expand its parents.
      */

      QModelIndexList list(dooble::s_bookmarksFolderModel->
			   match(dooble::s_bookmarksFolderModel->index(0, 0),
				 Qt::UserRole + 2,
				 m_folderOid,
				 1,
				 Qt::MatchRecursive));

      if(!list.isEmpty())
	{
	  ui.folders->setExpanded(list.first(), true);
	  ui.folders->selectionModel()->select
	    (list.first(), QItemSelectionModel::ClearAndSelect);
	  ui.folders->selectionModel()->setCurrentIndex
	    (list.first(), QItemSelectionModel::Current);
	}
    }
}

void dbookmarkspopup::updateBookmark(void)
{
  if(dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    goto done_label;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						"bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.prepare
	  ("UPDATE bookmarks SET title = ?, "
	   "description = ? "
	   "WHERE url_hash = ? AND "
	   "folder_oid = ? AND "
	   "temporary = ?");
	query.bindValue
	  (0,
	   dmisc::etm(m_title.toUtf8(), true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (1,
	     dmisc::etm(ui.descriptionTextEdit->toPlainText().
			trimmed().toUtf8(), true, &ok).
	     toBase64());

	if(ok)
	  query.bindValue
	    (2,
	     dmisc::hashedString(m_url.
				 toEncoded(QUrl::StripTrailingSlash),
				 &ok).toBase64());

	query.bindValue(3, m_folderOid);
	query.bindValue(4, temporary);

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");

 done_label:
  emit changed();
}

void dbookmarkspopup::slotSetIcons(void)
{
  QPixmap pixmap;
  QPixmap scaledPixmap;
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  pixmap.load(settings.value("bookmarksPopup/bookmarkIcon").toString());

  if(!pixmap.isNull())
    scaledPixmap = pixmap.scaled(QSize(32, 32),
				 Qt::KeepAspectRatio,
				 Qt::SmoothTransformation);

  if(scaledPixmap.isNull())
    ui.icon->setPixmap(pixmap);
  else
    ui.icon->setPixmap(scaledPixmap);

  ui.closePushButton->setIcon
    (QIcon(settings.value("bookmarksPopup/closeButtonIcon").toString()));
  ui.addFolderPushButton->setIcon
    (QIcon(settings.value("bookmarksPopup/addButtonIcon").toString()));
  emit iconsChanged();
}

void dbookmarkspopup::slotAddFolder(void)
{
  QItemSelection selection(ui.folders->selectionModel()->selection());

  if(dooble::s_bookmarksWindow)
    dooble::s_bookmarksWindow->addFolder();

  if(selection.indexes().value(0).isValid())
    ui.folders->selectionModel()->select
      (selection, QItemSelectionModel::ClearAndSelect);
  else
    ui.folders->selectionModel()->select
      (dooble::s_bookmarksFolderModel->index(0, 0),
       QItemSelectionModel::ClearAndSelect);
}

void dbookmarkspopup::slotTitleChanged(void)
{
  QString title(ui.titleLineEdit->text().trimmed());

  if(title.isEmpty())
    {
      ui.titleLineEdit->setText(m_title);
      ui.titleLineEdit->selectAll();
      return;
    }
  else
    {
      m_title = title;
      ui.titleLineEdit->selectAll();
    }

  updateBookmark();
}

void dbookmarkspopup::slotDeleteBookmark(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "bookmarks");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

	query.exec("PRAGMA secure_delete = ON");
	query.prepare("DELETE FROM bookmarks WHERE "
		      "folder_oid = ? AND "
		      "url_hash = ? AND "
		      "temporary = ?");
	query.bindValue(0, m_folderOid);
	query.bindValue
	  (1,
	   dmisc::hashedString(m_url.
			       toEncoded(QUrl::StripTrailingSlash), &ok).
	   toBase64());
	query.bindValue(2, temporary);

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks");
  emit changed();
  emit closed();
}

void dbookmarkspopup::slotFolderSelected(const QModelIndex &index)
{
  if(index.isValid())
    {
      QStandardItem *item = dooble::s_bookmarksFolderModel->itemFromIndex
	(index);

      if(item)
	{
	  m_folderOid = item->data(Qt::UserRole + 2).toLongLong();

	  if(dooble::s_settings.value("settingsWindow/"
				      "disableAllEncryptedDatabaseWrites",
				      false).
	     toBool())
	    goto done_label;

	  {
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							"bookmarks");

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() + "bookmarks.db");

	    if(db.open())
	      {
		QSqlQuery query(db);
		bool ok = true;
		int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;

		query.prepare
		  ("UPDATE bookmarks SET folder_oid = ? "
		   "WHERE url_hash = ? AND "
		   "temporary = ?");
		query.bindValue(0, m_folderOid);
		query.bindValue
		  (1,
		   dmisc::hashedString(m_url.
				       toEncoded(QUrl::StripTrailingSlash),
				       &ok).
		   toBase64());
		query.bindValue(2, temporary);

		if(ok)
		  query.exec();
	      }

	    db.close();
	  }

	  QSqlDatabase::removeDatabase("bookmarks");
	done_label:
	  emit changed();
	}
    }
}

void dbookmarkspopup::slotFolderDataChanged
(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  if(!dooble::s_bookmarksWindow)
    return;

  Q_UNUSED(bottomRight);

  QModelIndexList list(ui.folders->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    if(topLeft == list.first())
      dooble::s_bookmarksWindow->renameFolder(topLeft);
}

void dbookmarkspopup::slotDescriptionChanged(void)
{
  updateBookmark();
}
