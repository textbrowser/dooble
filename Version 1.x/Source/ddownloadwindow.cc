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

#include <QAuthenticator>
#include <QDateTime>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProgressBar>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlIndex>
#include <QSqlQuery>

#include "ddownloadprompt.h"
#include "ddownloadwindow.h"
#include "dmisc.h"
#include "dooble.h"
#include "ui_passwordPrompt.h"

ddownloadwindow::ddownloadwindow(void):QMainWindow()
{
  ui.setupUi(this);
  ui.urlLineEdit->setMaxLength(2500);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  ui.searchLineEdit->setPlaceholderText(tr("Search Downloads"));
  connect(ui.closePushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotClose(void)));
  connect(ui.clearListPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotClearList(void)));
  connect(ui.enterUrlPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotEnterUrl(void)));
  connect(ui.cancelPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotCancelDownloadUrl(void)));
  connect(ui.downloadPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotDownloadUrl(void)));
  connect(ui.clearPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotClear(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.table,
	  SIGNAL(cellDoubleClicked(int, int)),
	  this,
	  SLOT(slotCellDoubleClicked(int, int)));
  connect(ui.bitRateCheckBox,
	  SIGNAL(stateChanged(int)),
	  this,
	  SLOT(slotBitRateChanged(int)));
  slotSetIcons();
  createDownloadDatabase();
}

ddownloadwindow::~ddownloadwindow()
{
  saveState();
}

void ddownloadwindow::purge(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "downloads");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM downloads");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("downloads");
}

void ddownloadwindow::populate(void)
{
  for(int i = ui.table->rowCount() - 1; i >= 0; i--)
    {
      ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
	(ui.table->cellWidget(i, 0));

      if(item && !item->isDownloading())
	{
	  /*
	  ** If the user authenticates after a download has been
	  ** initiated for a session-based process, they may
	  ** wish to continue the download.
	  */

	  item->deleteLater();
	  ui.table->removeRow(i);
	}
    }

  if(!dmisc::passphraseWasAuthenticated())
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "downloads");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT filename, url, "
		      "download_started_date FROM downloads"))
	  while(query.next())
	    {
	      QUrl url;
	      QString fileName("");
	      QDateTime dateTime;
	      bool ok = true;

	      url = QUrl::fromEncoded
		(dmisc::daa
		 (QByteArray::fromBase64(query.value(1).toByteArray()), &ok),
		 QUrl::StrictMode);

	      if(ok)
		fileName = QString::fromUtf8
		  (dmisc::daa
		   (QByteArray::fromBase64(query.value(0).toByteArray()),
		    &ok));

	      if(ok)
		dateTime = QDateTime::fromString
		  (QString::fromUtf8
		   (dmisc::daa
		    (QByteArray::fromBase64
		     (query.value(2).toByteArray()), &ok)),
		   Qt::ISODate);

	      if(!ok)
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare("DELETE FROM downloads WHERE "
				      "filename = ?");
		  deleteQuery.bindValue(0, query.value(0));
		  deleteQuery.exec();
		  continue;
		}

	      /*
	      ** These if-statements should take care of
	      ** decoding mishaps.
	      */

	      if(QFileInfo(url.toLocalFile()).exists())
		addFileItem(url.toLocalFile(), fileName, false, dateTime);
	      else if(url.isValid() &&
		      dmisc::isSchemeAcceptedByDooble(url.scheme()))
		addUrlItem(url, fileName, false, dateTime, 0);
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("downloads");
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void ddownloadwindow::addFileItem(const QString &srcFileName,
				  const QString &dstFileName,
				  const bool isNew,
				  const QDateTime &dateTime)
{
  ui.stackedWidget->setCurrentIndex(0);

  ddownloadwindowitem *item = new ddownloadwindowitem(ui.table);

  connect(this,
	  SIGNAL(iconsChanged(void)),
	  item,
	  SLOT(slotSetIcons(void)));
  connect(item,
	  SIGNAL(recordDownload(const QString &,
				const QUrl &,
				const QDateTime &)),
	  this,
	  SLOT(slotRecordDownload(const QString &,
				  const QUrl &,
				  const QDateTime &)));
  connect(item,
	  SIGNAL(downloadFinished(void)),
	  this,
	  SLOT(slotDownloadFinished(void)));
  item->downloadFile(srcFileName, dstFileName, isNew, dateTime);
  ui.table->insertRow(0);
  ui.table->setCellWidget(0, 0, item);
  ui.table->resizeRowToContents(0);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void ddownloadwindow::addUrlItem(const QUrl &url, const QString &dstFileName,
				 const bool isNew,
				 const QDateTime &dateTime,
				 const int choice)
{
  ui.stackedWidget->setCurrentIndex(0);

  ddownloadwindowitem *item = new ddownloadwindowitem(ui.table);

  connect(this,
	  SIGNAL(iconsChanged(void)),
	  item,
	  SLOT(slotSetIcons(void)));
  connect(item,
	  SIGNAL(recordDownload(const QString &,
				const QUrl &,
				const QDateTime &)),
	  this,
	  SLOT(slotRecordDownload(const QString &,
				  const QUrl &,
				  const QDateTime &)));
  connect(item,
	  SIGNAL(downloadFinished(void)),
	  this,
	  SLOT(slotDownloadFinished(void)));
  item->downloadUrl(url, dstFileName, isNew, dateTime,
		    choice);
  connect(item,
	  SIGNAL(authenticationRequired(QNetworkReply *,
					QAuthenticator *)),
	  this,
	  SLOT(slotAuthenticationRequired(QNetworkReply *,
					  QAuthenticator *)));
  connect(item,
	  SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &,
					     QAuthenticator *)),
	  this,
	  SLOT(slotProxyAuthenticationRequired(const QNetworkProxy &,
					       QAuthenticator *)));
  ui.table->insertRow(0);
  ui.table->setCellWidget(0, 0, item);
  ui.table->resizeRowToContents(0);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void ddownloadwindow::addHtmlItem(const QString &html,
				  const QString &dstFileName)
{
  ui.stackedWidget->setCurrentIndex(0);

  ddownloadwindowitem *item = new ddownloadwindowitem(ui.table);

  connect(this,
	  SIGNAL(iconsChanged(void)),
	  item,
	  SLOT(slotSetIcons(void)));
  connect(item,
	  SIGNAL(recordDownload(const QString &,
				const QUrl &,
				const QDateTime &)),
	  this,
	  SLOT(slotRecordDownload(const QString &,
				  const QUrl &,
				  const QDateTime &)));
  item->downloadHtml(html, dstFileName);
  ui.table->insertRow(0);
  ui.table->setCellWidget(0, 0, item);
  ui.table->resizeRowToContents(0);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void ddownloadwindow::slotClose(void)
{
  close();
}

void ddownloadwindow::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void ddownloadwindow::show(QWidget *parent)
{
  ui.bitRateCheckBox->setChecked
    (dooble::s_settings.value("downloadWindow/showDownloadRateInBits",
			      false).toBool());

  QRect rect(100, 100, 800, 600);

  if(parent)
    {
      rect = parent->geometry();
      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }

  if(!isVisible())
    {
      /*
      ** Don't annoy the user.
      */

      if(dooble::s_settings.contains("downloadWindow/geometry"))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.
			value("downloadWindow/geometry",
			      rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.
			   value("downloadWindow/geometry").
			   toByteArray());

	      if(!restoreGeometry(g))
		setGeometry(rect);
	    }
	}
      else
	setGeometry(rect);
    }

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, parent);

  showNormal();
  raise();

  if(ui.stackedWidget->currentIndex() == 1)
    ui.urlLineEdit->setFocus();
  else
    ui.closePushButton->setFocus();
}

bool ddownloadwindow::isActive(void) const
{
  for(int i = ui.table->rowCount() - 1; i >= 0; i--)
    {
      ddownloadwindowitem *item =
	qobject_cast<ddownloadwindowitem *> (ui.table->cellWidget(i, 0));

      if(item && item->isDownloading())
	return true;
    }

  return false;
}

void ddownloadwindow::slotClearList(void)
{
  if(dmisc::passphraseWasAuthenticated())
    {
      QStringList fileNames;

      for(int i = ui.table->rowCount() - 1; i >= 0; i--)
	if(!ui.table->isRowHidden(i))
	  {
	    ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
	      (ui.table->cellWidget(i, 0));

	    if(item && !item->isDownloading())
	      {
		fileNames.append(item->fileName());
		item->deleteLater();
		ui.table->removeRow(i);
	      }
	  }

      if(!fileNames.isEmpty())
	{
	  {
	    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
							"downloads");

	    db.setDatabaseName(dooble::s_homePath +
			       QDir::separator() + "downloads.db");

	    if(db.open())
	      while(!fileNames.isEmpty())
		{
		  QSqlQuery query(db);
		  QString fileName(fileNames.takeFirst());
		  bool ok = true;

		  query.exec("PRAGMA secure_delete = ON");
		  query.prepare("DELETE FROM downloads WHERE "
				"filename_hash = ?");
		  query.bindValue
		    (0,
		     dmisc::hashedString(fileName.toUtf8(),
					 &ok).toBase64());

		  if(ok)
		    query.exec();
		}

	    db.close();
	  }

	  QSqlDatabase::removeDatabase("downloads");
	}
    }
  else
    {
      for(int i = ui.table->rowCount() - 1; i >= 0; i--)
	if(!ui.table->isRowHidden(i))
	  {
	    ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
	      (ui.table->cellWidget(i, 0));

	    if(item && !item->isDownloading())
	      {
		item->deleteLater();
		ui.table->removeRow(i);
	      }
	  }
    }
}

void ddownloadwindow::slotEnterUrl(void)
{
  ui.stackedWidget->setCurrentIndex(1);
  ui.urlLineEdit->setFocus();
}

void ddownloadwindow::slotCancelDownloadUrl(void)
{
  ui.stackedWidget->setCurrentIndex(0);
}

void ddownloadwindow::slotDownloadUrl(void)
{
  QUrl url(dmisc::correctedUrlPath(QUrl::fromUserInput(ui.
						       urlLineEdit->
						       text().trimmed())));

  if(url.host().toLower().trimmed().startsWith("gopher"))
    url.setScheme("gopher");

  if(url.isValid())
    {
      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      emit saveUrl(url, 0);
    }
}

void ddownloadwindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("downloadWindow/windowIcon").toString()));
  ui.closePushButton->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  ui.cancelPushButton->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  ui.clearPushButton->setIcon
    (QIcon(settings.value("downloadWindow/clearListButtonIcon").toString()));
  ui.clearListPushButton->setIcon
    (QIcon(settings.value("downloadWindow/clearListButtonIcon").toString()));
  ui.downloadPushButton->setIcon
    (QIcon(settings.value("downloadWindow/downloadButtonIcon").toString()));
  ui.enterUrlPushButton->setIcon
    (QIcon(settings.value("downloadWindow/enterUrlButtonIcon").
	   toString()));
  emit iconsChanged();
}

void ddownloadwindow::slotClear(void)
{
  ui.urlLineEdit->clear();
  ui.urlLineEdit->setFocus();
}

void ddownloadwindow::createDownloadDatabase(void)
{
  if(!dooble::s_settings.value("settingsWindow/rememberDownloads",
			       true).toBool())
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "downloads");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS downloads ("
		   "filename TEXT NOT NULL, "
		   "filename_hash TEXT PRIMARY KEY NOT NULL, "
		   "url TEXT NOT NULL, "
		   "download_started_date TEXT NOT NULL DEFAULT '')");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("downloads");
}

void ddownloadwindow::slotRecordDownload(const QString &fileName,
					 const QUrl &url,
					 const QDateTime &dateTime)
{
  if(!dooble::s_settings.value("settingsWindow/rememberDownloads",
			       true).toBool())
    ui.table->scrollToTop();
  else if(dooble::s_settings.
	  value("settingsWindow/"
		"disableAllEncryptedDatabaseWrites", false).
	  toBool())
    return;

  if(!dmisc::passphraseWasAuthenticated())
    return;

  if(!dooble::s_settings.value("settingsWindow/rememberDownloads",
			       true).toBool())
    return;

  createDownloadDatabase();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "downloads");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "downloads.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.prepare
	  ("INSERT OR REPLACE INTO downloads "
	   "(filename, url, download_started_date, filename_hash) "
	   "VALUES (?, ?, ?, ?)");
	query.bindValue
	  (0,
	   dmisc::etm(fileName.toUtf8(),
		      true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (1,
	     dmisc::etm
	     (url.toEncoded(QUrl::StripTrailingSlash), true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (2,
	     dmisc::etm(dateTime.toString(Qt::ISODate).
			toUtf8(), true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (3,
	     dmisc::hashedString(fileName.toUtf8(),
				 &ok).toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("downloads");
}

void ddownloadwindow::saveState(void)
{
  /*
  ** geometry() may return (0, 0) coordinates if the window is
  ** not visible.
  */

  if(!isVisible())
    return;

  if(dmisc::isGnome())
    dooble::s_settings["downloadWindow/geometry"] = geometry();
  else
    dooble::s_settings["downloadWindow/geometry"] = saveGeometry();

  QSettings settings;

  if(dmisc::isGnome())
    settings.setValue("downloadWindow/geometry", geometry());
  else
    settings.setValue("downloadWindow/geometry", saveGeometry());
}

void ddownloadwindow::slotDownloadFinished(void)
{
  ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *> (sender());

  if(!item)
    return;

  if(item->choice() == 2)
    {
      /*
      ** Open the file, if we can.
      */

      QString action("");
      QString suffix(QFileInfo(item->fileName()).suffix().trimmed());

      if(dooble::s_applicationsActions.contains(suffix))
	{
	  action = dooble::s_applicationsActions[suffix];
	  dmisc::launchApplication(action, QStringList(item->fileName()));
	}
    }

  if(!dooble::s_settings.value("settingsWindow/closeDownloads",
			       false).toBool())
    return;

  if(item && item->abortedByUser())
    /*
    ** If the item that emitted the downloadFinished() signal did so because
    ** of user interaction, the Downloads window should not be closed.
    ** Note, if the user aborts a download and another download completes
    ** within some minute amount of time, the Downloads window will be closed.
    */

    return;

  foreach(ddownloadwindowitem *item, findChildren<ddownloadwindowitem *> ())
    if(item->isDownloading())
      return;

  close();
}

void ddownloadwindow::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();
  else if(event &&
	  event->key() == Qt::Key_F &&
	  event->modifiers() == Qt::ControlModifier)
    {
      ui.searchLineEdit->setFocus();
      ui.searchLineEdit->selectAll();
    }
  else if(event && (event->key() == Qt::Key_Backspace ||
		    event->key() == Qt::Key_Delete) &&
	  ui.stackedWidget->currentIndex() == 0 &&
	  ui.table->currentRow() > -1)
    {
      ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
	(ui.table->cellWidget(ui.table->currentRow(), 0));

      if(item && !item->isDownloading() &&
	 !ui.table->isRowHidden(ui.table->currentRow()))
	{
	  QString fileName(item->fileName());

	  ui.table->removeRow(ui.table->currentRow());
	  item->deleteLater();
	  ui.table->hide();
	  ui.table->show();

	  if(dmisc::passphraseWasAuthenticated())
	    {
	      {
		QSqlDatabase db = QSqlDatabase::addDatabase
		  ("QSQLITE", "downloads");

		db.setDatabaseName(dooble::s_homePath +
				   QDir::separator() + "downloads.db");

		if(db.open())
		  {
		    QSqlQuery query(db);
		    bool ok = true;

		    query.exec("PRAGMA secure_delete = ON");
		    query.prepare("DELETE FROM downloads WHERE "
				  "filename_hash = ?");
		    query.bindValue
		      (0,
		       dmisc::hashedString(fileName.toUtf8(),
					   &ok).toBase64());

		    if(ok)
		      query.exec();
		  }

		db.close();
	      }

	      QSqlDatabase::removeDatabase("downloads");
	    }
	}
    }

  QMainWindow::keyPressEvent(event);
}

void ddownloadwindow::slotTextChanged(const QString &text)
{
  /*
  ** Search text changed.
  */

  for(int i = 0; i < ui.table->rowCount(); i++)
    {
      ddownloadwindowitem *item =
	qobject_cast<ddownloadwindowitem *> (ui.table->cellWidget(i, 0));

      if(item)
	{
	  if(item->text().toLower().contains(text.toLower().trimmed()))
	    ui.table->showRow(i);
	  else
	    ui.table->hideRow(i);
	}
      else
	ui.table->hideRow(i);
    }
}

void ddownloadwindow::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;

  /*
  ** This method assumes that the respective container
  ** is as current as possible. It will use the container's
  ** contents to create new database.
  ** Obsolete data will be deleted via the purge method.
  */

  purge();

  if(progress)
    {
      progress->setMaximum(ui.table->rowCount());
      progress->setVisible(true);
      progress->update();
    }

  for(int i = ui.table->rowCount() - 1, j = 1; i >= 0; i--, j++)
    {
      ddownloadwindowitem *item =
	qobject_cast<ddownloadwindowitem *> (ui.table->cellWidget(i, 0));

      if(item)
	slotRecordDownload(item->fileName(),
			   item->url(),
			   item->dateTime());

      if(progress)
	progress->setValue(j);
    }

  if(progress)
    progress->setVisible(false);
}

void ddownloadwindow::slotAuthenticationRequired
(QNetworkReply *reply, QAuthenticator *authenticator)
{
  if(!reply || !authenticator)
    return;

  QDialog dialog(this);
  Ui_passwordDialog ui_p;

  ui_p.setupUi(&dialog);
#ifdef Q_OS_MAC
  dialog.setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050200
  ui_p.passwordLineEdit->setEchoMode(QLineEdit::NoEcho);
#endif
#endif
  ui_p.messageLabel->setText
    (QString(tr("The site %1 is requesting "
		"credentials.").
	     arg(reply->url().
		 toString(QUrl::RemovePath | QUrl::StripTrailingSlash))));

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  dialog.setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < ui_p.buttonBox->buttons().size(); i++)
    if(ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::AcceptRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::ApplyRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::YesRole)
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(true);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(false);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setUser(ui_p.usernameLineEdit->text());
      authenticator->setPassword(ui_p.passwordLineEdit->text());
    }
}

void ddownloadwindow::slotProxyAuthenticationRequired
(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
  if(!authenticator)
    return;

  Q_UNUSED(proxy);
  QDialog dialog(this);
  Ui_passwordDialog ui_p;

  ui_p.setupUi(&dialog);
#ifdef Q_OS_MAC
  dialog.setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050200
  ui_p.passwordLineEdit->setEchoMode(QLineEdit::NoEcho);
#endif
#endif
  ui_p.messageLabel->setText
    (QString(tr("The proxy %1:%2 is requesting "
		"credentials.").
	     arg(proxy.hostName()).
	     arg(proxy.port())));

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  dialog.setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < ui_p.buttonBox->buttons().size(); i++)
    if(ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::AcceptRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::ApplyRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::YesRole)
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(true);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(false);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setUser(ui_p.usernameLineEdit->text());
      authenticator->setPassword(ui_p.passwordLineEdit->text());
    }
}

void ddownloadwindow::slotCellDoubleClicked(int row, int col)
{
  ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
    (ui.table->cellWidget(row, col));

  if(item && !item->isDownloading())
    {
      QFileInfo fileInfo(item->fileName());

      if(!fileInfo.isReadable())
	{
	  QMessageBox mb(this);

#ifdef Q_OS_MAC
	  mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
	  mb.setIcon(QMessageBox::Information);
	  mb.setWindowTitle(tr("Dooble Web Browser: Information"));
	  mb.setStandardButtons(QMessageBox::Ok);
	  mb.setText(QString(tr("The file %1 is not accessible.").
			     arg(item->fileName())));

	  QSettings settings(dooble::s_settings.value("iconSet").toString(),
			     QSettings::IniFormat);

	  for(int i = 0; i < mb.buttons().size(); i++)
	    if(mb.buttonRole(mb.buttons().at(i)) == QMessageBox::AcceptRole ||
	       mb.buttonRole(mb.buttons().at(i)) == QMessageBox::ApplyRole ||
	       mb.buttonRole(mb.buttons().at(i)) == QMessageBox::YesRole)
	      {
		mb.buttons().at(i)->setIcon
		  (QIcon(settings.value("okButtonIcon").toString()));
		mb.buttons().at(i)->setIconSize(QSize(16, 16));
	      }

	  mb.setWindowIcon
	    (QIcon(settings.value("mainWindow/windowIcon").toString()));
	  mb.exec();
	}
      else
	{
	  ddownloadprompt dialog(this, item->fileName(),
				 ddownloadprompt::SingleChoice);

	  if(dialog.exec() != QDialog::Rejected)
	    {
	      QString action("");
	      QString suffix(QFileInfo(item->fileName()).suffix().trimmed());

	      if(dooble::s_applicationsActions.contains(suffix))
		{
		  action = dooble::s_applicationsActions[suffix];
		  dmisc::launchApplication
		    (action, QStringList(item->fileName()));
		}
	    }
	}
    }
}

void ddownloadwindow::clear(void)
{
  for(int i = ui.table->rowCount() - 1; i >= 0; i--)
    {
      ddownloadwindowitem *item = qobject_cast<ddownloadwindowitem *>
	(ui.table->cellWidget(i, 0));

      if(item && !item->isDownloading())
	{
	  item->deleteLater();
	  ui.table->removeRow(i);
	}
    }

  if(dmisc::passphraseWasAuthenticated())
    purge();
}

qint64 ddownloadwindow::size(void) const
{
  return QFileInfo(dooble::s_homePath + QDir::separator() + "downloads.db").
    size();
}

void ddownloadwindow::abort(void)
{
  foreach(ddownloadwindowitem *item, findChildren<ddownloadwindowitem *> ())
    item->abort();
}

void ddownloadwindow::slotBitRateChanged(int state)
{
  dooble::s_settings["downloadWindow/showDownloadRateInBits"] = state;

  QSettings settings;

  settings.setValue("downloadWindow/showDownloadRateInBits", state);
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool ddownloadwindow::event(QEvent *event)
{
  if(event)
    if(event->type() == QEvent::WindowStateChange)
      if(windowState() == Qt::WindowNoState)
	{
	  /*
	  ** Minimizing the window on OS 10.6.8 and Qt 5.x will cause
	  ** the window to become stale once it has resurfaced.
	  */

	  hide();
	  show(0);
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool ddownloadwindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool ddownloadwindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
