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

#include <QDateTime>
#include <QDir>
#include <QKeyEvent>
#include <QShortcut>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStatusBar>
#include <QWebEngineCookieStore>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_cookies_window::dooble_cookies_window(bool is_private, QWidget *parent):
  QMainWindow(parent)
{
  m_domain_filter_timer.setInterval(750);
  m_domain_filter_timer.setSingleShot(true);
  m_is_private = is_private;
  m_purge_domains_timer.setInterval(30000);
  m_ui.setupUi(this);
  m_ui.domain->setText("");
  m_ui.expiration_date->setText("");
  m_ui.name->setText("");
  m_ui.path->setText("");
  m_ui.periodically_purge->setChecked
    (dooble_settings::setting("periodically_purge_temporary_domains").toBool());

  if(m_ui.periodically_purge->isChecked())
    m_purge_domains_timer.start();

  m_ui.tree->sortItems(0, Qt::AscendingOrder);
  m_ui.tool_bar->addWidget(m_ui.periodically_purge);
  m_ui.value->setText("");

  QLabel *label = new QLabel();
  QString icon_set(dooble_settings::setting("icon_set").toString());

  label->setPixmap
    (QIcon(QString(":/%1/16/private.png").arg(icon_set)).
     pixmap(QSize(16, 16)));
  label->setToolTip(tr("Private cookies exist within "
		       "the scope of this window's parent page. Neither "
		       "window geometry nor window state will be retained."));
  statusBar()->addPermanentWidget(label);

  if(dooble_settings::setting("denote_private_widgets").toBool())
    statusBar()->setVisible(m_is_private);
  else
    statusBar()->setVisible(false);

  connect(dooble::s_application,
	  SIGNAL(containers_cleared(void)),
	  this,
	  SLOT(slot_containers_cleared(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(&m_domain_filter_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_domain_filter_timer_timeout(void)));
  connect(&m_purge_domains_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_domains_timer_timeout(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.delete_shown,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_shown(void)));
  connect(m_ui.domain_filter,
	  SIGNAL(textChanged(const QString &)),
	  &m_domain_filter_timer,
	  SLOT(start(void)));
  connect(m_ui.periodically_purge,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_periodically_purge_temporary_domains(bool)));
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
  connect(m_ui.tree,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slot_item_selection_changed(void)));
  new QShortcut
    (QKeySequence(tr("Ctrl+F")), m_ui.domain_filter, SLOT(setFocus(void)));
  restoreState
    (QByteArray::fromBase64(dooble_settings::
			    setting("dooble_cookies_window_state").
			    toByteArray()));
  setContextMenuPolicy(Qt::NoContextMenu);
}

void dooble_cookies_window::closeEvent(QCloseEvent *event)
{
  if(!m_is_private)
    dooble_settings::set_setting
      ("dooble_cookies_window_state", saveState().toBase64());

  QMainWindow::closeEvent(event);
}

void dooble_cookies_window::delete_top_level_items
(QList<QTreeWidgetItem *> list)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_cookie_store && m_cookies)
    disconnect(m_cookie_store,
	       SIGNAL(cookieRemoved(const QNetworkCookie &)),
	       m_cookies,
	       SLOT(slot_cookie_removed(const QNetworkCookie &)));

  while(!list.isEmpty())
    {
      QTreeWidgetItem *item = list.takeFirst();

      if(!item)
	continue;

      m_child_items.remove(item->text(0));
      m_top_level_items.remove(item->text(0));

      foreach(QTreeWidgetItem *i, item->takeChildren())
	if(i)
	  {
	    QList<QNetworkCookie> cookie
	      (QNetworkCookie::
	       parseCookies(i->data(1, Qt::UserRole).toByteArray()));

	    if(!cookie.isEmpty())
	      {
		if(m_cookie_store)
		  m_cookie_store->deleteCookie(cookie.at(0));

		emit delete_cookie(cookie.at(0));
	      }

	    delete i;
	  }

      item = m_ui.tree->takeTopLevelItem(m_ui.tree->indexOfTopLevelItem(item));

      if(item)
	{
	  QList<QNetworkCookie> cookie
	    (QNetworkCookie::
	     parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	  if(!cookie.isEmpty())
	    {
	      if(m_cookie_store)
		m_cookie_store->deleteCookie(cookie.at(0));

	      emit delete_cookie(cookie.at(0));
	    }
	}

      delete item;
    }

  if(m_cookie_store && m_cookies)
    connect(m_cookie_store,
	    SIGNAL(cookieRemoved(const QNetworkCookie &)),
	    m_cookies,
	    SLOT(slot_cookie_removed(const QNetworkCookie &)));

  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::filter(const QString &text)
{
  m_ui.domain_filter->setText(text);
}

void dooble_cookies_window::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_cookies_window::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_cookies_window::save_settings(void)
{
  if(m_is_private)
    return;

  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("dooble_cookies_window_geometry", saveGeometry().toBase64());

  dooble_settings::set_setting
    ("dooble_cookies_window_state", saveState().toBase64());
}

void dooble_cookies_window::setCookieStore(QWebEngineCookieStore *cookie_store)
{
  m_cookie_store = cookie_store;
}

void dooble_cookies_window::setCookies(dooble_cookies *cookies)
{
  m_cookies = cookies;
}

void dooble_cookies_window::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("dooble_cookies_window_geometry").
			      toByteArray()));

  restoreState
    (QByteArray::fromBase64(dooble_settings::
			    setting("dooble_cookies_window_state").
			    toByteArray()));
  QMainWindow::show();
}

void dooble_cookies_window::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("dooble_cookies_window_geometry").
			      toByteArray()));

  restoreState
    (QByteArray::fromBase64(dooble_settings::
			    setting("dooble_cookies_window_state").
			    toByteArray()));
  QMainWindow::showNormal();
}

void dooble_cookies_window::slot_containers_cleared(void)
{
  m_child_items.clear();

  if(m_cookie_store)
    m_cookie_store->deleteAllCookies();

  m_top_level_items.clear();
  m_ui.tree->clear();
}

void dooble_cookies_window::slot_cookie_added(const QNetworkCookie &cookie,
					      bool is_favorite)
{
  disconnect(m_ui.tree,
	     SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	     this,
	     SLOT(slot_item_changed(QTreeWidgetItem *, int)));

  if(!m_top_level_items.contains(cookie.domain()))
    {
      QString text(m_ui.domain_filter->text().toLower().trimmed());
      QTreeWidgetItem *item = new QTreeWidgetItem
	(m_ui.tree, QStringList() << cookie.domain());

      if(is_favorite)
	item->setCheckState(0, Qt::Checked);
      else
	item->setCheckState(0, Qt::Unchecked);

      item->setData(1, Qt::UserRole, cookie.toRawForm());
      item->setFlags
	(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);

      if(!text.isEmpty())
	item->setHidden(!cookie.domain().contains(text));

      m_top_level_items[cookie.domain()] = item;
      m_ui.tree->addTopLevelItem(item);
    }

  QHash<QByteArray, QTreeWidgetItem *> hash
    (m_child_items.value(cookie.domain()));

  if(!hash.contains(cookie.name()))
    {
      QTreeWidgetItem *item = new QTreeWidgetItem
	(m_top_level_items[cookie.domain()],
	 QStringList() << "" << cookie.name());

      hash[cookie.name()] = item;
      item->setData(1, Qt::UserRole, cookie.toRawForm());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_child_items[cookie.domain()] = hash;
      m_top_level_items[cookie.domain()]->addChild(item);
    }

  m_ui.tree->sortItems
    (m_ui.tree->sortColumn(), m_ui.tree->header()->sortIndicatorOrder());
  m_ui.tree->resizeColumnToContents(0);
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
}

void dooble_cookies_window::slot_cookie_removed(const QNetworkCookie &cookie)
{
  QHash<QByteArray, QTreeWidgetItem *> hash
    (m_child_items.value(cookie.domain()));

  if(hash.isEmpty())
    {
      QTreeWidgetItem *item = m_top_level_items.value(cookie.domain());

      if(item && item->checkState(0) != Qt::Checked)
	{
	  m_top_level_items.remove(cookie.domain());
	  delete m_ui.tree->takeTopLevelItem
	    (m_ui.tree->indexOfTopLevelItem(item));
	}

      return;
    }

  QTreeWidgetItem *item = hash.value(cookie.name());

  if(item && item->parent())
    delete item->parent()->takeChild(item->parent()->indexOfChild(item));

  hash.remove(cookie.name());

  if(hash.isEmpty())
    {
      m_child_items.remove(cookie.domain());
      item = m_top_level_items.value(cookie.domain());

      if(item && item->checkState(0) != Qt::Checked)
	{
	  m_top_level_items.remove(cookie.domain());
	  delete m_ui.tree->takeTopLevelItem
	    (m_ui.tree->indexOfTopLevelItem(item));
	}
    }
  else
    m_child_items[cookie.domain()] = hash;
}

void dooble_cookies_window::slot_delete_selected(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_cookie_store && m_cookies)
    disconnect(m_cookie_store,
	       SIGNAL(cookieRemoved(const QNetworkCookie &)),
	       m_cookies,
	       SLOT(slot_cookie_removed(const QNetworkCookie &)));

  QList<QTreeWidgetItem *> list(m_ui.tree->selectedItems());

  while(!list.isEmpty())
    {
      QTreeWidgetItem *item = list.takeFirst();

      if(!item)
	continue;

      if(m_ui.tree->indexOfTopLevelItem(item) != -1)
	{
	  m_child_items.remove(item->text(0));
	  m_top_level_items.remove(item->text(0));

	  foreach(QTreeWidgetItem *i, item->takeChildren())
	    if(i)
	      {
		QList<QNetworkCookie> cookie
		  (QNetworkCookie::
		   parseCookies(i->data(1, Qt::UserRole).toByteArray()));

		if(!cookie.isEmpty())
		  {
		    if(m_cookie_store)
		      m_cookie_store->deleteCookie(cookie.at(0));

		    emit delete_cookie(cookie.at(0));
		  }

		delete i;
	      }

	  item = m_ui.tree->takeTopLevelItem
	    (m_ui.tree->indexOfTopLevelItem(item));

	  if(item)
	    {
	      QList<QNetworkCookie> cookie
		(QNetworkCookie::
		 parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	      if(!cookie.isEmpty())
		{
		  if(m_cookie_store)
		    m_cookie_store->deleteCookie(cookie.at(0));

		  emit delete_cookie(cookie.at(0));
		}
	    }

	  delete item;
	}
      else
	{
	  QList<QNetworkCookie> cookie
	    (QNetworkCookie::
	     parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	  if(!cookie.isEmpty())
	    {
	      QHash<QByteArray, QTreeWidgetItem *> hash
		(m_child_items.value(cookie.at(0).domain()));

	      hash.remove(cookie.at(0).name());

	      if(hash.isEmpty())
		m_child_items.remove(cookie.at(0).domain());
	      else
		m_child_items[cookie.at(0).domain()] = hash;

	      if(m_cookie_store)
		m_cookie_store->deleteCookie(cookie.at(0));

	      emit delete_cookie(cookie.at(0));
	    }

	  if(item->parent())
	    delete item->parent()->takeChild
	      (item->parent()->indexOfChild(item));
	}
    }

  if(m_cookie_store && m_cookies)
    connect(m_cookie_store,
	    SIGNAL(cookieRemoved(const QNetworkCookie &)),
	    m_cookies,
	    SLOT(slot_cookie_removed(const QNetworkCookie &)));

  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_delete_shown(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTreeWidgetItem *> list;

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      QTreeWidgetItem *item = m_ui.tree->topLevelItem(i);

      if(!(!item || item->isHidden()))
	list << item;
    }

  delete_top_level_items(list);
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_domain_filter_timer_timeout(void)
{
  QString text(m_ui.domain_filter->text().toLower().trimmed());

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      QTreeWidgetItem *item = m_ui.tree->topLevelItem(i);

      if(!item)
	continue;
      else if(text.isEmpty())
	item->setHidden(false);
      else if(item->text(0).contains(text))
	item->setHidden(false);
      else
	item->setHidden(true);
    }

  m_ui.tree->resizeColumnToContents(0);
}

void dooble_cookies_window::slot_item_changed(QTreeWidgetItem *item, int column)
{
  if(column != 0 || !item)
    return;
  else if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name("dooble_cookies_window");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("UPDATE dooble_cookies_domains SET favorite_digest = ? "
		      "WHERE domain_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(item->checkState(0) == Qt::Checked ?
		QByteArray("true") : QByteArray("false")).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->hmac(item->text(0)).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies_window::slot_item_selection_changed(void)
{
  QTreeWidgetItem *item = m_ui.tree->currentItem();

  if(!item)
    {
      m_ui.domain->setText("");
      m_ui.expiration_date->setText("");
      m_ui.name->setText("");
      m_ui.path->setText("");
      m_ui.value->setText("");
      return;
    }
  else
    item->setSelected(true);

  QList<QNetworkCookie> cookie
    (QNetworkCookie::parseCookies(item->data(1, Qt::UserRole).toByteArray()));

  if(cookie.isEmpty())
    {
      m_ui.domain->setText(item->text(0));
      m_ui.domain->setCursorPosition(0);
      m_ui.expiration_date->setText("");
      m_ui.name->setText("");
      m_ui.path->setText("");
      m_ui.value->setText("");
      return;
    }

  if(item->flags() & Qt::ItemIsUserCheckable)
    {
      m_ui.domain->setText(cookie.at(0).domain());
      m_ui.domain->setCursorPosition(0);
      m_ui.expiration_date->setText("");
      m_ui.http_only->setChecked(false);
      m_ui.name->setText("");
      m_ui.path->setText("");
      m_ui.secure->setChecked(false);
      m_ui.session->setChecked(false);
      m_ui.value->setText("");
    }
  else
    {
      m_ui.domain->setText(cookie.at(0).domain());
      m_ui.domain->setCursorPosition(0);
      m_ui.expiration_date->setText(cookie.at(0).expirationDate().toString());
      m_ui.expiration_date->setCursorPosition(0);
      m_ui.http_only->setChecked(cookie.at(0).isHttpOnly());
      m_ui.name->setText(cookie.at(0).name());
      m_ui.name->setCursorPosition(0);
      m_ui.path->setText(cookie.at(0).path());
      m_ui.path->setCursorPosition(0);
      m_ui.secure->setChecked(cookie.at(0).isSecure());
      m_ui.session->setChecked(cookie.at(0).isSessionCookie());
      m_ui.value->setText(cookie.at(0).value());
      m_ui.value->setCursorPosition(0);
    }
}

void dooble_cookies_window::slot_periodically_purge_temporary_domains
(bool state)
{
  dooble_settings::set_setting("periodically_purge_temporary_domains", state);

  if(state)
    {
      if(!m_purge_domains_timer.isActive())
	m_purge_domains_timer.start();
    }
  else
    m_purge_domains_timer.stop();
}

void dooble_cookies_window::slot_purge_domains_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTreeWidgetItem *> list;

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      QTreeWidgetItem *item = m_ui.tree->topLevelItem(i);

      if(!(!item || item->checkState(0) == Qt::Checked))
	list << item;
    }

  delete_top_level_items(list);
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_settings_applied(void)
{
  if(dooble_settings::setting("denote_private_widgets").toBool())
    statusBar()->setVisible(true);
  else
    statusBar()->setVisible(false);
}
