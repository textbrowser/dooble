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

#include <QClipboard>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_downloads_item.h"
#include "dooble_page.h"

dooble_downloads::dooble_downloads(void):QMainWindow()
{
  m_download_path_inspection_timer.start(2500);
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.download_path->setText
    (dooble_settings::setting("download_path").toString());

  if(m_ui.download_path->text().isEmpty())
    m_ui.download_path->setText
      (QStandardPaths::
       standardLocations(QStandardPaths::DesktopLocation).value(0));

  connect(&m_download_path_inspection_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_download_path_inspection_timer_timeout(void)));
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(QWebEngineProfile::defaultProfile(),
	  SIGNAL(downloadRequested(QWebEngineDownloadItem *)),
	  this,
	  SLOT(slot_download_requested(QWebEngineDownloadItem *)));
  connect(m_ui.clear_finished_downloads,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_clear_finished_downloads(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.select,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_path(void)));
  connect(m_ui.table,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
  m_ui.download_path->setCursorPosition(0);
  m_ui.download_path->setToolTip(m_ui.download_path->text());
  m_ui.table->setContextMenuPolicy(Qt::CustomContextMenu);
}

QString dooble_downloads::download_path(void) const
{
  return m_ui.download_path->text();
}

bool dooble_downloads::contains(QWebEngineDownloadItem *download) const
{
  return m_downloads.contains(download);
}

bool dooble_downloads::has_downloads_for_profile
(QWebEngineProfile *profile) const
{
  for(int i = 0; i < m_ui.table->rowCount(); i++)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

      if(downloads_item)
	if(!downloads_item->is_finished())
	  if(downloads_item->profile() == profile)
	    return true;
    }

  return false;
}

bool dooble_downloads::is_finished(void) const
{
  for(int i = 0; i < m_ui.table->rowCount(); i++)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

      if(downloads_item)
	if(!downloads_item->is_finished())
	  return false;
    }

  return true;
}

void dooble_downloads::abort(void)
{
  for(int i = 0; i < m_ui.table->rowCount(); i++)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

      if(downloads_item)
	downloads_item->cancel();
    }
}

void dooble_downloads::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
  save_settings();
}

void dooble_downloads::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlDatabase query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_downloads ("
	     "download_path TEXT NOT NULL, "
	     "file_name TEXT NOT NULL, "
	     "information TEXT NOT NULL, "

	     /*
	     ** For ordering.
	     */

	     "insert_order INTEGER PRIMARY KEY AUTOINCREMENT, "
	     "url TEXT NOT NULL, "
	     "url_digest TEXT NOT NULL)");
}

void dooble_downloads::delete_selected(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedIndexes());

  if(!list.isEmpty())
    {
      QApplication::restoreOverrideCursor();

      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected item(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

  for(int i = list.size() - 1; i >= 0; i--)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(list.at(i).row(), 0));

      if(!downloads_item)
	{
	  m_ui.table->removeRow(list.at(i).row());
	  continue;
	}
      else if(!downloads_item->is_finished())
	continue;

      remove_entry(downloads_item->oid());
      m_ui.table->removeRow(list.at(i).row());
    }

  QApplication::restoreOverrideCursor();
}

void dooble_downloads::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Delete)
	delete_selected();
      else if(event->key() == Qt::Key_Escape)
	{
	  if(parent())
	    {
	      event->ignore();
	      return;
	    }
	  else
	    close();
	}
    }

  QMainWindow::keyPressEvent(event);
}

void dooble_downloads::purge(void)
{
  abort();
  m_ui.table->setRowCount(0);

  QString database_name("dooble_downloads");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_downloads");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_downloads::record_download(QWebEngineDownloadItem *download)
{
  if(!download)
    return;
  else if(m_downloads.contains(download))
    return;
  else
    m_downloads[download] = 0;

  connect(download,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(slot_download_destroyed(void)));

  dooble_downloads_item *item = nullptr;
  int index = -1;

  for(int i = m_ui.table->rowCount() - 1; i >= 0; i--)
    {
      dooble_downloads_item *downloads_item =
	qobject_cast<dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

      if(!downloads_item)
	{
	  m_ui.table->removeRow(i);
	  continue;
	}

      if(download->path() == downloads_item->download_path())
	{
	  index = i;
	  item = downloads_item;
	  break;
	}
    }

  dooble_downloads_item *downloads_item = new dooble_downloads_item
    (download, index, this);

  connect(downloads_item,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_download_finished(void)));
  connect(downloads_item,
	  SIGNAL(reload(const QString &, const QUrl &)),
	  this,
	  SLOT(slot_reload(const QString &, const QUrl &)));

  if(index == -1)
    {
      m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
      m_ui.table->setCellWidget(m_ui.table->rowCount() - 1, 0, downloads_item);
      m_ui.table->resizeRowToContents(m_ui.table->rowCount() - 1);
      m_ui.table->setRowHeight(m_ui.table->rowCount(), 100);
      m_ui.table->scrollToBottom();
    }
  else
    {
      m_ui.table->setCellWidget(index, 0, downloads_item);
      item->deleteLater();
    }

  emit started();
}

void dooble_downloads::remove_entry(qintptr oid)
{
  QString database_name("dooble_downloads");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare("DELETE FROM dooble_downloads WHERE OID = ?");
	query.addBindValue(oid);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_downloads::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_downloads::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("downloads_geometry", saveGeometry().toBase64());
}

void dooble_downloads::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::setting("downloads_geometry").
			      toByteArray()));

  QMainWindow::show();
}

void dooble_downloads::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::setting("downloads_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
}

void dooble_downloads::slot_clear_finished_downloads(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_ui.table->rowCount() > 0)
    {
      QApplication::restoreOverrideCursor();

      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected item(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

  for(int i = m_ui.table->rowCount() - 1; i >= 0; i--)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

      if(!downloads_item)
	{
	  m_ui.table->removeRow(i);
	  continue;
	}
      else if(!downloads_item->is_finished())
	continue;

      remove_entry(downloads_item->oid());
      m_ui.table->removeRow(i);
    }

  QApplication::restoreOverrideCursor();
}

void dooble_downloads::slot_copy_download_location(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QClipboard *clipboard = QApplication::clipboard();

  if(clipboard)
    clipboard->setText(action->property("url").toUrl().toString());
}

void dooble_downloads::slot_delete_row(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  dooble_downloads_item *downloads_item = qobject_cast<dooble_downloads_item *>
    (m_ui.table->cellWidget(action->property("row").toInt(), 0));

  if(downloads_item && downloads_item->is_finished())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected entry?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
      remove_entry(downloads_item->oid());
      m_ui.table->removeRow(action->property("row").toInt());
    }
}

void dooble_downloads::slot_download_destroyed(void)
{
  m_downloads.remove(sender());
}

void dooble_downloads::slot_download_path_inspection_timer_timeout(void)
{
  QFileInfo file_info(m_ui.download_path->text());
  QPalette palette(m_ui.download_path->palette());
  static QPalette s_palette(m_ui.download_path->palette());

  if(file_info.isReadable() && file_info.isWritable())
    {
      m_ui.download_path->setToolTip(m_ui.download_path->text());
      palette = s_palette;
    }
  else
    {
      if(!file_info.isReadable() && !file_info.isWritable())
	m_ui.download_path->setToolTip
	  (tr("The path %1 is neither readable nor writable.").
	   arg(m_ui.download_path->text()));
      else if(!file_info.isReadable())
	m_ui.download_path->setToolTip
	  (tr("The path %1 is not readable.").arg(m_ui.download_path->text()));
      else
	m_ui.download_path->setToolTip
	  (tr("The path %1 is not writable.").arg(m_ui.download_path->text()));

      palette.setColor
	(m_ui.download_path->backgroundRole(), QColor(240, 128, 128));
    }

  m_ui.download_path->setPalette(palette);
}

void dooble_downloads::slot_download_finished(void)
{
  emit finished();
}

void dooble_downloads::slot_download_requested(QWebEngineDownloadItem *download)
{
  if(!download)
    return;
  else if(contains(download))
    {
      /*
      ** Do not cancel the download.
      */

      return;
    }

  if(download->state() == QWebEngineDownloadItem::DownloadRequested)
    {
      QFileInfo file_info(download->path());

      download->setPath
	(download_path() + QDir::separator() + file_info.fileName());
    }

  record_download(download);
  download->accept();
}

void dooble_downloads::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_downloads::slot_open_download_page(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QUrl url(action->property("url").toUrl().adjusted(QUrl::RemoveFilename));

  if(url.isEmpty() || !url.isValid())
    return;

  /*
  ** Locate a Dooble window.
  */

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(this,
	     SIGNAL(open_link(const QUrl &)));

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; i < list.size(); i++)
    if(qobject_cast<dooble *> (list.at(i)) &&
       qobject_cast<dooble *> (list.at(i))->isVisible())
      {
	connect(this,
		SIGNAL(open_link(const QUrl &)),
		list.at(i),
		SLOT(slot_open_link(const QUrl &)),
		Qt::UniqueConnection);
	break;
      }

  QApplication::restoreOverrideCursor();
  emit open_link(url);
}

void dooble_downloads::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      m_downloads.clear();
      m_ui.table->setRowCount(0);
      emit populated();
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_downloads");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_downloads.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);
	int row = 0;
	int total_rows = 0;

	query.setForwardOnly(true);

	if(query.exec("SELECT COUNT(*) FROM dooble_downloads"))
	  if(query.next())
	    m_ui.table->setRowCount(query.value(0).toInt());

	if(query.exec("SELECT download_path, file_name, information, url, OID "
		      "FROM dooble_downloads ORDER BY insert_order"))
	  while(query.next() && total_rows < m_ui.table->rowCount())
	    {
	      QSqlRecord record(query.record());
	      QString download_path("");
	      QString file_name("");
	      QString information("");
	      QUrl url;
	      qintptr oid = -1;

	      for(int i = 0; i < record.count(); i++)
		switch(i)
		  {
		  case 0:
		  case 1:
		  case 2:
		  case 3:
		    {
		      QByteArray bytes
			(QByteArray::fromBase64(query.value(i).toByteArray()));

		      bytes = dooble::s_cryptography->mac_then_decrypt(bytes);

		      if(i == 0)
			download_path = QString::fromUtf8(bytes);
		      else if(i == 1)
			file_name = QString::fromUtf8(bytes);
		      else if(i == 2)
			information = QString::fromUtf8(bytes);
		      else
			url = QUrl::fromEncoded(bytes);

		      break;
		    }
		  default:
		    {
		      if(sizeof(qintptr) == 4)
			oid = query.value(i).toInt();
		      else
			oid = query.value(i).toLongLong();

		      break;
		    }
		  }

	      if(download_path.isEmpty() ||
		 file_name.isEmpty() ||
		 information.isEmpty() ||
		 url.isEmpty() ||
		 !url.isValid())
		{
		  QSqlQuery delete_query(db);

		  delete_query.prepare("DELETE FROM dooble_downloads "
				       "WHERE OID = ?");
		  delete_query.addBindValue(query.value(record.count() - 1));
		  delete_query.exec();
		  continue;
		}

	      dooble_downloads_item *downloads_item = new dooble_downloads_item
		(download_path, file_name, information, url, oid, this);

	      connect(downloads_item,
		      SIGNAL(finished(void)),
		      this,
		      SLOT(slot_download_finished(void)));
	      connect(downloads_item,
		      SIGNAL(reload(const QString &, const QUrl &)),
		      this,
		      SLOT(slot_reload(const QString &, const QUrl &)));
	      m_ui.table->setCellWidget(row, 0, downloads_item);
	      m_ui.table->resizeRowToContents(row);
	      m_ui.table->setRowHeight(row, 100);
	      row += 1;
	      total_rows += 1;
	    }

	m_ui.table->setRowCount(total_rows);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
  emit populated();
}

void dooble_downloads::slot_reload(const QString &file_name, const QUrl &url)
{
  foreach(dooble_downloads_item *item, findChildren<dooble_downloads_item *> ())
    if(item)
      if(item->url() == url)
	{
	  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	  QWidgetList list(QApplication::topLevelWidgets());

	  for(int i = 0; i < list.size(); i++)
	    if(qobject_cast<dooble *> (list.at(i)))
	      {
		dooble *d = qobject_cast<dooble *> (list.at(i));

		foreach(dooble_page *page, d->findChildren<dooble_page *> ())
		  if(page)
		    {
		      page->download(file_name, url);
		      goto done_label;
		    }
	      }

	done_label:
	  QApplication::restoreOverrideCursor();
	  break;
	}
}

void dooble_downloads::slot_search_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	dooble_downloads_item *downloads_item = qobject_cast
	  <dooble_downloads_item *> (m_ui.table->cellWidget(i, 0));

	if(!downloads_item)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	if(downloads_item->url().toString().toLower().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);
      }

  QApplication::restoreOverrideCursor();
}

void dooble_downloads::slot_select_path(void)
{
  QFileDialog dialog(this);

  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setWindowTitle(tr("Dooble: Select Download Path"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      dooble_settings::set_setting
	("download_path", dialog.selectedFiles().value(0));
      m_ui.download_path->setText(dialog.selectedFiles().value(0));
      m_ui.download_path->setToolTip(m_ui.download_path->text());
      m_ui.download_path->setCursorPosition(0);
    }

  QApplication::processEvents();
}

void dooble_downloads::slot_show_context_menu(const QPoint &point)
{
  int row = m_ui.table->rowAt(point.y());

  if(row < 0)
    return;

  dooble_downloads_item *downloads_item = qobject_cast
    <dooble_downloads_item *> (m_ui.table->cellWidget(row, 0));

  if(!downloads_item)
    return;

  QAction *action = nullptr;
  QMenu menu(this);
  QUrl url(downloads_item->url());

  action = menu.addAction
    (tr("&Copy Download Location"),
     this,
     SLOT(slot_copy_download_location(void)));
  action->setEnabled(!url.isEmpty() && url.isValid());
  action->setProperty("url", url);
  action = menu.addAction
    (tr("&Open Download Page"),
     this,
     SLOT(slot_open_download_page(void)));
  action->setEnabled(!url.isEmpty() && url.isValid());
  action->setProperty("url", url);
  menu.addSeparator();
  action = menu.addAction(tr("&Delete"), this, SLOT(slot_delete_row(void)));
  action->setEnabled(downloads_item->is_finished());
  action->setProperty("row", row);
  menu.exec(mapToGlobal(point));
}
