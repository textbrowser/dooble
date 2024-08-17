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
  m_ui.buttons->button(QDialogButtonBox::Ok)->setText(tr("&Execute!"));
  connect(m_ui.buttons->button(QDialogButtonBox::Ok),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_execute(void)));
  connect(m_ui.buttons->button(QDialogButtonBox::Save),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save(void)));
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));
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

void dooble_javascript::slot_execute(void)
{
  if(!m_page)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_page->runJavaScript(m_ui.text->toPlainText().trimmed());
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
	  (m_page->url().toEncoded());

	if(!bytes.isEmpty())
	  query.addBindValue(bytes.toBase64());
	else
	  goto done_label;

	query.addBindValue
	  (dooble::s_cryptography->hmac(m_page->url().toEncoded()).toBase64());
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
  m_ui.url->setText(url.toString());
  m_ui.url->setCursorPosition(0);

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      m_ui.text->clear();
      return;
    }

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
	  (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());

	if(query.exec() && query.next())
	  {
	    auto javascript
	      (QByteArray::fromBase64(query.value(0).toByteArray()));

	    javascript = dooble::s_cryptography->mac_then_decrypt(javascript);
	    m_ui.text->setPlainText(javascript);
	  }
	else
	  m_ui.text->clear();
      }
    else
      m_ui.text->clear();

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}
