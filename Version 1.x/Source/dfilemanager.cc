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
#include <QDir>
#include <QFileDialog>
#include <QFileInfoList>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QStack>
#include <QTreeView>
#include <QWebFrame>

#include "dfilemanager.h"
#include "dmisc.h"
#include "dooble.h"

QPointer<dfilesystemmodel> dfilemanager::tableModel = 0;
QPointer<QFileSystemModel> dfilemanager::treeModel = 0;

dfilemanager::dfilemanager(QWidget *parent):QWidget(parent)
{
  ui.setupUi(this);
  m_webPage = new QWebPage(this);

  if(!qobject_cast<dview *> (parent))
    connect(this,
	    SIGNAL(loadPage(const QUrl &)),
	    this,
	    SLOT(slotLoad(const QUrl &)));

  connect(ui.treeView,
	  SIGNAL(clicked(const QModelIndex &)),
	  this,
	  SLOT(slotTreeClicked(const QModelIndex &)));
  connect(ui.tableView,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slotTableClicked(const QModelIndex &)));
  connect(ui.pathLabel,
	  SIGNAL(linkActivated(const QString &)),
	  this,
	  SLOT(slotLabelClicked(const QString &)));
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);
  connect(treeModel,
	  SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
	  this,
	  SLOT(slotDirectoryRemoved(const QModelIndex &, int, int)));
  connect(tableModel,
	  SIGNAL(directoryLoaded(const QString &)),
	  this,
	  SLOT(slotTableDirectoryLoaded(const QString &)));
  ui.treeView->setModel(treeModel);
  ui.treeView->setSortingEnabled(true);
  ui.treeView->sortByColumn(0, Qt::AscendingOrder);
  ui.treeView->hideColumn(1);
  ui.treeView->hideColumn(2);
  ui.treeView->hideColumn(3);
  ui.treeView->setExpandsOnDoubleClick(false);
  ui.tableView->setModel(tableModel);
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
#else
  ui.tableView->hideColumn(5);
  ui.tableView->hideColumn(6);
#endif
  ui.tableView->setSortingEnabled(true);
  ui.tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  ui.tableView->sortByColumn(0, Qt::AscendingOrder);
  ui.tableView->setShowGrid(false);
  ui.treeView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.treeView,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotCustomContextMenuRequest(const QPoint &)));
  ui.tableView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.tableView,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotCustomContextMenuRequest(const QPoint &)));
  connect(ui.tableView->horizontalHeader(),
	  SIGNAL(sectionMoved(int, int, int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));
  connect(ui.tableView->horizontalHeader(),
	  SIGNAL(sectionClicked(int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));
  connect(ui.tableView->horizontalHeader(),
	  SIGNAL(sectionResized(int, int, int)),
	  this,
	  SLOT(slotSaveTableHeaderState(void)));
  connect
    (ui.treeView->itemDelegate(),
     SIGNAL(closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint)),
     this,
     SLOT(slotCloseEditor(QWidget *, QAbstractItemDelegate::EndEditHint)));
  connect
    (ui.tableView->itemDelegate(),
     SIGNAL(closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint)),
     this,
     SLOT(slotCloseEditor(QWidget *, QAbstractItemDelegate::EndEditHint)));

  if(dooble::s_settings.contains("mainWindow/fileManagerColumnsState1"))
    if(!ui.tableView->horizontalHeader()->restoreState
       (dooble::s_settings.value
	("mainWindow/fileManagerColumnsState1", "").toByteArray()))
      {
	ui.tableView->sortByColumn(0, Qt::AscendingOrder);
	ui.tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

	for(int i = 0; i < ui.tableView->horizontalHeader()->count(); i++)
	  ui.tableView->resizeColumnToContents(i);
      }
}

void dfilemanager::slotSaveTableHeaderState(void)
{
  dooble::s_settings["mainWindow/fileManagerColumnsState1"] =
    ui.tableView->horizontalHeader()->saveState();
}

dfilemanager::~dfilemanager()
{
  /*
  ** Do not attempt to save the table header's state here.
  */
}

void dfilemanager::load(const QUrl &url)
{
  emit loadStarted();

  if(url.isEmpty() || !url.isValid())
    {
      emit loadFinished(false);
      return;
    }

  QString path(url.toLocalFile());

  if(path.isEmpty())
    path = QDir::rootPath();

  QFileInfo fileInfo(path);

  if(!fileInfo.exists())
    {
      emit loadFinished(false);
      return;
    }

  m_url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
  m_url = QUrl::fromEncoded(m_url.toEncoded(QUrl::StripTrailingSlash));

  if("file:" == m_url.toString(QUrl::StripTrailingSlash))
    m_url = QUrl::fromLocalFile(path);

  emit urlChanged(m_url);
  emit titleChanged(m_url.toString(QUrl::StripTrailingSlash));
  emit loadProgress(0);
  ui.treeView->selectionModel()->clear();

  QModelIndex index;

  if(qobject_cast<QFileSystemModel *> (ui.treeView->model()))
    index = qobject_cast<QFileSystemModel *> (ui.treeView->model())->
      index(m_url.toLocalFile());

  QModelIndex tmpIndex(index);

  ui.treeView->selectionModel()->select
    (index, QItemSelectionModel::ClearAndSelect);

  while(tmpIndex.isValid())
    {
      ui.treeView->expand(tmpIndex);
      tmpIndex = tmpIndex.parent();
    }

  ui.treeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
  dfilemanager::tableModel->setRootPath(m_url.toLocalFile());

  if(qobject_cast<dfilesystemmodel *> (ui.tableView->model()))
    ui.tableView->setRootIndex
      (qobject_cast<dfilesystemmodel *> (ui.tableView->model())->
       index(m_url.toLocalFile()));

  ui.pathLabel->clear();
  prepareLabel(m_url);
  emit iconChanged();

  /*
  ** Emit a loadFinished() signal. QFileSystemModel contains cached
  ** information. As a result, the directoryLoaded() signal may not
  ** be emitted.
  */

  emit loadFinished(true);
  ui.tableView->update();
  ui.treeView->update();
}

QUrl dfilemanager::url(void) const
{
  return m_url;
}

QString dfilemanager::title(void) const
{
  return m_url.toString(QUrl::StripTrailingSlash);
}

QString dfilemanager::html(void) const
{
  QString str("");
  QModelIndex index(ui.tableView->rootIndex());
  dfilesystemmodel *model = qobject_cast<dfilesystemmodel *>
    (ui.tableView->model());

  str += "<html>\n";
  str += ui.pathLabel->text() + "\n";
  str += "<table border=\"0\">\n";
  str += "<tr>\n";

  if(model)
    for(int i = 0; i < ui.tableView->horizontalHeader()->count(); i++)
      str += QString("<th>%1</th>\n").arg
	(model->headerData(i, Qt::Horizontal).toString());

  str += "</tr>\n";

  for(int i = 0; index.child(i, 0).isValid(); i++)
    {
      str += "<tr>\n";

      if(model)
	for(int j = 0; j < ui.tableView->horizontalHeader()->count(); j++)
	  str += QString("<td>%1</td>").arg
	    (model->data(index.child(i, j)).toString());

      str += "</tr>\n";
    }

  str += "</table>\n";
  str += "</html>";
  return str;
}

void dfilemanager::slotTableClicked(const QModelIndex &index)
{
  dfilesystemmodel *model = qobject_cast<dfilesystemmodel *>
    (ui.tableView->model());

  if(!model)
    return;

  if(model->isDir(index) ||
     dmisc::canDoobleOpenLocalFile(QUrl::fromLocalFile(model->
						       fileInfo(index).
						       absoluteFilePath())))
    {
      m_url = QUrl::fromLocalFile(model->filePath(index));
      m_url = QUrl::fromEncoded(m_url.toEncoded(QUrl::StripTrailingSlash));
      emit loadPage(m_url);
    }
  else
    {
      /*
      ** Attempt to open the selected file or prompt the user for an
      ** application.
      */

      QString action("");
      QFileInfo info(model->fileInfo(index));

      if(dooble::s_applicationsActions.contains(info.suffix().trimmed()))
	{
	  action = dooble::s_applicationsActions[info.suffix().trimmed()];

	  QFileInfo info(action);

	  if(!info.isExecutable() || !info.isReadable())
	    action = "prompt";
	}
      else
	action = "prompt";

      if(action == "prompt")
	{
	  QFileDialog fileDialog(this);

	  fileDialog.setOption
	    (QFileDialog::DontUseNativeDialog,
	     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
				       false).toBool());
	  fileDialog.setFilter(QDir::AllDirs | QDir::Files
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	                       | QDir::Readable | QDir::Executable);
#else
	                      );
#endif
	  fileDialog.setWindowTitle
	    (tr("Dooble Web Browser: Select Application"));
	  fileDialog.setFileMode(QFileDialog::ExistingFile);
	  fileDialog.setDirectory(QDir::homePath());
	  fileDialog.setLabelText(QFileDialog::Accept, tr("Select"));
	  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

	  if(fileDialog.exec() == QDialog::Accepted)
	    action = fileDialog.selectedFiles().value(0);
	  else
	    action.clear();
	}

      if(!action.isEmpty())
	dmisc::launchApplication
	  (action, QStringList(model->fileInfo(index).absoluteFilePath()));
    }
}

void dfilemanager::slotTreeClicked(const QModelIndex &index)
{
  if(qobject_cast<QFileSystemModel *> (ui.treeView->model()))
    {
      m_url = QUrl::fromLocalFile
	(qobject_cast<QFileSystemModel *> (ui.treeView->model())->
	 filePath(index));
      m_url = QUrl::fromEncoded(m_url.toEncoded(QUrl::StripTrailingSlash));
      emit loadPage(m_url);
    }
}

void dfilemanager::prepareLabel(const QUrl &url)
{
  QString path(url.toLocalFile());
  QString richText("");
  QString compositePath("");

#ifdef Q_OS_WIN32
  path.remove("file:///");
#else
  if(path.startsWith("file:///"))
    path.remove("file://");
  else if(path == "file:")
    path = "/";
#endif

  /*
  ** Do not provide QString::SkipEmptyParts.
  */

  QStringList list(path.split(QDir::separator()));

  while(!list.isEmpty())
    {
      compositePath += list.first() + QDir::separator();
      richText += QString("<a href = \"%1\">%2</a>")
	.arg(compositePath).arg(list.takeFirst());

      if(!list.isEmpty())
	richText += QDir::separator();
    }

  pathLabelPlainText = path;
  ui.pathLabel->setText(richText);
}

void dfilemanager::slotLabelClicked(const QString &link)
{
  QUrl url(QUrl::fromLocalFile(link));

  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
  emit loadPage(url);
}

void dfilemanager::slotDirectoryRemoved(const QModelIndex &index,
					int start, int end)
{
  if(start < 0) // How about validating end?
    return;

  QFileSystemModel *model = qobject_cast<QFileSystemModel *>
    (ui.treeView->model());

  if(model)
    for(int i = start; i <= end; i++)
      if(model->filePath(index.child(i, 0)) == pathLabelPlainText)
	/*
	** Proceed to the parent directory.
	*/

	slotTreeClicked(index);
}

void dfilemanager::slotCustomContextMenuRequest(const QPoint &pos)
{
  QModelIndex index;

  if(qobject_cast<QTreeView *> (sender()))
    {
      QFileSystemModel *model =
	qobject_cast<QFileSystemModel *> (ui.treeView->model());

      if(!model)
	return;

      QMenu menu(this);
      QAction *action = 0;

      index = ui.treeView->indexAt(pos);

      QFile::Permissions permissions = model->permissions(index);

      if(!model->fileName(index).isEmpty() && model->isDir(index))
	{
	  action = menu.addAction(tr("Create &Sub-Directory"),
				  this,
				  SLOT(slotTreeMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("create_subdirectory");
	  menu.addSeparator();
	}

      if(index.isValid())
	{
	  action = menu.addAction
	    (tr("&Delete"),
	     this,
	     SLOT(slotTreeMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("delete");
	  menu.addSeparator();
	  action = menu.addAction
	    (tr("&Rename"),
	     this,
	     SLOT(slotTreeMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("rename");
	  menu.exec(ui.treeView->mapToGlobal(pos));
	}
    }
  else if(qobject_cast<QTableView *> (sender()))
    {
      dfilesystemmodel *model =
	qobject_cast<dfilesystemmodel *> (ui.tableView->model());

      if(!model)
	return;

      QMenu menu(this);
      QAction *action = 0;

      index = model->index(pathLabelPlainText);

      QFile::Permissions permissions = model->permissions(index);

      action = menu.addAction(tr("Create &Directory"),
			      this,
			      SLOT(slotTableMenuAction(void)));
      action->setEnabled
	(permissions & QFile::WriteUser);
      action->setData("create_directory");
      index = ui.tableView->indexAt(pos);

      if(!model->fileName(index).isEmpty() && model->isDir(index))
	{
	  action = menu.addAction(tr("Create &Sub-Directory"),
				  this,
				  SLOT(slotTableMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("create_subdirectory");
	}

      if(index.isValid())
	{
	  QFile::Permissions permissions = model->permissions(index);

	  menu.addSeparator();
	  action = menu.addAction
	    (tr("&Delete"),
	     this,
	     SLOT(slotTableMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("delete");
	  menu.addSeparator();
	  action = menu.addAction
	    (tr("&Rename"),
	     this,
	     SLOT(slotTableMenuAction(void)));
	  action->setEnabled
	    (permissions & QFile::WriteUser);
	  action->setData("rename");
	}

      menu.exec(ui.tableView->mapToGlobal(pos));
    }
}

void dfilemanager::slotTreeMenuAction(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    menuAction(action->data().toString(), ui.treeView);
}

void dfilemanager::slotTableMenuAction(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    menuAction(action->data().toString(), ui.tableView);
}

void dfilemanager::menuAction(const QString &action, QAbstractItemView *view)
{
  if(!view)
    return;

  /*
  ** Care should be taken when calling this method programatically.
  */

  if(action == "delete")
    deleteSelections(view);
  else if(action == "rename")
    {
      view->openPersistentEditor(view->currentIndex());
      view->setCurrentIndex(view->currentIndex());
      view->edit(view->currentIndex());
    }
  else if(action == "create_directory")
    {
      dfilesystemmodel *model = qobject_cast<dfilesystemmodel *>
	(ui.tableView->model());

      if(!model)
	return;

      QModelIndex index;

      for(int i = 1;; i++)
	{
	  QString filePath(pathLabelPlainText);

	  if(QFile(QString("%1/%2_%3").arg(filePath).arg("untitled").
		   arg(i)).exists())
	    continue;

	  index = model->mkdir(model->index(filePath),
			       QString("%1_%2").arg("untitled").arg(i));
	  ui.tableView->openPersistentEditor(index);
	  ui.tableView->setCurrentIndex(index);
	  ui.tableView->edit(index);
	  break;
	}
    }
  else if(action == "create_subdirectory")
    {
      QFileSystemModel *model = qobject_cast<QFileSystemModel *>
	(view->model());

      if(!model)
	return;

      QModelIndex index(view->currentIndex());

      for(int i = 1;; i++)
	{
	  QString filePath(model->filePath(index));

	  if(QFile(QString("%1/%2_%3").arg(filePath).arg("untitled").
		   arg(i)).exists())
	    continue;

	  index = model->mkdir(model->index(filePath),
			       QString("%1_%2").arg("untitled").arg(i));

	  if(qobject_cast<QTreeView *> (view) == ui.treeView)
	    {
	      view->openPersistentEditor(index);
	      view->setCurrentIndex(index);
	      view->edit(index);
	    }

	  break;
	}
    }
}

void dfilemanager::slotCloseEditor(QWidget *editor,
				   QAbstractItemDelegate::EndEditHint hint)
{
  Q_UNUSED(editor);
  Q_UNUSED(hint);

  if(sender() == ui.treeView->itemDelegate())
    {
      ui.treeView->closePersistentEditor(ui.treeView->currentIndex());
      ui.treeView->sortByColumn(0);
    }
  else if(sender() == ui.tableView->itemDelegate())
    {
      ui.tableView->closePersistentEditor(ui.tableView->currentIndex());
      ui.tableView->sortByColumn(0);
    }
}

void dfilemanager::keyPressEvent(QKeyEvent *event)
{
  if(event && (event->key() == Qt::Key_Backspace ||
	       event->key() == Qt::Key_Delete))
    deleteSelections(ui.tableView);

  QWidget::keyPressEvent(event);
}

void dfilemanager::deleteSelections(QAbstractItemView *view)
{
  QFileSystemModel *model = qobject_cast<QFileSystemModel *>
    (view->model());

  if(!model)
    return;

  QModelIndexList list(view->selectionModel()->selectedRows());

  if(list.size() == 1)
    {
      QSettings settings(dooble::s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);
      QMessageBox mb(QMessageBox::Warning,
		     tr("Dooble Web Browser: Warning"),
		     QString(tr("Are you sure that you wish to delete "
				"%1?")).arg(model->fileName(list.at(0))),
		     QMessageBox::No | QMessageBox::Yes,
		     this);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setWindowIcon
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      for(int i = 0; i < mb.buttons().size(); i++)
	if(mb.buttonRole(mb.buttons().at(i)) == QMessageBox::AcceptRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::ApplyRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::YesRole)
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("okButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }
	else
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("cancelButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }

      if(mb.exec() == QMessageBox::Yes)
	if(model && list.at(0).isValid())
	  model->remove(list.at(0));
    }
  else if(!list.isEmpty())
    {
      QSettings settings(dooble::s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);
      QMessageBox mb(QMessageBox::Warning,
		     tr("Dooble Web Browser: Warning"),
		     tr("Are you sure that you wish to delete the selected "
			"entries?"),
		     QMessageBox::No | QMessageBox::Yes,
		     this);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setWindowIcon
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      for(int i = 0; i < mb.buttons().size(); i++)
	if(mb.buttonRole(mb.buttons().at(i)) == QMessageBox::AcceptRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::ApplyRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::YesRole)
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("okButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }
	else
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("cancelButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }

      if(mb.exec() == QMessageBox::Yes)
	while(!list.isEmpty())
	  {
	    if(model && list.first().isValid())
	      model->remove(list.takeFirst());
	    else
	      break;
	  }
    }
}

void dfilemanager::stop(void)
{
}

void dfilemanager::slotLoad(const QUrl &url)
{
  load(url);
}

QWebFrame *dfilemanager::mainFrame(void) const
{
  m_webPage->mainFrame()->setHtml(html());
  return m_webPage->mainFrame();
}

QString dfilemanager::statusMessage(void) const
{
  dfilesystemmodel *model = qobject_cast<dfilesystemmodel *>
    (ui.tableView->model());

  if(model)
    return QString(tr("%1 Entry(ies)").
		   arg(model->rowCount(model->index(m_url.toLocalFile()))));
  else
    return QString(tr("0 Entries"));
}

void dfilemanager::slotTableDirectoryLoaded(const QString &path)
{
  /*
  ** Emitting loadFinished() here will cause an update of the history.db
  ** database. If the file manager's current directory is the directory
  ** that houses the history.db file, the directoryLoaded() signal will
  ** be emitted recursively.
  */

  if(path != dooble::s_homePath)
    emit loadFinished(true);
}
