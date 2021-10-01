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
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_style_sheet.h"
#include "dooble_web_engine_page.h"

/*
** body { -webkit-transform: rotate(180deg); }
*/

QMap<QPair<QString, QUrl>, QString> dooble_style_sheet::s_style_sheets;

dooble_style_sheet::dooble_style_sheet(dooble_web_engine_page *web_engine_page,
				       QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  m_web_engine_page = web_engine_page;
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
  connect(m_ui.names,
	  SIGNAL(itemSelectionChanged(void)),
	  this,
	  SLOT(slot_item_selection_changed(void)));
  connect(m_ui.remove,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove(void)));
  populate();
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
}

dooble_style_sheet::dooble_style_sheet(void)
{
  m_web_engine_page = nullptr;
}

void dooble_style_sheet::inject(dooble_web_engine_page *web_engine_page)
{
  if(!web_engine_page)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QMapIterator<QPair<QString, QUrl>, QString> it(s_style_sheets);

  while(it.hasNext())
    {
      it.next();

      if(it.key().second == web_engine_page->simplified_url())
	{
	  auto style_sheet
	    (QString::fromLatin1("(function() {"
				 "css = document.createElement('style');"
				 "css.id = '%1';"
				 "css.type = 'text/css';"
				 "css.innerText = '%2';"
				 "document.head.appendChild(css);"
				 "})()").
	     arg(it.key().first).
	     arg(it.value()));
	  QWebEngineScript web_engine_script;

	  web_engine_script.setInjectionPoint(QWebEngineScript::DocumentReady);
	  web_engine_script.setName(it.key().first);
	  web_engine_script.setRunsOnSubFrames(true);
	  web_engine_script.setSourceCode(style_sheet);
	  web_engine_script.setWorldId(QWebEngineScript::ApplicationWorld);
	  web_engine_page->runJavaScript
	    (style_sheet, QWebEngineScript::ApplicationWorld);
	  web_engine_page->scripts().insert(web_engine_script);
	}
      else
	{
	  auto style_sheet
	    (QString::fromLatin1("(function() {"
				 "var element = document.getElementById('%1');"
				 "if(element) element.outerHTML = '';"
				 "delete element;})()").arg(it.key().first));

	  web_engine_page->runJavaScript
	    (style_sheet, QWebEngineScript::ApplicationWorld);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	  web_engine_page->scripts().remove
	    (web_engine_page->scripts().findScript(it.key().first));
#else
	  web_engine_page->scripts().remove
	    (web_engine_page->scripts().find(it.key().first));
#endif
	}
    }

  QApplication::restoreOverrideCursor();
}

void dooble_style_sheet::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QDialog::keyPressEvent(event);
}

void dooble_style_sheet::populate(void)
{
  if(!m_web_engine_page)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.names->clear();

  QMapIterator<QPair<QString, QUrl>, QString> it(s_style_sheets);

  while(it.hasNext())
    {
      it.next();

      if(it.key().second == m_web_engine_page->simplified_url())
	m_ui.names->addItem(it.key().first);
    }

  m_ui.names->sortItems();
  m_ui.names->setCurrentRow(0);
  QApplication::restoreOverrideCursor();
}

void dooble_style_sheet::purge(void)
{
  s_style_sheets.clear();

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_style_sheets.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_style_sheets");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_style_sheet::slot_add(void)
{
  if(!m_web_engine_page)
    return;

  auto name(m_ui.name->text().trimmed());

  if(m_ui.style_sheet->toPlainText().trimmed().isEmpty() || name.isEmpty())
    return;

  auto style_sheet
    (QString::fromLatin1("(function() {"
			 "css = document.createElement('style');"
			 "css.id = '%1';"
			 "css.type = 'text/css';"
			 "css.innerText = '%2';"
			 "document.head.appendChild(css);"
			 "})()").
     arg(name).
     arg(m_ui.style_sheet->toPlainText().trimmed()));
  QWebEngineScript web_engine_script;

  web_engine_script.setInjectionPoint(QWebEngineScript::DocumentReady);
  web_engine_script.setName(name);
  web_engine_script.setRunsOnSubFrames(true);
  web_engine_script.setSourceCode(style_sheet);
  web_engine_script.setWorldId(QWebEngineScript::ApplicationWorld);

  if(m_ui.names->findItems(name, Qt::MatchExactly).isEmpty())
    {
      m_ui.names->addItem(name);
      m_ui.names->sortItems();
    }

  m_ui.style_sheet->setPlainText(m_ui.style_sheet->toPlainText().trimmed());
  m_web_engine_page->runJavaScript
    (style_sheet, QWebEngineScript::ApplicationWorld);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_web_engine_page->scripts().remove
    (m_web_engine_page->scripts().findScript(name));
#else
  m_web_engine_page->scripts().remove(m_web_engine_page->scripts().find(name));
#endif
  m_web_engine_page->scripts().insert(web_engine_script);
  s_style_sheets
    [QPair<QString, QUrl> (name, m_web_engine_page->simplified_url())] =
    m_ui.style_sheet->toPlainText().trimmed();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_style_sheets.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_style_sheets ("
		   "name TEXT NOT NULL, "
		   "name_digest TEXT NOT NULL, "
		   "style_sheet TEXT NOT NULL, "
		   "url TEXT NOT NULL, "
		   "url_digest TEXT NOT NULL, "
		   "PRIMARY KEY(name_digest, url_digest))");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_style_sheets "
	   "(name, name_digest, style_sheet, url, url_digest) "
	   "VALUES (?, ?, ?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac(name.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(name.toUtf8()).toBase64());
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_ui.style_sheet->toPlainText().trimmed().toLatin1());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	bytes = dooble::s_cryptography->encrypt_then_mac
	  (m_web_engine_page->simplified_url().toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(m_web_engine_page->simplified_url().toEncoded()).toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_style_sheet::slot_item_selection_changed(void)
{
  auto list(m_ui.names->selectedItems());

  if(list.isEmpty() || !list.at(0) || !m_web_engine_page)
    {
      m_ui.name->clear();
      m_ui.style_sheet->clear();
      return;
    }

  m_ui.name->setText(list.at(0)->text());
  m_ui.style_sheet->setPlainText
    (s_style_sheets.
     value(QPair<QString, QUrl> (m_ui.name->text(),
				 m_web_engine_page->simplified_url())));
}

void dooble_style_sheet::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_style_sheets.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT name, style_sheet, url, OID FROM "
		      "dooble_style_sheets"))
	  while(query.next())
	    {
	      auto name
		(QByteArray::fromBase64(query.value(0).toByteArray()));
	      auto style_sheet
		(QByteArray::fromBase64(query.value(1).toByteArray()));
	      auto url
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      name = dooble::s_cryptography->mac_then_decrypt(name);
	      style_sheet = dooble::s_cryptography->mac_then_decrypt
		(style_sheet);
	      url = dooble::s_cryptography->mac_then_decrypt(url);

	      if(name.isEmpty() || style_sheet.isEmpty() || url.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db, "dooble_style_sheets", query.value(3).toLongLong());
		  continue;
		}

	      s_style_sheets
		[QPair<QString, QUrl> (name, QUrl::fromEncoded(url))] =
		style_sheet;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  emit populated();
}

void dooble_style_sheet::slot_remove(void)
{
  if(!m_web_engine_page)
    return;

  auto list(m_ui.names->selectedItems());

  if(list.isEmpty() || !list.at(0))
    return;

  auto name(list.at(0)->text());
  auto style_sheet
    (QString::fromLatin1("(function() {"
			 "var element = document.getElementById('%1');"
			 "if(element) element.outerHTML = '';"
			 "delete element;})()").arg(name));

  delete m_ui.names->takeItem(m_ui.names->row(list.at(0)));
  m_web_engine_page->runJavaScript
    (style_sheet, QWebEngineScript::ApplicationWorld);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_web_engine_page->scripts().remove
    (m_web_engine_page->scripts().findScript(name));
#else
  m_web_engine_page->scripts().remove(m_web_engine_page->scripts().find(name));
#endif
  s_style_sheets.remove
    (QPair<QString, QUrl> (name, m_web_engine_page->simplified_url()));

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_style_sheets.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("DELETE FROM dooble_style_sheets WHERE "
		      "name_digest = ? AND url_digest = ?");
	query.addBindValue
	  (dooble::s_cryptography->hmac(name.toUtf8()).toBase64());
	query.addBindValue
	  (dooble::s_cryptography->
	   hmac(m_web_engine_page->simplified_url().toEncoded()).toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
