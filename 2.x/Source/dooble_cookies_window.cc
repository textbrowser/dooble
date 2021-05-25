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
#include <QStatusBar>
#include <QWebEngineCookieStore>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_ui_utilities.h"

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

  m_ui.toggle_shown->setProperty("state", true);
  m_ui.tool_bar->addWidget(m_ui.periodically_purge);
  m_ui.tree->sortItems(0, Qt::AscendingOrder);
  m_ui.value->setText("");

  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto label = new QLabel();
  auto use_material_icons(dooble_settings::use_material_icons());

  label->setPixmap
    (QIcon::fromTheme(use_material_icons + "view-private",
		      QIcon(QString(":/%1/18/private.png").arg(icon_set))).
     pixmap(QSize(16, 16)));
  label->setToolTip
    (tr("<html>Private cookies exist within "
	"the scope of this window's parent Dooble window. Neither "
	"window geometry nor window state will be retained.</html>"));
  statusBar()->addPermanentWidget(label);

  if(dooble_settings::setting("denote_private_widgets").toBool())
    statusBar()->setVisible(m_is_private);
  else
    statusBar()->setVisible(false);

  connect(&m_domain_filter_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_domain_filter_timer_timeout(void)));
  connect(&m_purge_domains_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_domains_timer_timeout(void)));
  connect(dooble::s_application,
	  SIGNAL(cookies_cleared(void)),
	  this,
	  SLOT(slot_cookies_cleared(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_ui.delete_selected,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_selected(void)));
  connect(m_ui.delete_shown,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_shown(void)));
  connect(m_ui.delete_unchecked,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_unchecked(void)));
  connect(m_ui.domain_filter,
	  SIGNAL(textChanged(const QString &)),
	  &m_domain_filter_timer,
	  SLOT(start(void)));
  connect(m_ui.periodically_purge,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_periodically_purge_temporary_domains(bool)));
  connect(m_ui.toggle_shown,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_toggle_shown(void)));
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
  connect(m_ui.tree,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slot_item_selection_changed(void)));
  new QShortcut(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_find(void)));
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
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
(const QList<QTreeWidgetItem *> &list)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_cookie_store && m_cookies)
    disconnect(m_cookie_store,
	       SIGNAL(cookieRemoved(const QNetworkCookie &)),
	       m_cookies,
	       SLOT(slot_cookie_removed(const QNetworkCookie &)));

  QList<QNetworkCookie> cookies;
  QStringList domains;

  for(auto item : list)
    {
      if(!item)
	continue;

      m_child_items.remove(item->text(0));
      m_top_level_items.remove(item->text(0));

      foreach(auto i, item->takeChildren())
	if(i)
	  {
	    QList<QNetworkCookie> cookie
	      (QNetworkCookie::
	       parseCookies(i->data(1, Qt::UserRole).toByteArray()));

	    if(!cookie.isEmpty())
	      {
		if(m_cookie_store)
		  m_cookie_store->deleteCookie(cookie.at(0));

		cookies << cookie.at(0);
	      }

	    delete i;
	  }

      item = m_ui.tree->takeTopLevelItem(m_ui.tree->indexOfTopLevelItem(item));

      if(item)
	{
	  auto cookie
	    (QNetworkCookie::
	     parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	  if(!cookie.isEmpty())
	    {
	      if(m_cookie_store)
		m_cookie_store->deleteCookie(cookie.at(0));

	      cookies << cookie.at(0);
	    }
	  else
	    domains << item->text(0);
	}

      delete item;
    }

  emit delete_items(cookies, domains);

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

void dooble_cookies_window::showNormal(QWidget *parent)
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

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(parent, this);

  QMainWindow::showNormal();
}

void dooble_cookies_window::slot_cookie_removed(const QNetworkCookie &cookie)
{
  auto hash(m_child_items.value(cookie.domain()));

  if(hash.isEmpty())
    {
      auto item = m_top_level_items.value(cookie.domain());

      if(item && item->checkState(0) != Qt::Checked)
	{
	  m_top_level_items.remove(cookie.domain());
	  delete m_ui.tree->takeTopLevelItem
	    (m_ui.tree->indexOfTopLevelItem(item));
	}

      return;
    }

  auto item = hash.value(dooble_cookies::identifier(cookie), nullptr);

  if(item && item->parent())
    delete item->parent()->takeChild(item->parent()->indexOfChild(item));

  hash.remove(dooble_cookies::identifier(cookie));

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

void dooble_cookies_window::slot_cookies_added
(const QList<QNetworkCookie> &cookies,
 const QList<bool> &is_favorites)
{
  disconnect(m_ui.tree,
	     SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	     this,
	     SLOT(slot_item_changed(QTreeWidgetItem *, int)));

  for(int i = 0; i < cookies.size(); i++)
    {
      auto is_favorite = is_favorites.value(i);
      const auto &cookie(cookies.at(i));

      if(!m_top_level_items.contains(cookie.domain()))
	{
	  auto item = new QTreeWidgetItem
	    (m_ui.tree, QStringList() << cookie.domain());
	  auto text(m_ui.domain_filter->text().toLower().trimmed());

	  if(is_favorite)
	    item->setCheckState(0, Qt::Checked);
	  else
	    item->setCheckState(0, Qt::Unchecked);

	  item->setData(1, Qt::UserRole, cookie.toRawForm());
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);

	  if(!text.isEmpty())
	    item->setHidden(!cookie.domain().contains(text));

	  m_top_level_items[cookie.domain()] = item;
	  m_ui.tree->addTopLevelItem(item);
	}

      if(!cookie.name().isEmpty())
	{
	  auto hash(m_child_items.value(cookie.domain()));

	  if(!hash.contains(dooble_cookies::identifier(cookie)))
	    {
	      auto item = new QTreeWidgetItem
		(m_top_level_items[cookie.domain()],
		 QStringList() << "" << cookie.name());

	      hash[dooble_cookies::identifier(cookie)] = item;
	      item->setData(1, Qt::UserRole, cookie.toRawForm());
	      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	      m_child_items[cookie.domain()] = hash;
	      m_top_level_items[cookie.domain()]->addChild(item);
	    }
	}
    }

  m_ui.tree->sortItems
    (m_ui.tree->sortColumn(), m_ui.tree->header()->sortIndicatorOrder());
  m_ui.tree->resizeColumnToContents(0);
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
}

void dooble_cookies_window::slot_cookies_cleared(void)
{
  m_child_items.clear();

  if(m_cookie_store)
    m_cookie_store->deleteAllCookies();

  m_top_level_items.clear();
  m_ui.domain_filter->clear();
  m_ui.tree->clear();
}

void dooble_cookies_window::slot_delete_selected(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_cookie_store && m_cookies)
    disconnect(m_cookie_store,
	       SIGNAL(cookieRemoved(const QNetworkCookie &)),
	       m_cookies,
	       SLOT(slot_cookie_removed(const QNetworkCookie &)));

  QList<QNetworkCookie> cookies;
  QStringList domains;
  auto list(m_ui.tree->selectedItems());

  for(auto item : list)
    {
      if(!item)
	continue;

      if(m_ui.tree->indexOfTopLevelItem(item) != -1)
	{
	  m_child_items.remove(item->text(0));
	  m_top_level_items.remove(item->text(0));

	  foreach(auto i, item->takeChildren())
	    if(i)
	      {
		auto cookie
		  (QNetworkCookie::
		   parseCookies(i->data(1, Qt::UserRole).toByteArray()));

		if(!cookie.isEmpty())
		  {
		    if(m_cookie_store)
		      m_cookie_store->deleteCookie(cookie.at(0));

		    cookies << cookie.at(0);
		  }

		delete i;
	      }

	  item = m_ui.tree->takeTopLevelItem
	    (m_ui.tree->indexOfTopLevelItem(item));

	  if(item)
	    {
	      auto cookie
		(QNetworkCookie::
		 parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	      if(!cookie.isEmpty())
		{
		  if(m_cookie_store)
		    m_cookie_store->deleteCookie(cookie.at(0));

		  cookies << cookie.at(0);
		}
	      else
		domains << item->text(0);
	    }

	  delete item;
	}
      else
	{
	  auto cookie
	    (QNetworkCookie::
	     parseCookies(item->data(1, Qt::UserRole).toByteArray()));

	  if(!cookie.isEmpty())
	    {
	      auto hash(m_child_items.value(cookie.at(0).domain()));

	      hash.remove(dooble_cookies::identifier(cookie.at(0)));

	      if(hash.isEmpty())
		m_child_items.remove(cookie.at(0).domain());
	      else
		m_child_items[cookie.at(0).domain()] = hash;

	      if(m_cookie_store)
		m_cookie_store->deleteCookie(cookie.at(0));

	      cookies << cookie.at(0);
	    }

	  if(item->parent())
	    delete item->parent()->takeChild
	      (item->parent()->indexOfChild(item));
	}
    }

  emit delete_items(cookies, domains);

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
      auto item = m_ui.tree->topLevelItem(i);

      if(!(!item || item->isHidden()))
	list << item;
    }

  delete_top_level_items(list);
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_delete_unchecked(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTreeWidgetItem *> list;

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      auto item = m_ui.tree->topLevelItem(i);

      if(item && item->checkState(0) == Qt::Unchecked && !item->isHidden())
	list << item;
    }

  delete_top_level_items(list);
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_domain_filter_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto text(m_ui.domain_filter->text().toLower().trimmed());

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      auto item = m_ui.tree->topLevelItem(i);

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
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_find(void)
{
  m_ui.domain_filter->selectAll();
  m_ui.domain_filter->setFocus();
}

void dooble_cookies_window::slot_item_changed(QTreeWidgetItem *item, int column)
{
  if(column != 0 || !item)
    return;
  else if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto database_name("dooble_cookies_window");

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_cookies.db");

    if(db.open())
      {
	dooble_cookies::create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_cookies_domains "
	   "(domain, domain_digest, favorite_digest) VALUES (?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item->text(0).toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(item->text(0)).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(item->checkState(0) == Qt::Checked ?
		QByteArray("true") : QByteArray("false")).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_cookies_window::slot_item_selection_changed(void)
{
  auto item = m_ui.tree->currentItem();

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

  auto cookie
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
      auto item = m_ui.tree->topLevelItem(i);

      if(!(!item || item->checkState(0) == Qt::Checked))
	list << item;
    }

  delete_top_level_items(list);
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_settings_applied(void)
{
  if(dooble_settings::setting("denote_private_widgets").toBool())
    statusBar()->setVisible(m_is_private);
  else
    statusBar()->setVisible(false);
}

void dooble_cookies_window::slot_toggle_shown(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto state = m_ui.toggle_shown->property("state").toBool();

  m_ui.toggle_shown->setProperty("state", !state);
  m_ui.toggle_shown->setText
    (!state ? tr("&All Shown Checked") : tr("&All Shown Unchecked"));

  for(int i = 0; i < m_ui.tree->topLevelItemCount(); i++)
    {
      auto item = m_ui.tree->topLevelItem(i);

      if(!(!item || item->isHidden()))
	item->setCheckState(0, state ? Qt::Checked : Qt::Unchecked);
    }

  QApplication::restoreOverrideCursor();
}
