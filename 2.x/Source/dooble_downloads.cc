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

#include <QFileDialog>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QWebEngineDownloadItem>

#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_downloads_item.h"
#include "dooble_settings.h"

dooble_downloads::dooble_downloads(void):QMainWindow()
{
  m_download_path_inspection_timer.start(2500);
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
  connect(m_ui.select,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_path(void)));
}

QString dooble_downloads::download_path(void) const
{
  return m_ui.download_path->text();
}

void dooble_downloads::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_downloads::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_downloads::populate(void)
{
}

void dooble_downloads::record_download(QWebEngineDownloadItem *download)
{
  if(!download)
    return;

  dooble_downloads_item *download_item = new dooble_downloads_item
    (download, this);

  m_download_items[download->id()] = download_item;
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setCellWidget(m_ui.table->rowCount() - 1, 0, download_item);
  m_ui.table->resizeRowsToContents();
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
  populate();
}

void dooble_downloads::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::setting("downloads_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
  populate();
}

void dooble_downloads::slot_download_path_inspection_timer_timeout(void)
{
  QFileInfo file_info(m_ui.download_path->text());
  QPalette palette(m_ui.download_path->palette());
  static QPalette s_palette(m_ui.download_path->palette());

  if(file_info.isWritable())
    palette = s_palette;
  else
    palette.setColor
      (m_ui.download_path->backgroundRole(), QColor(240, 128, 128));

  m_ui.download_path->setPalette(palette);
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
      dooble_settings::set_setting
	("download_path", dialog.selectedFiles().value(0));
      m_ui.download_path->setText(dialog.selectedFiles().value(0));
    }
}
