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
#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_favorites_popup.h"
#include "dooble_history.h"
#include "dooble_history_window.h"

dooble_favorites_popup::dooble_favorites_popup(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);

  if(parent)
    m_ui.delete_selected->setVisible(false);

  m_entries_timer.start(150);
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);

  if(dooble::s_history &&
     dooble::s_history->favorites_model() &&
     dooble::s_history_window)
    {
      m_ui.view->setModel(dooble::s_history->favorites_model());
      m_ui.view->setColumnHidden(2, true);
      m_ui.view->setColumnHidden(3, true);
      connect(dooble::s_history->favorites_model(),
	      SIGNAL(dataChanged(const QModelIndex &,
				 const QModelIndex &,
				 const QVector<int> &)),
	      this,
	      SLOT(slot_sort(void)));
      connect(dooble::s_history->favorites_model(),
	      SIGNAL(rowsInserted(const QModelIndex &, int, int)),
	      this,
	      SLOT(slot_sort(void)));
      connect(this,
	      SIGNAL(favorite_changed(const QUrl &, bool)),
	      dooble::s_history_window,
	      SIGNAL(favorite_changed(const QUrl &, bool)));
      connect(this,
	      SIGNAL(favorite_changed(const QUrl &, bool)),
	      dooble::s_history_window,
	      SLOT(slot_favorite_changed(const QUrl &, bool)));
    }
  else
    QTimer::singleShot(1500, this, SLOT(slot_set_favorites_model(void)));

  m_ui.sort_order->setCurrentIndex
    (qBound(0,
	    dooble_settings::setting("favorites_sort_index").toInt(),
	    m_ui.sort_order->count() - 1));
  slot_sort(m_ui.sort_order->currentIndex());
  connect(&m_entries_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_entries_timer_timeout(void)));
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
  connect(m_ui.sort_order,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slot_sort(int)));
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

void dooble_favorites_popup::closeEvent(QCloseEvent *event)
{
  QDialog::closeEvent(event);
  m_entries_timer.stop();
}

void dooble_favorites_popup::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    {
      accept();
      m_entries_timer.stop();
    }

  QDialog::keyPressEvent(event);
}

void dooble_favorites_popup::prepare_icons(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.delete_selected->setIcon
    (QIcon::fromTheme("edit-delete",
		      QIcon(QString(":/%1/36/delete.png").arg(icon_set))));
}

void dooble_favorites_popup::prepare_viewport_icons(void)
{
  m_ui.view->prepare_viewport_icons();
}

void dooble_favorites_popup::resizeEvent(QResizeEvent *event)
{
  QDialog::resizeEvent(event);

  if(!parent())
    save_settings();
}

void dooble_favorites_popup::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("favorites_window_geometry", saveGeometry().toBase64());
}

void dooble_favorites_popup::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool() && !parent())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("favorites_window_geometry").
					   toByteArray()));

  QDialog::show();

  if(!m_entries_timer.isActive())
    m_entries_timer.start();
}

void dooble_favorites_popup::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool() && !parent())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("favorites_window_geometry").
					   toByteArray()));

  QDialog::showNormal();

  if(!m_entries_timer.isActive())
    m_entries_timer.start();
}

void dooble_favorites_popup::slot_delete_selected(void)
{
  if(!dooble::s_history)
    return;

  auto list(m_ui.view->selectionModel()->selectedRows(1));

  if(list.isEmpty())
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to delete the selected entries?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end());

  for(int i = list.size() - 1; i >= 0; i--)
    if(!m_ui.view->isRowHidden(list.at(i).row()))
      {
	QUrl url(list.at(i).data().toString());

	dooble::s_history->remove_favorite(url);
	emit favorite_changed(url, false);
      }

  prepare_viewport_icons();
  QApplication::restoreOverrideCursor();
}

void dooble_favorites_popup::slot_double_clicked(const QModelIndex &index)
{
  if(QApplication::keyboardModifiers() & Qt::ControlModifier)
    emit open_link_in_new_tab(index.sibling(index.row(), 1).data().toString());
  else
    emit open_link(index.sibling(index.row(), 1).data().toString());
}

void dooble_favorites_popup::slot_entries_timer_timeout(void)
{
  if(m_ui.view->model())
    m_ui.entries->setText(tr("%1 Row(s)").arg(m_ui.view->model()->rowCount()));
}

void dooble_favorites_popup::slot_favorites_sorted(void)
{
  m_ui.sort_order->setCurrentIndex
    (qBound(0,
	    dooble_settings::setting("favorites_sort_index").toInt(),
	    m_ui.sort_order->count() - 1));
}

void dooble_favorites_popup::slot_find(void)
{
  m_ui.search->selectAll();
  m_ui.search->setFocus();
}

void dooble_favorites_popup::slot_search_timer_timeout(void)
{
  auto model = qobject_cast<QStandardItemModel *> (m_ui.view->model());

  if(!model)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto count = model->rowCount();
  auto text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < model->rowCount(); i++)
    if(text.isEmpty())
      {
	if(!m_entries_timer.isActive())
	  m_entries_timer.start();

	m_ui.view->setRowHidden(i, false);
      }
    else
      {
	auto item1 = model->item(i, 0);
	auto item2 = model->item(i, 1);
	auto item3 = model->item(i, 2);

	if(!item1 || !item2 || !item3)
	  m_ui.view->setRowHidden(i, false);
	else if(item1->text().toLower().contains(text) ||
		item2->text().toLower().contains(text) ||
		item3->text().toLower().contains(text))
	  m_ui.view->setRowHidden(i, false);
	else
	  {
	    count -= 1;
	    m_entries_timer.stop();
	    m_ui.view->setRowHidden(i, true);
	  }
      }

  m_ui.entries->setText(tr("%1 Row(s)").arg(count));
  QApplication::restoreOverrideCursor();
  prepare_viewport_icons();
}

void dooble_favorites_popup::slot_set_favorites_model(void)
{
  if(dooble::s_history_window && m_ui.view->model())
    return;
  else if(dooble::s_history &&
	  dooble::s_history->favorites_model() &&
	  dooble::s_history_window)
    {
      if(!m_ui.view->model())
	{
	  m_ui.view->setModel(dooble::s_history->favorites_model());
	  m_ui.view->setColumnHidden(2, true);
	  m_ui.view->setColumnHidden(3, true);
	  slot_sort(m_ui.sort_order->currentIndex());
	}

      connect(dooble::s_history->favorites_model(),
	      SIGNAL(dataChanged(const QModelIndex &,
				 const QModelIndex &,
				 const QVector<int> &)),
	      this,
	      SLOT(slot_sort(void)),
	      Qt::UniqueConnection);
      connect(dooble::s_history->favorites_model(),
	      SIGNAL(rowsInserted(const QModelIndex &, int, int)),
	      this,
	      SLOT(slot_sort(void)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(favorite_changed(const QUrl &, bool)),
	      dooble::s_history_window,
	      SIGNAL(favorite_changed(const QUrl &, bool)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(favorite_changed(const QUrl &, bool)),
	      dooble::s_history_window,
	      SLOT(slot_favorite_changed(const QUrl &, bool)),
	      Qt::UniqueConnection);
    }
  else
    QTimer::singleShot(1500, this, SLOT(slot_set_favorites_model(void)));
}

void dooble_favorites_popup::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_favorites_popup::slot_sort(int index)
{
  if(index == 0) // Last Visited
    m_ui.view->sortByColumn(2, Qt::DescendingOrder);
  else if(index == 1) // Most Popular
    m_ui.view->sortByColumn(3, Qt::DescendingOrder);
  else // Title
    m_ui.view->sortByColumn(0, Qt::AscendingOrder);

  prepare_viewport_icons();
  dooble_settings::set_setting("favorites_sort_index", index);
  emit favorites_sorted();
}

void dooble_favorites_popup::slot_sort(void)
{
  auto index = m_ui.sort_order->currentIndex();

  if(index == 0) // Last Visited
    m_ui.view->sortByColumn(2, Qt::DescendingOrder);
  else if(index == 1) // Most Popular
    m_ui.view->sortByColumn(3, Qt::DescendingOrder);
  else // Title
    m_ui.view->sortByColumn(0, Qt::AscendingOrder);

  prepare_viewport_icons();
  slot_search_timer_timeout();
}
