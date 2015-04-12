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

#include <limits>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>

#include "ddownloadwindowitem.h"
#include "dftp.h"
#include "dgopher.h"
#include "dmisc.h"
#include "dooble.h"

ddownloadwindowitem::ddownloadwindowitem(QWidget *parent):QWidget(parent)
{
  ui.setupUi(this);
  ui.fileHash->clear();
  m_choice = 0;
  m_networkAccessManager = new QNetworkAccessManager(this);
  m_networkAccessManager->setCookieJar(dooble::s_cookies);
  dooble::s_cookies->setParent(0);
  connect(ui.computeFileHash, SIGNAL(clicked(void)),
	  this, SLOT(slotComputeFileHash(void)));
  connect(ui.retryToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotDownloadAgain(void)));
  connect(ui.abortToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotAbortDownload(void)));
  connect(ui.pauseToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotPauseDownload(void)));
  connect(m_networkAccessManager,
	  SIGNAL(authenticationRequired(QNetworkReply *,
					QAuthenticator *)),
	  this,
	  SIGNAL(authenticationRequired(QNetworkReply *,
					QAuthenticator *)));
  connect(m_networkAccessManager,
	  SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &,
					     QAuthenticator *)),
	  this,
	  SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &,
					     QAuthenticator *)));
  init_ddownloadwindowitem();
#if QT_VERSION < 0x050000
  ui.computeFileHash->setToolTip(tr("Compute SHA-1 Hash"));
#else
  ui.computeFileHash->setToolTip(tr("Compute SHA-256 Hash"));
#endif
#ifdef Q_OS_MAC
  ui.abortToolButton->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}");
  ui.computeFileHash->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}");
  ui.retryToolButton->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}");
  ui.pauseToolButton->setStyleSheet
	("QToolButton {border: none;}"
	 "QToolButton::menu-button {border: none;}");
#endif
}

int ddownloadwindowitem::choice(void) const
{
  return m_choice;
}

void ddownloadwindowitem::init_ddownloadwindowitem(void)
{
  m_rate = 0;
  m_total = -1;
  m_paused = false;
  m_lastSize = 0;
  m_abortedByUser = false;
  ui.progressBar->setValue(0);
  ui.progressBar->setMinimum(0);
  ui.progressBar->setMaximum(100);
  ui.pauseToolButton->setChecked(false);
  ui.pauseToolButton->setToolTip(tr("Pause Download"));
  ui.retryToolButton->setVisible(false);

  if(dooble::s_settings.value("downloadWindow/showDownloadRateInBits",
			      false).toBool())
    ui.downloadInformationLabel->setText(tr("0 kbit/s (0 MiB)"));
  else
    ui.downloadInformationLabel->setText(tr("0 KiB/s (0 MiB)"));

  slotSetIcons();
}

void ddownloadwindowitem::downloadFile(const QString &srcFileName,
				       const QString &dstFileName,
				       const bool isNew,
				       const QDateTime &dateTime)
{
  m_url = QUrl::fromLocalFile(srcFileName);
  m_url = QUrl::fromEncoded(m_url.toEncoded(QUrl::StripTrailingSlash));
  m_dateTime = dateTime;
  m_srcFileName = srcFileName;
  m_dstFileName = dstFileName;
  ui.sourceLabel->setText(m_srcFileName);
  ui.downloadDateLabel->setText
    (m_dateTime.toString("MM/dd/yyyy hh:mm:ss AP"));

  if(QFileInfo(m_dstFileName).fileName().isEmpty())
    {
      if(m_dstFileName.isEmpty())
	ui.destinationLabel->setText("dooble.download");
      else
	ui.destinationLabel->setText(m_dstFileName);
    }
  else
    ui.destinationLabel->setText
      (QFileInfo(m_dstFileName).absoluteFilePath());

  ui.abortToolButton->setVisible(false);
  ui.pauseToolButton->setVisible(false);
  ui.progressBar->setVisible(false);

  if(!isNew)
    {
      /*
      ** The item is being created on behalf of the downloads.db
      ** database.
      */

      if(m_srcFileName != m_dstFileName)
	ui.retryToolButton->setVisible(true);

      ui.downloadInformationLabel->setText
	(dmisc::formattedSize(QFileInfo(m_dstFileName).size()));
      return;
    }

  /*
  ** Copy the local file.
  */

  QFile file(m_srcFileName);

  if(file.open(QIODevice::ReadOnly))
    {
      QFileInfo fileInfo(m_dstFileName);

      emit recordDownload(fileInfo.absoluteFilePath(), m_url, m_dateTime);
      QFile::remove(m_dstFileName);
      file.copy(m_dstFileName);
      file.close();
    }

  downloadFinished(file.size());
  ui.retryToolButton->setVisible(true);
  emit downloadFinished();
}

void ddownloadwindowitem::downloadUrl
(const QUrl &url,
 const QString &fileName,
 const bool isNew,
 const QDateTime &dateTime,
 const int choice)
{
  m_url = url;
  m_choice = choice;
  m_dateTime = dateTime;
  m_dstFileName = fileName;
  ui.sourceLabel->setText
    (m_url.toString(QUrl::StripTrailingSlash));
  ui.destinationLabel->setText
    (QFileInfo(m_dstFileName).absoluteFilePath());
  ui.downloadDateLabel->setText
    (m_dateTime.toString("MM/dd/yyyy hh:mm:ss AP"));

  if(!isNew)
    {
      /*
      ** The item is being created on behalf of the downloads.db
      ** database.
      */

      ui.abortToolButton->setVisible(false);
      ui.pauseToolButton->setVisible(false);
      ui.progressBar->setVisible(false);

      if(url.toString(QUrl::StripTrailingSlash) !=
	 QUrl::fromLocalFile(fileName).toString(QUrl::StripTrailingSlash))
	ui.retryToolButton->setVisible(true);

      ui.downloadInformationLabel->setText
	(dmisc::formattedSize(QFileInfo(m_dstFileName).size()));
      return;
    }

  ui.abortToolButton->setVisible(true);

  if(m_url.scheme().toLower().trimmed() != "gopher")
    ui.pauseToolButton->setVisible(true);
  else
    {
      ui.pauseToolButton->setVisible(false);
      ui.progressBar->setMaximum(0);
    }

  ui.progressBar->setVisible(true);

  QFile *file = new QFile(m_dstFileName, this);

  if(file->open(QIODevice::WriteOnly))
    {
      if(m_url.scheme().toLower().trimmed() == "ftp")
	{
	  dftp *ftp = new dftp(this);

	  connect(ftp, SIGNAL(readyRead(void)),
		  this, SLOT(slotReadyRead(void)));
	  connect(ftp, SIGNAL(downloadProgress(qint64, qint64)),
		  this, SLOT(slotDataTransferProgress(qint64, qint64)));
	  connect(ftp, SIGNAL(finished(void)),
		  this, SLOT(slotDownloadFinished(void)));
	  ftp->get(m_url);
	  emit recordDownload(QFileInfo(m_dstFileName).absoluteFilePath(),
			      m_url, m_dateTime);
	}
      else if(m_url.scheme().toLower().trimmed() == "gopher")
	{
	  dgopher *reply = new dgopher(this, QNetworkRequest(m_url));

	  reply->download();
	  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		  this, SLOT(slotError(QNetworkReply::NetworkError)));
	  connect(reply, SIGNAL(readyRead(void)),
		  this, SLOT(slotReadyRead(void)));
	  connect(reply, SIGNAL(metaDataChanged(void)),
		  this, SLOT(slotMetaDataChanged(void)));
	  connect(reply, SIGNAL(finished(void)),
		  this, SLOT(slotDownloadFinished(void)));
	  emit recordDownload(QFileInfo(m_dstFileName).absoluteFilePath(),
			      m_url, m_dateTime);
	}
      else if(m_url.scheme().toLower().trimmed() == "http" ||
	      m_url.scheme().toLower().trimmed() == "https")
	{
	  ui.pauseToolButton->setEnabled(false);

	  QNetworkReply *reply = 0;
	  QNetworkRequest request;

	  m_networkAccessManager->setProxy
	    (dmisc::proxyByFunctionAndUrl(DoobleDownloadType::Http, m_url));
	  request.setUrl(m_url);
	  request.setAttribute
	    (QNetworkRequest::HttpPipeliningAllowedAttribute, true);
#if QT_VERSION >= 0x050300
	  request.setAttribute
	    (QNetworkRequest::SpdyAllowedAttribute,
	     dooble::s_settings.value("settingsWindow/speedy", false).
	     toBool());
#endif
	  reply = m_networkAccessManager->get(request);
	  reply->setParent(this);
	  reply->ignoreSslErrors();
	  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		  this, SLOT(slotError(QNetworkReply::NetworkError)));
	  connect(reply, SIGNAL(readyRead(void)),
		  this, SLOT(slotReadyRead(void)));
	  connect(reply, SIGNAL(metaDataChanged(void)),
		  this, SLOT(slotMetaDataChanged(void)));
	  connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
		  this, SLOT(slotDataTransferProgress(qint64, qint64)));
	  connect(reply, SIGNAL(finished(void)),
		  this, SLOT(slotDownloadFinished(void)));
	  emit recordDownload(QFileInfo(m_dstFileName).absoluteFilePath(),
			      m_url, m_dateTime);
	}
      else
	file->deleteLater();
    }
  else
    file->deleteLater();
}

void ddownloadwindowitem::downloadHtml(const QString &html,
				       const QString &dstFileName)
{
  m_url = QUrl::fromLocalFile(dstFileName);
  m_url = QUrl::fromEncoded(m_url.toEncoded(QUrl::StripTrailingSlash));
  m_html = html;
  m_dateTime = QDateTime::currentDateTime();
  m_dstFileName = dstFileName;

  QFileInfo fileInfo(m_dstFileName);

  ui.sourceLabel->setVisible(false);
  ui.destinationLabel->setText(fileInfo.absoluteFilePath());
  ui.downloadDateLabel->setText
    (m_dateTime.toString("MM/dd/yyyy hh:mm:ss AP"));
  ui.abortToolButton->setVisible(false);
  ui.pauseToolButton->setVisible(false);
  ui.progressBar->setVisible(false);

  QFile file(dstFileName);

  if(file.open(QIODevice::WriteOnly | QFile::Truncate))
    {
      QTextStream stream(&file);

      stream << m_html;
      file.close();
    }

  downloadFinished(file.size());
  emit recordDownload(fileInfo.absoluteFilePath(), m_url, m_dateTime);
  emit downloadFinished();
}

ddownloadwindowitem::~ddownloadwindowitem()
{
}

void ddownloadwindowitem::slotDownloadFinished(void)
{
  QFile *file = findChild<QFile *> ();

  if(file)
    {
      file->close();
      downloadFinished(file->size());
      ui.progressBar->setVisible(false);
      ui.abortToolButton->setVisible(false);
      ui.pauseToolButton->setVisible(false);
      ui.retryToolButton->setVisible(true);
      file->deleteLater();
    }

  dftp *ftp = qobject_cast<dftp *> (sender());

  if(ftp)
    ftp->deleteLater();

  QNetworkReply *reply = qobject_cast<QNetworkReply *> (sender());

  if(reply)
    reply->deleteLater();

  emit downloadFinished();
}

void ddownloadwindowitem::slotDataTransferProgress(qint64 done, qint64 total)
{
  updateProgress(done, total);
}

void ddownloadwindowitem::updateProgress(const qint64 done,
					 const qint64 total)
{
  Q_UNUSED(done);

  if(m_total == -1)
    {
      m_total = total <= 0 ? 0 : total;
      m_lastTime = QTime::currentTime();

      if(!m_total)
	ui.progressBar->setMaximum(0);
      else
	ui.progressBar->setMaximum(100);
    }

  if(m_total)
    {
      QFileInfo fileInfo(m_dstFileName);
      int completed = static_cast<int>
	(100 * static_cast<double> (fileInfo.size()) /
	 qMax(1.0, static_cast<double> (m_total)));

      if(completed <= ui.progressBar->maximum())
	ui.progressBar->setValue(completed);
    }

  /*
  ** QApplication::processEvents() may cause segmentation faults.
  ** Please do not use it here.
  */

  int secs = 0;

  if((secs = m_lastTime.secsTo(QTime::currentTime())) >= 1)
    {
      QFileInfo fileInfo(m_dstFileName);

      if(fileInfo.exists())
	{
	  bool inBits = dooble::s_settings.value
	    ("downloadWindow/showDownloadRateInBits", false).toBool();
	  qint64 currentSize = 0;

	  if(inBits)
	    {
	      /*
	      ** Prevent an overflow.
	      */

	      if(fileInfo.size() > std::numeric_limits<qint64>::max() / 8)
		{
		  currentSize = qRound
		    (static_cast<double> (fileInfo.size()) / 1024.0);
		  inBits = false;
		}
	      else
		currentSize = 8 * fileInfo.size();
	    }
	  else
	    currentSize = qRound
	      (static_cast<double> (fileInfo.size()) / 1024.0);

	  if(currentSize >= m_lastSize)
	    if(200.0 * qAbs(m_rate -
			    static_cast<long double> (currentSize -
						      m_lastSize) / secs)
	       / qMax(static_cast<long double> (1), m_rate +
		      static_cast<long double> (currentSize - m_lastSize) /
		      secs) >= 1.0)
	      m_rate = static_cast<qint64>
		(static_cast<long double> (currentSize -
					   m_lastSize) / secs);

	  if(m_total > 0 && m_total >= fileInfo.size())
	    {
	      if(inBits)
		{
		  if(m_rate < 1000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 bit/s (%2 of %3)")).arg(m_rate).
		       arg(dmisc::formattedSize(fileInfo.size())).
		       arg(dmisc::formattedSize(m_total)));
		  else if(m_rate >= 1000 && m_rate < 1000000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 kbit/s (%2 of %3)")).arg(m_rate / 1000).
		       arg(dmisc::formattedSize(fileInfo.size())).
		       arg(dmisc::formattedSize(m_total)));
		  else if(m_rate >= 1000000 && m_rate < 1000000000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 Mbit/s (%2 of %3)")).
		       arg(m_rate / 1000000).
		       arg(dmisc::formattedSize(fileInfo.size())).
		       arg(dmisc::formattedSize(m_total)));
		  else
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 Gbit/s (%2 of %3)")).
		       arg(m_rate / 1000000000).
		       arg(dmisc::formattedSize(fileInfo.size())).
		       arg(dmisc::formattedSize(m_total)));
		}
	      else
		ui.downloadInformationLabel->setText
		  (QString(tr("%1 KiB/s (%2 of %3)")).arg(m_rate).
		   arg(dmisc::formattedSize(fileInfo.size())).
		   arg(dmisc::formattedSize(m_total)));
	    }
	  else
	    {
	      if(inBits)
		{
		  if(m_rate < 1000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 bit/s (%2)")).arg(m_rate).
		       arg(dmisc::formattedSize(fileInfo.size())));
		  else if(m_rate >= 1000 && m_rate < 1000000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 kbit/s (%2)")).arg(m_rate / 1000).
		       arg(dmisc::formattedSize(fileInfo.size())));
		  else if(m_rate >= 1000000 && m_rate < 1000000000)
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 Mbit/s (%2)")).arg(m_rate / 1000000).
		       arg(dmisc::formattedSize(fileInfo.size())));
		  else
		    ui.downloadInformationLabel->setText
		      (QString(tr("%1 Gbit/s (%2)")).arg(m_rate / 1000000000).
		       arg(dmisc::formattedSize(fileInfo.size())));
		}
	      else
		ui.downloadInformationLabel->setText
		  (QString(tr("%1 KiB/s (%2)")).arg(m_rate).
		   arg(dmisc::formattedSize(fileInfo.size())));
	    }

	  m_lastSize = currentSize;
	  m_lastTime = QTime::currentTime();
	}
      else
	slotAbortDownload();
    }
}

void ddownloadwindowitem::abort(void)
{
  /*
  ** Please note that this method does not modify the interface's state.
  */

  if(isDownloading())
    QFile::remove(m_dstFileName);

  dftp *ftp = findChild<dftp *> ();

  if(ftp)
    {
      ftp->abort();
      ftp->deleteLater();
    }

  QFile *file = findChild<QFile *> ();

  if(file)
    file->deleteLater();

  QNetworkReply *reply = findChild<QNetworkReply *> ();

  if(reply)
    {
      reply->abort();
      reply->deleteLater();
    }
}

void ddownloadwindowitem::slotAbortDownload(void)
{
  if(sender())
    m_abortedByUser = true;
  else
    m_abortedByUser = false;

  abort();
  ui.progressBar->setVisible(false);
  ui.abortToolButton->setVisible(false);
  ui.pauseToolButton->setVisible(false);
  QFile::remove(m_dstFileName);
  ui.downloadInformationLabel->setText(tr("Download Aborted"));
  ui.retryToolButton->setVisible(true);

  QWidget *table = parentWidget();

  do
    {
      if(qobject_cast<QTableWidget *> (table))
	break;
      else if(table)
	table = table->parentWidget();
    }
  while(table != 0);

  if(qobject_cast<QTableWidget *> (table))
    for(int i = 0; i < qobject_cast<QTableWidget *> (table)->rowCount(); i++)
      if(this == qobject_cast<QTableWidget *> (table)->cellWidget(i, 0))
	{
	  qobject_cast<QTableWidget *> (table)->resizeRowToContents(i);
	  break;
	}
}

bool ddownloadwindowitem::isDownloading(void) const
{
  /*
  ** What should we do for local copies of large files?
  */

  QFile *file = findChild<QFile *> ();

  if(file && file->isOpen())
    return true;

  return ui.abortToolButton->isVisible();
}

void ddownloadwindowitem::downloadFinished(const qint64 fileSize)
{
  ui.downloadInformationLabel->setText(dmisc::formattedSize(fileSize));
}

void ddownloadwindowitem::slotPauseDownload(void)
{
  m_paused = !m_paused;
  slotSetIcons();

  if(m_paused)
    ui.pauseToolButton->setToolTip(tr("Resume Download"));
  else
    ui.pauseToolButton->setToolTip(tr("Pause Download"));

  if(m_url.scheme().toLower().trimmed() == "ftp")
    {
      if(m_paused)
	{
	  dftp *ftp = findChild<dftp *> ();

	  if(ftp)
	    {
	      disconnect(ftp, SIGNAL(readyRead(void)),
			 this, SLOT(slotReadyRead(void)));
	      disconnect(ftp, SIGNAL(downloadProgress(qint64, qint64)),
			 this, SLOT(slotDataTransferProgress(qint64, qint64)));
	      disconnect(ftp, SIGNAL(finished(void)),
			 this, SLOT(slotDownloadFinished(void)));
	      ftp->abort();
	      ftp->deleteLater();
	    }
	}
      else
	{
	  dftp *ftp = new dftp(this);

	  connect(ftp, SIGNAL(readyRead(void)),
		  this, SLOT(slotReadyRead(void)));
	  connect(ftp, SIGNAL(downloadProgress(qint64, qint64)),
		  this, SLOT(slotDataTransferProgress(qint64, qint64)));
	  connect(ftp, SIGNAL(finished(void)),
		  this, SLOT(slotDownloadFinished(void)));

	  /*
	  ** According to FTP documentation, REST N instructs the FTP
	  ** server to restart file transfer at byte N when the client
	  ** issues a RETR.
	  */

	  QFile *file = findChild<QFile *> ();

	  if(file)
	    ftp->get(m_url, QString("REST %1").arg(file->size()));
	  else
	    ftp->get(m_url);
	}
    }
  else
    {
      if(m_paused)
	{
	  QNetworkReply *reply = findChild<QNetworkReply *> ();

	  if(reply)
	    {
	      disconnect(reply, SIGNAL(readyRead(void)),
			 this, SLOT(slotReadyRead(void)));
	      disconnect(reply, SIGNAL(metaDataChanged(void)),
			 this, SLOT(slotMetaDataChanged(void)));
	      disconnect(reply, SIGNAL(downloadProgress(qint64, qint64)),
			 this, SLOT(slotDataTransferProgress(qint64, qint64)));
	      disconnect(reply, SIGNAL(finished(void)),
			 this, SLOT(slotDownloadFinished(void)));
	      reply->abort();
	      reply->deleteLater();
	    }
	}
      else
	{
	  QNetworkRequest request;

	  request.setUrl(m_url);
	  request.setAttribute
	    (QNetworkRequest::HttpPipeliningAllowedAttribute, true);
#if QT_VERSION >= 0x050300
	  request.setAttribute
	    (QNetworkRequest::SpdyAllowedAttribute,
	     dooble::s_settings.value("settingsWindow/speedy", false).
	     toBool());
#endif

	  QFile *file = findChild<QFile *> ();

	  if(file)
	    {
	      QByteArray range;

	      range.append("bytes=" + QString::number(file->size()) + "-");
	      request.setRawHeader("Range", range);
	    }

	  QNetworkReply *reply = m_networkAccessManager->get(request);

	  reply->setParent(this);
	  reply->ignoreSslErrors();
	  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		  this, SLOT(slotError(QNetworkReply::NetworkError)));
	  connect(reply, SIGNAL(readyRead(void)),
		  this, SLOT(slotReadyRead(void)));
	  connect(reply, SIGNAL(metaDataChanged(void)),
		  this, SLOT(slotMetaDataChanged(void)));
	  connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
		  this, SLOT(slotDataTransferProgress(qint64, qint64)));
	  connect(reply, SIGNAL(finished(void)),
		  this, SLOT(slotDownloadFinished(void)));
	}
    }

  if(m_paused)
    {
      QFileInfo fileInfo(m_dstFileName);

      ui.downloadInformationLabel->setText
	(QString(tr("Paused (%1)")).
	 arg(dmisc::formattedSize(fileInfo.size())));
    }
}

void ddownloadwindowitem::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  ui.abortToolButton->setIcon
    (QIcon(settings.value("downloadWindowItem/abortIcon").toString()));
  ui.computeFileHash->setIcon
    (QIcon(settings.value("downloadWindowItem/computeHashIcon").
	   toString()));
  ui.retryToolButton->setIcon
    (QIcon(settings.value("downloadWindowItem/retryIcon").toString()));

  if(m_paused)
    ui.pauseToolButton->setIcon
      (QIcon(settings.value("downloadWindowItem/resumeIcon").toString()));
  else
    ui.pauseToolButton->setIcon
      (QIcon(settings.value("downloadWindowItem/pauseIcon").toString()));
}

void ddownloadwindowitem::slotReadyRead(void)
{
  QFile *file = findChild<QFile *> ();

  if(file)
    {
      dftp *ftp = qobject_cast<dftp *> (sender());
      QNetworkReply *reply = qobject_cast<QNetworkReply *> (sender());

      if(ftp)
	file->write(ftp->readAll());
      else if(reply)
	{
	  if(!ui.pauseToolButton->isEnabled())
	    foreach(QNetworkReply::RawHeaderPair header,
		    reply->rawHeaderPairs())
	      if((header.first.toLower() == "accept-ranges" &&
		  header.second.toLower() != "none") ||
		 (header.first.toLower() == "content-range"))
		{
		  ui.pauseToolButton->setEnabled(true);
		  break;
		}

	  file->write(reply->readAll());
	}
    }
}

void ddownloadwindowitem::slotDownloadAgain(void)
{
  if(!m_srcFileName.isEmpty() && !m_dstFileName.isEmpty())
    {
      init_ddownloadwindowitem();
      downloadFile(m_srcFileName, m_dstFileName, true,
		   QDateTime::currentDateTime());
    }
  else if(!m_url.isEmpty() && !m_dstFileName.isEmpty())
    {
      init_ddownloadwindowitem();
      downloadUrl(m_url, m_dstFileName, true,
		  QDateTime::currentDateTime(), m_choice);
    }
  else
    return;

  QWidget *table = parentWidget();

  do
    {
      if(qobject_cast<QTableWidget *> (table))
	break;
      else
	table = table->parentWidget();
    }
  while(table != 0);

  if(qobject_cast<QTableWidget *> (table))
    for(int i = 0; i < qobject_cast<QTableWidget *> (table)->rowCount(); i++)
      if(this == qobject_cast<QTableWidget *> (table)->cellWidget(i, 0))
	{
	  qobject_cast<QTableWidget *> (table)->resizeRowToContents(i);
	  break;
	}
}

bool ddownloadwindowitem::abortedByUser(void) const
{
  return m_abortedByUser;
}

QString ddownloadwindowitem::text(void) const
{
  return m_url.toString(QUrl::StripTrailingSlash) + "\n" +
    m_dstFileName;
}

QUrl ddownloadwindowitem::url(void) const
{
  return m_url;
}

QString ddownloadwindowitem::fileName(void) const
{
  return m_dstFileName;
}

void ddownloadwindowitem::slotMetaDataChanged(void)
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> (sender());

  if(reply)
    {
      QUrl url(reply->header(QNetworkRequest::LocationHeader).toUrl());

      if(!url.isEmpty() && url.isValid())
	{
	  /*
	  ** The slotDownloadFinished() method should eliminate all items
	  ** that are currently in use.
	  */

	  reply->abort();
	  m_url = url;
	  init_ddownloadwindowitem();
	  downloadUrl(m_url, m_dstFileName, true,
		      QDateTime::currentDateTime(), m_choice);
	}
    }
}

QDateTime ddownloadwindowitem::dateTime(void) const
{
  return m_dateTime;
}

void ddownloadwindowitem::slotError(QNetworkReply::NetworkError code)
{
  Q_UNUSED(code);
}

void ddownloadwindowitem::slotComputeFileHash(void)
{
  QByteArray buffer(4096, 0);
#if QT_VERSION < 0x050000
  QCryptographicHash hash(QCryptographicHash::Sha1);
#else
  QCryptographicHash hash(QCryptographicHash::Sha256);
#endif
  QFile file(ui.destinationLabel->text());

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(file.open(QIODevice::ReadOnly))
    {
      qint64 rc = 0;

      while((rc = file.read(buffer.data(), buffer.length())) > 0)
	hash.addData(buffer, static_cast<int> (rc));

      file.close();
      ui.fileHash->setText(hash.result().toHex());
    }
  else
    ui.fileHash->clear();

  QApplication::restoreOverrideCursor();
}
