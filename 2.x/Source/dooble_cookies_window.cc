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
#include <QSqlDatabase>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_cookies_window::dooble_cookies_window(QWidget *parent):
  QMainWindow(parent)
{
  m_domain_filter_timer.setInterval(1500);
  m_domain_filter_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.domain->setText("");
  m_ui.expiration_date->setText("");
  m_ui.name->setText("");
  m_ui.path->setText("");
  m_ui.tree->sortItems(0, Qt::AscendingOrder);
  m_ui.value->setText("");
  connect(&m_domain_filter_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_domain_filter_timer_timeout(void)));
  connect(m_ui.domain_filter,
	  SIGNAL(textChanged(const QString &)),
	  &m_domain_filter_timer,
	  SLOT(start(void)));
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
  connect(m_ui.tree,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slot_item_selection_changed(void)));
}

void dooble_cookies_window::filter(const QString &text)
{
  m_ui.domain_filter->setText(text);
}

void dooble_cookies_window::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("dooble_cookies_window_geometry").
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

  QMainWindow::showNormal();
}

void dooble_cookies_window::slot_cookie_added(const QNetworkCookie &cookie,
					      bool is_favorite)
{
  if(!dooble::s_cryptography)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(m_ui.tree,
	     SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	     this,
	     SLOT(slot_item_changed(QTreeWidgetItem *, int)));

  if(!m_top_level_items.contains(cookie.domain()))
    {
      QString text(m_ui.domain_filter->text().trimmed());
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
  else
    {
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
    }

  m_ui.tree->sortItems
    (m_ui.tree->sortColumn(), m_ui.tree->header()->sortIndicatorOrder());
  m_ui.tree->resizeColumnToContents(0);
  connect(m_ui.tree,
	  SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	  this,
	  SLOT(slot_item_changed(QTreeWidgetItem *, int)));
  QApplication::restoreOverrideCursor();
}

void dooble_cookies_window::slot_domain_filter_timer_timeout(void)
{
  QString text(m_ui.domain_filter->text().trimmed());

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

	query.exec("PRAGMA synchronous = OFF");
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
    return;

  QList<QNetworkCookie> cookie
    (QNetworkCookie::parseCookies(item->data(1, Qt::UserRole).toByteArray()));

  if(cookie.isEmpty())
    {
      m_ui.domain->setText(item->text(0));
      m_ui.expiration_date->setText("");
      m_ui.name->setText("");
      m_ui.path->setText("");
      m_ui.value->setText("");
      return;
    }

  if(item->flags() & Qt::ItemIsUserCheckable)
    {
      m_ui.domain->setText(cookie.at(0).domain());
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
      m_ui.expiration_date->setText(cookie.at(0).expirationDate().toString());
      m_ui.http_only->setChecked(cookie.at(0).isHttpOnly());
      m_ui.name->setText(cookie.at(0).name());
      m_ui.path->setText(cookie.at(0).path());
      m_ui.secure->setChecked(cookie.at(0).isSecure());
      m_ui.session->setChecked(cookie.at(0).isSessionCookie());
      m_ui.value->setText(cookie.at(0).value());
    }
}
