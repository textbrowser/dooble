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

#include <QWebEngineHistory>
#include <QWebEnginePage>

#include "dmisc.h"
#include "dooble.h"
#include "dreinstatedooble.h"

dreinstatedooble::dreinstatedooble(QWidget *parent):QWidget(parent)
{
  m_action = 0;
  ui.setupUi(this);
  ui.treeWidget->setColumnCount(1);
  connect(ui.okPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotRestore(void)));
  connect(ui.cancelPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotCancel(void)));

  QFileInfo fileInfo(dooble::s_homePath + QDir::separator() + ".crashed");

  if(fileInfo.exists())
    {
      QDir dir(dooble::s_homePath + QDir::separator() + "Histories");
      QStringList ids;
      QFileInfoList files(dir.entryInfoList(QDir::Files | QDir::Readable,
					    QDir::Name));
      QTreeWidgetItem *item1 = 0;

      for(int i = 0; i < files.size(); i++)
	{
	  QFile file;
	  QFileInfo fileInfo(files.at(i));

	  /*
	  ** Ignore files that belong to this session.
	  */

	  if(fileInfo.fileName().startsWith(dooble::s_id.toString()))
	    continue;

	  QByteArray bytes;
	  QDataStream in(&bytes, QIODevice::ReadOnly);

	  if(in.status() != QDataStream::Ok)
	    continue;

	  QTreeWidgetItem *item2 = 0;

	  file.setFileName(fileInfo.absoluteFilePath());

	  if(file.open(QIODevice::ReadOnly))
	    if(!(bytes = file.readAll()).isEmpty())
	      {
		bool ok = true;

		bytes = dmisc::daa(bytes, &ok);

		if(!ok)
		  {
		    file.remove();
		    continue;
		  }

		QWebEnginePage page;

		in >> *page.history();

		if(!page.history() ||
		   !page.history()->currentItem().isValid())
		  {
		    file.close();
		    continue;
		  }

		QUrl url(page.history()->currentItem().url());
		QIcon icon(dmisc::iconForUrl(url));
		QString id(fileInfo.fileName());
		QString tabId(id.right(10));
		QString title(page.history()->currentItem().title());
		QString windowId(id.right(30).mid(0, 20));

		if(!item1 || !ids.contains(windowId))
		  {
		    ids.append(windowId);
		    item1 = new QTreeWidgetItem(ui.treeWidget);
		    item1->setText(0,
				   QString(tr("Window %1")).
				   arg(ids.count()));
		  }

		item2 = new QTreeWidgetItem(item1);
		item2->setIcon(0, icon);

		if(title.isEmpty())
		  {
		    if(url.isEmpty() || !url.isValid())
		      item2->setText(0, tr("(Untitled)"));
		    else
		      item2->setText
			(0, url.toString(QUrl::StripTrailingSlash));
		  }
		else
		  item2->setText(0, title);

		item2->setData(0, Qt::UserRole, bytes);
		item2->setData(0, Qt::UserRole + 1,
			       fileInfo.absoluteFilePath());
		item2->setCheckState(0, Qt::Checked);
		item1->setExpanded(true);
	      }

	  file.close();
	}
    }
}

dreinstatedooble::~dreinstatedooble()
{
  if(m_action)
    m_action->deleteLater();
}

void dreinstatedooble::setTabAction(QAction *action)
{
  if(m_action)
    {
      removeAction(m_action);
      m_action->deleteLater();
    }

  m_action = action;

  if(m_action)
    addAction(m_action);
}

QAction *dreinstatedooble::tabAction(void) const
{
  return m_action;
}

void dreinstatedooble::slotCancel(void)
{
  QFile::remove(dooble::s_homePath + QDir::separator() + ".crashed");

  for(int i = 0; i < ui.treeWidget->topLevelItemCount(); i++)
    {
      QTreeWidgetItem *item = ui.treeWidget->topLevelItem(i);

      if(item)
	{
	  for(int j = 0; j < item->childCount(); j++)
	    {
	      QTreeWidgetItem *child = item->child(j);

	      if(child)
		QFile::remove(child->data(0, Qt::UserRole + 1).toString());
	    }
	}
    }

  emit close();
}

void dreinstatedooble::slotRestore(void)
{
  for(int i = 0; i < ui.treeWidget->topLevelItemCount(); i++)
    {
      dooble *d = 0;
      QTreeWidgetItem *item = ui.treeWidget->topLevelItem(i);

      if(item)
	{
	  for(int j = 0; j < item->childCount(); j++)
	    {
	      QTreeWidgetItem *child = item->child(j);

	      if(!child)
		continue;

	      QFile::remove(child->data(0, Qt::UserRole + 1).toString());

	      if(child->checkState(0) != Qt::Checked)
		continue;

	      if(!d)
		d = new dooble(child->data(0, Qt::UserRole).toByteArray(),
			       qobject_cast<dooble *> (parentWidget()));
	      else
		d->newTab(child->data(0, Qt::UserRole).toByteArray());
	    }
	}
    }

  QFile::remove(dooble::s_homePath + QDir::separator() + ".crashed");
  emit close();
}

bool dreinstatedooble::isEmpty(void) const
{
  return ui.treeWidget->topLevelItemCount() == 0;
}
