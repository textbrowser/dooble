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

#include <QKeyEvent>
#include <QSettings>

#include "dexceptionswindow.h"
#include "dmisc.h"
#include "dooble.h"

dexceptionswindow::dexceptionswindow(dexceptionsmodel *model):QMainWindow()
{
  setObjectName(model->objectName());
  ui.setupUi(this);
  ui.tableView->setModel(model);
  ui.tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  ui.tableView->setSortingEnabled(true);
  ui.tableView->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
  ui.tableView->horizontalHeader()->setSortIndicatorShown(true);
  ui.tableView->horizontalHeader()->setStretchLastSection(true);

  for(int i = 0; i < ui.tableView->horizontalHeader()->count(); i++)
    ui.tableView->resizeColumnToContents(i);

#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  ui.lineEdit->setMaxLength(2500);
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(ui.accept,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slotRadioToggled(bool)));
  connect(ui.block,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slotRadioToggled(bool)));
  connect(ui.tableView->horizontalHeader(),
	  SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
	  this,
	  SLOT(slotSort(int, Qt::SortOrder)));
  connect(ui.lineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotAllow(void)));
  connect(ui.allowPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotAllow(void)));
  connect(ui.closePushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotClose(void)));
  connect(ui.deletePushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotDelete(void)));
  connect(ui.deleteAllPushButton, SIGNAL(clicked(void)), this,
	  SLOT(slotDeleteAll(void)));
  connect(ui.tableView->selectionModel(),
	  SIGNAL(selectionChanged(const QItemSelection &,
				  const QItemSelection &)),
	  this,
	  SLOT(slotItemsSelected(const QItemSelection &,
				 const QItemSelection &)));
  connect(ui.searchLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  slotSetIcons();

  QStringList list;

  if(dooble::s_settings.
     value(QString("%1/accept_or_exempt").arg(objectName()),
	   "exempt").toString() == "accept")
    {
      list << tr("Site");
      list << tr("Originating URL");
      list << tr("Event Date");
      list << tr("Accept");
      ui.accept->setChecked(true);
      ui.allowPushButton->setText(tr("&Accept"));
    }
  else
    {
      list << tr("Site");
      list << tr("Originating URL");
      list << tr("Event Date");
      list << tr("Exempt");
      ui.allowPushButton->setText(tr("&Exempt"));
      ui.block->setChecked(true);
    }

  model->setHorizontalHeaderLabels(list);

  if(dooble::s_settings.contains(QString("%1/tableColumnsState").
				 arg(objectName())))
    {
      if(!ui.tableView->
	 horizontalHeader()->
	 restoreState(dooble::s_settings.value(QString("%1/tableColumnsState").
					       arg(objectName()),
					       "").toByteArray()))
	{
	  ui.tableView->horizontalHeader()->
	    setDefaultAlignment(Qt::AlignLeft);
	  ui.tableView->horizontalHeader()->setSortIndicator
	    (0, Qt::AscendingOrder);
	  ui.tableView->horizontalHeader()->setSortIndicatorShown(true);
	  ui.tableView->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
	  ui.tableView->horizontalHeader()->setSectionResizeMode
	    (QHeaderView::Interactive);
#else
	  ui.tableView->horizontalHeader()->setResizeMode
	    (QHeaderView::Interactive);
#endif
	}
      else
	for(int i = 0; i < ui.tableView->horizontalHeader()->count(); i++)
	  ui.tableView->resizeColumnToContents(i);
    }
  else
    for(int i = 0; i < ui.tableView->horizontalHeader()->count(); i++)
      ui.tableView->resizeColumnToContents(i);

  ui.accept->setEnabled(false);
  ui.block->setEnabled(false);
}

dexceptionswindow::~dexceptionswindow()
{
  saveState();
}

void dexceptionswindow::populate(void)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      model->populate();
      slotSort(ui.tableView->horizontalHeader()->sortIndicatorSection(),
	       ui.tableView->horizontalHeader()->sortIndicatorOrder());
      slotTextChanged(ui.searchLineEdit->text().trimmed());
    }
}

void dexceptionswindow::slotAllow(void)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      QString host(ui.lineEdit->text().trimmed());
      QUrl url;

      if(host.startsWith("."))
	host.remove(0, 1);

      url = QUrl::fromUserInput(host);

      if(host.toLower().trimmed().startsWith("gopher"))
	url.setScheme("gopher");

      if(!url.isEmpty() && url.isValid())
	if(model->allow(url.host()))
	  {
	    ui.lineEdit->clear();
	    slotSort(ui.tableView->horizontalHeader()->sortIndicatorSection(),
		     ui.tableView->horizontalHeader()->sortIndicatorOrder());
	    slotTextChanged(ui.searchLineEdit->text().trimmed());
	  }
    }
}

void dexceptionswindow::slotClose(void)
{
  close();
}

void dexceptionswindow::slotDelete(void)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      QModelIndexList list;

      for(int i = 0; i < model->rowCount(); i++)
	if(!ui.tableView->isRowHidden(i) &&
	   ui.tableView->selectionModel()->isRowSelected(i, ui.tableView->
							 rootIndex()))
	  list.append(model->index(i, 0));

      model->deleteList(list);
      slotTextChanged(ui.searchLineEdit->text().trimmed());

      if(!list.isEmpty())
	{
	  if(list.first().row() >= model->rowCount())
	    ui.tableView->selectRow(model->rowCount() - 1);
	  else
	    ui.tableView->selectRow(list.first().row());
	}
    }
}

void dexceptionswindow::slotDeleteAll(void)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      QModelIndexList list;

      for(int i = 0; i < model->rowCount(); i++)
	if(!ui.tableView->isRowHidden(i))
	  list.append(model->index(i, 0));

      model->deleteList(list);
      slotTextChanged(ui.searchLineEdit->text().trimmed());
    }
}

void dexceptionswindow::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void dexceptionswindow::slotShow(void)
{
  QAction *action = qobject_cast<QAction *> (sender());
  QPushButton *pushButton = qobject_cast<QPushButton *> (sender());
  QRect rect(100, 100, 800, 600);
  QToolButton *toolButton = qobject_cast<QToolButton *> (sender());
  QWidget *parent = 0;
  dview *p = qobject_cast<dview *> (sender());

  /*
  ** dooble::slotExceptionRaised() connects a toolbutton's clicked()
  ** signal to this slot.
  */

  if(action && action->parentWidget())
    {
      parent = action->parentWidget();
      rect = parent->geometry();
      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }
  else
    {
      if(p && p->parentWidget())
	{
	  QObject *prnt(p->parentWidget());
	  dooble *dbl = 0;

	  do
	    {
	      prnt = prnt->parent();
	      dbl = qobject_cast<dooble *> (prnt);
	    }
	  while(prnt != 0 && dbl == 0);

	  parent = dbl;
	}
      else if(pushButton && pushButton->parentWidget())
	parent = pushButton->parentWidget();
      else if(toolButton && toolButton->parentWidget())
	parent = toolButton->parentWidget();

      if(parent)
	rect = parent->geometry();

      while(parent)
	{
	  if(pushButton)
	    {
	      if(qobject_cast<dsettings *> (parent))
		{
		  rect = parent->geometry();
		  break;
		}
	    }
	  else
	    {
	      if(qobject_cast<dooble *> (parent))
		{
		  rect = parent->geometry();
		  break;
		}
	    }

	  parent = parent->parentWidget();
	}

      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }

  if(toolButton && this != toolButton->parent())
    toolButton->disconnect(this);

  if(!isVisible())
    {
      /*
      ** Don't annoy the user.
      */

      if(dooble::s_settings.contains(QString("%1/geometry").
				     arg(objectName())))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.value(QString("%1/geometry").
						 arg(objectName()),
						 rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.
			   value(QString("%1/geometry").
				 arg(objectName())).
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

  ui.tableView->resizeColumnToContents(0);
  ui.tableView->resizeColumnToContents(2);
  showNormal();
  raise();
}

void dexceptionswindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("exceptionsWindow/windowIcon").toString()));
  ui.closePushButton->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
  ui.deletePushButton->setIcon
    (QIcon(settings.value("exceptionsWindow/deleteButtonIcon").toString()));
  ui.deleteAllPushButton->setIcon
    (QIcon(settings.value("exceptionsWindow/deleteAllButtonIcon").toString()));
  ui.allowPushButton->setIcon
    (QIcon(settings.value("exceptionsWindow/allowButtonIcon").
	   toString()));
}

void dexceptionswindow::saveState(void)
{
  /*
  ** geometry() may return (0, 0) coordinates if the window is
  ** not visible.
  */

  if(isVisible())
    {
      if(dmisc::isGnome())
	dooble::s_settings[QString("%1/geometry").arg(objectName())] =
	  geometry();
      else
	dooble::s_settings[QString("%1/geometry").arg(objectName())] =
	  saveGeometry();
    }

  dooble::s_settings[QString("%1/tableColumnsState").arg(objectName())] =
    ui.tableView->horizontalHeader()->saveState();

  QSettings settings;

  if(isVisible())
    {
      if(dmisc::isGnome())
	settings.setValue(QString("%1/geometry").arg(objectName()),
			  geometry());
      else
	settings.setValue(QString("%1/geometry").arg(objectName()),
			  saveGeometry());
    }

  settings.setValue
    (QString("%1/tableColumnsState").arg(objectName()),
     ui.tableView->horizontalHeader()->saveState());
}

void dexceptionswindow::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
      else if(event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Delete)
	slotDelete();
      else if(event->key() == Qt::Key_F &&
	      event->modifiers() == Qt::ControlModifier)
	{
	  ui.searchLineEdit->setFocus();
	  ui.searchLineEdit->selectAll();
	}
      else if(event->key() == Qt::Key_L &&
	      event->modifiers() == Qt::ControlModifier)
	{
	  ui.lineEdit->setFocus();
	  ui.lineEdit->selectAll();
	}
    }

  QMainWindow::keyPressEvent(event);
}

bool dexceptionswindow::allowed(const QString &host) const
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    return model->allowed(host);
  else
    return false;
}

void dexceptionswindow::slotAdd(const QString &host,
				const QUrl &url,
				const QDateTime &dateTime)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      model->add(host, url, dateTime);
      slotSort(ui.tableView->horizontalHeader()->sortIndicatorSection(),
	       ui.tableView->horizontalHeader()->sortIndicatorOrder());
      slotTextChanged(ui.searchLineEdit->text().trimmed());
    }
}

void dexceptionswindow::slotItemsSelected(const QItemSelection &selected,
					  const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  slotTextChanged(ui.searchLineEdit->text().trimmed());
}

void dexceptionswindow::slotSort(int column, Qt::SortOrder order)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    model->sort(column, order);
}

void dexceptionswindow::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;

  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    model->reencode(progress);
}

bool dexceptionswindow::contains(const QString &host) const
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    return model->contains(host);
  else
    return false;
}

void dexceptionswindow::slotTextChanged(const QString &text)
{
  /*
  ** Search text changed.
  */

  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(!model)
    {
      statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			       arg(0).arg(0));
      return;
    }

  int count = 0;
  int selected = 0;

  for(int i = 0; i < model->rowCount(); i++)
    if((model->item(i, 0) &&
	model->item(i, 0)->text().toLower().
	contains(text.toLower().trimmed())) ||
       (model->item(i, 1) &&
	model->item(i, 1)->text().toLower().
	contains(text.toLower().trimmed())) ||
       (model->item(i, 2) &&
	model->item(i, 2)->text().toLower().
	contains(text.toLower().trimmed())))
      {
	count += 1;
	ui.tableView->setRowHidden(i, false);

	if(ui.tableView->selectionModel()->
	   isRowSelected(i, ui.tableView->rootIndex()))
	  selected += 1;
      }
    else
      ui.tableView->setRowHidden(i, true);

  statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			   arg(count).arg(selected));
}

void dexceptionswindow::clear(void)
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    {
      QModelIndexList list;

      for(int i = 0; i < model->rowCount(); i++)
	list.append(model->index(i, 0));

      model->deleteList(list);
      ui.tableView->selectionModel()->clear();
      statusBar()->showMessage(QString(tr("%1 Item(s) / %2 Item(s) Selected")).
			       arg(model->rowCount()).arg(0));
    }
}

QStringList dexceptionswindow::allowedHosts(void) const
{
  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    return model->allowedHosts();
  else
    return QStringList();
}

qint64 dexceptionswindow::size(void) const
{
  return QFileInfo(dooble::s_homePath + QDir::separator() +
		   QString("%1.db").arg(objectName())).size();
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool dexceptionswindow::event(QEvent *event)
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
	  show();
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool dexceptionswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool dexceptionswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif

void dexceptionswindow::slotRadioToggled(bool state)
{
  QRadioButton *radioButton = qobject_cast<QRadioButton *> (sender());

  if(!radioButton)
    return;

  QSettings settings;
  QStringList list;

  if(radioButton == ui.accept)
    {
      if(state)
	dooble::s_settings[QString("%1/accept_or_exempt").
			   arg(objectName())] = "accept";
      else
	dooble::s_settings[QString("%1/accept_or_exempt").
			   arg(objectName())] = "exempt";
    }
  else
    {
      if(state)
	dooble::s_settings[QString("%1/accept_or_exempt").
			   arg(objectName())] = "exempt";
      else
	dooble::s_settings[QString("%1/accept_or_exempt").
			   arg(objectName())] = "accept";
    }

  if(dooble::s_settings.value(QString("%1/accept_or_exempt").
			      arg(objectName()), "exempt") == "accept")
    {
      list << tr("Site");
      list << tr("Originating URL");
      list << tr("Event Date");
      list << tr("Accept");
      ui.allowPushButton->setText(tr("&Accept"));
    }
  else
    {
      list << tr("Site");
      list << tr("Originating URL");
      list << tr("Event Date");
      list << tr("Exempt");
      ui.allowPushButton->setText(tr("&Exempt"));
    }

  dexceptionsmodel *model = qobject_cast<dexceptionsmodel *>
    (ui.tableView->model());

  if(model)
    model->setHorizontalHeaderLabels(list);

  settings.setValue
    (QString("%1/accept_or_exempt").arg(objectName()),
     dooble::s_settings.value(QString("%1/accept_or_exempt").
			      arg(objectName()), "exempt").toString());
}

QString dexceptionswindow::approach(void) const
{
  if(ui.accept->isChecked())
    return "accept";
  else
    return "exempt";
}

void dexceptionswindow::enableApproachRadioButtons(const bool state)
{
  ui.accept->setEnabled(state);
  ui.block->setEnabled(state);
}
