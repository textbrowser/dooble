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
#include <QSqlQuery>
#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_downloads.h"
#include "dooble_downloads_item.h"
#include "dooble_ui_utilities.h"

dooble_downloads_item::dooble_downloads_item
(QWebEngineDownloadItem *download,
 const bool is_private,
 qintptr oid,
 QWidget *parent):QWidget(parent)
{
  m_download = download;
  m_is_private = is_private;
  m_last_bytes_received = 0;
  m_last_time = QTime::currentTime();
  m_oid = oid;
  m_rate = 0;
  m_stalled_timer.setInterval(15000);
  m_url = QUrl();
  m_ui.setupUi(this);
  m_ui.progress->setMaximum(0);
  m_ui.progress->setMinimum(0);
  connect(&m_stalled_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_stalled(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.cancel,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_cancel(void)));
  connect(m_ui.pause_resume,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_pause_or_resume(void)));

  if(m_download)
    {
      m_profile = qobject_cast<QWebEngineProfile *> (m_download->parent());
      m_download->setParent(this);
      connect(m_download,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(slot_finished(void)));
      connect(m_download,
	      SIGNAL(downloadProgress(qint64, qint64)),
	      this,
	      SLOT(slot_download_progress(qint64, qint64)));
      connect(m_download,
	      SIGNAL(finished(void)),
	      this,
	      SLOT(slot_finished(void)));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
      QFileInfo file_info(m_download->downloadDirectory() +
			  QDir::separator() +
			  m_download->downloadFileName());
#else
      QFileInfo file_info(m_download->path());
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
      m_download_path = m_download->downloadDirectory() +
	QDir::separator() +
	m_download->downloadFileName();
#else
      m_download_path = m_download->path();
#endif
      m_file_name = file_info.fileName();
      m_stalled_timer.start();
      m_ui.file_name->setText(file_info.fileName());
      m_ui.progress->setMaximum(100);
      m_url = m_download->url();

      if(m_oid == -1)
	/*
	** New record.
	*/

	record();
    }
  else
    {
      m_ui.cancel->setVisible(false);
      m_ui.file_name->setVisible(false);
      m_ui.information->setText(tr("The download object is zero. Error!"));
      m_ui.pause_resume->setVisible(false);
      m_ui.progress->setVisible(false);
    }

  m_progress_bar_animation.setDuration(1000);
  m_progress_bar_animation.setEasingCurve(QEasingCurve::OutCubic);
  m_progress_bar_animation.setLoopCount(1);
  m_progress_bar_animation.setPropertyName("value");
  m_progress_bar_animation.setStartValue(0);
  m_progress_bar_animation.setTargetObject(m_ui.progress);
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
  m_ui.pause_resume->setVisible(false);
#endif
#ifdef Q_OS_MACOS
  m_ui.cancel->setStyleSheet("QToolButton {border: none;}"
			     "QToolButton::menu-button {border: none;}");
  m_ui.pause_resume->setStyleSheet("QToolButton {border: none;}"
				   "QToolButton::menu-button {border: none;}");
#endif
  prepare_icons();
}

dooble_downloads_item::dooble_downloads_item(const QString &download_path,
					     const QString &file_name,
					     const QString &information,
					     const QUrl &url,
					     qintptr oid,
					     QWidget *parent):QWidget(parent)
{
  m_download_path = download_path;
  m_file_name = file_name;
  m_is_private = false;
  m_last_bytes_received = 0;
  m_oid = oid;
  m_rate = 0;
  m_stalled_timer.setInterval(15000);
  m_url = url;
  m_ui.setupUi(this);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  connect(m_ui.cancel,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reload(void)));

  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  m_ui.cancel->setIcon
    (QIcon::fromTheme(use_material_icons + "view-refresh",
		      QIcon(QString(":/%1/20/reload.png").arg(icon_set))));
#ifdef Q_OS_MACOS
  m_ui.cancel->setStyleSheet("QToolButton {border: none;}"
			     "QToolButton::menu-button {border: none;}");
#endif
  m_ui.cancel->setToolTip(tr("Restart"));
  m_ui.cancel->setVisible(true);
#else
  m_ui.cancel->setVisible(false);
#endif
  m_ui.file_name->setText(file_name);
  m_ui.information->setText(information);
  m_ui.pause_resume->setVisible(false);
  m_ui.progress->setVisible(false);
  connect(&m_stalled_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_stalled(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  m_progress_bar_animation.setDuration(1000);
  m_progress_bar_animation.setEasingCurve(QEasingCurve::OutCubic);
  m_progress_bar_animation.setLoopCount(1);
  m_progress_bar_animation.setPropertyName("value");
  m_progress_bar_animation.setStartValue(0);
  m_progress_bar_animation.setTargetObject(m_ui.progress);
}

dooble_downloads_item::~dooble_downloads_item()
{
  if(m_download)
    m_download->cancel();
}

QPointer<QWebEngineProfile> dooble_downloads_item::profile(void) const
{
  return m_profile;
}

QString dooble_downloads_item::download_path(void) const
{
  return m_download_path;
}

QUrl dooble_downloads_item::url(void) const
{
  return m_url;
}

bool dooble_downloads_item::is_finished(void) const
{
  if(m_download)
    return m_download->isFinished();
  else
    return true;
}

qintptr dooble_downloads_item::oid(void) const
{
  return m_oid;
}

void dooble_downloads_item::cancel(void)
{
  if(m_download)
    m_download->cancel();

  m_stalled_timer.stop();
}

void dooble_downloads_item::prepare_icons(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  m_ui.cancel->setIcon
    (QIcon::fromTheme(use_material_icons + "media-playback-stop",
		      QIcon(QString(":/%1/20/stop.png").arg(icon_set))));
  m_ui.pause_resume->setIcon
    (QIcon::fromTheme(use_material_icons + "media-playback-start",
		      QIcon(QString(":/%1/20/resume.png").arg(icon_set))));
}

void dooble_downloads_item::record(void)
{
  if(!dooble::s_cryptography ||
     !dooble::s_cryptography->authenticated() ||
     m_is_private)
    return;

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_downloads.db");

    if(db.open())
      {
	dooble_downloads::create_tables(db);

	QSqlQuery query(db);

	/*
	** Recreate dooble_downloads if necessary.
	*/

	if(query.exec("PRAGMA TABLE_INFO(dooble_downloads)"))
	  {
	    auto exists = false;

	    while(query.next())
	      if(query.value(1).toString().toLower() == "download_path")
		{
		  exists = true;
		  break;
		}

	    if(!exists)
	      query.exec("DROP TABLE dooble_downloads");
	  }

	QByteArray bytes;
	auto file_name(m_ui.file_name->text());
	auto information(m_ui.information->text());

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_downloads "
	   "(download_path, file_name, information, url, url_digest) "
	   "VALUES (?, ?, ?, ?, ?)");
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_download_path.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac(file_name.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	if(information.isEmpty())
	  information = file_name;

	bytes = dooble::s_cryptography->encrypt_then_mac(information.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac(m_url.toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(m_url.toEncoded()).toBase64());

	if(query.exec())
	  m_oid = query.lastInsertId().isValid() ?
	    query.lastInsertId().toLongLong() : -1;
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_downloads_item::record_information(void)
{
  if(!dooble::s_cryptography ||
     !dooble::s_cryptography->authenticated() ||
     m_is_private)
    return;

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_downloads.db");

    if(db.open())
      {
	QByteArray bytes;
	QSqlQuery query(db);
	auto information(m_ui.information->text());

	query.prepare
	  ("UPDATE dooble_downloads SET information = ? WHERE OID = ?");
	bytes = dooble::s_cryptography->encrypt_then_mac(information.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue(m_oid);
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_downloads_item::slot_cancel(void)
{
  if(m_download)
    m_download->cancel();

  m_stalled_timer.stop();
}

void dooble_downloads_item::slot_download_progress(qint64 bytes_received,
						   qint64 bytes_total)
{
  m_stalled_timer.start();

  int seconds = 0;

  if((seconds = qAbs(m_last_time.secsTo(QTime::currentTime()))) >= 1)
    {
      if(bytes_received > m_last_bytes_received)
	if(200.0 * qAbs(static_cast<double> (m_rate) -
			static_cast<double> (bytes_received -
					     m_last_bytes_received) /
			static_cast<double> (seconds))
	   / qMax(static_cast<double> (1),
		  static_cast<double> (m_rate) +
		  static_cast<double> (bytes_received - m_last_bytes_received) /
		  static_cast<double> (seconds)) >= 1.0)
	  m_rate = static_cast<qint64>
	    (static_cast<double> (bytes_received - m_last_bytes_received) /
	     static_cast<double> (seconds));

      m_last_bytes_received = bytes_received;
      m_last_time = QTime::currentTime();
    }

  auto paused = false;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  if(m_download)
    if(m_download->isPaused())
      paused = true;
#endif
#endif

  if(bytes_total > 0)
    {
      if(paused)
	m_ui.information->setText
	  (tr("%1 of %2 - Paused").
	   arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)).
	   arg(dooble_ui_utilities::pretty_size(bytes_total)));
      else
	m_ui.information->setText
	  (tr("%1 of %2 - %3 / second").
	   arg(dooble_ui_utilities::pretty_size(bytes_received)).
	   arg(dooble_ui_utilities::pretty_size(bytes_total)).
	   arg(dooble_ui_utilities::pretty_size(m_rate)));

      m_ui.progress->setMaximum(100);
      m_progress_bar_animation.setEndValue
	(static_cast<int> (100 * (static_cast<double> (bytes_received) /
				  static_cast<double> (bytes_total))));
      m_progress_bar_animation.setStartValue(m_ui.progress->value());

      if(m_progress_bar_animation.state() == QAbstractAnimation::Stopped)
	m_progress_bar_animation.start();
    }
  else
    {
      if(paused)
	m_ui.information->setText
	  (tr("%1 of Unknown - Stalled").
	   arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)));
      else
	m_ui.information->setText
	  (tr("%1 of Unknown - %2 / second").
	   arg(dooble_ui_utilities::pretty_size(bytes_received)).
	   arg(dooble_ui_utilities::pretty_size(m_rate)));

      m_ui.progress->setMaximum(0);
    }
}

void dooble_downloads_item::slot_finished(void)
{
  m_stalled_timer.stop();
  m_ui.cancel->setVisible(false);
  m_ui.pause_resume->setVisible(false);
  m_ui.progress->setVisible(false);

  if(m_download)
    {
      if(m_download->state() == QWebEngineDownloadItem::DownloadCancelled)
	m_ui.information->setText
	  (tr("Canceled - %1").arg(m_download->url().host()));
      else if(m_download->state() == QWebEngineDownloadItem::DownloadCompleted)
	m_ui.information->setText
	  (tr("Completed - %1 - %2").
	   arg(m_download->url().host()).
	   arg(dooble_ui_utilities::pretty_size(m_download->totalBytes())));
      else if(m_download->state() ==
	      QWebEngineDownloadItem::DownloadInterrupted)
	m_ui.information->setText
	  (tr("Interrupted - %1 - %2").
	   arg(m_download->url().host()).
	   arg(dooble_ui_utilities::pretty_size(m_download->receivedBytes())));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
      connect(m_ui.cancel,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slot_reload(void)),
	      Qt::UniqueConnection);

      auto icon_set(dooble_settings::setting("icon_set").toString());
      auto use_material_icons(dooble_settings::use_material_icons());

      m_ui.cancel->setIcon
	(QIcon::fromTheme(use_material_icons + "view-refresh",
			  QIcon(QString(":/%1/20/reload.png").arg(icon_set))));
      m_ui.cancel->setToolTip(tr("Restart"));
      m_ui.cancel->setVisible(true);
#endif
    }
  else
    m_ui.information->setText(tr("Interrupted"));

  record_information();
  emit finished();
}

void dooble_downloads_item::slot_pause_or_resume(void)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  if(m_download)
    {
      if(m_download->isPaused())
	{
	  m_download->resume();
	  m_stalled_timer.start();
	}
      else
	m_download->pause();

      auto icon_set(dooble_settings::setting("icon_set").toString());
      auto use_material_icons(dooble_settings::use_material_icons());

      if(m_download->isPaused())
	{
	  m_ui.pause_resume->setIcon
	    (QIcon::fromTheme(use_material_icons + "media-playback-pause",
			      QIcon(QString(":/%1/20/pause.png").
				    arg(icon_set))));
	  m_ui.pause_resume->setToolTip(tr("Resume"));

	  if(m_download->totalBytes() > 0)
	    m_ui.information->setText
	      (tr("%1 of %2 - Paused").
	       arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)).
	       arg(dooble_ui_utilities::pretty_size(m_download->totalBytes())));
	  else
	    m_ui.information->setText
	      (tr("%1 of Unknown - Paused").
	       arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)));
	}
      else
	{
	  m_ui.pause_resume->setIcon
	    (QIcon::fromTheme(use_material_icons + "media-playback-start",
			      QIcon(QString(":/%1/20/resume.png").
				    arg(icon_set))));
	  m_ui.pause_resume->setToolTip(tr("Pause"));
	}
    }
#endif
#endif
}

void dooble_downloads_item::slot_reload(void)
{
  emit reload(m_file_name, m_url);
}

void dooble_downloads_item::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_downloads_item::slot_stalled(void)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  if(m_download)
    if(m_download->isPaused())
      return;
#endif
#endif

  if(m_download && m_download->totalBytes() > 0)
    m_ui.information->setText
      (tr("%1 of %2 - Stalled").
       arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)).
       arg(dooble_ui_utilities::pretty_size(m_download->totalBytes())));
  else
    m_ui.information->setText
      (tr("%1 of Unknown - Stalled").
       arg(dooble_ui_utilities::pretty_size(m_last_bytes_received)));
}
