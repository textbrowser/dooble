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
#include <QMessageBox>
#include <QNetworkProxy>
#include <QSqlQuery>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cookies.h"
#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_hmac.h"
#include "dooble_pbkdf2.h"
#include "dooble_random.h"
#include "dooble_settings.h"

QAtomicInteger<quintptr> dooble_settings::s_db_id;
QHash<QUrl, char> dooble_settings::s_javascript_block_popup_exceptions;
QMap<QString, QVariant> dooble_settings::s_settings;
QReadWriteLock dooble_settings::s_settings_mutex;
QString dooble_settings::s_http_user_agent;
QStringList dooble_settings::s_spell_checker_dictionaries;

dooble_settings::dooble_settings(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(&m_pbkdf2_future_watcher,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_pbkdf2_future_finished(void)));
  connect(m_ui.allow_javascript_block_popup_exception,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_new_javascript_block_popup_exception(void)));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Apply),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_apply(void)));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Reset),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reset(void)));
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
  connect(m_ui.new_javascript_block_popup_exception,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_new_javascript_block_popup_exception(void)));
  connect(m_ui.privacy,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.proxy_type,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slot_proxy_type_changed(int)));
  connect(m_ui.remove_all_javascript_block_popup_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_all_javascript_block_popup_exceptions(void)));
  connect(m_ui.remove_selected_javascript_block_popup_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_selected_javascript_block_popup_exceptions(void)));
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
  m_ui.cookie_policy->setItemData
    (0, tr("Cookies are not saved to disk."), Qt::ToolTipRole);
  m_ui.cookie_policy->setItemData
    (1,
     tr("Persistent and session cookies are saved to and restored "
	"from disk."),
     Qt::ToolTipRole);
  m_ui.cookie_policy->setItemData
    (2,
     tr("Cookies marked persistent are saved to and restored from disk."),
     Qt::ToolTipRole);

  QString path(QDir::currentPath());

  path.append(QDir::separator());
  path.append("Translations");
  path.append(QDir::separator());
  path.append("dooble_" + QLocale::system().name() + ".qm");

  QFileInfo file_info(path);

  if(!file_info.isReadable())
    {
      m_ui.language->model()->setData(m_ui.language->model()->index(1, 0),
				      0,
				      Qt::UserRole - 1);
      m_ui.language_directory->setStyleSheet
	("QLabel {background-color: #f2dede; border: 1px solid #ebccd1;"
	 "color:#a94442;}");
      m_ui.language_directory->setText
	(tr("<b>Warning!</b> The file %1 is not readable. "
	    "The System option has been disabled.").
	 arg(file_info.absoluteFilePath()));
    }
  else if(file_info.size() <= 1024)
    {
      m_ui.language->model()->setData(m_ui.language->model()->index(1, 0),
				      0,
				      Qt::UserRole - 1);
      m_ui.language_directory->setStyleSheet
	("QLabel {background-color: #f2dede; border: 1px solid #ebccd1;"
	 "color:#a94442;}");
      m_ui.language_directory->setText
	(tr("<b>Warning!</b> The file %1 is perhaps incomplete. "
	    "The System option has been disabled.").
	 arg(file_info.absoluteFilePath()));
    }
  else
    m_ui.language_directory->setVisible(false);

  s_http_user_agent = QWebEngineProfile::defaultProfile()->httpUserAgent();
  s_settings["accepted_or_blocked_domains_mode"] = "block";
  s_settings["access_new_tabs"] = true;
  s_settings["auto_hide_tab_bar"] = false;
  s_settings["block_cipher_type"] = "AES-256";
  s_settings["block_cipher_type_index"] = 0;
  s_settings["browsing_history_days"] = 15;
  s_settings["center_child_windows"] = true;
  s_settings["cookie_policy_index"] = 2;
  s_settings["credentials_enabled"] = false;
  s_settings["denote_private_widgets"] = true;
  s_settings["do_not_track"] = true;
  s_settings["favorites_sort_index"] = 1; // Most Popular
  s_settings["icon_set"] = "Material Design";
  s_settings["javascript_block_popups"] = true;
  s_settings["language_index"] = 0;
  s_settings["main_menu_bar_visible"] = true;
  s_settings["pin_accepted_or_blocked_window"] = true;
  s_settings["pin_downloads_window"] = true;
  s_settings["pin_history_window"] = true;
  s_settings["pin_settings_window"] = true;
  s_settings["save_geometry"] = true;
  s_settings["status_bar_visible"] = true;
  s_settings["theme_color"] = "blue-grey";
  s_settings["theme_color_index"] = 0;
  s_settings["user_agent"] = QWebEngineProfile::defaultProfile()->
    httpUserAgent();
  s_settings["webgl"] = true;
  s_settings["zoom_frame_location_index"] = 0;
#ifdef Q_OS_MACOS
  s_spell_checker_dictionaries << "af_ZA"
			       << "an_ES"
			       << "ar"
			       << "be_BY"
			       << "bn_BD"
			       << "ca"
			       << "ca-valencia"
			       << "da_DK"
			       << "de_AT_frami"
			       << "de_CH_frami"
			       << "de_DE_frami"
			       << "en_AU"
			       << "en_CA"
			       << "en_GB"
			       << "en_US"
			       << "en_ZA"
			       << "es_ANY"
			       << "gd_GB"
			       << "gl_ES"
			       << "gug"
			       << "he_IL"
			       << "hi_IN"
			       << "hr_HR"
			       << "hu_HU"
			       << "is"
			       << "kmr_Latn"
			       << "lo_LA"
			       << "ne_NP"
			       << "nl_NL"
			       << "nb_NO"
			       << "nn_NO"
			       << "pt_BR"
			       << "pt_PT"
			       << "ro_RO"
			       << "si_LK"
			       << "sk_SK"
			       << "sr"
			       << "sr-Latn"
			       << "sw_TZ"
			       << "te_IN"
			       << "uk_UA"
			       << "vi_VN";
#else
  s_spell_checker_dictionaries << "af_ZA"
			       << "an_ES"
			       << "ar"
			       << "be_BY"
			       << "bn_BD"
			       << "br_FR"
			       << "bs_BA"
			       << "ca"
			       << "ca-valencia"
			       << "cs_CZ"
			       << "da_DK"
			       << "de_AT_frami"
			       << "de_CH_frami"
			       << "de_DE_frami"
			       << "el_GR"
			       << "en_AU"
			       << "en_CA"
			       << "en_GB"
			       << "en_US"
			       << "en_ZA"
			       << "es_ANY"
			       << "et_EE"
			       << "gd_GB"
			       << "gl_ES"
			       << "gug"
			       << "he_IL"
			       << "hi_IN"
			       << "hr_HR"
			       << "hu_HU"
			       << "is"
			       << "it_IT"
			       << "kmr_Latn"
			       << "lo_LA"
			       << "lt"
			       << "lv_LV"
			       << "ne_NP"
			       << "nl_NL"
			       << "nb_NO"
			       << "nn_NO"
			       << "oc_FR"
			       << "pl_PL"
			       << "pt_BR"
			       << "pt_PT"
			       << "ro_RO"
			       << "ru_RU"
			       << "si_LK"
			       << "sk_SK"
			       << "sl_SI"
			       << "sr"
			       << "sr-Latn"
			       << "sw_TZ"
			       << "te_IN"
			       << "uk_UA"
			       << "vi_VN";
#endif

  if(s_spell_checker_dictionaries.isEmpty())
    {
      m_ui.dictionaries_group_box->setEnabled(false);
      m_ui.dictionaries_group_box->setToolTip
	(tr("A valid list of dictionaries has not been prepared."));
    }
  else
    for(int i = 0; i < s_spell_checker_dictionaries.size(); i++)
      {
	QListWidgetItem *item = new QListWidgetItem
	  (s_spell_checker_dictionaries.at(i));

	item->setFlags(Qt::ItemIsEnabled |
		       Qt::ItemIsSelectable |
		       Qt::ItemIsUserCheckable);
	item->setCheckState(Qt::Unchecked);
	m_ui.dictionaries->addItem(item);
      }

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

QString dooble_settings::zoom_frame_location_string(int index)
{
  Q_UNUSED(index);
  return "popup_menu";
}

QVariant dooble_settings::setting(const QString &key)
{
  QReadLocker lock(&s_settings_mutex);

  if(!s_settings.contains(key))
    {
      QString database_name
	(QString("dooble_settings_%1").arg(s_db_id.fetchAndAddOrdered(1)));
      QString value("");

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_settings.db");

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.setForwardOnly(true);
	    query.prepare("SELECT value FROM dooble_settings WHERE key = ?");
	    query.addBindValue(key);

	    if(query.exec())
	      if(query.next())
		value = query.value(0).toString().trimmed();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
      return value;
    }

  return s_settings.value(key);
}

bool dooble_settings::has_dooble_credentials(void)
{
  return !setting("authentication_iteration_count").isNull() &&
    !setting("authentication_salt").isNull() &&
    !setting("authentication_salted_password").isNull() &&
    setting("credentials_enabled").toBool();
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

bool dooble_settings::site_has_javascript_block_popup_exception(const QUrl &url)
{
  return s_javascript_block_popup_exceptions.value(url, 0) == 1;
}

void dooble_settings::closeEvent(QCloseEvent *event)
{
  m_ui.password_1->clear();
  m_ui.password_2->clear();
  QMainWindow::closeEvent(event);
}

void dooble_settings::keyPressEvent(QKeyEvent *event)
{
  if(!parent())
    {
      if(event && event->key() == Qt::Key_Escape)
	close();

      QMainWindow::keyPressEvent(event);
    }
  else if(event)
    event->ignore();
}

void dooble_settings::new_javascript_block_popup_exception(const QUrl &url)
{
  if(s_javascript_block_popup_exceptions.contains(url))
    return;
  else if(url.isEmpty() || !url.isValid())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  m_ui.javascript_block_popups_exceptions->setRowCount
    (m_ui.javascript_block_popups_exceptions->rowCount() + 1);
  m_ui.new_javascript_block_popup_exception->clear();

  QTableWidgetItem *item = new QTableWidgetItem();

  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, url);
  item->setFlags(Qt::ItemIsEnabled |
		 Qt::ItemIsSelectable |
		 Qt::ItemIsUserCheckable);
  m_ui.javascript_block_popups_exceptions->setItem
    (m_ui.javascript_block_popups_exceptions->rowCount() - 1, 0, item);
  item = new QTableWidgetItem(url.toString());
  item->setData(Qt::UserRole, url);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.javascript_block_popups_exceptions->setItem
    (m_ui.javascript_block_popups_exceptions->rowCount() - 1, 1, item);
  m_ui.javascript_block_popups_exceptions->sortItems(1);
  connect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  QApplication::restoreOverrideCursor();
  save_javascript_block_popup_exception(url, true);
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

      m_ui.proxy_frame->setEnabled(false);
      m_ui.proxy_host->setText("");
      m_ui.proxy_password->setText("");
      m_ui.proxy_port->setValue(0);
      m_ui.proxy_user->setText("");
    }
  else
    {
      QNetworkProxy proxy;

      proxy.setHostName(m_ui.proxy_host->text().trimmed());
      proxy.setPassword(m_ui.proxy_password->text());
      proxy.setPort(static_cast<quint16> (m_ui.proxy_port->value()));

      if(m_ui.proxy_type->currentIndex() == 0)
	proxy.setType(QNetworkProxy::HttpProxy);
      else
	proxy.setType(QNetworkProxy::Socks5Proxy);

      proxy.setUser(m_ui.proxy_user->text().trimmed());
      QNetworkProxy::setApplicationProxy(proxy);
      m_ui.proxy_frame->setEnabled(true);
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

void dooble_settings::purge_database_data(void)
{
  dooble::s_accepted_or_blocked_domains->purge();
  dooble::s_certificate_exceptions->purge();
  dooble::s_downloads->purge();
  dooble::s_history->purge_favorites();
  dooble::s_history->purge_history();
  dooble_certificate_exceptions_menu_widget::purge();
  dooble_cookies::purge();
  dooble_favicons::purge();
  purge_javascript_block_popup_exceptions();
  slot_remove_all_javascript_block_popup_exceptions();
}

void dooble_settings::purge_javascript_block_popup_exceptions(void)
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

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_javascript_block_popup_exceptions");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
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
  m_ui.cipher->setCurrentIndex
    (qBound(0,
	    s_settings.value("block_cipher_type_index", 0).toInt(),
	    m_ui.cipher->count() - 1));
  m_ui.cookie_policy->setCurrentIndex
    (qBound(0,
	    s_settings.value("cookie_policy_index", 2).toInt(),
	    m_ui.cookie_policy->count() - 1));
  m_ui.credentials->setChecked
    (s_settings.value("credentials_enabled", false).toBool());
  m_ui.denote_private_widgets->setChecked
    (s_settings.value("denote_private_widgets", true).toBool());
  m_ui.do_not_track->setChecked
    (s_settings.value("do_not_track", true).toBool());
  m_ui.icon_set->setCurrentIndex
    (qBound(0,
	    s_settings.value("icon_set_index", 0).toInt(),
	    m_ui.icon_set->count() - 1));
  m_ui.iterations->setValue
    (s_settings.value("authentication_iteration_count", 15000).toInt());
  m_ui.javascript->setChecked(s_settings.value("javascript", true).toBool());
  m_ui.javascript_access_clipboard->setChecked
    (s_settings.value("javascript_access_clipboard", false).toBool());
  m_ui.javascript_block_popups->setChecked
    (s_settings.value("javascript_block_popups", true).toBool());

  if(m_ui.language_directory->isVisible())
    m_ui.language->setCurrentIndex(0);
  else
    m_ui.language->setCurrentIndex
      (qBound(0,
	      s_settings.value("language_index", 0).toInt(),
	      m_ui.language->count()));

  m_ui.local_storage->setChecked
    (s_settings.value("local_storage", true).toBool());
  m_ui.main_menu_bar_visible->setChecked
    (s_settings.value("main_menu_bar_visible", true).toBool());
  m_ui.pages->setCurrentIndex
    (qBound(0,
	    s_settings.value("settings_page_index", 0).toInt(),
	    m_ui.pages->count() - 1));
  m_ui.pin_accepted_or_blocked_domains->setChecked
    (s_settings.value("pin_accepted_or_blocked_window", true).toBool());
  m_ui.pin_downloads->setChecked
    (s_settings.value("pin_downloads_window", true).toBool());
  m_ui.pin_history->setChecked
    (s_settings.value("pin_history_window", true).toBool());
  m_ui.pin_settings->setChecked
    (s_settings.value("pin_settings_window", true).toBool());
  m_ui.proxy_host->setText(s_settings.value("proxy_host").toString().trimmed());
  m_ui.proxy_host->setCursorPosition(0);
  m_ui.proxy_password->setText(s_settings.value("proxy_password").toString());
  m_ui.proxy_password->setCursorPosition(0);
  m_ui.proxy_port->setValue(s_settings.value("proxy_port", 0).toInt());
  m_ui.proxy_type->setCurrentIndex
    (qBound(0,
	    s_settings.value("proxy_type_index", 1).toInt(),
	    m_ui.proxy_type->count() - 1));
  m_ui.proxy_user->setText(s_settings.value("proxy_user").toString().trimmed());
  m_ui.proxy_user->setCursorPosition(0);
  m_ui.save_geometry->setChecked
    (s_settings.value("save_geometry", true).toBool());
  m_ui.theme_color->setCurrentIndex
    (qBound(0,
	    s_settings.value("theme_color_index", 0).toInt(),
	    m_ui.theme_color->count() - 1));
  m_ui.user_agent->setText(s_settings.value("user_agent").toString().trimmed());
  m_ui.user_agent->setToolTip("<html>" + m_ui.user_agent->text() + "</html>");
  m_ui.user_agent->setCursorPosition(0);
  m_ui.web_plugins->setChecked(s_settings.value("web_plugins", false).toBool());
  m_ui.webgl->setChecked(s_settings.value("webgl", true).toBool());
  m_ui.zoom_frame_location->setCurrentIndex
    (qBound(0,
	    s_settings.value("zoom_frame_location_index", 0).toInt(),
	    m_ui.zoom_frame_location->count() - 1));
  s_settings["accepted_or_blocked_domains_mode"] =
    s_settings.value("accepted_or_blocked_domains_mode", "block").
    toString().toLower();

  if(m_ui.cipher->currentIndex() == 0)
    s_settings["block_cipher_type"] = "AES-256";
  else
    s_settings["block_cipher_type"] = "Threefish-256";

  s_settings["icon_set"] = "Material Design";

  if(m_ui.theme_color->currentIndex() == 0)
    s_settings["theme_color"] = "blue-grey";
  else
    s_settings["theme_color"] = "indigo";

  s_settings["theme_color_index"] = m_ui.theme_color->currentIndex();
  m_ui.utc_time_zone->setChecked
    (s_settings.value("utc_time_zone", false).toBool());

  if(m_ui.utc_time_zone->isChecked())
    qputenv("TZ", ":UTC");

  m_ui.visited_links->setChecked
    (s_settings.value("visited_links", false).toBool());

  {
    QFile file(s_settings.value("home_path").toString() +
	       QDir::separator() +
	       "WebEnginePersistentStorage" +
	       QDir::separator() +
	       "Visited Links");

    if(m_ui.visited_links->isChecked())
      file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    else
      {
	file.open(QIODevice::Truncate | QIODevice::WriteOnly);
	file.setPermissions(QFileDevice::ReadOwner);
      }
  }

  m_ui.xss_auditing->setChecked
    (s_settings.value("xss_auditing", false).toBool());
  lock.unlock();
  m_ui.reset_credentials->setEnabled(has_dooble_credentials());
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  QWebEngineProfile::defaultProfile()->setHttpUserAgent
    (m_ui.user_agent->text());

  {
    QStringList list
      (setting("dictionaries").toString().split(";", QString::SkipEmptyParts));

    std::sort(list.begin(), list.end());

    for(int i = 0; i < m_ui.dictionaries->count(); i++)
      {
	QListWidgetItem *item = m_ui.dictionaries->item(i);

	if(!item)
	  continue;

	if(list.contains(item->text()))
	  item->setCheckState(Qt::Checked);
      }

    QWebEngineProfile::defaultProfile()->setSpellCheckLanguages(list);
  }

  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());

  {
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
  }

  prepare_proxy(false);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::save_javascript_block_popup_exception
(const QUrl &url, bool state)
{
  if(url.isEmpty() || !url.isValid())
    return;

  s_javascript_block_popup_exceptions[url] = state ? 1 : 0;

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

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

	query.exec
	  ("CREATE TABLE IF NOT EXISTS "
	   "dooble_javascript_block_popup_exceptions ("
	   "state TEXT NOT NULL, "
	   "url TEXT NOT NULL, "
	   "url_digest TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_javascript_block_popup_exceptions "
	   "(state, url, url_digest) VALUES (?, ?, ?)");

	QByteArray data
	  (dooble::s_cryptography->
	   encrypt_then_mac(state ? QByteArray("true") : QByteArray("false")));
	bool ok = true;

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(url.toEncoded());

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
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
  if(m_ui.credentials->isChecked() != setting("credentials_enabled").toBool())
    {
      show_panel(PRIVACY_PANEL);

      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);

      if(m_ui.credentials->isChecked())
	mb.setText
	  (tr("You are about to enable temporary credentials. "
	      "Existing database data will be removed. New data will be "
	      "stored as ciphertext. Continue?"));
      else
	mb.setText
	  (tr("You are about to disable credentials. Existing database data "
	      "will be removed. New data will be stored as plaintext. "
	      "Continue?"));

      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  m_ui.credentials->setChecked(true);
	  return;
	}

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      /*
      ** Remove settings.
      */

      m_ui.cipher->setCurrentIndex(0);
      m_ui.iterations->setValue(m_ui.iterations->minimum());
      m_ui.reset_credentials->setEnabled(false);

      /*
      ** Purge existing database data.
      */

      purge_database_data();
      remove_setting("authentication_iteration_count");
      remove_setting("authentication_salt");
      remove_setting("authentication_salted_password");
      remove_setting("block_cipher_type");
      remove_setting("block_cipher_type_index");

      {
	QWriteLocker locker(&s_settings_mutex);

	s_settings["block_cipher_type"] = "AES-256";
	s_settings["block_cipher_type_index"] = 0;
      }

      dooble::s_cryptography->set_block_cipher_type("AES-256");

      if(m_ui.credentials->isChecked())
	{
	  /*
	  ** Generate temporary credentials.
	  */

	  QByteArray authentication_key
	    (dooble_random::
	     random_bytes(dooble_cryptography::s_authentication_key_length));
	  QByteArray encryption_key
	    (dooble_random::
	     random_bytes(dooble_cryptography::s_encryption_key_length));

	  dooble::s_cryptography->set_authenticated(false);
	  dooble::s_cryptography->set_keys(authentication_key, encryption_key);
	  dooble_cryptography::memzero(authentication_key);
	  dooble_cryptography::memzero(encryption_key);
	  emit dooble_credentials_authenticated(false);
	}
      else
	{
	  dooble::s_cryptography->set_keys(QByteArray(), QByteArray());
	  emit dooble_credentials_authenticated(true);
	}

      QApplication::restoreOverrideCursor();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);

  if(m_ui.user_agent->text().trimmed().isEmpty())
    m_ui.user_agent->setText(s_http_user_agent);

  QWebEngineProfile::defaultProfile()->setHttpUserAgent
    (m_ui.user_agent->text().trimmed());

  {
    QString text("");
    QStringList list;

    for(int i = 0; i < m_ui.dictionaries->count(); i++)
      {
	QListWidgetItem *item = m_ui.dictionaries->item(i);

	if(!item)
	  continue;

	if(item->checkState() == Qt::Checked)
	  {
	    list << item->text();
	    text.append(item->text());
	    text.append(";");
	  }
      }

    QWebEngineProfile::defaultProfile()->setSpellCheckLanguages(list);
    set_setting("dictionaries", text);
  }

  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());

  {
    QWriteLocker locker(&s_settings_mutex);

    if(m_ui.theme_color->currentIndex() == 0)
      s_settings["theme_color"] = "blue-grey";
    else
      s_settings["theme_color"] = "indigo";
  }

  m_ui.user_agent->setText
    (QWebEngineProfile::defaultProfile()->httpUserAgent());
  m_ui.user_agent->setToolTip(m_ui.user_agent->text());
  m_ui.user_agent->setCursorPosition(0);

  if(m_ui.utc_time_zone->isChecked())
    qputenv("TZ", ":UTC");
  else
    qunsetenv("TZ");

  {
    QFile file(setting("home_path").toString() +
	       QDir::separator() +
	       "WebEnginePersistentStorage" +
	       QDir::separator() +
	       "Visited Links");

    if(m_ui.visited_links->isChecked())
      file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    else
      {
	file.open(QIODevice::Truncate | QIODevice::WriteOnly);
	file.setPermissions(QFileDevice::ReadOwner);
      }
  }

  prepare_proxy(true);
  set_setting("access_new_tabs", m_ui.access_new_tabs->isChecked());
  set_setting("animated_scrolling", m_ui.animated_scrolling->isChecked());
  set_setting("browsing_history_days", m_ui.browsing_history->value());
  set_setting("cache_size", m_ui.cache_size->value());
  set_setting("cache_type_index", m_ui.cache_type->currentIndex());
  set_setting("center_child_windows", m_ui.center_child_windows->isChecked());
  set_setting("cookie_policy_index", m_ui.cookie_policy->currentIndex());
  set_setting("credentials_enabled", m_ui.credentials->isChecked());
  set_setting("do_not_track", m_ui.do_not_track->isChecked());
  set_setting
    ("denote_private_widgets", m_ui.denote_private_widgets->isChecked());
  set_setting("icon_set_index", m_ui.icon_set->currentIndex());

  {
    QWriteLocker locker(&s_settings_mutex);

    s_settings["icon_set"] = "Material Design";
  }

  set_setting("javascript", m_ui.javascript->isChecked());
  set_setting("javascript_access_clipboard",
	      m_ui.javascript_access_clipboard->isChecked());
  set_setting
    ("javascript_block_popups", m_ui.javascript_block_popups->isChecked());
  set_setting("language_index", m_ui.language->currentIndex());
  set_setting("local_storage", m_ui.local_storage->isChecked());
  set_setting("main_menu_bar_visible", m_ui.main_menu_bar_visible->isChecked());
  set_setting("pin_accepted_or_blocked_window",
	      m_ui.pin_accepted_or_blocked_domains->isChecked());
  set_setting("pin_downloads_window", m_ui.pin_downloads->isChecked());
  set_setting("pin_history_window", m_ui.pin_history->isChecked());
  set_setting("pin_settings_window", m_ui.pin_settings->isChecked());
  set_setting("save_geometry", m_ui.save_geometry->isChecked());
  set_setting("theme_color_index", m_ui.theme_color->currentIndex());
  set_setting("utc_time_zone", m_ui.utc_time_zone->isChecked());
  set_setting("visited_links", m_ui.visited_links->isChecked());
  set_setting("user_agent", m_ui.user_agent->text().trimmed());
  set_setting("web_plugins", m_ui.web_plugins->isChecked());
  set_setting("webgl", m_ui.webgl->isChecked());
  set_setting("xss_auditing", m_ui.xss_auditing->isChecked());
  set_setting
    ("zoom_frame_location_index", m_ui.zoom_frame_location->currentIndex());
  prepare_icons();
  QApplication::restoreOverrideCursor();
  emit applied();
}

void dooble_settings::slot_clear_cache(void)
{
  QWebEngineProfile::defaultProfile()->clearHttpCache();
}

void dooble_settings::slot_javascript_block_popups_exceptions_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  bool state = item->checkState() == Qt::Checked;

  item = m_ui.javascript_block_popups_exceptions->item(item->row(), 1);

  if(!item)
    return;

  save_javascript_block_popup_exception(item->text(), state);
}

void dooble_settings::slot_new_javascript_block_popup_exception(const QUrl &url)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QTableWidgetItem *> list
    (m_ui.javascript_block_popups_exceptions->
     findItems(url.toString(), Qt::MatchExactly));

  if(!list.isEmpty())
    if(list.at(0))
      m_ui.javascript_block_popups_exceptions->removeRow(list.at(0)->row());

  s_javascript_block_popup_exceptions.remove(url);
  QApplication::restoreOverrideCursor();
  new_javascript_block_popup_exception(url);
}

void dooble_settings::slot_new_javascript_block_popup_exception(void)
{
  new_javascript_block_popup_exception
    (QUrl::fromUserInput(m_ui.new_javascript_block_popup_exception->text()));
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

      if(!list.isEmpty())
	{
	  ok &= set_setting
	    ("authentication_iteration_count", list.value(2).toInt());
	  ok &= set_setting("authentication_salt", list.value(4).toHex());
	  ok &= set_setting
	    ("authentication_salted_password",
	     QCryptographicHash::hash(list.value(3) + list.value(4),
				      QCryptographicHash::Sha3_512).toHex());
	  ok &= set_setting
	    ("block_cipher_type_index", list.value(1).toInt());
	  ok &= set_setting("credentials_enabled", true);

	  /*
	  ** list[0] - Keys
	  ** list[1] - Block Cipher Type
	  ** list[2] - Iteration Count
	  ** list[3] - Password
	  ** list[4] - Salt
	  */

	  {
	    QWriteLocker locker(&s_settings_mutex);

	    if(list.value(1).toInt() == 0)
	      s_settings["block_cipher_type"] = "AES-256";
	    else
	      s_settings["block_cipher_type"] = "Threefish-256";
	  }

	  if(ok)
	    {
	      dooble::s_cryptography->set_authenticated(true);

	      if(list.value(1).toInt() == 0)
		dooble::s_cryptography->set_block_cipher_type("AES-256");
	      else
		dooble::s_cryptography->set_block_cipher_type("Threefish-256");

	      QByteArray authentication_key
		(list.value(0).
		 mid(0, dooble_cryptography::s_authentication_key_length));
	      QByteArray encryption_key
		(list.value(0).
		 mid(dooble_cryptography::s_authentication_key_length,
		     dooble_cryptography::s_encryption_key_length));

	      dooble::s_cryptography->set_keys
		(authentication_key, encryption_key);
	      dooble_cryptography::memzero(authentication_key);
	      dooble_cryptography::memzero(encryption_key);
	      m_ui.reset_credentials->setEnabled(true);
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

void dooble_settings::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.javascript_block_popups_exceptions->setRowCount(0);
  s_javascript_block_popup_exceptions.clear();

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

	if(query.exec("SELECT state, url "
		      "FROM dooble_javascript_block_popup_exceptions"))
	  while(query.next())
	    {
	      QByteArray data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		continue;

	      QByteArray data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		continue;

	      QUrl url(data2);

	      if(url.isEmpty() || !url.isValid())
		continue;

	      s_javascript_block_popup_exceptions[url] =
		(data1 == "true") ? 1 : 0;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  disconnect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  m_ui.javascript_block_popups_exceptions->setRowCount
    (s_javascript_block_popup_exceptions.size());

  QHashIterator<QUrl, char> it(s_javascript_block_popup_exceptions);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      if(it.value())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.javascript_block_popups_exceptions->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key().toString());
      item->setData(Qt::UserRole, it.key());
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.javascript_block_popups_exceptions->setItem(i, 1, item);
      i += 1;
    }

  connect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  m_ui.javascript_block_popups_exceptions->sortItems(1);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_proxy_type_changed(int index)
{
  if(index == 1 || index == 3) // None, System
    {
      m_ui.proxy_frame->setEnabled(false);
      m_ui.proxy_host->setText("");
      m_ui.proxy_password->setText("");
      m_ui.proxy_port->setValue(0);
      m_ui.proxy_user->setText("");
    }
  else
    m_ui.proxy_frame->setEnabled(true);
}

void dooble_settings::slot_remove_all_javascript_block_popup_exceptions(void)
{
  m_ui.javascript_block_popups_exceptions->setRowCount(0);
  s_javascript_block_popup_exceptions.clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  purge_javascript_block_popup_exceptions();
}

void dooble_settings::
slot_remove_selected_javascript_block_popup_exceptions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.javascript_block_popups_exceptions->
		       selectionModel()->selectedRows(1));

  std::sort(list.begin(), list.end());

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
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

	    query.exec("PRAGMA synchronous = OFF");

	    for(int i = list.size() - 1; i >= 0; i--)
	      {
		query.prepare
		  ("DELETE FROM dooble_javascript_block_popup_exceptions "
		   "WHERE url_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data(Qt::UserRole).toUrl().toEncoded()).
		   toBase64());

		if(query.exec())
		  {
		    s_javascript_block_popup_exceptions.remove
		      (list.at(i).data().toString());
		    m_ui.javascript_block_popups_exceptions->
		      removeRow(list.at(i).row());
		  }
	      }

	    query.exec("VACUUM");
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }
  else
    for(int i = list.size() - 1; i >= 0; i--)
      {
	s_javascript_block_popup_exceptions.remove
	  (list.at(i).data().toString());
	m_ui.javascript_block_popups_exceptions->removeRow(list.at(i).row());
      }

  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_reset(void)
{
  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText
    (tr("Are you sure that you wish to reset Dooble? "
	"All known data will be removed and Dooble will be restarted. Please "
	"remove the directory WebEnginePersistentStorage after the reset "
	"completes."));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    return;

  QApplication::processEvents();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QStringList list;

  list << "dooble_accepted_or_blocked_domains.db"
       << "dooble_certificate_exceptions.db"
       << "dooble_cookies.db"
       << "dooble_downloads.db"
       << "dooble_favicons.db"
       << "dooble_history.db"
       << "dooble_settings.db";

  while(!list.isEmpty())
    QFile::remove(dooble_settings::setting("home_path").toString() +
		  QDir::separator() +
		  list.takeFirst());

  QApplication::restoreOverrideCursor();
  QApplication::processEvents();
  QProcess::startDetached(QCoreApplication::applicationDirPath() +
			  QDir::separator() +
			  QCoreApplication::applicationName());
  QApplication::exit(0);
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

  m_ui.cipher->setCurrentIndex(0);
  m_ui.iterations->setValue(m_ui.iterations->minimum());
  m_ui.reset_credentials->setEnabled(false);

  /*
  ** Purge existing database data.
  */

  purge_database_data();
  remove_setting("authentication_iteration_count");
  remove_setting("authentication_salt");
  remove_setting("authentication_salted_password");
  remove_setting("block_cipher_type");
  remove_setting("block_cipher_type_index");

  /*
  ** Generate temporary credentials.
  */

  QByteArray authentication_key
    (dooble_random::
     random_bytes(dooble_cryptography::s_authentication_key_length));
  QByteArray encryption_key
    (dooble_random::random_bytes(dooble_cryptography::s_encryption_key_length));

  dooble::s_cryptography->set_authenticated(false);
  dooble::s_cryptography->set_block_cipher_type("AES-256");
  dooble::s_cryptography->set_keys(authentication_key, encryption_key);
  dooble_cryptography::memzero(authentication_key);
  dooble_cryptography::memzero(encryption_key);
  emit dooble_credentials_authenticated(false);
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
  purge_database_data();
  QApplication::restoreOverrideCursor();
  m_pbkdf2_dialog = new QProgressDialog(this);
  m_pbkdf2_dialog->setCancelButtonText(tr("Interrupt"));
  m_pbkdf2_dialog->setLabelText(tr("Preparing credentials..."));
  m_pbkdf2_dialog->setMaximum(0);
  m_pbkdf2_dialog->setMinimum(0);
  m_pbkdf2_dialog->setStyleSheet("QWidget {background-color: white;}");
  m_pbkdf2_dialog->setWindowIcon(windowIcon());
  m_pbkdf2_dialog->setWindowModality(Qt::ApplicationModal);
  m_pbkdf2_dialog->setWindowTitle(tr("Dooble: Preparing Credentials"));

  QScopedPointer<dooble_pbkdf2> pbkdf2;

  pbkdf2.reset(new dooble_pbkdf2(password1.toUtf8(),
				 salt,
				 m_ui.cipher->currentIndex(),
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
