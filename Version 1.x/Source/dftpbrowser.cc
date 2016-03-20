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

#include <QContextMenuEvent>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QMenu>
#include <QWebFrame>

#include "dftp.h"
#include "dftpbrowser.h"
#include "dmisc.h"
#include "dooble.h"
#include "dtablewidgetquint64item.h"

dftpbrowser::dftpbrowser(QWidget *parent):QWidget(parent)
{
  ui.setupUi(this);
  m_ftp = 0;
  m_webPage = new QWebPage(this);
  m_lastButtonPressed = Qt::NoButton;
  ui.tableWidget->setColumnCount(3);
  ui.tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.tableWidget->setSortingEnabled(true);
  ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui.tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui.tableWidget->setFrameShape(QFrame::StyledPanel);
  ui.tableWidget->sortByColumn(0, Qt::AscendingOrder);
  connect(ui.tableWidget,
	  SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	  this,
	  SLOT(slotItemDoubleClicked(QTableWidgetItem *)));
  connect(ui.tableWidget,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotCustomContextMenuRequest(const QPoint &)));
  connect(ui.tableWidget->horizontalHeader(),
	  SIGNAL(sectionMoved(int, int, int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));
  connect(ui.tableWidget->horizontalHeader(),
	  SIGNAL(sectionClicked(int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));
  connect(ui.tableWidget->horizontalHeader(),
	  SIGNAL(sectionResized(int, int, int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));

  QStringList labels;

  labels << tr("Name") << tr("Size") << tr("Date Modified");
  ui.tableWidget->setHorizontalHeaderLabels(labels);
  ui.tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
  ui.tableWidget->verticalHeader()->setVisible(false);
  ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  ui.tableWidget->setShowGrid(false);

  if(dooble::s_settings.contains("mainWindow/ftpManagerColumnsState1"))
    if(!ui.tableWidget->horizontalHeader()->restoreState
       (dooble::s_settings.value
	("mainWindow/ftpManagerColumnsState1", "").toByteArray()))
      {
	ui.tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);

	for(int i = 0; i < ui.tableWidget->horizontalHeader()->count(); i++)
	  ui.tableWidget->resizeColumnToContents(i);
      }

  ui.splitter->setStretchFactor(1, 1);
}

void dftpbrowser::slotSaveTableHeaderState(void)
{
  dooble::s_settings["mainWindow/ftpManagerColumnsState1"] =
    ui.tableWidget->horizontalHeader()->saveState();
}

dftpbrowser::~dftpbrowser()
{
  ui.tableWidget->clear();
}

void dftpbrowser::load(const QUrl &url, QPointer<dftp> ftp)
{
  emit loadStarted();

  if(url.isEmpty() || !url.isValid() ||
     url.scheme().toLower().trimmed() != "ftp")
    {
      emit loadFinished(false);
      return;
    }

  ui.tableWidget->clearContents();
  ui.tableWidget->setRowCount(0);
  m_url = url;
  m_url.setPath(m_url.path().replace("//", "/"));
  m_directoryCount = 0;
  m_fileCount = 0;

  /*
  ** We need to emit some signals before fetching the required information.
  */

  emit loadProgress(0);

  if(m_ftp)
    m_ftp->deleteLater();

  if(ftp)
    {
      m_ftp = ftp;
      m_ftp->setParent(this);
    }
  else
    m_ftp = new dftp(this);

  connect(m_ftp,
	  SIGNAL(loadStarted(void)),
	  ui.messageTextBrowser,
	  SLOT(clear(void)));
  connect(m_ftp,
	  SIGNAL(directoryChanged(const QUrl &)),
	  this,
	  SLOT(slotUrlChanged(const QUrl &)));
  connect(m_ftp,
	  SIGNAL(finished(const bool)),
	  this,
	  SLOT(slotFinished(const bool)));
  connect(m_ftp,
	  SIGNAL(listInfos(const QList<dftpfileinfo> &)),
	  this,
	  SLOT(slotListInfos(const QList<dftpfileinfo> &)));
  connect(m_ftp,
	  SIGNAL(statusMessageReceived(const QString &)),
	  this,
	  SLOT(slotAppendMessage(const QString &)));
  connect(m_ftp,
	  SIGNAL(unsupportedContent(const QUrl &)),
	  this,
	  SLOT(slotUnsupportedContent(const QUrl &)));
  m_ftp->fetchList(m_url);
}

QString dftpbrowser::html(void) const
{
  QString str("");

  str += "<html>\n";
  str += "<table border=\"0\">\n";
  str += "<tr>\n";

  for(int i = 0; i < ui.tableWidget->columnCount(); i++)
    if(ui.tableWidget->horizontalHeaderItem(i))
      str += QString("<th>%1</th>\n").
	arg(ui.tableWidget->horizontalHeaderItem(i)->text());

  str += "</tr>\n";

  for(int i = 0; i < ui.tableWidget->rowCount(); i++)
    {
      str += "<tr>\n";

      for(int j = 0; j < ui.tableWidget->columnCount(); j++)
	if(ui.tableWidget->item(i, j))
	  str += QString("<td>%1</td>").arg(ui.tableWidget->
					    item(i, j)->text());

      str += "</tr>\n";
    }

  str += "</table>\n";
  str += "</html>";
  return str;
}

QUrl dftpbrowser::url(void) const
{
  return m_url;
}

QString dftpbrowser::title(void) const
{
  return m_url.toString(QUrl::StripTrailingSlash);
}

void dftpbrowser::slotListInfos(const QList<dftpfileinfo> &infos)
{
  if(infos.isEmpty())
    return;

  ui.tableWidget->setSortingEnabled(false);

  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  QTableWidgetItem *item = 0;

  if(ui.tableWidget->rowCount() == 0)
    {
      m_directoryCount += 1;
      ui.tableWidget->setRowCount(1);

      for(int i = 0; i < ui.tableWidget->columnCount(); i++)
	{
	  if(i == 0)
	    {
	      QFileIconProvider iconProvider;

	      item = new QTableWidgetItem
		(iconProvider.icon(QFileIconProvider::Folder), "..", 1);
	    }
	  else
	    item = new QTableWidgetItem("", 1);

	  item->setFlags(flags);
	  ui.tableWidget->setItem(0, i, item);
	}
    }

  ui.tableWidget->setRowCount(ui.tableWidget->rowCount() + infos.size());

  for(int i = 0; i < infos.size(); i++)
    {
      dftpfileinfo info(infos.at(i));
      QFileIconProvider iconProvider;

      if(info.isDir())
	{
	  m_directoryCount += 1;
	  item = new QTableWidgetItem
	    (iconProvider.icon(QFileIconProvider::Folder), info.name(), 1);
	  item->setData(Qt::UserRole, info.path());
	}
      else
	{
	  m_fileCount += 1;
	  item = new QTableWidgetItem
	    (iconProvider.icon(QFileIconProvider::File), info.name(), 0);
	}

      item->setFlags(flags);
      ui.tableWidget->setItem
	(ui.tableWidget->rowCount() - infos.size() + i, 0, item);

      if(info.isDir())
	item = new QTableWidgetItem("", 1);
      else
	{
	  item = new dtablewidgetquint64item(info.size(), 0);
	  item->setText(dmisc::formattedSize(info.size()));
	}

      item->setFlags(flags);
      ui.tableWidget->setItem
	(ui.tableWidget->rowCount() - infos.size() + i, 1, item);
      item = new QTableWidgetItem(info.lastModified().
				  toString("M/dd/yy h:mm AP"),
				  info.isDir());
      item->setFlags(flags);
      ui.tableWidget->setItem
	(ui.tableWidget->rowCount() - infos.size() + i, 2, item);
    }

  ui.tableWidget->setSortingEnabled(true);
  emit loadProgress(0);
}

void dftpbrowser::slotItemDoubleClicked(QTableWidgetItem *item)
{
  if(item)
    if(item->type() == 1)
      {
	item = ui.tableWidget->item(ui.tableWidget->currentRow(), 0);

	if(item)
	  {
	    if(item->text() == "..")
	      {
		QUrl url(m_url);
		QString path(m_url.path());

		if(path.lastIndexOf('/') != -1)
		  {
		    path = path.mid(0, path.lastIndexOf('/'));

		    if(!path.startsWith("/"))
		      {
			if(!url.path().endsWith("/"))
			  url.setPath("/" + path);
			else
			  url.setPath(path);
		      }
		    else
		      url.setPath(path);
		  }

		if(m_lastButtonPressed == Qt::MiddleButton ||
		   QApplication::keyboardModifiers() == Qt::ControlModifier)
		  emit openLinkInNewTab(url);
		else
		  emit loadPage(url);
	      }
	    else
	      {
		QUrl url(m_url);

		url = QUrl
		  (url.toString(QUrl::StripTrailingSlash) + "/" +
		   item->data(Qt::UserRole).toString());

		if(m_lastButtonPressed == Qt::MiddleButton ||
		   QApplication::keyboardModifiers() == Qt::ControlModifier)
		  emit openLinkInNewTab(url);
		else
		  emit loadPage(url);
	      }
	  }
      }
}

void dftpbrowser::slotCustomContextMenuRequest(const QPoint &point)
{
  QTableWidgetItem *item = ui.tableWidget->itemAt(point);

  if(item)
    {
      QMenu menu(this);

      item = ui.tableWidget->item(item->row(), 0);
      m_selectedUrl = QUrl
	(m_url.toString(QUrl::StripTrailingSlash) + "/" +
	 item->text());
      menu.addAction(tr("Copy &Link Location"),
		     this, SLOT(slotCopyLinkLocation(void)));
      menu.addSeparator();

      if(item->type() == 1)
	{
	  menu.addAction(tr("Open Link in New &Tab"),
			 this, SLOT(slotOpenLinkInNewTab(void)));
	  menu.addAction(tr("Open Link in &New Window"),
			 this, SLOT(slotOpenLinkInNewWindow(void)));
	  menu.addSeparator();
	}
      else
	menu.addAction(tr("S&ave Link"),
		       this, SLOT(slotSaveLink(void)));

      menu.exec(ui.tableWidget->mapToGlobal(point));
    }
}

void dftpbrowser::slotSaveLink(void)
{
  emit saveUrl(m_selectedUrl);
}

void dftpbrowser::slotCopyLinkLocation(void)
{
  emit copyLink(m_selectedUrl);
}

void dftpbrowser::slotOpenLinkInNewTab(void)
{
  emit openLinkInNewTab(m_selectedUrl);
}

void dftpbrowser::slotOpenLinkInNewWindow(void)
{
  emit openLinkInNewWindow(m_selectedUrl);
}

void dftpbrowser::stop(void)
{
  if(m_ftp)
    {
      m_ftp->abort();
      emit loadFinished(false);
    }
}

void dftpbrowser::mousePressEvent(QMouseEvent *event)
{
  if(event)
    m_lastButtonPressed = event->button();

  QWidget::mousePressEvent(event);
}

void dftpbrowser::slotFinished(const bool ok)
{
  emit loadFinished(ok);

  if(m_ftp)
    m_ftp->deleteLater();
}

void dftpbrowser::slotUrlChanged(const QUrl &url)
{
  QFileIconProvider iconProvider;

  dmisc::saveIconForUrl(iconProvider.icon(QFileIconProvider::Network), url);
  emit urlChanged(url);
  emit iconChanged();
  emit titleChanged(url.toString(QUrl::StripTrailingSlash));
}

QWebFrame *dftpbrowser::mainFrame(void) const
{
  m_webPage->mainFrame()->setHtml(html());
  return m_webPage->mainFrame();
}

QString dftpbrowser::statusMessage(void) const
{
  return QString(tr("%1 Directory(ies) / %2 File(s)")).
    arg(m_directoryCount).arg(m_fileCount);
}

void dftpbrowser::slotAppendMessage(const QString &text)
{
  ui.messageTextBrowser->append(text);
}

void dftpbrowser::slotUnsupportedContent(const QUrl &url)
{
  emit loadFinished(false);
  emit unsupportedContent(url);

  if(m_ftp)
    m_ftp->deleteLater();
}
