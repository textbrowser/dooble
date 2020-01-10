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
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_database_utilities.h"
#include "dooble_style_sheet.h"

/*
** body { -webkit-transform: rotate(180deg); }
*/

dooble_style_sheet::dooble_style_sheet(QWebEnginePage *web_engine_page,
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
}

void dooble_style_sheet::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QDialog::keyPressEvent(event);
}

void dooble_style_sheet::purge(void)
{
  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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

  QString name(m_ui.name->text().trimmed());

  if(!m_ui.names->findItems(name, Qt::MatchExactly).isEmpty())
    return;

  if(m_ui.style_sheet->toPlainText().trimmed().isEmpty() || name.isEmpty())
    return;

  QString style_sheet
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
  m_ui.names->addItem(name);
  m_ui.names->sortItems();
  m_web_engine_page->runJavaScript
    (style_sheet, QWebEngineScript::ApplicationWorld);
  m_web_engine_page->scripts().insert(web_engine_script);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
		   "url_digest TEXT NOT NULL, "
		   "PRIMARY KEY(name_digest, url_digest))");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_style_sheets "
	   "(name_digest, style_sheet, url_digest) "
	   "VALUES (?, ?, ?, ?)");

	QByteArray bytes;

	bytes = dooble::s_cryptography->encrypt_then_mac(name.toUtf8());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(name.toUtf8()).toBase64());
	bytes = dooble::s_cryptography->encrypt_then_mac
	  (style_sheet.toLatin1());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(m_web_engine_page->url().toEncoded()).
	   toBase64());
	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_style_sheet::slot_item_selection_changed(void)
{
  QList<QListWidgetItem *> list(m_ui.names->selectedItems());

  if(list.isEmpty() || !list.at(0) || !m_web_engine_page)
    {
      m_ui.name->clear();
      m_ui.style_sheet->clear();
      return;
    }

  m_ui.name->setText(list.at(0)->text());

  QWebEngineScript script
    (m_web_engine_page->scripts().findScript(list.at(0)->text()));

  m_ui.style_sheet->setPlainText(script.sourceCode());
}

void dooble_style_sheet::slot_remove(void)
{
  if(!m_web_engine_page)
    return;

  QList<QListWidgetItem *> list(m_ui.names->selectedItems());

  if(list.isEmpty() || !list.at(0))
    return;

  QString name(list.at(0)->text());
  QString style_sheet
    (QString::fromLatin1("(function() {"
			 "var element = document.getElementById('%1');"
			 "element.outerHTML = '';"
			 "delete element;})()").arg(name));

  delete m_ui.names->takeItem(m_ui.names->row(list.at(0)));
  m_web_engine_page->runJavaScript
    (style_sheet, QWebEngineScript::ApplicationWorld);
  m_web_engine_page->scripts().remove
    (m_web_engine_page->scripts().findScript(name));

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
	  (dooble::s_cryptography->hmac(m_web_engine_page->url().toEncoded()).
	   toBase64());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
