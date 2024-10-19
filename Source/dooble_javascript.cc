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
#include <QPushButton>
#include <QShortcut>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_javascript.h"
#include "dooble_web_engine_page.h"

dooble_javascript::dooble_javascript(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  m_ui.buttons_1->button(QDialogButtonBox::Ok)->setText(tr("&Execute!"));
  m_ui.buttons_1->button(QDialogButtonBox::Retry)->setText(tr("&Refresh"));
  m_ui.buttons_2->button(QDialogButtonBox::Discard)->setText
    (tr("&Delete Selected"));
  m_ui.buttons_2->button(QDialogButtonBox::Retry)->setText(tr("&Refresh"));
  m_ui.buttons_2->button(QDialogButtonBox::Save)->setText(tr("&Save"));
  m_ui.url->setStyleSheet
    (QString("QLineEdit {padding-left: %1px;}").arg(20));
  connect(m_ui.buttons_1->button(QDialogButtonBox::Ok),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_execute(void)));
  connect(m_ui.buttons_1->button(QDialogButtonBox::Retry),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_refresh(void)));
  connect(m_ui.buttons_1->button(QDialogButtonBox::Save),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save(void)));
  connect(m_ui.buttons_2->button(QDialogButtonBox::Discard),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_others(void)));
  connect(m_ui.buttons_2->button(QDialogButtonBox::Retry),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_refresh_others(void)));
  connect(m_ui.buttons_2->button(QDialogButtonBox::Save),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save_others(void)));
  connect(m_ui.list,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slot_item_selection_changed(void)));
  m_script_injected_label = new QLabel(m_ui.url);
  m_script_injected_label->resize(QSize(32, 32));
  m_script_injected_label->setText("&#9989;");
  m_script_injected_label->setTextFormat(Qt::RichText);
  m_script_injected_label->setToolTip(tr("Script injected."));
  m_script_injected_label->setVisible(false);
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
  setModal(false);
}

void dooble_javascript::resizeEvent(QResizeEvent *event)
{
  QDialog::resizeEvent(event);
  m_script_injected_label->move
    (QPoint(10, qMax(1,
		     -m_script_injected_label->height() / 2 +
		     m_ui.url->height() / 2)));
  m_script_injected_label->raise();
}

void dooble_javascript::set_page(QWebEnginePage *page)
{
  if(m_page)
    return;
  else if(qobject_cast<dooble_web_engine_page *> (page))
    {
      connect(page,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(deleteLater(void)));
      connect(page,
	      SIGNAL(loadFinished(bool)),
	      this,
	      SLOT(slot_load_finished(bool)));
      connect(page,
	      SIGNAL(loadStarted(void)),
	      m_script_injected_label,
	      SLOT(hide(void)));
      connect(page,
	      SIGNAL(titleChanged(const QString &)),
	      this,
	      SLOT(slot_title_changed(const QString &)));
      connect(page,
	      SIGNAL(urlChanged(const QUrl &)),
	      this,
	      SLOT(slot_url_changed(const QUrl &)));
      m_page = qobject_cast<dooble_web_engine_page *> (page);
      setWindowTitle
	(m_page->title().trimmed().isEmpty() ?
	 tr("Dooble: JavaScript Console") :
	 tr("%1 - Dooble: JavaScript Console").arg(m_page->title().trimmed()));
      slot_url_changed(m_page->url());
    }
}

void dooble_javascript::slot_delete_others(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	foreach(auto item, m_ui.list->selectedItems())
	  {
	    query.prepare("DELETE FROM dooble_javascript WHERE url = ?");
	    query.addBindValue
	      (dooble::s_cryptography->
	       encrypt_then_mac(item->text().toUtf8()).toBase64());

	    if(query.exec())
	      delete m_ui.list->takeItem(m_ui.list->row(item));
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_javascript::slot_execute(void)
{
  if(m_page == nullptr || m_ui.text->toPlainText().trimmed().isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_page->runJavaScript
    (m_ui.text->toPlainText().trimmed(),
     [](const QVariant &value)
     {
       qDebug() << value.toString();
     });
  m_script_injected_label->setVisible(true);
  QApplication::restoreOverrideCursor();
}

void dooble_javascript::slot_load_finished(bool state)
{
  Q_UNUSED(state);
  slot_execute();
}

void dooble_javascript::slot_item_selection_changed(void)
{
  m_ui.edit->clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto item = m_ui.list->currentItem();

  if(!item)
    return;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare
	  ("SELECT javascript FROM dooble_javascript WHERE url_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(item->text()).toBase64());

	if(query.exec() && query.next())
	  {
	    auto javascript
	      (QByteArray::fromBase64(query.value(0).toByteArray()));

	    javascript = dooble::s_cryptography->mac_then_decrypt(javascript);
	    m_ui.edit->setPlainText(javascript.trimmed());
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_javascript::slot_refresh(void)
{
  if(!m_page)
    return;

  auto const state = m_script_injected_label->isVisible();

  slot_url_changed(m_page->url());
  m_script_injected_label->setVisible(state);
}

void dooble_javascript::slot_refresh_others(void)
{
  m_ui.edit->clear();
  m_ui.list->clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT url FROM dooble_javascript");

	if(query.exec())
	  {
	    while(query.next())
	      {
		auto url(QByteArray::fromBase64(query.value(0).toByteArray()));

		url = dooble::s_cryptography->mac_then_decrypt(url).trimmed();

		if(url.isEmpty() == false)
		  {
		    auto item = new QListWidgetItem(url);

		    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		    m_ui.list->addItem(item);
		  }
	      }

	    m_ui.list->scrollToTop();
	    m_ui.list->sortItems();
	    m_ui.list->setCurrentRow(0);
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_javascript::slot_save(void)
{
  if(!dooble::s_cryptography ||
     !dooble::s_cryptography->authenticated() ||
     !m_page)
    return;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_javascript ("
		   "javascript TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_javascript "
	   "(javascript, url, url_digest) VALUES (?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_ui.text->toPlainText().trimmed().toUtf8()).toBase64();
	query.addBindValue(bytes);
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_page->url().host().toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(m_page->url().host()).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_javascript::slot_save_others(void)
{
  auto item = m_ui.list->currentItem();

  if(!dooble::s_cryptography ||
     !dooble::s_cryptography->authenticated() ||
     !item)
    return;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_javascript ("
		   "javascript TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_javascript "
	   "(javascript, url, url_digest) VALUES (?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_ui.edit->toPlainText().trimmed().toUtf8()).toBase64();
	query.addBindValue(bytes);
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (item->text().trimmed().toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(item->text().trimmed()).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_javascript::slot_title_changed(const QString &title)
{
  setWindowTitle
    (title.trimmed().isEmpty() ?
     tr("Dooble: JavaScript Console") :
     tr("%1 - Dooble: JavaScript Console").arg(title.trimmed()));
}

void dooble_javascript::slot_url_changed(const QUrl &url)
{
  m_script_injected_label->setVisible(false);
  m_ui.text->clear();
  m_ui.url->setText(url.toString());
  m_ui.url->setCursorPosition(0);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_javascript.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare
	  ("SELECT javascript FROM dooble_javascript WHERE url_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(url.host()).toBase64());

	if(query.exec() && query.next())
	  {
	    auto javascript
	      (QByteArray::fromBase64(query.value(0).toByteArray()));

	    javascript = dooble::s_cryptography->mac_then_decrypt(javascript);
	    m_ui.text->setPlainText(javascript.trimmed());
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
