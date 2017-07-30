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

#include <QDialogButtonBox>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QWebEngineProfile>

#include "dooble_settings.h"

QAtomicInteger<quint64> dooble_settings::s_db_id;
QMap<QString, QVariant> dooble_settings::s_settings;
QReadWriteLock dooble_settings::s_settings_mutex;

dooble_settings::dooble_settings(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(m_ui.buttonBox->button(QDialogButtonBox::Apply),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_apply(void)));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Close),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(close(void)));
  connect(m_ui.cache,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.clear_cache,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_clear_cache(void)));
  connect(m_ui.display,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.history,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.privacy,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.save_credentials,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save_credentials(void)));
  connect(m_ui.web,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.windows,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  restore();
  s_settings["center_child_windows"] = true;
  s_settings["icon_set"] = "Snipicons";
}

void dooble_settings::closeEvent(QCloseEvent *event)
{
  if(setting("save_geometry").toBool())
    set_setting("settings_geometry", saveGeometry().toBase64());

  QMainWindow::closeEvent(event);
}

void dooble_settings::restore(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name
    (QString("dooble_settings_%1").arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT key, value FROM dooble_settings"))
	  while(query.next())
	    {
	      QString key(query.value(0).toString().trimmed());
	      QString value(query.value(1).toString().trimmed());

	      if(key.isEmpty() || value.isEmpty())
		continue;

	      QWriteLocker lock(&s_settings_mutex);

	      s_settings[key] = value;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);

  QReadLocker lock(&s_settings_mutex);

  m_ui.cache_size->setValue(s_settings.value("cache_size", 0).toInt());
  m_ui.cache_type->setCurrentIndex
    (qBound(0,
	    s_settings.value("cache_type_index", 0).toInt(),
	    m_ui.cache_type->count() - 1));
  m_ui.center_child_windows->setChecked
    (s_settings.value("center_child_windows", true).toBool());
  m_ui.pages->setCurrentIndex
    (qBound(0,
	    s_settings.value("settings_page_index", 0).toInt(),
	    m_ui.pages->count() - 1));
  m_ui.save_geometry->setChecked
    (s_settings.value("save_geometry", false).toBool());
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  lock.unlock();

  static QList<QToolButton *> list(QList<QToolButton *> () << m_ui.cache
				                           << m_ui.display
				                           << m_ui.history
				                           << m_ui.privacy
				                           << m_ui.web
				                           << m_ui.windows);

  for(int i = 0; i < list.size(); i++)
    if(i != m_ui.pages->currentIndex())
      list.at(i)->setChecked(false);
    else
      list.at(i)->setChecked(true);

  QApplication::restoreOverrideCursor();
}

QVariant dooble_settings::setting(const QString &key)
{
  QReadLocker lock(&s_settings_mutex);

  return s_settings.value(key);
}

void dooble_settings::set_setting(const QString &key, const QVariant &value)
{
  if(key.trimmed().isEmpty())
    return;
  else if(value.isNull())
    {
      QWriteLocker lock(&s_settings_mutex);

      s_settings.remove(key);
      return;
    }

  QWriteLocker lock(&s_settings_mutex);

  s_settings[key.trimmed()] = value;
  lock.unlock();

  QString database_name
    (QString("dooble_settings_%1").arg(s_db_id.fetchAndAddOrdered(1)));

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS dooble_settings ("
		   "key TEXT NOT NULL PRIMARY KEY, "
		   "value TEXT NOT NULL)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_settings (key, value) VALUES (?, ?)");
	query.addBindValue(key.trimmed());
	query.addBindValue(value.toString());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_settings::show(void)
{
  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble_settings::showNormal(void)
{
  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_settings::slot_apply(void)
{
  /*
  ** Cache
  */

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  set_setting("cache_size", m_ui.cache_size->value());
  set_setting("cache_type_index", m_ui.cache_type->currentIndex());
  set_setting("center_child_windows", m_ui.center_child_windows->isChecked());
  set_setting("save_geometry", m_ui.save_geometry->isChecked());
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_clear_cache(void)
{
  QWebEngineProfile::defaultProfile()->clearHttpCache();
}

void dooble_settings::slot_page_button_clicked(void)
{
  QToolButton *tool_button = qobject_cast<QToolButton *> (sender());

  if(!tool_button)
    return;

  static QList<QToolButton *> list(QList<QToolButton *> () << m_ui.cache
				                           << m_ui.display
				                           << m_ui.history
				                           << m_ui.privacy
				                           << m_ui.web
				                           << m_ui.windows);

  for(int i = 0; i < list.size(); i++)
    if(list.at(i) != tool_button)
      list.at(i)->setChecked(false);
    else
      m_ui.pages->setCurrentIndex(i);

  set_setting("settings_page_index", m_ui.pages->currentIndex());
  tool_button->setChecked(true);
}

void dooble_settings::slot_save_credentials(void)
{
  if(m_pbkdf2_dialog)
    return;

  QString password1(m_ui.password_1->text());
  QString password2(m_ui.password_2->text());

  if(password1.isEmpty())
    {
      QMessageBox::critical
	(this, tr("Dooble: User Error"), tr("Empty password(s)."));
      return;
    }
  else if(password1 != password2)
    {
      QMessageBox::critical
	(this, tr("Dooble: User Error"), tr("Passwords are not equal."));
      return;
    }

  m_pbkdf2_dialog = new QProgressDialog(this);
  m_pbkdf2_dialog->setCancelButtonText(tr("Interrupt"));
  m_pbkdf2_dialog->setLabelText
    (tr("Preparing credentials... Please remain calm."));
  m_pbkdf2_dialog->setMaximum(0);
  m_pbkdf2_dialog->setMinimum(0);
  m_pbkdf2_dialog->setWindowIcon(windowIcon());
  m_pbkdf2_dialog->setWindowModality(Qt::ApplicationModal);
  m_pbkdf2_dialog->setWindowTitle(tr("Dooble: Preparing Credentials"));
  m_pbkdf2_dialog->exec();
  m_pbkdf2_dialog->deleteLater();
}
