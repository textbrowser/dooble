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
#include <QFontDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QToolTip>
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
#include "dooble_database_utilities.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_hmac.h"
#include "dooble_pbkdf2.h"
#include "dooble_random.h"
#include "dooble_scroll_filter.h"
#include "dooble_settings.h"
#include "dooble_style_sheet.h"
#include "dooble_text_utilities.h"
#include "dooble_ui_utilities.h"
#include "dooble_version.h"

QHash<QString, QString> dooble_settings::s_user_agents;
QHash<QString, QString> dooble_settings::s_web_engine_settings_environment;
QHash<QString, char> dooble_settings::s_javascript_disable;
QHash<QUrl, char> dooble_settings::s_javascript_block_popup_exceptions;
QMap<QString, QVariant> dooble_settings::s_getenv;
QMap<QString, QVariant> dooble_settings::s_settings;
QMultiMap<QUrl, QPair<int, bool> > dooble_settings::s_site_features_permissions;
QReadWriteLock dooble_settings::s_getenv_mutex;
QReadWriteLock dooble_settings::s_settings_mutex;
QString dooble_settings::s_http_user_agent;
QStringList dooble_settings::s_spell_checker_dictionaries;
bool dooble_settings::s_reading_from_canvas_enabled = true;

dooble_settings::dooble_settings(void):dooble_main_window()
{
  m_shortcuts_model = new QStandardItemModel(this);
  m_shortcuts_model->setHorizontalHeaderLabels
    (QStringList() << tr("Action") << tr("Shortcut"));
  m_timer.setInterval(2500);
  m_ui.setupUi(this);
  connect(&m_pbkdf2_future_watcher,
	  SIGNAL(finished(void)),
	  this,
	  SLOT(slot_pbkdf2_future_finished(void)));
  connect(&m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_general_timer_timeout(void)));
  connect(m_ui.add_javascript_disable,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_new_javascript_disable(void)));
  connect(m_ui.add_user_agent,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_new_user_agent(void)));
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
  connect(m_ui.display_application_font,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_application_font(void)));
  connect(m_ui.history,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.new_javascript_block_popup_exception,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_new_javascript_block_popup_exception(void)));
  connect(m_ui.new_javascript_disable,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_new_javascript_disable(void)));
  connect(m_ui.new_user_agent,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_new_user_agent(void)));
  connect(m_ui.password_1,
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_password_changed(void)));
  connect(m_ui.password_2,
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_password_changed(void)));
  connect(m_ui.privacy,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_page_button_clicked(void)));
  connect(m_ui.proxy_http,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_proxy_type_changed(void)));
  connect(m_ui.proxy_none,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_proxy_type_changed(void)));
  connect(m_ui.proxy_socks5,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_proxy_type_changed(void)));
  connect(m_ui.proxy_system,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slot_proxy_type_changed(void)));
  connect(m_ui.remove_all_features_permissions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_all_features_permissions(void)));
  connect(m_ui.remove_all_javascript_block_popup_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_all_javascript_block_popup_exceptions(void)));
  connect(m_ui.remove_all_javascript_disable,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_all_javascript_disable(void)));
  connect(m_ui.remove_all_user_agents,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_all_user_agents(void)));
  connect(m_ui.remove_selected_features_permissions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_selected_features_permissions(void)));
  connect(m_ui.remove_selected_javascript_block_popup_exceptions,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_selected_javascript_block_popup_exceptions(void)));
  connect(m_ui.remove_selected_javascript_disable,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_selected_javascript_disable(void)));
  connect(m_ui.remove_selected_user_agents,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_remove_selected_user_agents(void)));
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
     tr("Persistent and session cookies are restored from and saved to disk."),
     Qt::ToolTipRole);
  m_ui.cookie_policy->setItemData
    (2,
     tr("Persistent cookies are restored from and saved to disk."),
     Qt::ToolTipRole);
  m_ui.download_version_information->setToolTip
    (tr("<html>Dooble will download the file <b>%1</b> shortly after launch "
	"and determine if an official version is available. See Help -> "
	"About.</html>").arg(DOOBLE_VERSION_FILE_URL));
  m_ui.shortcuts->setModel(m_shortcuts_model);

  auto const language = QLocale::system().language();

  if(language == QLocale::C || language == QLocale::English)
    m_ui.language_directory->setVisible(false);
  else
    {
      QString path("");
      auto const variable
	(qEnvironmentVariable("DOOBLE_TRANSLATIONS_PATH").trimmed());

      if(!variable.isEmpty())
	path = variable;
      else
	{
	  path = QDir::currentPath();
	  path.append(QDir::separator());
	  path.append("Translations");
	}

      if(!path.endsWith(QDir::separator()))
	path.append(QDir::separator());

      path.append("dooble_" + QLocale::system().name() + ".qm");

      QFileInfo const file_info(path);

      if(!file_info.exists() || !file_info.isReadable())
	{
	  m_ui.language_directory->setStyleSheet
	    ("QLabel {background-color: #f2dede; border: 1px solid #ebccd1;"
	     "color:#a94442;}");

	  if(!file_info.exists())
	    m_ui.language_directory->setText
	      (tr("<b>Warning!</b> The file %1 does not exist. "
		  "Dooble searched DOOBLE_TRANSLATIONS_PATH and "
		  "the relative Translations directories. "
		  "English will be assumed. Please read %2, line %3.").
	       arg(file_info.absoluteFilePath()).
	       arg(__FILE__).
	       arg(__LINE__));
	  else
	    m_ui.language_directory->setText
	      (tr("<b>Warning!</b> The file %1 is not readable. "
		  "Dooble searched DOOBLE_TRANSLATIONS_PATH and "
		  "the relative Translations directories. "
		  "English will be assumed. Please read %2, line %3.").
	       arg(file_info.absoluteFilePath()).
	       arg(__FILE__).
	       arg(__LINE__));
	}
      else if(file_info.size() <= 1024)
	{
	  m_ui.language_directory->setStyleSheet
	    ("QLabel {background-color: #f2dede; border: 1px solid #ebccd1;"
	     "color:#a94442;}");
	  m_ui.language_directory->setText
	    (tr("<b>Warning!</b> The file %1 is perhaps incomplete. It "
		"contains only %2 bytes. "
		"English will be assumed. Please read %3, line %4.").
	     arg(file_info.absoluteFilePath()).
	     arg(file_info.size()).
	     arg(__FILE__).
	     arg(__LINE__));
	}
      else
	m_ui.language_directory->setVisible(false);
    }

#ifdef Q_OS_OS2
  /*
  ** Temporarily correct UserAgent.
  ** https://github.com/bitwiseworks/dooble-os2/issues/3
  ** https://github.com/bitwiseworks/qtwebengine-chromium-os2/issues/48
  */

  s_http_user_agent = dooble::s_default_web_engine_profile->httpUserAgent();
  s_http_user_agent.replace("Unknown", "OS/2");
  s_http_user_agent +=
#else
  s_http_user_agent = dooble::s_default_web_engine_profile->httpUserAgent() +
#endif
    " Dooble/" DOOBLE_VERSION_STRING;
#ifdef Q_OS_WINDOWS
#else
  m_ui.theme->setEnabled(false);
  m_ui.theme->setToolTip(tr("Windows only."));
#endif
  s_settings["accepted_or_blocked_domains_mode"] = "block";
  s_settings["access_new_tabs"] = true;
  s_settings["add_tab_behavior_index"] = 1; // At End
  s_settings["address_widget_completer_mode_index"] = 1; // Popup
  s_settings["allow_closing_of_single_tab"] = true;
  s_settings["application_font"] = false;
  s_settings["auto_hide_tab_bar"] = false;
  s_settings["auto_load_images"] = true;
  s_settings["block_cipher_type"] = "AES-256";
  s_settings["block_cipher_type_index"] = 0;
  s_settings["block_third_party_cookies"] = true;
  s_settings["browsing_history_days"] = 15;
  s_settings["cache_size"] = 0;
  s_settings["cache_type_index"] = 0;
  s_settings["center_child_windows"] = true;
  s_settings["cookie_policy_index"] = 2;
  s_settings["credentials_enabled"] = false;
  s_settings["denote_private_widgets"] = true;
  s_settings["do_not_track"] = true;
  s_settings["download_version_information"] = false;
  s_settings["favicons"] = true;
  s_settings["favorites_sort_index"] = 1; // Most Popular
  s_settings["features_permissions"] = true;
  s_settings["full_screen"] = true;
  s_settings["hash_type"] = "SHA3-512";
  s_settings["hash_type_index"] = 1;
  s_settings["home_url"] = QUrl();
  s_settings["icon_set"] = "Material Design";
  s_settings["icon_set_index"] = 0;
  s_settings["javascript_block_popups"] = true;
  s_settings["language_index"] = 0;
  s_settings["lefty_buttons"] = false;
  s_settings["local_storage"] = true;
  s_settings["main_menu_bar_visible"] = true;
  s_settings["main_menu_bar_visible_shortcut_index"] = 1; // F10
  s_settings["pin_accepted_or_blocked_window"] = true;
  s_settings["pin_downloads_window"] = true;
  s_settings["pin_history_window"] = true;
  s_settings["pin_settings_window"] = true;
  s_settings["referrer"] = false;
  s_settings["relative_location_character"] = "";
  s_settings["retain_session_tabs"] = false;
  s_settings["save_geometry"] = true;
  s_settings["show_address_widget_completer"] = true;
  s_settings["show_hovered_links_tool_tips"] = false;
  s_settings["show_left_corner_widget"] = true;
  s_settings["show_loading_gradient"] = true;
  s_settings["show_new_downloads"] = true;
  s_settings["show_title_bar"] = true;
  s_settings["splash_screen"] = true;
  s_settings["status_bar_visible"] = true;
  s_settings["tab_document_mode"] = true;
  s_settings["tab_position"] = "north";
  s_settings["temporarily_disable_javascript"] = false;
  s_settings["theme_color"] = "default";
  s_settings["theme_color_index"] = 2; // Default
  s_settings["user_agent"] = s_http_user_agent;
  s_settings["web_plugins"] = false;
  s_settings["webgl"] = true;
  s_settings["webrtc_public_interfaces_only"] = true;
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
			       << "fr_FR"
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
			       << "fr_FR"
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
#ifndef DOOBLE_MMAN_PRESENT
  m_ui.mman_message->setStyleSheet
    ("QLabel {background-color: #f2dede; border: 1px solid #ebccd1; "
     "color:#a94442;}");
  m_ui.mman_message->setText
    (tr("Memory locking is not available on this system."));
#else
  m_ui.mman_message->setText
    (tr("Memory locking is provided by mlock() and munlock()."));
#endif

  if(s_spell_checker_dictionaries.isEmpty())
    {
      m_ui.dictionaries_group_box->setEnabled(false);
      m_ui.dictionaries_group_box->setToolTip
	(tr("A valid list of dictionaries has not been prepared."));
    }
  else
    foreach(auto const &i, s_spell_checker_dictionaries)
      {
	auto item = new QListWidgetItem(i);

	item->setFlags(Qt::ItemIsEnabled |
		       Qt::ItemIsSelectable |
		       Qt::ItemIsUserCheckable);
	item->setCheckState(Qt::Unchecked);
	m_ui.dictionaries->addItem(item);
      }

  foreach(auto widget, findChildren<QWidget *> ())
    if(qobject_cast<QComboBox *> (widget) ||
       qobject_cast<QDoubleSpinBox *> (widget) ||
       qobject_cast<QSpinBox *> (widget))
      widget->installEventFilter(new dooble_scroll_filter(this));

  restore(true);
  prepare_icons();
  prepare_shortcuts();
  show_qtwebengine_dictionaries_warning_label();
  slot_password_changed();
}

QString dooble_settings::shortcut(const QString &action) const
{
  auto item = m_shortcuts_model->findItems(action).value(0);

  if(item && m_shortcuts_model->index(item->row(), 1).isValid())
    return m_shortcuts_model->index(item->row(), 1).data().toString();
  else
    return "";
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

QString dooble_settings::use_material_icons(void)
{
  return setting("icon_set_index") == "0" ? "true" : "";
}

QString dooble_settings::user_agent(const QUrl &url)
{
  return s_user_agents.value(url.host(), s_http_user_agent);
}

QString dooble_settings::zoom_frame_location_string(int index)
{
  Q_UNUSED(index);
  return "popup_menu";
}

QVariant dooble_settings::getenv(const QString &n)
{
  auto const name(n.trimmed());

  if(name.isEmpty())
    return QVariant();

  QWriteLocker locker(&s_getenv_mutex);

  if(s_getenv.contains(name))
    return s_getenv.value(name);

  s_getenv[name] = qEnvironmentVariable(name.toUtf8().constData());
  return s_getenv.value(name);
}

QVariant dooble_settings::setting(const QString &k,
				  const QVariant &default_value)
{
  QReadLocker locker(&s_settings_mutex);
  auto const key(k.toLower().trimmed());

  if(!s_settings.contains(key))
    {
      auto const home_path = s_settings.value("home_path").toString();

      locker.unlock();

      auto const database_name(dooble_database_utilities::database_name());
      auto value(default_value);

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName
	  (home_path + QDir::separator() + "dooble_settings.db");

	if(db.open())
	  {
	    create_tables(db);

	    QSqlQuery query(db);

	    query.setForwardOnly(true);
	    query.prepare("SELECT value FROM dooble_settings WHERE key = ?");
	    query.addBindValue(key);

	    if(query.exec() && query.next())
	      {
		value = query.value(0).toString().trimmed();

		QWriteLocker locker(&s_settings_mutex);

		s_settings[key] = value;
	      }
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
      return value;
    }

  if(key == "add_tab_behavior_index")
    return qBound
      (0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "address_widget_completer_mode_index")
    return qBound
      (0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "authentication_iteration_count")
    return qBound
      (15000, s_settings.value(key, default_value).toInt(), 999999999);
  else if(key == "block_cipher_type_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "browsing_history_days")
    return qBound(0, s_settings.value(key, default_value).toInt(), 365);
  else if(key == "cache_size")
    return qBound(0, s_settings.value(key, default_value).toInt(), 2048);
  else if(key == "cache_type_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "cookie_policy_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 2);
  else if(key == "dooble_accepted_or_blocked_domains_"
	          "maximum_session_rejections")
    return qBound(1, s_settings.value(key, default_value).toInt(), 1000000);
  else if(key == "favorites_sort_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 2);
  else if(key == "hash_type_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "icon_set_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "language_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 1);
  else if(key == "proxy_port")
    return qBound(0, s_settings.value(key, default_value).toInt(), 65535);
  else if(key == "theme_color_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 4);
  else if(key == "zoom")
    return qBound(25, s_settings.value(key, default_value).toInt(), 500);
  else if(key == "zoom_frame_location_index")
    return qBound(0, s_settings.value(key, default_value).toInt(), 0);
  else if(key.startsWith("history_horizontal_header_section_size_"))
    return qBound(0, s_settings.value(key, default_value).toInt(), 1000000);
  else
    return s_settings.value(key, default_value);
}

bool dooble_settings::has_dooble_credentials(void)
{
  return setting("authentication_iteration_count").toInt() > 0 &&
    setting("authentication_salt").toString().length() > 0 &&
    setting("authentication_salted_password").toString().length() > 0 &&
    setting("credentials_enabled").toBool();
}

bool dooble_settings::has_dooble_credentials_temporary(void)
{
  return (setting("authentication_iteration_count").toInt() == 0 ||
	  setting("authentication_salt").toString().isEmpty() ||
	  setting("authentication_salted_password").toString().isEmpty()) &&
    setting("credentials_enabled").toBool();
}

bool dooble_settings::reading_from_canvas_enabled(void)
{
  return s_reading_from_canvas_enabled;
}

bool dooble_settings::set_setting(const QString &key, const QVariant &value)
{
  if(key.trimmed().isEmpty())
    return false;
  else if(value.isNull())
    {
      if(key != "home_url")
	{
	  QWriteLocker locker(&s_settings_mutex);

	  s_settings.remove(key.toLower().trimmed());
	  return false;
	}
    }

  QWriteLocker locker(&s_settings_mutex);

  s_settings[key.toLower().trimmed()] = value;
  locker.unlock();

  auto const database_name(dooble_database_utilities::database_name());
  auto ok = false;

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = NORMAL");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_settings (key, value) VALUES (?, ?)");
	query.addBindValue(key.toLower().trimmed());

	if(key == "home_url" && value.toString().trimmed().isEmpty())
	  query.addBindValue("");
	else
	  query.addBindValue(value.toString().trimmed());

	ok = query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  return ok;
}

bool dooble_settings::site_has_javascript_block_popup_exception
(const QUrl &url)
{
  return s_javascript_block_popup_exceptions.value(url, 0) == 1;
}

bool dooble_settings::site_has_javascript_disabled(const QUrl &url)
{
  return s_javascript_disable.value(url.host(), 0) == 1;
}

int dooble_settings::main_menu_bar_visible_key(void)
{
  auto const index = setting("main_menu_bar_visible_shortcut_index").toInt();

  switch(index)
    {
    case 0:
      {
	return Qt::Key_Alt;
      }
    case 1:
    default:
      {
	return Qt::Key_F10;
      }
    }
}

int dooble_settings::site_feature_permission
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
(const QUrl &url, const QWebEnginePage::Feature feature)
#else
(const QUrl &url, const QWebEnginePermission::PermissionType feature)
#endif
{
  if(!s_site_features_permissions.contains(url))
    return -1;

  auto const values(s_site_features_permissions.values(url));

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
  foreach(auto const &value, values)
    if(feature == QWebEnginePage::Feature(value.first) && value.first != -1)
      return value.second ? 1 : 0;
#else
  foreach(auto const &value, values)
    if(feature == QWebEnginePermission::PermissionType(value.first) &&
       value.first != -1)
      return value.second ? 1 : 0;
#endif

  return -1;
}

void dooble_settings::add_shortcut(QObject *object)
{
  auto action = qobject_cast<QAction *> (object);

  if(action && action->text().trimmed().length() > 0)
    {
      auto const text(action->text().trimmed());

      if(m_shortcuts_model->findItems(text).isEmpty())
	{
	  QList<QStandardItem *> items;
	  auto item = new QStandardItem(text);

	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  items << item;
	  item = new QStandardItem(action->shortcut().toString());
	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  items << item;
	  m_shortcuts_model->appendRow(items);
	  m_ui.shortcuts->resizeColumnToContents(0);
	  m_ui.shortcuts->sortByColumn(0, Qt::AscendingOrder);
	}
    }
}

void dooble_settings::add_shortcut
(const QString &action, const QString &shortcut)
{
  if(action.trimmed().isEmpty() || shortcut.trimmed().isEmpty())
    return;

  if(m_shortcuts_model->findItems(action.trimmed()).isEmpty())
    {
      QList<QStandardItem *> items;
      auto item = new QStandardItem(action.trimmed());

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      items << item;
      item = new QStandardItem(shortcut);
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      items << item;
      m_shortcuts_model->appendRow(items);
      m_ui.shortcuts->resizeColumnToContents(0);
      m_ui.shortcuts->sortByColumn(0, Qt::AscendingOrder);
    }
}

void dooble_settings::closeEvent(QCloseEvent *event)
{
  m_timer.stop();
  m_ui.password_1->setText
    (dooble_random::random_bytes(m_ui.password_1->text().length()).toHex());
  m_ui.password_1->clear();
  m_ui.password_2->setText
    (dooble_random::random_bytes(m_ui.password_2->text().length()).toHex());
  m_ui.password_2->clear();
  slot_password_changed();
  dooble_main_window::closeEvent(event);
}

void dooble_settings::create_tables(QSqlDatabase &db)
{
  db.open();

  QSqlQuery query(db);

  query.exec
    ("CREATE TABLE IF NOT EXISTS dooble_features_permissions "
     "(feature TEXT NOT NULL, "
     "feature_digest TEXT NOT NULL, "
     "permission TEXT NOT NULL, "
     "url TEXT NOT NULL, "
     "url_digest TEXT NOT NULL, "
     "PRIMARY KEY (feature_digest, url_digest))");
  query.exec
    ("CREATE TABLE IF NOT EXISTS dooble_javascript_block_popup_exceptions "
     "(state TEXT NOT NULL, "
     "url TEXT NOT NULL, "
     "url_digest TEXT NOT NULL PRIMARY KEY)");
  query.exec
    ("CREATE TABLE IF NOT EXISTS dooble_javascript_disable "
     "(state TEXT NOT NULL, "
     "url_domain TEXT NOT NULL, "
     "url_domain_digest TEXT NOT NULL PRIMARY KEY)");
  query.exec("CREATE TABLE IF NOT EXISTS dooble_settings "
	     "(key TEXT NOT NULL PRIMARY KEY, value TEXT NOT NULL)");
  query.exec
    ("CREATE TABLE IF NOT EXISTS dooble_user_agents "
     "(url_domain TEXT NOT NULL, "
     "url_domain_digest TEXT NOT NULL PRIMARY KEY, "
     "user_agent TEXT NOT NULL)");
  query.exec("CREATE TABLE IF NOT EXISTS dooble_web_engine_settings "
	     "(environment_variable INTEGER NOT NULL DEFAULT 0, "
	     "key TEXT NOT NULL PRIMARY KEY, "
	     "translate INTEGER NOT NULL DEFAULT 0, "
	     "value TEXT NOT NULL)");
}

void dooble_settings::keyPressEvent(QKeyEvent *event)
{
  if(!parent())
    {
      if(event && event->key() == Qt::Key_Escape)
	close();

      dooble_main_window::keyPressEvent(event);
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

  auto item = new QTableWidgetItem();

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
  item->setToolTip(item->text());
  m_ui.javascript_block_popups_exceptions->setItem
    (m_ui.javascript_block_popups_exceptions->rowCount() - 1, 1, item);
  m_ui.javascript_block_popups_exceptions->sortItems(1);
  connect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
  save_javascript_block_popup_exception(url, true);
}

void dooble_settings::new_javascript_disable(const QString &d, bool state)
{
  auto const domain(d.toLower().trimmed());

  if(domain.isEmpty())
    return;

  s_javascript_disable[domain] = state ? 1 : 0;

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_javascript_disable "
	   "(state, url_domain, url_domain_digest) VALUES (?, ?, ?)");

	auto data
	  (dooble::s_cryptography->
	   encrypt_then_mac(state ? QByteArray("true") : QByteArray("false")));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(domain.toUtf8());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(domain);

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::new_user_agent(const QString &d, const QString &u)
{
  auto const domain(d.toLower().trimmed());

  if(domain.isEmpty())
    return;

  auto user_agent(u.trimmed());

  if(user_agent.isEmpty())
    user_agent = s_http_user_agent;

  s_user_agents[domain] = user_agent;

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_user_agents "
	   "(url_domain, url_domain_digest, user_agent) VALUES (?, ?, ?)");

	auto data(dooble::s_cryptography->encrypt_then_mac(domain.toUtf8()));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(domain);

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(user_agent.toUtf8());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::prepare_application_fonts(void)
{
  if(!dooble::s_application)
    return;

  QFont font;

  if(s_settings.value("application_font", false).toBool())
    {
      auto const string
	(m_ui.display_application_font->text().remove('&').trimmed());

      if(string.isEmpty() || !font.fromString(string))
	font = dooble_application::font();
    }
  else
    font = dooble::s_application->default_font();

  auto const before = font.bold();

  dooble::s_application->setFont(font);

  foreach(auto widget, QApplication::allWidgets())
    if(widget)
      {
	font.setBold(widget->font().bold());
	widget->setFont(font);
	font.setBold(before);
	widget->updateGeometry();
      }
}

void dooble_settings::prepare_fonts(void)
{
  /*
  ** Fonts
  */

  m_ui.display_application_font->setText
    (s_settings.value("display_application_font").toString().trimmed());

  if(m_ui.display_application_font->text().isEmpty())
    m_ui.display_application_font->setText
      (dooble_application::font().toString().trimmed());

  prepare_application_fonts();

  {
    QFont font;
    QList<QWebEngineSettings::FontFamily> families;
    QStringList fonts;
    QStringList list;

    families << QWebEngineSettings::CursiveFont
	     << QWebEngineSettings::FantasyFont
	     << QWebEngineSettings::FixedFont
	     << QWebEngineSettings::PictographFont
	     << QWebEngineSettings::SansSerifFont
	     << QWebEngineSettings::SerifFont
	     << QWebEngineSettings::StandardFont;

    foreach(auto const family, families)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      fonts << QWebEngineSettings::defaultSettings()->fontFamily(family);
#else
      fonts << dooble::s_default_web_engine_profile->settings()->fontFamily
	(family);
#endif

    {
      QReadLocker lock(&s_settings_mutex);

      list << s_settings.value("web_font_cursive").toString()
	   << s_settings.value("web_font_fantasy").toString()
	   << s_settings.value("web_font_fixed").toString()
	   << s_settings.value("web_font_pictograph").toString()
	   << s_settings.value("web_font_sans_serif").toString()
	   << s_settings.value("web_font_serif").toString()
	   << s_settings.value("web_font_standard").toString();

      for(int i = 0; i < list.size(); i++)
	{
	  if(list.at(i).isEmpty())
	    list.replace(i, fonts.at(i));

	  list.replace(i, list.at(i).trimmed());

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	  QWebEngineSettings::defaultSettings()->setFontFamily
	    (families.at(i), list.at(i));
#else
	  dooble::s_default_web_engine_profile->settings()->setFontFamily
	    (families.at(i), list.at(i));
#endif
	}
    }

    if(list.at(0).isEmpty() || !font.fromString(list.at(0)))
      font = dooble_application::font();

    m_ui.web_font_cursive->setCurrentFont(font);

    if(list.at(1).isEmpty() || !font.fromString(list.at(1)))
      font = dooble_application::font();

    m_ui.web_font_fantasy->setCurrentFont(font);

    if(list.at(2).isEmpty() || !font.fromString(list.at(2)))
      font = dooble_application::font();

    m_ui.web_font_fixed->setCurrentFont(font);

    if(list.at(3).isEmpty() || !font.fromString(list.at(3)))
      font = dooble_application::font();

    m_ui.web_font_pictograph->setCurrentFont(font);

    if(list.at(4).isEmpty() || !font.fromString(list.at(4)))
      font = dooble_application::font();

    m_ui.web_font_sans_serif->setCurrentFont(font);

    if(list.at(5).isEmpty() || !font.fromString(list.at(5)))
      font = dooble_application::font();

    m_ui.web_font_serif->setCurrentFont(font);

    if(list.at(6).isEmpty() || !font.fromString(list.at(6)))
      font = dooble_application::font();

    m_ui.web_font_standard->setCurrentFont(font);
  }

  /*
  ** Font Sizes
  */

  {
    QList<QWebEngineSettings::FontSize> types;
    QList<int> list;
    QList<int> sizes;

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    sizes << QWebEngineSettings::defaultSettings()->fontSize
             (QWebEngineSettings::DefaultFixedFontSize)
	  << QWebEngineSettings::defaultSettings()->fontSize
             (QWebEngineSettings::DefaultFontSize)
	  << QWebEngineSettings::defaultSettings()->fontSize
             (QWebEngineSettings::MinimumFontSize)
	  << QWebEngineSettings::defaultSettings()->fontSize
             (QWebEngineSettings::MinimumLogicalFontSize);
#else
    sizes << dooble::s_default_web_engine_profile->settings()->fontSize
             (QWebEngineSettings::DefaultFixedFontSize)
	  << dooble::s_default_web_engine_profile->settings()->fontSize
             (QWebEngineSettings::DefaultFontSize)
	  << dooble::s_default_web_engine_profile->settings()->fontSize
             (QWebEngineSettings::MinimumFontSize)
	  << dooble::s_default_web_engine_profile->settings()->fontSize
             (QWebEngineSettings::MinimumLogicalFontSize);
#endif
    types << QWebEngineSettings::DefaultFixedFontSize
	  << QWebEngineSettings::DefaultFontSize
	  << QWebEngineSettings::MinimumFontSize
	  << QWebEngineSettings::MinimumLogicalFontSize;

    {
      QReadLocker lock(&s_settings_mutex);

      list << s_settings.value("web_font_size_default_fixed").toInt()
           << s_settings.value("web_font_size_default").toInt()
	   << s_settings.value("web_font_size_minimum").toInt()
	   << s_settings.value("web_font_size_minimum_logical").toInt();

      for(int i = 0; i < list.size(); i++)
	{
	  if(list.at(i) <= 0)
	    list.replace(i, sizes.at(i));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	  QWebEngineSettings::defaultSettings()->setFontSize
	    (types.at(i), list.at(i));
#else
	  dooble::s_default_web_engine_profile->settings()->setFontSize
	    (types.at(i), list.at(i));
#endif
	}
    }

    m_ui.web_font_size_default_fixed->setValue(list.at(0));
    m_ui.web_font_size_default->setValue(list.at(1));
    m_ui.web_font_size_minimum->setValue(list.at(2));
    m_ui.web_font_size_minimum_logical->setValue(list.at(3));
  }
}

void dooble_settings::prepare_icons(void)
{
  auto const icon_set(setting("icon_set").toString());
  auto const use_material_icons(this->use_material_icons());

  m_ui.cache->setIcon
    (QIcon::fromTheme(use_material_icons + "drive-harddisk",
		      QIcon(QString(":/%1/64/cache.png").arg(icon_set))));
  m_ui.display->setIcon
    (QIcon::fromTheme(use_material_icons + "video-display",
		      QIcon(QString(":/%1/64/display.png").arg(icon_set))));
  m_ui.history->setIcon
    (QIcon::fromTheme(use_material_icons + "deep-history",
		      QIcon(QString(":/%1/64/history.png").arg(icon_set))));
  m_ui.privacy->setIcon
    (QIcon::fromTheme(use_material_icons + "dialog-password",
		      QIcon(QString(":/%1/64/privacy.png").arg(icon_set))));
  m_ui.web->setIcon
    (QIcon::fromTheme(use_material_icons + "applications-internet",
		      QIcon(QString(":/%1/64/webengine.png").arg(icon_set))));
  m_ui.windows->setIcon
    (QIcon::fromTheme(use_material_icons + "preferences-desktop",
		      QIcon(QString(":/%1/64/windows.png").arg(icon_set))));

  QSize size(0, 0);
  static auto const list(QList<QPushButton *> () << m_ui.cache
			                         << m_ui.display
		                                 << m_ui.history
		                                 << m_ui.privacy
		                                 << m_ui.web
		                                 << m_ui.windows);

  foreach(auto i, list)
    {
      if(i->height() >= size.height() || i->width() >= size.width())
	size = i->size();

      if(i->styleSheet().isEmpty())
	i->setStyleSheet("QPushButton {padding: 10px; text-align: left;}");
    }

  size.setHeight(size.height() + 10);
  size.setWidth(size.width() + 10);

  foreach(auto i, list)
    i->resize(size);
}

void dooble_settings::prepare_proxy(bool save)
{
  QNetworkProxyFactory::setUseSystemConfiguration(false);

  if(m_ui.proxy_none->isChecked() || m_ui.proxy_system->isChecked())
    {
      if(m_ui.proxy_none->isChecked())
	{
	  QNetworkProxy const proxy(QNetworkProxy::NoProxy);

	  QNetworkProxy::setApplicationProxy(proxy);
	}
      else
	QNetworkProxyFactory::setUseSystemConfiguration(true);
    }
  else
    {
      QNetworkProxy proxy;

      proxy.setHostName(m_ui.proxy_host->text().trimmed());
      proxy.setPassword(m_ui.proxy_password->text());
      proxy.setPort(static_cast<quint16> (m_ui.proxy_port->value()));
      proxy.setType
	(m_ui.proxy_http->isChecked() ?
	 QNetworkProxy::HttpProxy : QNetworkProxy::Socks5Proxy);
      proxy.setUser(m_ui.proxy_user->text().trimmed());
      QNetworkProxy::setApplicationProxy(proxy);
    }

  if(save)
    {
      set_setting("proxy_host", m_ui.proxy_host->text().trimmed());
      set_setting("proxy_password", m_ui.proxy_password->text());
      set_setting("proxy_port", m_ui.proxy_port->value());

      if(m_ui.proxy_http->isChecked())
	set_setting("proxy_type_index", 2);
      else if(m_ui.proxy_none->isChecked())
	set_setting("proxy_type_index", 0);
      else if(m_ui.proxy_socks5->isChecked())
	set_setting("proxy_type_index", 3);
      else
	set_setting("proxy_type_index", 1);

      set_setting("proxy_user", m_ui.proxy_user->text().trimmed());
    }
}

void dooble_settings::prepare_shortcuts(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  add_shortcut(tr("VIM Scroll Down"), tr("Ctrl+Shift+J"));
  add_shortcut(tr("VIM Scroll Up"), tr("Ctrl+Shift+K"));
  QApplication::restoreOverrideCursor();
}

void dooble_settings::prepare_table_statistics(void)
{
  m_ui.features_permissions_entries->setText
    (tr("%1 Row(s)").arg(m_ui.features_permissions->rowCount()));
  m_ui.javascript_block_popups_exceptions_entries->setText
    (tr("%1 Row(s)").arg(m_ui.javascript_block_popups_exceptions->rowCount()));
  m_ui.javascript_disable_entries->setText
    (tr("%1 Row(s)").arg(m_ui.javascript_disable->rowCount()));
  m_ui.user_agent_entries->setText
    (tr("%1 Row(s)").arg(m_ui.user_agents->rowCount()));
}

void dooble_settings::prepare_web_engine_environment_variables(void)
{
  auto first_time = true;

  if((first_time = s_web_engine_settings_environment.isEmpty()))
    {
      s_web_engine_settings_environment["--allow-insecure-localhost"] =
	"singular";
      s_web_engine_settings_environment["--allow-running-insecure-content"] =
	"singular";
      s_web_engine_settings_environment
	["--blink-settings=forceDarkModeEnabled"] = "boolean";
      s_web_engine_settings_environment["--disable-reading-from-canvas"] =
	"singular";
      s_web_engine_settings_environment["--enable-gpu-rasterization"] =
	"boolean";
      s_web_engine_settings_environment["--enable-zero-copy"] = "boolean";
      s_web_engine_settings_environment["--ignore-certificate-errors"] =
	"singular";
      s_web_engine_settings_environment["--ignore-gpu-blocklist"] = "boolean";
      s_web_engine_settings_environment["--ignore-ssl-errors"] = "singular";
#ifdef Q_OS_OS2
      s_web_engine_settings_environment["--single-process"] = "singular";
#endif
    }

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);
	QString string("");

      	if(first_time)
      	  {
#ifdef Q_OS_OS2
	    /*
	    ** On OS/2, single-process mode should be default for now
	    ** due to stability issues with multi-process mode.
	    */

	    query.exec
	      ("INSERT OR IGNORE INTO dooble_web_engine_settings "
	       "(environment_variable, key, value) VALUES "
	       "(1, '--single-process', true)");
#endif
      	  }

	query.setForwardOnly(true);
	query.prepare
	  ("SELECT key, value FROM dooble_web_engine_settings "
	   "WHERE environment_variable = 1 ORDER BY 1");

	if(query.exec())
	  while(query.next())
	    {
	      auto const key(query.value(0).toString().trimmed());
	      auto const singular = s_web_engine_settings_environment.
		value(key) == "singular";

	      if(query.value(1).toBool() == false && singular)
		{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0))
		  if(dooble::s_default_web_engine_profile &&
		     key == "--disable-reading-from-canvas")
		    s_reading_from_canvas_enabled = false;
#endif

		  continue;
		}
	      else
		{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0))
		  if(dooble::s_default_web_engine_profile &&
		     key == "--disable-reading-from-canvas")
		    s_reading_from_canvas_enabled = false;
#endif
		}

	      string.append(key);

	      if(!singular)
		{
		  string.append("=");
		  string.append(query.value(1).toString().trimmed());
		}

	      string.append(" ");
	    }

	auto const old_environment
	  (qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS").trimmed());

	if(!old_environment.isEmpty())
	  {
	    string.prepend(" ");
	    string.prepend(old_environment);
	  }

	qputenv
	  ("QTWEBENGINE_CHROMIUM_FLAGS",
	   string.trimmed().toLocal8Bit().constData());
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_settings::prepare_web_engine_settings(void)
{
  disconnect(m_ui.web_engine_settings,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_web_engine_settings_item_changed(QTableWidgetItem *)));
  m_ui.web_engine_settings->setRowCount(0);

  QHash<QString, QVariant> values;
  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare
	  ("SELECT key, value FROM dooble_web_engine_settings "
	   "WHERE environment_variable = 1");

	if(query.exec())
	  while(query.next())
	    values[query.value(0).toString()] =
	      query.value(1).toString().trimmed();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);

  QHashIterator<QString, QString> it(s_web_engine_settings_environment);
  int i = -1;

  while(it.hasNext())
    {
      it.next();
      m_ui.web_engine_settings->setRowCount
	(m_ui.web_engine_settings->rowCount() + 1);

      auto item = new QTableWidgetItem(it.key());

      i += 1;
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item->setToolTip(item->text());
      m_ui.web_engine_settings->setItem(i, 0, item);
      item = new QTableWidgetItem();
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      item->setToolTip(tr("Not implemented."));
      m_ui.web_engine_settings->setItem(i, 1, item);
      item = new QTableWidgetItem();
      item->setData(Qt::UserRole, it.key());

      if(values.value(it.key()).toBool())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      if(it.value() == "boolean" || it.value() == "singular")
	item->setFlags
	  (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      else
	item->setFlags
	  (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      m_ui.web_engine_settings->setItem(i, 2, item);
    }

  connect(m_ui.web_engine_settings,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_web_engine_settings_item_changed(QTableWidgetItem *)));
  m_ui.web_engine_settings->resizeColumnToContents(0);
  m_ui.web_engine_settings->sortItems(0);
}

void dooble_settings::purge_database_data(void)
{
  dooble::s_accepted_or_blocked_domains->purge();
  dooble::s_certificate_exceptions->purge();
  dooble::s_downloads->purge();
  dooble::s_history->purge_all();
  dooble_certificate_exceptions_menu_widget::purge();
  dooble_cookies::purge();
  dooble_favicons::purge();
  dooble_style_sheet::purge();
  m_ui.new_javascript_block_popup_exception->clear();
  m_ui.new_javascript_disable->clear();
  m_ui.new_user_agent->clear();
  purge_features_permissions();
  purge_javascript_block_popup_exceptions();
  purge_javascript_disable();
  purge_user_agents();
  s_javascript_block_popup_exceptions.clear();
  s_javascript_disable.clear();
  s_site_features_permissions.clear();
  s_user_agents.clear();
  slot_remove_all_javascript_block_popup_exceptions();
  slot_remove_all_javascript_disable();
  slot_remove_all_user_agents();
}

void dooble_settings::purge_features_permissions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.features_permissions->setRowCount(0);

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_features_permissions");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::purge_javascript_block_popup_exceptions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.javascript_block_popups_exceptions->setRowCount(0);

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::purge_javascript_disable(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.javascript_disable->setRowCount(0);

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_javascript_disable");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::purge_user_agents(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.user_agents->setRowCount(0);

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_user_agents");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::remove_setting(const QString &key)
{
  if(key.trimmed().isEmpty())
    return;

  QWriteLocker lock(&s_settings_mutex);

  s_settings.remove(key.toLower().trimmed());
  lock.unlock();

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("DELETE FROM dooble_settings WHERE key = ?");
	query.addBindValue(key.toLower().trimmed());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_settings::resizeEvent(QResizeEvent *event)
{
  dooble_main_window::resizeEvent(event);
  save_settings();
}

void dooble_settings::restore(bool read_database)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(read_database)
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	db.setDatabaseName(setting("home_path").toString() +
			   QDir::separator() +
			   "dooble_settings.db");

	if(db.open())
	  {
	    create_tables(db);

	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT key, value, OID FROM dooble_settings"))
	      while(query.next())
		{
		  auto const key(query.value(0).toString().toLower().trimmed());
		  auto const value(query.value(1).toString().trimmed());

		  if(key.isEmpty() || value.isEmpty())
		    {
		      dooble_database_utilities::remove_entry
			(db,
			 "dooble_settings",
			 query.value(2).toLongLong());
		      continue;
		    }

		  QWriteLocker lock(&s_settings_mutex);

		  s_settings[key] = value;
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(database_name);
    }

  QWriteLocker lock(&s_settings_mutex);

  m_ui.access_new_tabs->setChecked
    (s_settings.value("access_new_tabs", true).toBool());
  m_ui.add_tab_behavior->setCurrentIndex
    (qBound(0,
	    s_settings.value("add_tab_behavior_index", 1).toInt(),
	    m_ui.add_tab_behavior->count() - 1));
  m_ui.address_widget_completer_mode->setCurrentIndex
    (qBound(0,
	    s_settings.value("address_widget_completer_mode_index", 1).toInt(),
	    m_ui.address_widget_completer_mode->count() - 1));
  m_ui.allow_closing_of_single_tab->setChecked
    (s_settings.value("allow_closing_of_single_tab", true).toBool());
  m_ui.animated_scrolling->setChecked
    (s_settings.value("animated_scrolling", false).toBool());
  m_ui.application_font->setChecked
    (s_settings.value("application_font", false).toBool());
  m_ui.automatic_loading_of_images->setChecked
    (s_settings.value("auto_load_images", true).toBool());
  m_ui.block_third_party_cookies->setChecked
    (s_settings.value("block_third_party_cookies", true).toBool());
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
  m_ui.dns_prefetch->setChecked
    (s_settings.value("dns_prefetch", false).toBool());
  m_ui.do_not_track->setChecked
    (s_settings.value("do_not_track", true).toBool());
  m_ui.download_version_information->setChecked
    (s_settings.value("download_version_information", false).toBool());
  m_ui.favicons->setChecked(s_settings.value("favicons", true).toBool());
  m_ui.features_permissions_groupbox->setChecked
    (s_settings.value("features_permissions", true).toBool());
  m_ui.full_screen->setChecked(s_settings.value("full_screen", true).toBool());
  m_ui.hash->setCurrentIndex
    (qBound(0,
	    s_settings.value("hash_type_index", 1).toInt(), // SHA3-512
	    m_ui.hash->count() - 1));

  auto const url
    (QUrl::fromEncoded(s_settings.value("home_url").toByteArray()));

  if(!url.isEmpty() && url.isValid())
    m_ui.home_url->setText(url.toString());
  else
    m_ui.home_url->setText("");

  m_ui.icon_set->setCurrentIndex
    (qBound(0,
	    s_settings.value("icon_set_index", 0).toInt(),
	    m_ui.icon_set->count() - 1));
  m_ui.iterations->setValue
    (s_settings.value("authentication_iteration_count", 15000).toInt());
  m_ui.javascript_access_clipboard->setChecked
    (s_settings.value("javascript_access_clipboard", false).toBool());
  m_ui.javascript_block_popups->setChecked
    (s_settings.value("javascript_block_popups", true).toBool());
  m_ui.language->setCurrentIndex
    (qBound(0,
	    s_settings.value("language_index", 0).toInt(),
	    m_ui.language->count()));
  m_ui.lefty_buttons->setChecked
    (s_settings.value("lefty_buttons", false).toBool());
  m_ui.local_storage->setChecked
    (s_settings.value("local_storage", true).toBool());
  m_ui.main_menu_bar_visible->setChecked
    (s_settings.value("main_menu_bar_visible", true).toBool());
  m_ui.main_menu_bar_visible_shortcut->setCurrentIndex
    (s_settings.value("main_menu_bar_visible_shortcut_index").toInt());

  if(m_ui.main_menu_bar_visible_shortcut->currentIndex() < 0)
    m_ui.main_menu_bar_visible_shortcut->setCurrentIndex(1); // F10

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
  m_ui.private_mode->setChecked
    (s_settings.value("private_mode", false).toBool());
  m_ui.proxy_host->setText(s_settings.value("proxy_host").toString().trimmed());
  m_ui.proxy_host->setCursorPosition(0);
  m_ui.proxy_password->setText(s_settings.value("proxy_password").toString());
  m_ui.proxy_password->setCursorPosition(0);
  m_ui.proxy_port->setValue(s_settings.value("proxy_port", 0).toInt());

  auto const index = s_settings.value("proxy_type_index", 0).toInt(); // None

  if(index == 0)
    m_ui.proxy_none->setChecked(true);
  else if(index == 1)
    m_ui.proxy_system->setChecked(true);
  else if(index == 2)
    m_ui.proxy_http->setChecked(true);
  else if(index == 3)
    m_ui.proxy_socks5->setChecked(true);
  else
    m_ui.proxy_none->setChecked(true);

  m_ui.proxy_user->setText(s_settings.value("proxy_user").toString().trimmed());
  m_ui.proxy_user->setCursorPosition(0);
  m_ui.referrer->setChecked(s_settings.value("referrer", false).toBool());
  m_ui.relative_location_character->setText
    (s_settings.value("relative_location_character", "").toString().trimmed());
  m_ui.retain_session_tabs->setChecked
    (s_settings.value("retain_session_tabs", false).toBool());
  m_ui.save_geometry->setChecked
    (s_settings.value("save_geometry", true).toBool());
  m_ui.show_address_widget_completer->setChecked
    (s_settings.value("show_address_widget_completer", true).toBool());
  m_ui.show_hovered_links_tool_tips->setChecked
    (s_settings.value("show_hovered_links_tool_tips", false).toBool());
  m_ui.show_left_corner_widget->setChecked
    (s_settings.value("show_left_corner_widget", true).toBool());
  m_ui.show_loading_gradient->setChecked
    (s_settings.value("show_loading_gradient", true).toBool());
  m_ui.show_new_downloads->setChecked
    (s_settings.value("show_new_downloads", true).toBool());
  m_ui.show_title_bar->setChecked
    (s_settings.value("show_title_bar", true).toBool());
  m_ui.splash_screen->setChecked
    (s_settings.value("splash_screen", true).toBool());
  m_ui.tab_document_mode->setChecked
    (s_settings.value("tab_document_mode", true).toBool());

  auto const tab_position
    (s_settings.value("tab_position").toString().trimmed());

  if(tab_position == "east")
    m_ui.tab_position->setCurrentIndex(0);
  else if(tab_position == "south")
    m_ui.tab_position->setCurrentIndex(2);
  else if(tab_position == "west")
    m_ui.tab_position->setCurrentIndex(3);
  else
    m_ui.tab_position->setCurrentIndex(1);

  m_ui.temporarily_disable_javascript->setChecked
    (s_settings.value("temporarily_disable_javascript", false).toBool());
#ifdef Q_OS_WINDOWS
  m_ui.theme->setCurrentIndex
    (qBound(0,
	    s_settings.value("theme_color_index", 2).toInt(),
	    m_ui.theme->count() - 1));
#else
    m_ui.theme->setCurrentIndex(2); // Default
#endif
  m_ui.web_plugins->setChecked(s_settings.value("web_plugins", false).toBool());
  m_ui.webgl->setChecked(s_settings.value("webgl", true).toBool());
  m_ui.zoom->setValue(s_settings.value("zoom", 100).toInt());
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

  if(m_ui.hash->currentIndex() == 0)
    s_settings["hash_type"] = "Keccak-512";
  else
    s_settings["hash_type"] = "SHA3-512";

  if(m_ui.home_url->text().trimmed().isEmpty())
    s_settings["home_url"] = QUrl();
  else
    s_settings["home_url"] =
      QUrl::fromUserInput(m_ui.home_url->text()).toEncoded();

  s_settings["icon_set"] = "Material Design";

  switch(m_ui.theme->currentIndex())
    {
    case 0:
      {
	s_settings["theme_color"] = "blue-grey";
	break;
      }
    case 1:
      {
	s_settings["theme_color"] = "dark";
	break;
      }
    case 2:
      {
	s_settings["theme_color"] = "default";
	break;
      }
    case 3:
      {
	s_settings["theme_color"] = "indigo";
	break;
      }
    case 4:
      {
	s_settings["theme_color"] = "orange";
	break;
      }
    default:
      {
	s_settings["theme_color"] = "default";
	break;
      }
    }

  s_settings["theme_color_index"] = m_ui.theme->currentIndex();
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

  m_ui.webrtc_public_interfaces_only->setChecked
    (s_settings.value("webrtc_public_interfaces_only", true).toBool());
  m_ui.xss_auditing->setChecked
    (s_settings.value("xss_auditing", false).toBool());
  lock.unlock();
  m_ui.reset_credentials->setEnabled(has_dooble_credentials());
  dooble::s_default_web_engine_profile->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    dooble::s_default_web_engine_profile->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    dooble::s_default_web_engine_profile->setHttpCacheType
      (QWebEngineProfile::NoCache);

  {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    auto list
      (setting("dictionaries").toString().split(";", Qt::SkipEmptyParts));
#else
    auto list
      (setting("dictionaries").toString().split(";", QString::SkipEmptyParts));
#endif

    std::sort(list.begin(), list.end());

    for(int i = 0; i < m_ui.dictionaries->count(); i++)
      {
	auto item = m_ui.dictionaries->item(i);

	if(!item)
	  continue;

	if(list.contains(item->text()))
	  item->setCheckState(Qt::Checked);
      }

    dooble::s_default_web_engine_profile->setSpellCheckLanguages(list);
  }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::AutoLoadImages,
     m_ui.automatic_loading_of_images->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly,
     m_ui.webrtc_public_interfaces_only->isChecked());
#endif
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());
#else
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::AutoLoadImages,
     m_ui.automatic_loading_of_images->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly,
     m_ui.webrtc_public_interfaces_only->isChecked());
#endif
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());
#endif
  {
    static auto const list(QList<QPushButton *> () << m_ui.cache
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

  prepare_fonts();
  prepare_proxy(false);
  prepare_web_engine_settings();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::save(void)
{
  slot_apply();
}

void dooble_settings::save_fonts(void)
{
  /*
  ** Fonts
  */

  {
    QMap<QWebEngineSettings::FontFamily, QPair<QString, QString> > fonts;

    fonts[QWebEngineSettings::CursiveFont] = QPair<QString, QString>
      ("web_font_cursive", m_ui.web_font_cursive->currentFont().family());
    fonts[QWebEngineSettings::FantasyFont] = QPair<QString, QString>
      ("web_font_fantasy", m_ui.web_font_fantasy->currentFont().family());
    fonts[QWebEngineSettings::FixedFont] = QPair<QString, QString>
      ("web_font_fixed", m_ui.web_font_fixed->currentFont().family());
    fonts[QWebEngineSettings::PictographFont] = QPair<QString, QString>
      ("web_font_pictograph", m_ui.web_font_pictograph->currentFont().family());
    fonts[QWebEngineSettings::SansSerifFont] = QPair<QString, QString>
      ("web_font_sans_serif", m_ui.web_font_sans_serif->currentFont().family());
    fonts[QWebEngineSettings::SerifFont] = QPair<QString, QString>
      ("web_font_serif", m_ui.web_font_serif->currentFont().family());
    fonts[QWebEngineSettings::StandardFont] = QPair<QString, QString>
      ("web_font_standard", m_ui.web_font_standard->currentFont().family());

    QMapIterator<QWebEngineSettings::FontFamily, QPair<QString, QString> >
      it(fonts);

    while(it.hasNext())
      {
	it.next();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QWebEngineSettings::defaultSettings()->setFontFamily
	  (it.key(), it.value().second);
#else
	dooble::s_default_web_engine_profile->settings()->setFontFamily
	  (it.key(), it.value().second);
#endif
	set_setting(it.value().first, it.value().second);
      }
  }

  /*
  ** Sizes
  */

  {
    QMap<QWebEngineSettings::FontSize, QPair<QString, int> > sizes;

    sizes[QWebEngineSettings::DefaultFixedFontSize] = QPair<QString, int>
      ("web_font_size_default_fixed",
       m_ui.web_font_size_default_fixed->value());
    sizes[QWebEngineSettings::DefaultFontSize] = QPair<QString, int>
      ("web_font_size_default", m_ui.web_font_size_default->value());
    sizes[QWebEngineSettings::MinimumFontSize] = QPair<QString, int>
      ("web_font_size_minimum", m_ui.web_font_size_minimum->value());
    sizes[QWebEngineSettings::MinimumLogicalFontSize] = QPair<QString, int>
      ("web_font_size_minimum_logical",
       m_ui.web_font_size_minimum_logical->value());

    QMapIterator<QWebEngineSettings::FontSize, QPair<QString, int> > it(sizes);

    while(it.hasNext())
      {
	it.next();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QWebEngineSettings::defaultSettings()->setFontSize
	  (it.key(), it.value().second);
#else
	dooble::s_default_web_engine_profile->settings()->setFontSize
	  (it.key(), it.value().second);
#endif
	set_setting(it.value().first, it.value().second);
      }
  }
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

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_javascript_block_popup_exceptions "
	   "(state, url, url_digest) VALUES (?, ?, ?)");

	auto data
	  (dooble::s_cryptography->
	   encrypt_then_mac(state ? QByteArray("true") : QByteArray("false")));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(url.toEncoded());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
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

void dooble_settings::set_settings_path(const QString &path)
{
  m_ui.settings_path->setText(path);
}

void dooble_settings::set_site_feature_permission
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
(const QUrl &url, const QWebEnginePage::Feature feature, bool state)
#else
(const QUrl &url,
 const QWebEnginePermission::PermissionType feature,
 bool state)
#endif
{
  if(url.isEmpty() || url.isValid() == false)
    return;
  else if(!setting("features_permissions").toBool())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(site_feature_permission(url, feature) == -1)
    {
      disconnect
	(m_ui.features_permissions,
	 SIGNAL(itemChanged(QTableWidgetItem *)),
	 this,
	 SLOT(slot_features_permissions_item_changed(QTableWidgetItem *)));
      m_ui.features_permissions->setRowCount
	(m_ui.features_permissions->rowCount() + 1);

      auto item = new QTableWidgetItem();

      if(state)
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, url);
      item->setData
	(Qt::ItemDataRole(Qt::UserRole + 1), static_cast<int> (feature));
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.features_permissions->setItem
	(m_ui.features_permissions->rowCount() - 1, 0, item);
      item = new QTableWidgetItem
	(dooble_text_utilities::
	 web_engine_page_feature_to_pretty_string(feature));
      item->setData(Qt::UserRole, url);
      item->setData
	(Qt::ItemDataRole(Qt::UserRole + 1), static_cast<int> (feature));
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.features_permissions->setItem
	(m_ui.features_permissions->rowCount() - 1, 1, item);
      item = new QTableWidgetItem(url.toString());
      item->setData(Qt::UserRole, url);
      item->setData
	(Qt::ItemDataRole(Qt::UserRole + 1), static_cast<int> (feature));
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item->setToolTip(item->text());
      m_ui.features_permissions->setItem
	(m_ui.features_permissions->rowCount() - 1, 2, item);
      m_ui.features_permissions->sortItems(2);
      connect(m_ui.features_permissions,
	      SIGNAL(itemChanged(QTableWidgetItem *)),
	      this,
	      SLOT(slot_features_permissions_item_changed(QTableWidgetItem *)));
      prepare_table_statistics();
    }
  else
    {
      auto const values(s_site_features_permissions.values(url));

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
      foreach(auto const &value, values)
	if(feature == QWebEnginePage::Feature(value.first) &&
	   value.first != -1)
	  {
	    s_site_features_permissions.remove
	      (url, QPair<int, bool> (value.first, false));
	    s_site_features_permissions.remove
	      (url, QPair<int, bool> (value.first, true));
	    break;
	  }
#else
      foreach(auto const &value, values)
	if(feature == QWebEnginePermission::PermissionType(value.first) &&
	   value.first != -1)
	  {
	    s_site_features_permissions.remove
	      (url, QPair<int, bool> (value.first, false));
	    s_site_features_permissions.remove
	      (url, QPair<int, bool> (value.first, true));
	    break;
	  }
#endif
    }

  s_site_features_permissions.insert
    (url, QPair<int, bool> (static_cast<int> (feature), state));
  QApplication::restoreOverrideCursor();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.prepare
	  ("INSERT OR REPLACE INTO dooble_features_permissions "
	   "(feature, feature_digest, permission, url, url_digest) "
	   "VALUES (?, ?, ?, ?, ?)");

	auto data
	  (dooble::s_cryptography->
	   encrypt_then_mac(QByteArray::number(static_cast<int> (feature))));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac
	  (QByteArray::number(static_cast<int> (feature)));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac
	  (state ? QByteArray("true") : QByteArray("false"));

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac(url.toEncoded());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(url.toEncoded());

	if(data.isEmpty())
	  goto done_label;
	else
	  query.addBindValue(data.toBase64());

	query.exec();
      }

  done_label:
    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_settings::show(void)
{
  if(!isVisible())
    restore(false);

  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  dooble_main_window::show();
}

void dooble_settings::show_normal(QWidget *parent)
{
  if(!isVisible())
    restore(false);

  if(setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(setting("settings_geometry").
					   toByteArray()));

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(parent, this);

  dooble_main_window::showNormal();
}

void dooble_settings::show_panel(dooble_settings::Panels panel)
{
  switch(panel)
    {
    case dooble_settings::Panels::DISPLAY_PANEL:
      {
	m_ui.display->click();
	break;
      }
    case dooble_settings::Panels::CACHE_PANEL:
      {
	m_ui.cache->click();
	break;
      }
    case dooble_settings::Panels::HISTORY_PANEL:
      {
	m_ui.history->click();
	break;
      }
    case dooble_settings::Panels::PRIVACY_PANEL:
      {
	m_ui.privacy->click();
	break;
      }
    case dooble_settings::Panels::WEB_PANEL:
      {
	m_ui.web->click();
	break;
      }
    case dooble_settings::Panels::WINDOWS_PANEL:
      {
	m_ui.windows->click();
	break;
      }
    default:
      {
	m_ui.display->click();
	break;
      }
    }
}

void dooble_settings::show_qtwebengine_dictionaries_warning_label(void)
{
  m_ui.qtwebengine_dictionaries_warning_label->setVisible(false);

  auto const bytes(qEnvironmentVariable("QTWEBENGINE_DICTIONARIES_PATH"));

  if(bytes.trimmed().isEmpty())
    {
      auto const directory
	(QDir::currentPath() + QDir::separator() + "qtwebengine_dictionaries");

      m_ui.qtwebengine_dictionaries_warning_label->setText
	(tr("<b>Warning!</b> "
	    "The directory qtwebengine_dictionaries cannot be accessed. "
	    "Dooble searched %1. Please read %2, line %3.").
	 arg(directory).arg(__FILE__).arg(__LINE__));

      if(!QFileInfo(directory).isReadable())
	{
	  m_ui.qtwebengine_dictionaries_warning_label->setVisible(true);
	  return;
	}
    }
  else if(!QFileInfo(bytes).isReadable())
    {
      m_ui.qtwebengine_dictionaries_warning_label->setText
	(tr("<b>Warning!</b> "
	    "The directory qtwebengine_dictionaries cannot be accessed. "
	    "Dooble searched %1. Please read %2, line %3.").
	 arg(bytes.constData()).arg(__FILE__).arg(__LINE__));
      m_ui.qtwebengine_dictionaries_warning_label->setVisible(true);
      return;
    }

  for(int i = 0; i < m_ui.dictionaries->count(); i++)
    {
      auto item = m_ui.dictionaries->item(i);

      if(item && item->checkState() == Qt::Checked)
	{
	  QString b(bytes);

	  b.append(QDir::separator());
	  b.append(item->text());
	  b.append(".bdic");

	  if(!QFileInfo(b).isReadable())
	    {
	      m_ui.qtwebengine_dictionaries_warning_label->setText
		(tr("<b>Warning!</b> %1 cannot be accessed!").arg(b));
	      m_ui.qtwebengine_dictionaries_warning_label->setVisible(true);
	      return;
	    }
	}
    }
}

void dooble_settings::slot_apply(void)
{
  if(m_ui.credentials->isChecked() != setting("credentials_enabled").toBool())
    {
      show_panel(dooble_settings::Panels::PRIVACY_PANEL);

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
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  m_ui.credentials->setChecked(true);
	  return;
	}

      QApplication::processEvents();
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      /*
      ** Remove settings.
      */

      m_ui.cipher->setCurrentIndex(0);
      m_ui.hash->setCurrentIndex(1); // SHA3-512
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
      remove_setting("hash_type");
      remove_setting("hash_type_index");

      {
	QWriteLocker locker(&s_settings_mutex);

	s_settings["block_cipher_type"] = "AES-256";
	s_settings["block_cipher_type_index"] = 0;
	s_settings["hash_type"] = "SHA3-512";
	s_settings["hash_type_index"] = 1;
      }

      dooble::s_cryptography->set_block_cipher_type("AES-256");
      dooble::s_cryptography->set_hash_type("SHA3-512");

      if(m_ui.credentials->isChecked())
	{
	  /*
	  ** Generate temporary credentials.
	  */

	  auto authentication_key
	    (dooble_random::
	     random_bytes(dooble_cryptography::s_authentication_key_length));
	  auto encryption_key
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
  dooble::s_default_web_engine_profile->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    dooble::s_default_web_engine_profile->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    dooble::s_default_web_engine_profile->setHttpCacheType
      (QWebEngineProfile::NoCache);

  {
    QString text("");
    QStringList list;

    for(int i = 0; i < m_ui.dictionaries->count(); i++)
      {
	auto item = m_ui.dictionaries->item(i);

	if(!item)
	  continue;

	if(item->checkState() == Qt::Checked)
	  {
	    list << item->text();
	    text.append(item->text());
	    text.append(";");
	  }
      }

    dooble::s_default_web_engine_profile->setSpellCheckLanguages(list);
    set_setting("dictionaries", text);
  }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::AutoLoadImages,
     m_ui.automatic_loading_of_images->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::DnsPrefetchEnabled,
     m_ui.dns_prefetch->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly,
     m_ui.webrtc_public_interfaces_only->isChecked());
#endif
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());
#else
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::AutoLoadImages,
     m_ui.automatic_loading_of_images->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::DnsPrefetchEnabled,
     m_ui.dns_prefetch->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::JavascriptCanAccessClipboard,
     m_ui.javascript_access_clipboard->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled, m_ui.javascript->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, m_ui.local_storage->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::PluginsEnabled, m_ui.web_plugins->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     m_ui.animated_scrolling->isChecked());
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::WebGLEnabled, m_ui.webgl->isChecked());
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly,
     m_ui.webrtc_public_interfaces_only->isChecked());
#endif
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled, m_ui.xss_auditing->isChecked());
#endif

  {
    QWriteLocker locker(&s_settings_mutex);

    switch(m_ui.theme->currentIndex())
      {
      case 0:
	{
	  s_settings["theme_color"] = "blue-grey";
	  break;
	}
      case 1:
	{
	  s_settings["theme_color"] = "dark";
	  break;
	}
      case 2:
	{
	  s_settings["theme_color"] = "default";
	  break;
	}
      case 3:
	{
	  s_settings["theme_color"] = "indigo";
	  break;
	}
      case 4:
	{
	  s_settings["theme_color"] = "orange";
	  break;
	}
      default:
	{
	  s_settings["theme_color"] = "default";
	  break;
	}
      }
  }

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
  save_fonts();
  set_setting("access_new_tabs", m_ui.access_new_tabs->isChecked());
  set_setting("add_tab_behavior_index", m_ui.add_tab_behavior->currentIndex());
  set_setting("address_widget_completer_mode_index",
	      m_ui.address_widget_completer_mode->currentIndex());
  set_setting("allow_closing_of_single_tab",
	      m_ui.allow_closing_of_single_tab->isChecked());
  set_setting("animated_scrolling", m_ui.animated_scrolling->isChecked());
  set_setting("application_font", m_ui.application_font->isChecked());
  set_setting
    ("auto_load_images", m_ui.automatic_loading_of_images->isChecked());
  set_setting
    ("block_third_party_cookies", m_ui.block_third_party_cookies->isChecked());
  set_setting("browsing_history_days", m_ui.browsing_history->value());
  set_setting("cache_size", m_ui.cache_size->value());
  set_setting("cache_type_index", m_ui.cache_type->currentIndex());
  set_setting("center_child_windows", m_ui.center_child_windows->isChecked());
  set_setting("cookie_policy_index", m_ui.cookie_policy->currentIndex());
  set_setting("credentials_enabled", m_ui.credentials->isChecked());
  set_setting
    ("denote_private_widgets", m_ui.denote_private_widgets->isChecked());
  set_setting
    ("display_application_font", m_ui.display_application_font->text());
  set_setting("dns_prefetch", m_ui.dns_prefetch->isChecked());
  set_setting("do_not_track", m_ui.do_not_track->isChecked());
  set_setting
    ("download_version_information",
     m_ui.download_version_information->isChecked());
  set_setting("favicons", m_ui.favicons->isChecked());
  set_setting
    ("features_permissions", m_ui.features_permissions_groupbox->isChecked());
  set_setting("full_screen", m_ui.full_screen->isChecked());

  if(m_ui.home_url->text().trimmed().isEmpty())
    set_setting("home_url", QUrl());
  else
    {
      auto const url(QUrl::fromUserInput(m_ui.home_url->text().trimmed()));

      m_ui.home_url->setText(url.toString());
      set_setting("home_url", url.toEncoded());
    }

  set_setting("icon_set_index", m_ui.icon_set->currentIndex());

  {
    QWriteLocker locker(&s_settings_mutex);

    s_settings["icon_set"] = "Material Design";
  }

  set_setting("javascript_access_clipboard",
	      m_ui.javascript_access_clipboard->isChecked());
  set_setting
    ("javascript_block_popups", m_ui.javascript_block_popups->isChecked());
  set_setting("language_index", m_ui.language->currentIndex());
  set_setting("lefty_buttons", m_ui.lefty_buttons->isChecked());
  set_setting("local_storage", m_ui.local_storage->isChecked());
  set_setting("main_menu_bar_visible", m_ui.main_menu_bar_visible->isChecked());
  set_setting("main_menu_bar_visible_shortcut_index",
	      m_ui.main_menu_bar_visible_shortcut->currentIndex());
  set_setting("pin_accepted_or_blocked_window",
	      m_ui.pin_accepted_or_blocked_domains->isChecked());
  set_setting("pin_downloads_window", m_ui.pin_downloads->isChecked());
  set_setting("pin_history_window", m_ui.pin_history->isChecked());
  set_setting("pin_settings_window", m_ui.pin_settings->isChecked());
  set_setting("private_mode", m_ui.private_mode->isChecked());
  set_setting("referrer", m_ui.referrer->isChecked());
  set_setting("relative_location_character",
	      m_ui.relative_location_character->text().trimmed());
  set_setting("retain_session_tabs", m_ui.retain_session_tabs->isChecked());
  set_setting("save_geometry", m_ui.save_geometry->isChecked());
  set_setting("show_address_widget_completer",
	      m_ui.show_address_widget_completer->isChecked());
  set_setting("show_hovered_links_tool_tips",
	      m_ui.show_hovered_links_tool_tips->isChecked());
  set_setting("show_left_corner_widget",
	      m_ui.show_left_corner_widget->isChecked());
  set_setting("show_loading_gradient", m_ui.show_loading_gradient->isChecked());
  set_setting("show_new_downloads", m_ui.show_new_downloads->isChecked());
  set_setting("show_title_bar", m_ui.show_title_bar->isChecked());
  set_setting("splash_screen", m_ui.splash_screen->isChecked());
  set_setting("tab_document_mode", m_ui.tab_document_mode->isChecked());

  switch(m_ui.tab_position->currentIndex())
    {
    case 0:
      {
	set_setting("tab_position", "east");
	break;
      }
    case 1:
    default:
      {
	set_setting("tab_position", "north");
	break;
      }
    case 2:
      {
	set_setting("tab_position", "south");
	break;
      }
    case 3:
      {
	set_setting("tab_position", "west");
	break;
      }
    }

  set_setting("temporarily_disable_javascript",
	      m_ui.temporarily_disable_javascript->isChecked());
  set_setting("theme_color_index", m_ui.theme->currentIndex());
  set_setting("utc_time_zone", m_ui.utc_time_zone->isChecked());
  set_setting("visited_links", m_ui.visited_links->isChecked());
  set_setting("web_plugins", m_ui.web_plugins->isChecked());
  set_setting("webgl", m_ui.webgl->isChecked());
  set_setting("webrtc_public_interfaces_only",
	      m_ui.webrtc_public_interfaces_only->isChecked());
  set_setting("xss_auditing", m_ui.xss_auditing->isChecked());
  set_setting("zoom", m_ui.zoom->value());
  set_setting
    ("zoom_frame_location_index", m_ui.zoom_frame_location->currentIndex());
  prepare_application_fonts();
  prepare_icons();
  QApplication::restoreOverrideCursor();
  emit applied();
}

void dooble_settings::slot_clear_cache(void)
{
  dooble::s_default_web_engine_profile->clearHttpCache();
}

void dooble_settings::slot_features_permissions_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
  set_site_feature_permission
    (item->data(Qt::UserRole).toUrl(),
     QWebEnginePage::Feature(item->data(Qt::ItemDataRole(Qt::UserRole + 1)).
			     toInt()),
     item->checkState() == Qt::Checked);
#else
  set_site_feature_permission
    (item->data(Qt::UserRole).toUrl(),
     QWebEnginePermission::
     PermissionType(item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt()),
     item->checkState() == Qt::Checked);
#endif
}

void dooble_settings::slot_general_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  show_qtwebengine_dictionaries_warning_label();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_javascript_block_popups_exceptions_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  auto const state = item->checkState() == Qt::Checked;

  item = m_ui.javascript_block_popups_exceptions->item(item->row(), 1);

  if(!item)
    return;

  save_javascript_block_popup_exception(item->text(), state);
}

void dooble_settings::slot_javascript_disable_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  auto const state = item->checkState() == Qt::Checked;

  item = m_ui.javascript_disable->item(item->row(), 1);

  if(!item)
    return;

  new_javascript_disable(item->text(), state);
}

void dooble_settings::slot_new_javascript_block_popup_exception
(const QUrl &url)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const list(m_ui.javascript_block_popups_exceptions->
		  findItems(url.toString(), Qt::MatchExactly));

  if(!list.isEmpty() && list.at(0))
    m_ui.javascript_block_popups_exceptions->removeRow(list.at(0)->row());

  prepare_table_statistics();
  s_javascript_block_popup_exceptions.remove(url);
  QApplication::restoreOverrideCursor();
  new_javascript_block_popup_exception(url);
}

void dooble_settings::slot_new_javascript_block_popup_exception(void)
{
  new_javascript_block_popup_exception
    (QUrl::fromUserInput(m_ui.new_javascript_block_popup_exception->text()));
}

void dooble_settings::slot_new_javascript_disable(const QUrl &url, bool state)
{
  auto const domain(url.host());

  if(domain.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const list
    (m_ui.javascript_disable->findItems(domain, Qt::MatchExactly));

  if(!list.isEmpty() && list.at(0))
    m_ui.javascript_disable->removeRow(list.at(0)->row());

  disconnect
    (m_ui.javascript_disable,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_disable_item_changed(QTableWidgetItem *)));
  m_ui.javascript_disable->setRowCount
    (m_ui.javascript_disable->rowCount() + 1);
  m_ui.new_javascript_disable->clear();

  auto item = new QTableWidgetItem();

  item->setCheckState(state ? Qt::Checked : Qt::Unchecked);
  item->setData(Qt::UserRole, domain);
  item->setFlags(Qt::ItemIsEnabled |
		 Qt::ItemIsSelectable |
		 Qt::ItemIsUserCheckable);
  m_ui.javascript_disable->setItem
    (m_ui.javascript_disable->rowCount() - 1, 0, item);
  item = new QTableWidgetItem(domain);
  item->setData(Qt::UserRole, domain);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item->setToolTip(item->text());
  m_ui.javascript_disable->setItem
    (m_ui.javascript_disable->rowCount() - 1, 1, item);
  m_ui.javascript_disable->sortItems(1);
  connect
    (m_ui.javascript_disable,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_disable_item_changed(QTableWidgetItem *)));
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
  new_javascript_disable(domain, state);
}

void dooble_settings::slot_new_javascript_disable(void)
{
  slot_new_javascript_disable
    (QUrl::fromUserInput(m_ui.new_javascript_disable->text().trimmed()), true);
}

void dooble_settings::slot_new_user_agent(const QString &u, const QUrl &url)
{
  auto const domain(url.host());

  if(domain.isEmpty())
    return;

  auto const user_agent(u.trimmed());

  if(user_agent.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto const list
    (m_ui.user_agents->findItems(domain, Qt::MatchExactly));

  if(!list.isEmpty() && list.at(0))
    m_ui.user_agents->removeRow(list.at(0)->row());

  disconnect
    (m_ui.user_agents,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  m_ui.new_user_agent->clear();
  m_ui.user_agents->setRowCount(m_ui.user_agents->rowCount() + 1);

  auto item = new QTableWidgetItem(domain);

  item->setData(Qt::UserRole, domain);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item->setToolTip(item->text());
  m_ui.user_agents->setItem(m_ui.user_agents->rowCount() - 1, 0, item);
  item = new QTableWidgetItem(user_agent);
  item->setData(Qt::UserRole, domain);
  item->setFlags
    (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item->setToolTip(item->text());
  m_ui.user_agents->setItem(m_ui.user_agents->rowCount() - 1, 1, item);
  m_ui.user_agents->sortItems(0);
  connect
    (m_ui.user_agents,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
  new_user_agent(url.host(), user_agent);
}

void dooble_settings::slot_new_user_agent(void)
{
  slot_new_user_agent
    (s_http_user_agent,
     QUrl::fromUserInput(m_ui.new_user_agent->text().trimmed()));
}

void dooble_settings::slot_page_button_clicked(void)
{
  auto tool_button = qobject_cast<QPushButton *> (sender());

  if(!tool_button)
    return;

  static auto const list(QList<QPushButton *> () << m_ui.cache
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

void dooble_settings::slot_password_changed(void)
{
  auto const password_1(m_ui.password_1->text());
  auto const password_2(m_ui.password_2->text());

  if(password_1.isEmpty() || password_1 != password_2)
    {
      m_ui.password_1->setStyleSheet("QLineEdit {background-color: #f2dede;}");
      m_ui.password_2->setStyleSheet(m_ui.password_1->styleSheet());
    }
  else
    {
      m_ui.password_1->setStyleSheet("QLineEdit {background-color: #90ee90;}");
      m_ui.password_2->setStyleSheet(m_ui.password_1->styleSheet());
    }
}

void dooble_settings::slot_pbkdf2_future_finished(void)
{
  m_ui.password_1->setText
    (dooble_random::random_bytes(m_ui.password_1->text().length()).toHex());
  m_ui.password_1->clear();
  m_ui.password_2->setText
    (dooble_random::random_bytes(m_ui.password_2->text().length()).toHex());
  m_ui.password_2->clear();
  slot_password_changed();

  auto was_canceled = false;

  if(m_pbkdf2_dialog)
    {
      if(m_pbkdf2_dialog->wasCanceled())
	was_canceled = true;

      m_pbkdf2_dialog->cancel();
      m_pbkdf2_dialog->deleteLater();
    }

  if(!was_canceled)
    {
      QString error("");
      auto const list(m_pbkdf2_future.result());
      auto ok = true;

      if(list.size() == 6)
	{
	  /*
	  ** list[0] - Keys
	  ** list[1] - Block Cipher Type Index
	  ** list[2] - Hash Type Index
	  ** list[3] - Iteration Count
	  ** list[4] - Password
	  ** list[5] - Salt
	  */

	  if(!set_setting("authentication_iteration_count",
			  list.at(3).toInt()))
	    {
	      error = "set_setting('authentication_iteration_count') failure";
	      ok = false;
	    }

	  if(ok && !set_setting("authentication_salt", list.at(5).toHex()))
	    {
	      error = "set_setting('authentication_salt') failure";
	      ok = false;
	    }

	  auto algorithm = QCryptographicHash::Sha3_512;

	  if(list.at(2).toInt() == 0)
	    algorithm = QCryptographicHash::Keccak_512;

	  if(ok && !set_setting("authentication_salted_password",
				QCryptographicHash::hash(list.at(4) +
							 list.at(5),
							 algorithm).toHex()))
	    {
	      error = "set_setting('authentication_salted_password') failure";
	      ok = false;
	    }

	  if(ok && !set_setting("block_cipher_type_index", list.at(1).toInt()))
	    {
	      error = "set_setting('block_cipher_type_index') failure";
	      ok = false;
	    }

	  if(ok && !set_setting("credentials_enabled", true))
	    {
	      error = "set_setting('credentials_enabled') failure";
	      ok = false;
	    }

	  if(ok && !set_setting("hash_type_index", list.at(2).toInt()))
	    {
	      error = "set_setting('hash_type_index') failure";
	      ok = false;
	    }

	  if(ok)
	    {
	      QWriteLocker locker(&s_settings_mutex);

	      if(list.at(1).toInt() == 0)
		s_settings["block_cipher_type"] = "AES-256";
	      else
		s_settings["block_cipher_type"] = "Threefish-256";

	      if(list.at(2).toInt() == 0)
		s_settings["hash_type"] = "Keccak-512";
	      else
		s_settings["hash_type"] = "SHA3-512";
	    }
	  else
	    {
	      remove_setting("authentication_iteration_count");
	      remove_setting("authentication_salt");
	      remove_setting("authentication_salted_password");
	      remove_setting("block_cipher_type");
	      remove_setting("block_cipher_type_index");
	      remove_setting("credentials_enabled");
	      remove_setting("hash_type");
	      remove_setting("hash_type_index");

	      {
		QWriteLocker locker(&s_settings_mutex);

		s_settings["block_cipher_type"] = "AES-256";
		s_settings["block_cipher_type_index"] = 0;
		s_settings["hash_type"] = "SHA3-512";
		s_settings["hash_type_index"] = 1;
	      }
	    }

	  if(ok)
	    {
	      dooble::s_cryptography->set_authenticated(true);

	      if(list.at(1).toInt() == 0)
		dooble::s_cryptography->set_block_cipher_type("AES-256");
	      else
		dooble::s_cryptography->set_block_cipher_type("Threefish-256");

	      if(list.at(2).toInt() == 0)
		dooble::s_cryptography->set_hash_type("Keccak-512");
	      else
		dooble::s_cryptography->set_hash_type("SHA3-512");

	      auto authentication_key
		(list.value(0).
		 mid(0, dooble_cryptography::s_authentication_key_length));
	      auto encryption_key
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
	{
	  error = "m_pbkdf2_future.result() is empty";
	  ok = false;
	}

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
	      "This is a curious problem (%1).").arg(error));

      QApplication::processEvents();
    }
}

void dooble_settings::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      emit populated();
      return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.features_permissions->setRowCount(0);
  m_ui.javascript_block_popups_exceptions->setRowCount(0);
  m_ui.javascript_disable->setRowCount(0);
  m_ui.user_agents->setRowCount(0);
  prepare_table_statistics();
  s_javascript_block_popup_exceptions.clear();
  s_javascript_disable.clear();
  s_site_features_permissions.clear();
  s_user_agents.clear();

  auto const database_name(dooble_database_utilities::database_name());
  int count_1 = 0;

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_settings.db");

    if(db.open())
      {
	create_tables(db);

	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT feature, permission, url, OID "
		      "FROM dooble_features_permissions"))
	  while(query.next())
	    {
	      auto data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_features_permissions",
		     query.value(3).toLongLong());
		  continue;
		}

	      auto data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_features_permissions",
		     query.value(3).toLongLong());
		  continue;
		}

	      auto data3
		(QByteArray::fromBase64(query.value(2).toByteArray()));

	      data3 = dooble::s_cryptography->mac_then_decrypt(data3);

	      if(data3.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_features_permissions",
		     query.value(3).toLongLong());
		  continue;
		}

	      QUrl const url(data3);

	      if(url.isEmpty() || !url.isValid())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_features_permissions",
		     query.value(3).toLongLong());
		  continue;
		}

	      count_1 += 1;
	      s_site_features_permissions.insert
		(url,
		 QPair<int, bool> (data1.toInt(),
				   data2 == "true" ? true : false));
	    }

	if(query.exec("SELECT state, url, OID "
		      "FROM dooble_javascript_block_popup_exceptions"))
	  while(query.next())
	    {
	      auto data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_javascript_block_popup_exceptions",
		     query.value(2).toLongLong());
		  continue;
		}

	      auto data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_javascript_block_popup_exceptions",
		     query.value(2).toLongLong());
		  continue;
		}

	      QUrl const url(data2);

	      if(url.isEmpty() || !url.isValid())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_javascript_block_popup_exceptions",
		     query.value(2).toLongLong());
		  continue;
		}

	      s_javascript_block_popup_exceptions[url] =
		(data1 == "true") ? 1 : 0;
	    }

	if(query.exec("SELECT state, url_domain, OID "
		      "FROM dooble_javascript_disable"))
	  while(query.next())
	    {
	      auto data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_javascript_disable",
		     query.value(2).toLongLong());
		  continue;
		}

	      auto data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_javascript_disable",
		     query.value(2).toLongLong());
		  continue;
		}

	      s_javascript_disable[data2] = (data1 == "true") ? 1 : 0;
	    }

	if(query.exec("SELECT url_domain, user_agent, OID "
		      "FROM dooble_user_agents"))
	  while(query.next())
	    {
	      auto data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_user_agents",
		     query.value(2).toLongLong());
		  continue;
		}

	      auto data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		{
		  dooble_database_utilities::remove_entry
		    (db,
		     "dooble_user_agents",
		     query.value(2).toLongLong());
		  continue;
		}

	      s_user_agents[data1] = data2;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  disconnect(m_ui.features_permissions,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_features_permissions_item_changed(QTableWidgetItem *)));
  disconnect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  disconnect(m_ui.javascript_disable,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_javascript_disable_item_changed(QTableWidgetItem *)));
  disconnect(m_ui.user_agents,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  m_ui.features_permissions->setRowCount(count_1);
  m_ui.javascript_block_popups_exceptions->setRowCount
    (s_javascript_block_popup_exceptions.size());
  m_ui.javascript_disable->setRowCount(s_javascript_disable.size());
  m_ui.user_agents->setRowCount(s_user_agents.size());
  prepare_table_statistics();

  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QMapIterator<QUrl, QPair<int, bool> > it(s_site_features_permissions);
#else
    QMultiMapIterator<QUrl, QPair<int, bool> > it(s_site_features_permissions);
#endif
    int i = 0;

    while(it.hasNext())
      {
	it.next();

	auto item = new QTableWidgetItem();

	if(it.value().second)
	  item->setCheckState(Qt::Checked);
	else
	  item->setCheckState(Qt::Unchecked);

	item->setData(Qt::UserRole, it.key());
	item->setData(Qt::ItemDataRole(Qt::UserRole + 1), it.value().first);
	item->setFlags(Qt::ItemIsEnabled |
		       Qt::ItemIsSelectable |
		       Qt::ItemIsUserCheckable);
	m_ui.features_permissions->setItem(i, 0, item);
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
	item = new QTableWidgetItem
	  (dooble_text_utilities::
	   web_engine_page_feature_to_pretty_string
	   (QWebEnginePage::Feature(it.value().first)));
#else
	item = new QTableWidgetItem
	  (dooble_text_utilities::
	   web_engine_page_feature_to_pretty_string
	   (QWebEnginePermission::PermissionType(it.value().first)));
#endif
	item->setData(Qt::UserRole, it.key());
	item->setData(Qt::ItemDataRole(Qt::UserRole + 1), it.value().first);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	m_ui.features_permissions->setItem(i, 1, item);
	item = new QTableWidgetItem(it.key().toString());
	item->setData(Qt::UserRole, it.key());
	item->setData(Qt::ItemDataRole(Qt::UserRole + 1), it.value().first);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	item->setToolTip(item->text());
	m_ui.features_permissions->setItem(i, 2, item);
	i += 1;
      }
  }

  {
    QHashIterator<QUrl, char> it(s_javascript_block_popup_exceptions);
    int i = 0;

    while(it.hasNext())
      {
	it.next();

	auto item = new QTableWidgetItem();

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
	item->setToolTip(item->text());
	m_ui.javascript_block_popups_exceptions->setItem(i, 1, item);
	i += 1;
      }
  }

  {
    QHashIterator<QString, char> it(s_javascript_disable);
    int i = 0;

    while(it.hasNext())
      {
	it.next();

	auto item = new QTableWidgetItem();

	if(it.value())
	  item->setCheckState(Qt::Checked);
	else
	  item->setCheckState(Qt::Unchecked);

	item->setData(Qt::UserRole, it.key());
	item->setFlags(Qt::ItemIsEnabled |
		       Qt::ItemIsSelectable |
		       Qt::ItemIsUserCheckable);
	m_ui.javascript_disable->setItem(i, 0, item);
	item = new QTableWidgetItem(it.key());
	item->setData(Qt::UserRole, it.key());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	item->setToolTip(item->text());
	m_ui.javascript_disable->setItem(i, 1, item);
	i += 1;
      }
  }

  {
    QHashIterator<QString, QString> it(s_user_agents);
    int i = 0;

    while(it.hasNext())
      {
	it.next();

	auto item = new QTableWidgetItem(it.key());

	item->setData(Qt::UserRole, it.key());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	item->setToolTip(item->text());
	m_ui.user_agents->setItem(i, 0, item);
	item = new QTableWidgetItem(it.value());
	item->setData(Qt::UserRole, it.value());
	item->setFlags
	  (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	item->setToolTip(item->text());
	m_ui.user_agents->setItem(i, 1, item);
	i += 1;
      }
  }

  m_ui.features_permissions->sortItems(2);
  m_ui.javascript_block_popups_exceptions->sortItems(1);
  m_ui.javascript_disable->sortItems(1);
  m_ui.user_agents->sortItems(0);
  connect(m_ui.features_permissions,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_features_permissions_item_changed(QTableWidgetItem *)));
  connect
    (m_ui.javascript_block_popups_exceptions,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_javascript_block_popups_exceptions_item_changed(QTableWidgetItem
							       *)));
  connect(m_ui.javascript_disable,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_javascript_disable_item_changed(QTableWidgetItem *)));
  connect(m_ui.user_agents,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  QApplication::restoreOverrideCursor();
  emit populated();
}

void dooble_settings::slot_proxy_type_changed(void)
{
  if(m_ui.proxy_none->isChecked() || m_ui.proxy_system->isChecked())
    m_ui.proxy_frame->setEnabled(false);
  else
    m_ui.proxy_frame->setEnabled(true);
}

void dooble_settings::slot_remove_all_features_permissions(void)
{
  if(m_ui.features_permissions->rowCount() == 0)
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to remove all of the "
		"feature permissions?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();
  m_ui.features_permissions->setRowCount(0);
  prepare_table_statistics();
  s_site_features_permissions.clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  purge_features_permissions();
}

void dooble_settings::slot_remove_all_javascript_block_popup_exceptions(void)
{
  if(m_ui.javascript_block_popups_exceptions->rowCount() > 0 && sender())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove all of the "
		    "JavaScript pop-up exceptions?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  m_ui.javascript_block_popups_exceptions->setRowCount(0);
  prepare_table_statistics();
  s_javascript_block_popup_exceptions.clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  purge_javascript_block_popup_exceptions();
}

void dooble_settings::slot_remove_all_javascript_disable(void)
{
  if(m_ui.javascript_disable->rowCount() > 0 && sender())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove all of the "
		    "blocked JavaScript domains?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  m_ui.javascript_disable->setRowCount(0);
  prepare_table_statistics();
  s_javascript_disable.clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  purge_javascript_disable();
}

void dooble_settings::slot_remove_all_user_agents(void)
{
  if(m_ui.user_agents->rowCount() > 0 && sender())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove all of the "
		    "user agents?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  m_ui.user_agents->setRowCount(0);
  prepare_table_statistics();
  s_user_agents.clear();

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  purge_user_agents();
}

void dooble_settings::slot_remove_selected_features_permissions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto list(m_ui.features_permissions->selectionModel()->selectedRows(2));

  QApplication::restoreOverrideCursor();

  if(!list.isEmpty())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove the selected "
		    "feature permission(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end(), dooble_ui_utilities::sort_by_row);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
		  ("DELETE FROM dooble_features_permissions "
		   "WHERE feature_digest = ? AND url_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(QByteArray::number(list.at(i).
					   data(Qt::
						ItemDataRole(Qt::UserRole + 1)).
					   toInt())).toBase64());
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data(Qt::UserRole).toUrl().toEncoded()).
		   toBase64());

		if(query.exec())
		  {
		    s_site_features_permissions.remove
		      (list.at(i).data().toUrl(),
		       QPair<int, bool> (list.at(i).
					 data(Qt::
					      ItemDataRole(Qt::UserRole + 1)).
					 toInt(),
					 false));
		    s_site_features_permissions.remove
		      (list.at(i).data().toUrl(),
		       QPair<int, bool> (list.at(i).
					 data(Qt::
					      ItemDataRole(Qt::UserRole + 1)).
					 toInt(),
					 true));
		    m_ui.features_permissions->removeRow
		      (list.at(i).row()); // Order.
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
	s_site_features_permissions.remove
	  (list.at(i).data().toUrl(),
	   QPair<int, bool> (list.at(i).
			     data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt(),
			     false));
	s_site_features_permissions.remove
	  (list.at(i).data().toUrl(),
	   QPair<int, bool> (list.at(i).
			     data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt(),
			     true));
	m_ui.features_permissions->removeRow(list.at(i).row()); // Order.
      }

  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::
slot_remove_selected_javascript_block_popup_exceptions(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto list
    (m_ui.javascript_block_popups_exceptions->
     selectionModel()->selectedRows(1));

  QApplication::restoreOverrideCursor();

  if(!list.isEmpty())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove the selected "
		    "JavaScript pop-up exception(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end(), dooble_ui_utilities::sort_by_row);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
		      (list.at(i).data().toUrl());
		    m_ui.javascript_block_popups_exceptions->removeRow
		      (list.at(i).row()); // Order.
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
	s_javascript_block_popup_exceptions.remove(list.at(i).data().toUrl());
	m_ui.javascript_block_popups_exceptions->removeRow
	  (list.at(i).row()); // Order.
      }

  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_remove_selected_javascript_disable(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto list(m_ui.javascript_disable->selectionModel()->selectedRows(1));

  QApplication::restoreOverrideCursor();

  if(!list.isEmpty())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove the selected "
		    "blocked JavaScript domains?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end(), dooble_ui_utilities::sort_by_row);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
		  ("DELETE FROM dooble_javascript_disable "
		   "WHERE url_domain_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data(Qt::UserRole).toString()).toBase64());

		if(query.exec())
		  {
		    s_javascript_disable.remove(list.at(i).data().toString());
		    m_ui.javascript_disable->removeRow
		      (list.at(i).row()); // Order.
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
	s_javascript_disable.remove(list.at(i).data().toString());
	m_ui.javascript_disable->removeRow(list.at(i).row()); // Order.
      }

  prepare_table_statistics();
  QApplication::restoreOverrideCursor();
}

void dooble_settings::slot_remove_selected_user_agents(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto list(m_ui.user_agents->selectionModel()->selectedRows(0));

  QApplication::restoreOverrideCursor();

  if(!list.isEmpty())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to remove the selected "
		    "user agent domains?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  std::sort(list.begin(), list.end(), dooble_ui_utilities::sort_by_row);

  if(dooble::s_cryptography && dooble::s_cryptography->authenticated())
    {
      auto const database_name(dooble_database_utilities::database_name());

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

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
		  ("DELETE FROM dooble_user_agents "
		   "WHERE url_domain_digest = ?");
		query.addBindValue
		  (dooble::s_cryptography->
		   hmac(list.at(i).data(Qt::UserRole).toString()).toBase64());

		if(query.exec())
		  {
		    s_user_agents.remove(list.at(i).data().toString());
		    m_ui.user_agents->removeRow(list.at(i).row()); // Order.
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
	s_user_agents.remove(list.at(i).data().toString());
	m_ui.user_agents->removeRow(list.at(i).row()); // Order.
      }

  prepare_table_statistics();
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
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QStringList list;

  list << "dooble_accepted_or_blocked_domains.db"
       << "dooble_certificate_exceptions.db"
       << "dooble_charts.db"
       << "dooble_cookies.db"
       << "dooble_downloads.db"
       << "dooble_favicons.db"
       << "dooble_history.db"
       << "dooble_javascript.db"
       << "dooble_search_engines.db"
       << "dooble_settings.db"
       << "dooble_style_sheets.db";

  foreach(auto const &i, list)
    QFile::remove(setting("home_path").toString() + QDir::separator() + i);

  QApplication::restoreOverrideCursor();
  QApplication::processEvents();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QProcess::startDetached(QCoreApplication::applicationDirPath() +
			  QDir::separator() +
			  QCoreApplication::applicationName(),
			  QStringList());
#else
  QProcess::startDetached(QCoreApplication::applicationDirPath() +
			  QDir::separator() +
			  QCoreApplication::applicationName());
#endif
  QApplication::exit(0);
}

void dooble_settings::slot_reset_credentials(void)
{
  if(!has_dooble_credentials())
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to reset your permanent "
		"credentials? New session-only credentials "
		"will be generated and database data will be removed."));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Dooble: Confirmation"));

  if(mb.exec() != QMessageBox::Yes)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Remove settings.
  */

  m_ui.cipher->setCurrentIndex(0);
  m_ui.hash->setCurrentIndex(1); // SHA3-512
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
  remove_setting("hash_type");
  remove_setting("hash_type_index");

  {
    QWriteLocker locker(&s_settings_mutex);

    s_settings["block_cipher_type"] = "AES-256";
    s_settings["block_cipher_type_index"] = 0;
    s_settings["hash_type"] = "SHA3-512";
    s_settings["hash_type_index"] = 1;
  }

  /*
  ** Generate temporary credentials.
  */

  auto authentication_key
    (dooble_random::
     random_bytes(dooble_cryptography::s_authentication_key_length));
  auto encryption_key
    (dooble_random::random_bytes(dooble_cryptography::s_encryption_key_length));

  dooble::s_cryptography->set_authenticated(false);
  dooble::s_cryptography->set_block_cipher_type("AES-256");
  dooble::s_cryptography->set_hash_type("SHA3-512");
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

  auto const password_1(m_ui.password_1->text());
  auto const password_2(m_ui.password_2->text());

  if(password_1.isEmpty())
    {
      m_ui.password_1->setFocus();
      QToolTip::showText(mapToGlobal(m_ui.password_1->pos()),
			 tr("Empty password(s)."),
			 m_ui.password_1,
			 QRect());
      return;
    }
  else if(password_1 != password_2)
    {
      m_ui.password_1->selectAll();
      m_ui.password_1->setFocus();
      QToolTip::showText(mapToGlobal(m_ui.password_1->pos()),
			 tr("Passwords are not equal."),
			 m_ui.password_1,
			 QRect());
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
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	{
	  QApplication::processEvents();
	  return;
	}

      QApplication::processEvents();
    }

  auto const salt(dooble_random::random_bytes(64));

  if(salt.isEmpty())
    {
      m_ui.password_1->selectAll();
      m_ui.password_1->setFocus();
      QMessageBox::critical
	(this,
	 tr("Dooble: Error"),
	 tr("Salt-generation failure! This is a curious problem."));
      QApplication::processEvents();
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

  pbkdf2.reset(new dooble_pbkdf2(password_1.toUtf8(),
				 salt,
				 m_ui.cipher->currentIndex(),
				 m_ui.hash->currentIndex(),
				 m_ui.iterations->value(),
				 1024));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  if(m_ui.hash->currentIndex() == 0)
    m_pbkdf2_future = QtConcurrent::run
      (pbkdf2.data(), &dooble_pbkdf2::pbkdf2, &dooble_hmac::keccak_512_hmac);
  else
    m_pbkdf2_future = QtConcurrent::run
      (pbkdf2.data(), &dooble_pbkdf2::pbkdf2, &dooble_hmac::sha3_512_hmac);
#else
  if(m_ui.hash->currentIndex() == 0)
    m_pbkdf2_future = QtConcurrent::run
      (&dooble_pbkdf2::pbkdf2, pbkdf2.data(), &dooble_hmac::keccak_512_hmac);
  else
    m_pbkdf2_future = QtConcurrent::run
      (&dooble_pbkdf2::pbkdf2, pbkdf2.data(), &dooble_hmac::sha3_512_hmac);
#endif

  m_pbkdf2_future_watcher.setFuture(m_pbkdf2_future);
  connect(m_pbkdf2_dialog,
	  SIGNAL(canceled(void)),
	  pbkdf2.data(),
	  SLOT(slot_interrupt(void)));
  m_pbkdf2_dialog->exec();
  QApplication::processEvents();
}

void dooble_settings::slot_select_application_font(void)
{
  QFont font;
  QFontDialog dialog(this);
  auto string(m_ui.display_application_font->text());

  if(!string.isEmpty() && font.fromString(string.remove('&')))
    dialog.setCurrentFont(font);
  else
    dialog.setCurrentFont(dooble_application::font());

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      m_ui.display_application_font->setText
	(dialog.selectedFont().toString().trimmed());
    }
}

void dooble_settings::slot_user_agent_item_changed(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 1)
    return;

  auto user_agent(item->text().trimmed());

  disconnect
    (m_ui.user_agents,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  user_agent = user_agent.isEmpty() ? s_http_user_agent : user_agent;
  item->setText(user_agent);
  item->setToolTip(item->text());
  connect
    (m_ui.user_agents,
     SIGNAL(itemChanged(QTableWidgetItem *)),
     this,
     SLOT(slot_user_agent_item_changed(QTableWidgetItem *)));
  item = m_ui.user_agents->item(item->row(), 0);

  if(!item)
    return;

  new_user_agent(item->text(), user_agent);
}

void dooble_settings::slot_web_engine_settings_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(Qt::ItemIsUserCheckable & item->flags())
    {
      auto const string(item->data(Qt::UserRole).toString().trimmed());

      if(!string.isEmpty())
	{
	  auto const database_name(dooble_database_utilities::database_name());

	  {
	    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

	    db.setDatabaseName(setting("home_path").toString() +
			       QDir::separator() +
			       "dooble_settings.db");

	    if(db.open())
	      {
		create_tables(db);

		QSqlQuery query(db);

		query.exec("PRAGMA synchronous = NORMAL");
		query.prepare
		  ("INSERT OR REPLACE INTO dooble_web_engine_settings "
		   "(environment_variable, key, value) VALUES (?, ?, ?)");

		if(s_web_engine_settings_environment.contains(string))
		  query.addBindValue(1);
		else
		  query.addBindValue(0);

		query.addBindValue(string);
		query.addBindValue
		  (item->checkState() == Qt::Unchecked ? "false" : "true");
		query.exec();
	      }

	    db.close();
	  }

	  QSqlDatabase::removeDatabase(database_name);
	}
    }
}
