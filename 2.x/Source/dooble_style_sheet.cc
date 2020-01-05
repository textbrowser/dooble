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
#include "dooble_database_utilities.h"
#include "dooble_style_sheet.h"

dooble_style_sheet::dooble_style_sheet(QWebEnginePage *web_engine_page,
				       QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  m_web_engine_page = web_engine_page;
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
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

void dooble_style_sheet::slot_add(void)
{
  if(!m_web_engine_page)
    return;

  QString name(m_ui.name->text().trimmed());

  if(name.isEmpty())
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
  m_web_engine_page->runJavaScript
    (style_sheet, QWebEngineScript::ApplicationWorld);
  m_web_engine_page->scripts().insert(web_engine_script);

  QString database_name(dooble_database_utilities::database_name());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_style_sheets.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_style_sheets "
	   "(name, style_sheet, url_digest) "
	   "VALUES (?, ?, ?)");
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_style_sheet::slot_remove(void)
{
}
