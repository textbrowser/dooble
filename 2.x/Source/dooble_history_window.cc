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
#include <QKeyEvent>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_history_window::dooble_history_window(void):QMainWindow()
{
  m_ui.setupUi(this);
  m_ui.splitter->setStretchFactor(0, 0);
  m_ui.splitter->setStretchFactor(1, 1);
  m_ui.splitter->restoreState
    (QByteArray::fromBase64(dooble_settings::
			    setting("history_window_splitter_state").
			    toByteArray()));
  m_ui.table->sortByColumn(0, Qt::AscendingOrder);
  connect(dooble::s_history,
	  SIGNAL(icon_updated(const QIcon &, const QUrl &)),
	  this,
	  SLOT(slot_icon_updated(const QIcon &, const QUrl &)));
  connect(dooble::s_history,
	  SIGNAL(new_item(const QIcon &, const QWebEngineHistoryItem &)),
	  this,
	  SLOT(slot_new_item(const QIcon &, const QWebEngineHistoryItem &)));
}

void dooble_history_window::closeEvent(QCloseEvent *event)
{
  save_settings();
  QMainWindow::closeEvent(event);
}

void dooble_history_window::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_history_window::populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("history_window_geometry", saveGeometry().toBase64());

  dooble_settings::set_setting
    ("history_window_splitter_state", m_ui.splitter->saveState().toBase64());
}

void dooble_history_window::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("history_window_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble_history_window::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("history_window_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_history_window::slot_icon_updated(const QIcon &icon,
					      const QUrl &url)
{
  QTableWidgetItem *item = m_items.value(url);

  if(!item)
    return;

  item->setIcon(icon);
}

void dooble_history_window::slot_new_item(const QIcon &icon,
					  const QWebEngineHistoryItem &item)
{
  if(!item.isValid())
    return;

  QTableWidgetItem *item1 = new QTableWidgetItem(icon, item.title());

  item1->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item1->setToolTip(item1->text());
  m_items[item.url()] = item1;

  QTableWidgetItem *item2 = new QTableWidgetItem(item.url().toString());

  item2->setData(Qt::UserRole, item.url());
  item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item2->setToolTip(item2->text());

  QTableWidgetItem *item3 = new QTableWidgetItem
    (item.lastVisited().toString(Qt::ISODate));

  item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.table->setSortingEnabled(false);
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 0, item1);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 1, item2);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 2, item3);
  m_ui.table->setSortingEnabled(true);
}
