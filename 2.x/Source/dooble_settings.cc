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

#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QPushButton>
#include <QScopedPointer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_blocked_domains.h"
#include "dooble_cookies.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_hmac.h"
#include "dooble_pbkdf2.h"
#include "dooble_random.h"
#include "dooble_settings.h"

QAtomicInteger<quint64> dooble_settings::s_db_id;
QMap<QString, QVariant> dooble_settings::s_settings;
QReadWriteLock dooble_settings::s_settings_mutex;

dooble_settings::dooble_settings(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(&m_pbkdf2_future_watcher,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_pbkdf2_future_finished(void)));
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
  connect(m_ui.reset_credentials,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reset_credentials(void)));
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
  s_settings["access_new_tabs"] = true;
  s_settings["browsing_history_days"] = 15;
  s_settings["center_child_windows"] = true;
  s_settings["cookie_policy_index"] = 2;
  s_settings["icon_set"] = "SnipIcons";
  s_settings["javascript_block_popups"] = true;
  s_settings["main_menu_bar_visible"] = true;
  restore();
  prepare_icons();
}

QString dooble_settings::cookie_policy_string(int index)
{
  if(index == 0)
    return "do_not_save";
  else if(index == 1)
    return "save_all";
  else
    return "save_persistent_only";
}

QVariant dooble_settings::setting(const QString &key)
{
  QReadLocker lock(&s_settings_mutex);

  return s_settings.value(key);
}

bool dooble_settings::has_dooble_credentials(void)
{
  return !setting("authentication_iteration_count").isNull() &&
    !setting("authentication_salt").isNull() &&
    !setting("authentication_salted_password").isNull();
}

bool dooble_settings::set_setting(const QString &key, const QVariant &value)
{
  if(key.trimmed().isEmpty())
    return false;
  else if(value.isNull())
    {
      QWriteLocker lock(&s_settings_mutex);

      s_settings.remove(key);
      return false;
    }

  QWriteLocker lock(&s_settings_mutex);

  s_settings[key.trimmed()] = value;
  lock.unlock();

  QString database_name
    (QString("dooble_settings_%1").arg(s_db_id.fetchAndAddOrdered(1)));
  bool ok = false;

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
	query.exec("PRAGMA synchronous = NORMAL");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_settings (key, value) VALUES (?, ?)");
	query.addBindValue(key.trimmed());
	query.addBindValue(value.toString());
	ok = query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  return ok;
}

void dooble_settings::closeEvent(QCloseEvent *event)
{
  m_ui.password_1->clear();
  m_ui.password_2->clear();
  QMainWindow::closeEvent(event);
}

void dooble_settings::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_settings::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.cache->setIcon(QIcon(QString(":/%1/64/cache.png").arg(icon_set)));
  m_ui.display->setIcon(QIcon(QString(":/%1/64/display.png").arg(icon_set)));
  m_ui.history->setIcon(QIcon(QString(":/%1/64/history.png").arg(icon_set)));
  m_ui.privacy->setIcon(QIcon(QString(":/%1/64/privacy.png").arg(icon_set)));
  m_ui.web->setIcon(QIcon(QString(":/%1/64/webengine.png").arg(icon_set)));
  m_ui.windows->setIcon(QIcon(QString(":/%1/64/windows.png").arg(icon_set)));
}

void dooble_settings::prepare_proxy(bool save)
{
  QNetworkProxyFactory::setUseSystemConfiguration(false);

  if(m_ui.proxy_type->currentIndex() == 1 || // None
     m_ui.proxy_type->currentIndex() == 3)   // System
    {
      if(m_ui.proxy_type->currentIndex() == 1)
	{
	  QNetworkProxy proxy(QNetworkProxy::NoProxy);

	  QNetworkProxy::setApplicationProxy(proxy);
	}
      else
	QNetworkProxyFactory::setUseSystemConfiguration(true);

      m_ui.proxy_host->setText("");
      m_ui.proxy_password->setText("");
      m_ui.proxy_port->setValue(0);
      m_ui.proxy_user->setText("");
    }
  else
    {
      QNetworkProxy proxy;

      proxy.setHostName(m_ui.proxy_host->text().trimmed());
      proxy.setPassword(m_ui.proxy_password->text().trimmed());
      proxy.setPort(static_cast<quint16> (m_ui.proxy_port->value()));

      if(m_ui.proxy_type->currentIndex() == 0)
	proxy.setType(QNetworkProxy::HttpProxy);
      else
	proxy.setType(QNetworkProxy::Socks5Proxy);

      proxy.setUser(m_ui.proxy_user->text().trimmed());
      QNetworkProxy::setApplicationProxy(proxy);
    }

  if(save)
    {
      set_setting("proxy_host", m_ui.proxy_host->text().trimmed());
      set_setting("proxy_password", m_ui.proxy_password->text());
      set_setting("proxy_port", m_ui.proxy_port->value());
      set_setting("proxy_type_index", m_ui.proxy_type->currentIndex());
      set_setting("proxy_user", m_ui.proxy_user->text().trimmed());
    }
}

void dooble_settings::remove_setting(const QString &key)
{
  if(key.trimmed().isEmpty())
    return;

  QWriteLocker lock(&s_settings_mutex);

  s_settings.remove(key.trimmed());
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

	query.prepare("DELETE FROM dooble_settings WHERE key = ?");
	query.addBindValue(key.trimmed());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_settings::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
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

  QWriteLocker lock(&s_settings_mutex);

  m_ui.access_new_tabs->setChecked
    (s_settings.value("access_new_tabs", true).toBool());
  m_ui.animated_scrolling->setChecked
    (s_settings.value("animated_scrolling", false).toBool());
  m_ui.browsing_history->setValue
    (qBound(m_ui.browsing_history->minimum(),
	    s_settings.value("browsing_history_days", 15).toInt(),
	    m_ui.browsing_history->maximum()));
  m_ui.cache_size->setValue(s_settings.value("cache_size", 0).toInt());
  m_ui.cache_type->setCurrentIndex
    (qBound(0,
	    s_settings.value("cache_type_index", 0).toInt(),
	    m_ui.cache_type->count() - 1));
  m_ui.center_child_windows->setChecked
    (s_settings.value("center_child_windows", true).toBool());
  m_ui.cookie_policy->setCurrentIndex
    (qBound(0,
	    s_settings.value("cookie_policy_index", 2).toInt(),
	    m_ui.cookie_policy->count() - 1));
  m_ui.denote_private_tabs->setChecked
    (s_settings.value("denote_private_tabs", false).toBool());
  m_ui.iterations->setValue
    (s_settings.value("authentication_iteration_count", 15000).toInt());
  m_ui.javascript->setChecked(s_settings.value("javascript", true).toBool());
  m_ui.javascript_access_clipboard->setChecked
    (s_settings.value("javascript_access_clipboard", false).toBool());
  m_ui.javascript_block_popups->setChecked
    (s_settings.value("javascript_block_popups", true).toBool());
  m_ui.javascript_popups->setChecked
    (s_settings.value("javascript_popups", true).toBool());
  m_ui.main_menu_bar_visible->setChecked
    (s_settings.value("main_menu_bar_visible", true).toBool());
  m_ui.pages->setCurrentIndex
    (qBound(0,
	    s_settings.value("settings_page_index", 0).toInt(),
	    m_ui.pages->count() - 1));
  m_ui.proxy_host->setText(s_settings.value("proxy_host").toString().trimmed());
  m_ui.proxy_password->setText
    (s_settings.value("proxy_password").toString().trimmed());
  m_ui.proxy_port->setValue(s_settings.value("proxy_port", 0).toInt());
  m_ui.proxy_type->setCurrentIndex
    (qBound(0,
	    s_settings.value("proxy_type_index", 1).toInt(),
	    m_ui.proxy_type->count() - 1));
  m_ui.proxy_user->setText(s_settings.value("proxy_user").toString().trimmed());
  m_ui.save_geometry->setChecked
    (s_settings.value("save_geometry", false).toBool());
  m_ui.theme->setCurrentIndex
    (qBound(0,
	    s_settings.value("icon_set_index", 1).toInt(),
	    m_ui.theme->count() - 1));

  if(m_ui.theme->currentIndex() == 0)
    s_settings["icon_set"] = "BlueBits";
  else if(m_ui.theme->currentIndex() == 1)
    s_settings["icon_set"] = "Google Material Design";
  else
    s_settings["icon_set"] = "SnipIcons";

  m_ui.xss_auditing->setChecked
    (s_settings.value("xss_auditing", false).toBool());
  lock.unlock();
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows,
     m_ui.javascript_popups->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());

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

  prepare_proxy(false);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::save_settings(void)
{
  if(setting("save_geometry").toBool())
    set_setting("settings_geometry", saveGeometry().toBase64());
}

void dooble_settings::show(void)
{
  if(!isVisible())
    restore();

  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble_settings::showNormal(void)
{
  if(!isVisible())
    restore();

  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_settings::show_panel(dooble_settings::Panels panel)
{
  switch(panel)
    {
    case DISPLAY_PANEL:
      m_ui.display->click();
      break;
    case CACHE_PANEL:
      m_ui.cache->click();
      break;
    case HISTORY_PANEL:
      m_ui.history->click();
      break;
    case PRIVACY_PANEL:
      m_ui.privacy->click();
      break;
    case WEB_PANEL:
      m_ui.web->click();
      break;
    case WINDOWS_PANEL:
      m_ui.windows->click();
      break;
    default:
      m_ui.display->click();
    }
}

void dooble_settings::slot_apply(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows,
     m_ui.javascript_popups->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());
  prepare_proxy(true);
  set_setting("access_new_tabs", m_ui.access_new_tabs->isChecked());
  set_setting("animated_scrolling", m_ui.animated_scrolling->isChecked());
  set_setting("browsing_history_days", m_ui.browsing_history->value());
  set_setting("cache_size", m_ui.cache_size->value());
  set_setting("cache_type_index", m_ui.cache_type->currentIndex());
  set_setting("center_child_windows", m_ui.center_child_windows->isChecked());
  set_setting("cookie_policy_index", m_ui.cookie_policy->currentIndex());
  set_setting("denote_private_tabs", m_ui.denote_private_tabs->isChecked());
  set_setting("icon_set_index", m_ui.theme->currentIndex());

  {
    QWriteLocker locker(&s_settings_mutex);

    if(m_ui.theme->currentIndex() == 0)
      s_settings["icon_set"] = "BlueBits";
    else if(m_ui.theme->currentIndex() == 1)
      s_settings["icon_set"] = "Google Material Design";
    else
      s_settings["icon_set"] = "SnipIcons";
  }

  set_setting
    ("javascript_block_popups", m_ui.javascript_block_popups->isChecked());
  set_setting("main_menu_bar_visible", m_ui.main_menu_bar_visible->isChecked());
  set_setting("save_geometry", m_ui.save_geometry->isChecked());
  set_setting("xss_auditing", m_ui.xss_auditing->isChecked());
  prepare_icons();
  QApplication::restoreOverrideCursor();
  emit applied();
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

void dooble_settings::slot_pbkdf2_future_finished(void)
{
  m_ui.password_1->clear();
  m_ui.password_2->clear();

  bool was_canceled = false;

  if(m_pbkdf2_dialog)
    {
      if(m_pbkdf2_dialog->wasCanceled())
	was_canceled = true;

      m_pbkdf2_dialog->cancel();
      m_pbkdf2_dialog->deleteLater();
    }

  if(!was_canceled)
    {
      QList<QByteArray> list(m_pbkdf2_future.result());
      bool ok = true;

      if(list.size() == 4)
	{
	  ok &= set_setting
	    ("authentication_iteration_count", list.at(1).toInt());
	  ok &= set_setting("authentication_salt", list.at(3).toHex());
	  ok &= set_setting
	    ("authentication_salted_password",
	     QCryptographicHash::hash(list.at(2) + list.at(3),
				      QCryptographicHash::Sha3_512).toHex());

	  if(ok)
	    {
	      dooble::s_cryptography->setAuthenticated(true);
	      dooble::s_cryptography->setKeys
		(list.at(0).mid(0, 64), list.at(0).mid(64, 32));
	      emit dooble_credentials_created();
	    }
	}
      else
	ok = false;

      if(ok)
	QMessageBox::information
	  (this,
	   tr("Dooble: Information"),
	   tr("Your credentials have been prepared."));
      else
	QMessageBox::critical
	  (this,
	   tr("Dooble: Error"),
	   tr("Credentials could not be generated. "
	      "This is a curious problem."));
    }
}

void dooble_settings::slot_reset_credentials(void)
{
  if(!dooble_settings::has_dooble_credentials())
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to reset your permanent "
		"credentials? New session-only credentials "
		"will be generated and database data will be removed."));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Remove settings.
  */

  m_ui.iterations->setValue(m_ui.iterations->minimum());
  remove_setting("authentication_iteration_count");
  remove_setting("authentication_salt");
  remove_setting("authentication_salted_password");
  emit dooble_credentials_authenticated(false);

  /*
  ** Generate temporary credentials.
  */

  QByteArray random_bytes(dooble_random::random_bytes(96));

  dooble::s_cryptography->setAuthenticated(false);
  dooble::s_cryptography->setKeys
    (random_bytes.mid(0, 64), random_bytes.mid(64, 32));

  /*
  ** Purge existing database data.
  */

  dooble_blocked_domains::purge();
  dooble_cookies::purge();
  dooble_favicons::purge();
  dooble_history::purge();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_save_credentials(void)
{
  if(m_pbkdf2_dialog || m_pbkdf2_future.isRunning())
    return;

  QString password1(m_ui.password_1->text());
  QString password2(m_ui.password_2->text());

  if(password1.isEmpty())
    {
      m_ui.password_1->setFocus();
      QMessageBox::critical
	(this, tr("Dooble: User Error"), tr("Empty password(s)."));
      return;
    }
  else if(password1 != password2)
    {
      m_ui.password_1->selectAll();
      m_ui.password_1->setFocus();
      QMessageBox::critical
	(this, tr("Dooble: User Error"), tr("Passwords are not equal."));
      return;
    }

  if(has_dooble_credentials())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to prepare new credentials? "
		    "Existing database data will be removed."));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	return;
    }

  QByteArray salt(dooble_random::random_bytes(64));

  if(salt.isEmpty())
    {
      m_ui.password_1->selectAll();
      m_ui.password_1->setFocus();
      QMessageBox::critical
	(this,
	 tr("Dooble: Error"),
	 tr("Salt-generation failure! This is a curious problem."));
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dooble_blocked_domains::purge();
  dooble_cookies::purge();
  dooble_favicons::purge();
  dooble_history::purge();
  QApplication::restoreOverrideCursor();
  m_pbkdf2_dialog = new QProgressDialog(this);
  m_pbkdf2_dialog->setCancelButtonText(tr("Interrupt"));
  m_pbkdf2_dialog->setLabelText(tr("Preparing credentials..."));
  m_pbkdf2_dialog->setMaximum(0);
  m_pbkdf2_dialog->setMinimum(0);
  m_pbkdf2_dialog->setWindowIcon(windowIcon());
  m_pbkdf2_dialog->setWindowModality(Qt::ApplicationModal);
  m_pbkdf2_dialog->setWindowTitle(tr("Dooble: Preparing Credentials"));

  QScopedPointer<dooble_pbkdf2> pbkdf2;

  pbkdf2.reset(new dooble_pbkdf2(password1.toUtf8(),
				 salt,
				 m_ui.iterations->value(),
				 1024));
  m_pbkdf2_future = QtConcurrent::run
    (pbkdf2.data(), &dooble_pbkdf2::pbkdf2, &dooble_hmac::sha3_512_hmac);
  m_pbkdf2_future_watcher.setFuture(m_pbkdf2_future);
  connect(m_pbkdf2_dialog,
	  SIGNAL(canceled(void)),
	  pbkdf2.data(),
	  SLOT(slot_interrupt(void)));
  m_pbkdf2_dialog->exec();
}
