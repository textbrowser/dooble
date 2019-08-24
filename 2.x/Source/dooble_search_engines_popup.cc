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
#include <QMessageBox>
#include <QSqlQuery>
#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_search_engines_popup.h"

dooble_search_engines_popup::dooble_search_engines_popup(QWidget *parent):
  QDialog(parent)
{
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.view->setModel(new QStandardItemModel(this));
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.view,
	  SIGNAL(doubleClicked(const QModelIndex &)),
	  this,
	  SLOT(slot_double_clicked(const QModelIndex &)));
#ifdef Q_OS_MACOS
  m_ui.delete_selected->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}");
#endif
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
  prepare_icons();
  setWindowFlags(Qt::WindowStaysOnTopHint | windowFlags());
}

void dooble_search_engines_popup::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec("CREATE TABLE IF NOT EXISTS dooble_search_engines ("
	     "title TEXT NOT NULL, "
	     "url TEXT NOT NULL, "
	     "url_digest TEXT PRIMARY KEY NOT NULL)");
}

void dooble_search_engines_popup::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    accept();

  QDialog::keyPressEvent(event);
}

void dooble_search_engines_popup::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.delete_selected->setIcon
    (QIcon(QString(":/%1/36/delete.png").arg(icon_set)));
}

void dooble_search_engines_popup::prepare_viewport_icons(void)
{
  m_ui.view->prepare_viewport_icons();
}

void dooble_search_engines_popup::resizeEvent(QResizeEvent *event)
{
  QDialog::resizeEvent(event);

  if(!parent())
    save_settings();
}

void dooble_search_engines_popup::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("search_engines_window_geometry", saveGeometry().toBase64());
}

void dooble_search_engines_popup::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool() && !parent())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("search_engines_window_geometry").
			      toByteArray()));

  QDialog::show();
}

void dooble_search_engines_popup::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool() && !parent())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("search_engines_window_geometry").
			      toByteArray()));

  QDialog::showNormal();
}

void dooble_search_engines_popup::slot_delete_selected(void)
{
  QModelIndexList list(m_ui.view->selectionModel()->selectedIndexes());

  if(list.isEmpty())
    return;
  else if(m_ui.view->isRowHidden(list.at(0).row()))
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to delete the selected entry?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    return;

  prepare_viewport_icons();
}

void dooble_search_engines_popup::slot_double_clicked(const QModelIndex &index)
{
  if(QApplication::keyboardModifiers() & Qt::ControlModifier)
    emit open_link_in_new_tab(index.sibling(index.row(), 1).data().toString());
  else
    emit open_link(index.sibling(index.row(), 1).data().toString());
}

void dooble_search_engines_popup::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_search_engines_popup::slot_search_timer_timeout(void)
{
  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_ui.view->model());

  if(!model)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < model->rowCount(); i++)
    if(text.isEmpty())
      m_ui.view->setRowHidden(i, false);
    else
      {
	QStandardItem *item1 = model->item(i, 0);
	QStandardItem *item2 = model->item(i, 1);
	QStandardItem *item3 = model->item(i, 2);

	if(!item1 || !item2 || !item3)
	  m_ui.view->setRowHidden(i, false);
	else if(item1->text().toLower().contains(text) ||
		item2->text().toLower().contains(text) ||
		item3->text().toLower().contains(text))
	  m_ui.view->setRowHidden(i, false);
	else
	  m_ui.view->setRowHidden(i, true);
      }

  QApplication::restoreOverrideCursor();
  m_ui.view->prepare_viewport_icons();
}

void dooble_search_engines_popup::slot_settings_applied(void)
{
  prepare_icons();
}
