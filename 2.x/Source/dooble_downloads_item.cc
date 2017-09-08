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

#include <QFileInfo>
#include <QUrl>
#include <QWebEngineDownloadItem>

#include "dooble.h"
#include "dooble_downloads_item.h"
#include "dooble_settings.h"
#include "dooble_ui_utilities.h"

dooble_downloads_item::dooble_downloads_item
(QWebEngineDownloadItem *download, QWidget *parent):QWidget(parent)
{
  m_download = download;
  m_ui.setupUi(this);
  m_ui.progress->setMaximum(0);
  m_ui.progress->setMinimum(0);
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.cancel,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_cancel(void)));

  if(m_download)
    {
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

      QFileInfo file_info(m_download->path());

      m_ui.file_name->setText(file_info.fileName());
      m_ui.progress->setMaximum(100);
    }
  else
    {
      m_ui.cancel->setEnabled(false);
      m_ui.progress->setVisible(false);
    }

#ifdef Q_OS_MACOS
  m_ui.cancel->setStyleSheet("QToolButton {border: none;}"
			     "QToolButton::menu-button {border: none;}");
#endif
  prepare_icons();
}

void dooble_downloads_item::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.cancel->setIcon(QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
}

void dooble_downloads_item::slot_cancel(void)
{
  if(m_download)
    m_download->cancel();
}

void dooble_downloads_item::slot_download_progress(qint64 bytes_received,
						   qint64 bytes_total)
{
  if(bytes_total > 0)
    {
      m_ui.information->setText
	(tr("%1 of %2").
	 arg(dooble_ui_utilities::pretty_size(bytes_received)).
	 arg(dooble_ui_utilities::pretty_size(bytes_total)));
      m_ui.progress->setValue
	(static_cast<int> (100 * (static_cast<double> (bytes_received) /
				  static_cast<double> (bytes_total))));
    }
  else
    m_ui.information->setText
      (tr("%1 of Unknown").
       arg(dooble_ui_utilities::pretty_size(bytes_received)));
}

void dooble_downloads_item::slot_finished(void)
{
  m_ui.progress->setVisible(false);

  if(m_download)
    {
      if(m_download->state() == QWebEngineDownloadItem::DownloadCancelled)
	m_ui.information->setText
	  (tr("Cancelled - %1").arg(m_download->url().host()));
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
    }
  else
    m_ui.information->setText(tr("Interrupted"));

  QSize size(this->size());

  size.setHeight(sizeHint().height());
  resize(size);
}

void dooble_downloads_item::slot_settings_applied(void)
{
  prepare_icons();
}
