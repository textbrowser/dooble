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

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLocale>
#include <QMap>
#include <QMessageBox>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextCodec>
#include <QThread>
#include <QUrl>
#include <QWebSettings>

#include "dbookmarkswindow.h"
#include "dbookmarkswindow.h"
#include "dmisc.h"
#include "dnetworkcache.h"
#include "dooble.h"
#include "dsettings.h"

dsettings::dsettings():QMainWindow()
{
  m_parentDooble = 0;
  ui.setupUi(this);
  ui.displaypriority->setItemData(0, QThread::HighPriority);
  ui.displaypriority->setItemData(1, QThread::HighestPriority);
  ui.displaypriority->setItemData(2, QThread::IdlePriority);
  ui.displaypriority->setItemData(3, QThread::InheritPriority);
  ui.displaypriority->setItemData(4, QThread::LowPriority);
  ui.displaypriority->setItemData(5, QThread::LowestPriority);
  ui.displaypriority->setItemData(6, QThread::NormalPriority);
  ui.displaypriority->setItemData(7, QThread::TimeCriticalPriority);
  ui.applicationsTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.applicationsTable,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this, SLOT(slotCustomContextMenuRequested(const QPoint &)));

  foreach(QProgressBar *progressBar, findChildren<QProgressBar *> ())
    progressBar->setVisible(false);

  ui.rememberClosedTabsSpinBox->setMaximum(dooble::MAX_HISTORY_ITEMS);
#if QT_VERSION >= 0x050000
  ui.thirdPartyCookiesComboBox->addItem(tr("allowed with existing cookies"));
#endif
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

  /*
  ** Move the title so that the box's border remains clean.
  */

  foreach(QGroupBox *groupBox, findChildren<QGroupBox *> ())
    groupBox->setStyleSheet("QGroupBox::title {"
			    "top: -1px;"
			    "subcontrol-origin: border;}");

#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050200
  ui.browsingFtpProxyPassword->setEchoMode(QLineEdit::NoEcho);
  ui.browsingHttpProxyPassword->setEchoMode(QLineEdit::NoEcho);
  ui.downloadFtpProxyPassword->setEchoMode(QLineEdit::NoEcho);
  ui.downloadHttpProxyPassword->setEchoMode(QLineEdit::NoEcho);
  ui.pass1LineEdit->setEchoMode(QLineEdit::NoEcho);
  ui.pass2LineEdit->setEchoMode(QLineEdit::NoEcho);
#endif
#else
  statusBar()->setSizeGripEnabled(true);
#endif

  foreach(QPushButton *button, ui.buttonBox->findChildren<QPushButton *>())
    button->setIcon(QIcon());

  QSettings settings;

  if(dooble::s_settings.value("iconSet", "").toString().trimmed().isEmpty())
    {
      settings.setValue("iconSet",
			QString("%1/%2").arg(QDir::currentPath()).
			arg("Icons/nuovext/configuration.cfg"));
      dooble::s_settings["iconSet"] =
	QString("%1/%2").arg(QDir::currentPath()).
	arg("Icons/nuovext/configuration.cfg");
    }

  if(dooble::s_settings.value("settingsWindow/iconSet1", "").toString().
     trimmed().isEmpty())
    {
      settings.setValue("settingsWindow/iconSet1", settings.value("iconSet"));
      dooble::s_settings["settingsWindow/iconSet1"] =
	dooble::s_settings["iconSet"];
    }

  if(!dooble::s_settings.values().
     contains(QString("%1/%2").arg(QDir::currentPath()).
	      arg("Icons/nuvola/configuration.cfg")))
    {
      int i = 2;

      do
	{
	  if(dooble::s_settings.value(QString("settingsWindow/iconSet%1").
				      arg(i), "").toString().trimmed().
	     isEmpty() ||
	     !dooble::s_settings.keys().
	     contains(QString("settingsWindow/iconSet%1").arg(i)))
	    {
	      settings.setValue(QString("settingsWindow/iconSet%1").arg(i),
				QString("%1/%2").arg(QDir::currentPath()).
				arg("Icons/nuvola/configuration.cfg"));
	      dooble::s_settings[QString("settingsWindow/iconSet%1").arg(i)] =
		settings.value(QString("settingsWindow/iconSet%1").arg(i));
	      break;
	    }

	  i += 1;
	}
      while(i <= 100); // Will we ever have so many themes?
    }

  QLocale locale;

  if(!dooble::s_settings.contains("settingsWindow/homeUrl"))
    {
      if(locale.language() == QLocale::C)
	{
	  settings.setValue("settingsWindow/homeUrl",
			    "qrc:/search_c.html");
	  dooble::s_settings["settingsWindow/homeUrl"] =
	    "qrc:/search_c.html";
	}
      else
	{
	  settings.setValue("settingsWindow/homeUrl",
			    QString("qrc:/search_%1_%2.html").
			    arg(QLocale::languageToString(locale.language()).
				toLower().trimmed()).
			    arg(QLocale::countryToString(locale.country()).
				toLower().trimmed()));
	  dooble::s_settings["settingsWindow/homeUrl"] =
	    QString("qrc:/search_%1_%2.html").
	    arg(QLocale::languageToString(locale.language()).
		toLower().trimmed()).
	    arg(QLocale::countryToString(locale.country()).
		toLower().trimmed());
	}
    }

  if(dooble::s_settings.value("settingsWindow/myRetrievedFiles", "").
     toString().trimmed().isEmpty())
    {
#if QT_VERSION >= 0x050000
      settings.setValue
	("settingsWindow/myRetrievedFiles",
	 QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
      dooble::s_settings["settingsWindow/myRetrievedFiles"] =
	QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
      settings.setValue
	("settingsWindow/myRetrievedFiles",
	 QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
      dooble::s_settings["settingsWindow/myRetrievedFiles"] =
	QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
#endif
    }

  if(dooble::s_settings.value("settingsWindow/spotOnSharedDatabase", "").
     toString().trimmed().isEmpty())
    {
      settings.setValue
	("settingsWindow/spotOnSharedDatabase",
	 QDir::homePath() + QDir::separator() + ".spot-on" +
	 QDir::separator() + "shared.db");
      dooble::s_settings["settingsWindow/spotOnSharedDatabase"] =
	QDir::homePath() + QDir::separator() + ".spot-on" +
	QDir::separator() + "shared.db";
    }

  QString str
    (dooble::s_settings.value("settingsWindow/tabBarPosition", "north").
     toString().toLower().trimmed());

  if(str == "east")
    ui.tab_bar_position->setCurrentIndex(0);
  else if(str == "north")
    ui.tab_bar_position->setCurrentIndex(1);
  else if(str == "south")
    ui.tab_bar_position->setCurrentIndex(2);
  else if(str == "west")
    ui.tab_bar_position->setCurrentIndex(3);
  else
    ui.tab_bar_position->setCurrentIndex(1);

  QSettings cfgSettings(dooble::s_settings.value("iconSet").toString(),
			QSettings::IniFormat);

  setWindowIcon
    (QIcon(cfgSettings.value("settingsWindow/windowIcon").toString()));
  connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)),
	  this, SLOT(slotClicked(QAbstractButton *)));
  connect(ui.chooseIconSetPushButton, SIGNAL(clicked(void)),
	  this, SLOT(slotSelectIconCfgFile(void)));
  connect(ui.chooseMyRetrievedFilesPushButton, SIGNAL(clicked(void)),
	  this, SLOT(slotChooseMyRetrievedFilesDirectory(void)));
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(ui.spotOnChooseDatabasePushButton, SIGNAL(clicked(void)),
	  this, SLOT(slotChooseSpotOnSharedDatabaseFile(void)));
#endif
  connect(ui.defaultFontCombinationBox,
	  SIGNAL(currentIndexChanged(const QString &)),
	  this, SLOT(slotWebFontChanged(const QString &)));
  connect(ui.fixedFontCombinationBox,
	  SIGNAL(currentIndexChanged(const QString &)),
	  this, SLOT(slotWebFontChanged(const QString &)));
  connect(ui.historyCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.historySpinBox,
	  SLOT(setEnabled(bool)));
  connect(ui.cookieTimerComboBox,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotCookieTimerTimeChanged(int)));
  connect(ui.cookieTimerCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.cookieTimerComboBox,
	  SLOT(setEnabled(bool)));
  connect(ui.cookieTimerCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.cookieTimerSpinBox,
	  SLOT(setEnabled(bool)));
  connect(ui.changePassphrasePushButton,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slotEnablePassphrase(void)));
  connect(ui.browsingProxyManualRadio,
	  SIGNAL(toggled(bool)),
	  ui.browsingFtpProxyGroupBox,
	  SLOT(setEnabled(bool)));
  connect(ui.browsingProxyManualRadio,
	  SIGNAL(toggled(bool)),
	  ui.browsingHttpProxyGroupBox,
	  SLOT(setEnabled(bool)));
  connect(ui.downloadProxyManualRadio,
	  SIGNAL(toggled(bool)),
	  ui.downloadFtpProxyGroupBox,
	  SLOT(setEnabled(bool)));
  connect(ui.downloadProxyManualRadio,
	  SIGNAL(toggled(bool)),
	  ui.downloadHttpProxyGroupBox,
	  SLOT(setEnabled(bool)));
  connect(ui.cookieGroupBox,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slotGroupBoxClicked(bool)));
  connect(ui.popupExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_popupsWindow,
	  SLOT(slotShow(void)));
  connect(ui.thirdPartyExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_adBlockWindow,
	  SLOT(slotShow(void)));
  connect(ui.cookiesExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_cookiesBlockWindow,
	  SLOT(slotShow(void)));
  connect(ui.httpRedirectExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_httpRedirectWindow,
	  SLOT(slotShow(void)));
  connect(ui.httpReferrerExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_httpReferrerWindow,
	  SLOT(slotShow(void)));
  connect(ui.javaScriptExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_javaScriptExceptionsWindow,
	  SLOT(slotShow(void)));
  connect(ui.rememberClosedTabsCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.rememberClosedTabsSpinBox,
	  SLOT(setEnabled(bool)));
  connect(ui.iconSetComboBox,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotIconsPreview(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  this,
	  SLOT(slotSetIcons(void)));
  connect(&m_updateLabelTimer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotUpdateLabels(void)));
  connect(&m_purgeMemoryCachesTimer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotPurgeMemoryCaches(void)));
  connect(ui.dntExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_dntWindow,
	  SLOT(slotShow(void)));
  connect(ui.loadImagesExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_imageBlockWindow,
	  SLOT(slotShow(void)));
  connect(ui.webCacheExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_cacheExceptionsWindow,
	  SLOT(slotShow(void)));
  connect(ui.alwaysHttpsExceptionsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_alwaysHttpsExceptionsWindow,
	  SLOT(slotShow(void)));
  connect(ui.httponlyexceptions,
	  SIGNAL(clicked(void)),
	  dooble::s_httpOnlyExceptionsWindow,
	  SLOT(slotShow(void)));
  connect(ui.sightSslErrorsPushButton,
	  SIGNAL(clicked(void)),
	  dooble::s_sslExceptionsWindow,
	  SLOT(slotShow(void)));
  connect(ui.automaticallyLoadImagesCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.loadImagesExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.blockPopupsCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.popupExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.thirdPartyContentCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.thirdPartyExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.httponlycookies,
	  SIGNAL(clicked(bool)),
	  ui.httponlyexceptions,
	  SLOT(setEnabled(bool)));
  connect(ui.dntCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.dntExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.suppressHttpRedirectCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.httpRedirectExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.suppressHttpReferrerCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.httpReferrerExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.alwaysHttpsCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.alwaysHttpsExceptionsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.sightSslErrorsCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.sightSslErrorsPushButton,
	  SLOT(setEnabled(bool)));
  connect(ui.sightSslErrorsCheckBox,
	  SIGNAL(clicked(bool)),
	  ui.sslLevel,
	  SLOT(setEnabled(bool)));
  connect(ui.clearDiskCachePushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotClearDiskCache(void)));
  connect(ui.clearFaviconsPushButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotClearFavicons(void)));

  foreach(dsettingshomelinewidget *textEdit,
	  findChildren<dsettingshomelinewidget *> ())
    connect(this,
	    SIGNAL(iconsChanged(void)),
	    textEdit,
	    SLOT(slotSetIcons(void)));

  /*
  ** As long as the tab order of the buttons is correct, this trick
  ** will always work.
  */

  int idx = 0;

  foreach(QToolButton *button,
	  ui.tabScrollArea->findChildren<QToolButton *> ())
    {
      connect(button,
	      SIGNAL(clicked(bool)),
	      this,
	      SLOT(slotChangePage(bool)));
      button->setProperty("page", idx);
      button->setStyleSheet("QToolButton:checked {"
			    "color: black;"
			    "background-color: rgb(85, 170, 255);"
			    "}");
      idx += 1;
    }

  slotSetIcons();
  slotCookieTimerTimeChanged
    (dooble::s_settings.value("settingsWindow/cookieTimerUnit",
			      1).toInt());
  m_updateLabelTimer.setInterval(2500);
  m_purgeMemoryCachesTimer.setInterval(10000);

  if(dooble::s_settings.value("settingsWindow/purgeMemoryCaches",
			      true).toBool())
    m_purgeMemoryCachesTimer.start();
}

dsettings::~dsettings()
{
  m_purgeMemoryCachesTimer.stop();
  m_updateLabelTimer.stop();
}

void dsettings::exec(dooble *parent)
{
  ui.httpStatusCodes->clearContents();
  ui.httpStatusCodes->setRowCount(599 - 400 + 1);

  for(int i = 400, j = 0; i <= 599; i++, j++)
    {
      QTableWidgetItem *item = new QTableWidgetItem(QString::number(i));

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      ui.httpStatusCodes->setItem(j, 0, item);
    }

  for(int i = 0; i < ui.httpStatusCodes->rowCount(); i++)
    {
      QTableWidgetItem *item = ui.httpStatusCodes->item(i, 0);

      if(!item)
	continue;

      int value = dmisc::s_httpStatusCodes.value(item->text().toInt(), 1);

      if(value)
	ui.httpStatusCodes->item(i, 0)->setCheckState(Qt::Checked);
      else
	ui.httpStatusCodes->item(i, 0)->setCheckState(Qt::Unchecked);
    }

  m_parentDooble = parent;
  m_previousIconSet.clear();
  slotUpdateLabels();

  if(!m_updateLabelTimer.isActive())
    m_updateLabelTimer.start();

  /*
  ** A little trick to force state changes.
  */

  ui.browsingProxyManualRadio->setChecked(true);
  ui.downloadProxyManualRadio->setChecked(true);
  ui.browsingProxyNoneRadio->setChecked(true);
  ui.downloadProxyNoneRadio->setChecked(true);
  ui.url01lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url1", "").toString().trimmed());
  ui.url02lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url2", "").toString().trimmed());
  ui.url03lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url3", "").toString().trimmed());
  ui.url04lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url4", "").toString().trimmed());
  ui.url05lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url5", "").toString().trimmed());
  ui.url06lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url6", "").toString().trimmed());
  ui.url07lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url7", "").toString().trimmed());
  ui.url08lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url8", "").toString().trimmed());
  ui.url09lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url9", "").toString().trimmed());
  ui.url10lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url10",
			      "").toString().trimmed());
  ui.url11lineEdit->setText
    (dooble::s_settings.value("settingsWindow/url11",
			      "").toString().trimmed());
#if QT_VERSION >= 0x050000
  ui.myRetrievedFilesLineEdit->setText
    (dooble::s_settings.
     value("settingsWindow/myRetrievedFiles",
	   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).
     toString());
#else
  ui.myRetrievedFilesLineEdit->setText
    (dooble::s_settings.
     value("settingsWindow/myRetrievedFiles",
	   QDesktopServices::storageLocation(QDesktopServices::
					     DesktopLocation)).toString());
#endif
  ui.spotOnSharedDatabaseLineEdit->setText
    (dooble::s_settings.value("settingsWindow/spotOnSharedDatabase",
			      QDir::homePath() + QDir::separator() +
			      ".spot-on" + QDir::separator() + "shared.db").
     toString());
#ifndef DOOBLE_LINKED_WITH_LIBSPOTON
  ui.spotOnChooseDatabasePushButton->setEnabled(false);
  ui.spotOnLabel->setEnabled(false);
  ui.spotOnSharedDatabaseLineEdit->setEnabled(false);
#endif
  ui.ircLineEdit->setText
    (dooble::s_settings.value("settingsWindow/ircChannel",
			      "https://webchat.freenode.net?channels=dooble").
     toString());

  QLocale locale;

  ui.homeLineEdit->setText
    (dooble::s_settings.value("settingsWindow/homeUrl",
			      "qrc:/search_c.html").toString());
  ui.p2pLineEdit->setText
    (dooble::s_settings.value("settingsWindow/p2pUrl",
			      "about: blank").toString().trimmed());

  foreach(dsettingshomelinewidget *widget,
	  findChildren<dsettingshomelinewidget *> ())
    if(widget != ui.myRetrievedFilesLineEdit)
      {
	QUrl url(dmisc::correctedUrlPath(QUrl::fromUserInput(widget->
							     text())));

	if(url.host().toLower().trimmed().startsWith("gopher"))
	  url.setScheme("gopher");

	if(!url.isEmpty() && url.isValid())
	  widget->setText(url.toString(QUrl::StripTrailingSlash));
      }

  ui.showAuthenticationCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/showAuthentication",
			      true).toBool());
  ui.displayDesktopCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/displayDesktopCheckBox",
			      false).toBool());
  ui.proceedToNewTabCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/proceedToNewTab",
			      true).toBool());
  ui.openInNewTabsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/openInNewTab", true).toBool());
  ui.addTabWithDoubleClick->setChecked
    (dooble::s_settings.value("settingsWindow/addTabWithDoubleClick",
			      true).toBool());
  ui.webPluginsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/enableWebPlugins",
			      true).toBool());
#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
  ui.webPluginsCheckBox->setText(tr("Web plugins."));
#endif
  ui.browsingHttpProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/httpBrowsingProxyEnabled",
			      false).toBool());
  ui.downloadFtpProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/ftpDownloadProxyEnabled",
			      false).toBool());
  ui.browsingFtpProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyEnabled",
			      false).toBool());
  ui.downloadHttpProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/httpDownloadProxyEnabled",
			      false).toBool());
  ui.browsingHttpProxyType->setCurrentIndex
    (ui.browsingHttpProxyType->findText
     (dooble::s_settings.value("settingsWindow/httpBrowsingProxyType",
			       "Socks5").toString().trimmed()));
  ui.browsingHttpProxyHostName->setText
    (dooble::s_settings.value("settingsWindow/httpBrowsingProxyHost",
			      "").toString().trimmed());
  ui.browsingHttpProxyPort->setValue
    (dooble::s_settings.value("settingsWindow/httpBrowsingProxyPort",
			      1080).toInt());
  ui.browsingHttpProxyUserName->setText
    (dooble::s_settings.value("settingsWindow/httpBrowsingProxyUser",
			      "").toString());
  ui.browsingHttpProxyPassword->setText
    (dooble::s_settings.value
     ("settingsWindow/httpBrowsingProxyPassword",
      "").toString());
  ui.browsingI2pProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/i2pBrowsingProxyEnabled",
			      true).toBool());
  ui.browsingI2pProxyType->setCurrentIndex
    (ui.browsingI2pProxyType->findText
     (dooble::s_settings.value("settingsWindow/i2pBrowsingProxyType",
			       "Http").toString().trimmed()));
  ui.browsingI2pProxyHostName->setText(dooble::s_settings.value
				       ("settingsWindow/i2pBrowsingProxyHost",
					"127.0.0.1").toString().trimmed());
  ui.browsingI2pProxyPort->setValue(dooble::s_settings.value
				    ("settingsWindow/i2pBrowsingProxyPort",
				     4444).toInt());
  ui.downloadI2pProxyGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/i2pDownloadProxyEnabled",
			      true).toBool());
  ui.downloadI2pProxyType->setCurrentIndex
    (ui.downloadI2pProxyType->findText
     (dooble::s_settings.value("settingsWindow/i2pDownloadProxyType",
			       "Http").toString().trimmed()));
  ui.downloadI2pProxyHostName->setText(dooble::s_settings.value
				       ("settingsWindow/i2pDownloadProxyHost",
					"127.0.0.1").toString().trimmed());
  ui.downloadI2pProxyPort->setValue(dooble::s_settings.value
				    ("settingsWindow/i2pDownloadProxyPort",
				     4444).toInt());
  ui.downloadFtpProxyHostName->setText
    (dooble::s_settings.value("settingsWindow/ftpDownloadProxyHost",
			      "").toString().trimmed());
  ui.downloadFtpProxyPort->setValue
    (dooble::s_settings.value("settingsWindow/ftpDownloadProxyPort",
			      1080).toInt());
  ui.downloadFtpProxyType->setCurrentIndex
    (ui.downloadFtpProxyType->findText
     (dooble::s_settings.value("settingsWindow/ftpDownloadProxyType",
			       "Socks5").toString().trimmed()));
  ui.downloadFtpProxyUserName->setText
    (dooble::s_settings.value("settingsWindow/ftpDownloadProxyUser",
			      "").toString());
  ui.downloadFtpProxyPassword->setText
    (dooble::s_settings.value("settingsWindow/ftpDownloadProxyPassword",
			      "").toString());
  ui.browsingFtpProxyHostName->setText
    (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyHost",
			      "").toString().trimmed());
  ui.browsingFtpProxyPort->setValue
    (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyPort",
			      1080).toInt());
  ui.browsingFtpProxyType->setCurrentIndex
    (ui.browsingFtpProxyType->findText
     (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyType",
			       "Socks5").toString().trimmed()));
  ui.browsingFtpProxyUserName->setText
    (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyUser",
			      "").toString());
  ui.browsingFtpProxyPassword->setText
    (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyPassword",
			      "").toString());
  ui.downloadHttpProxyType->setCurrentIndex
    (ui.downloadHttpProxyType->findText
     (dooble::s_settings.value("settingsWindow/httpDownloadProxyType",
			       "Socks5").toString().trimmed()));
  ui.downloadHttpProxyHostName->setText
    (dooble::s_settings.value("settingsWindow/httpDownloadProxyHost",
			      "").toString().trimmed());
  ui.downloadHttpProxyPort->setValue
    (dooble::s_settings.value("settingsWindow/httpDownloadProxyPort",
			      1080).toInt());
  ui.downloadHttpProxyUserName->setText
    (dooble::s_settings.value("settingsWindow/httpDownloadProxyUser",
			      "").toString());
  ui.downloadHttpProxyPassword->setText
    (dooble::s_settings.value("settingsWindow/httpDownloadProxyPassword",
			      "").toString());
  ui.automaticallyLoadImagesCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/automaticallyLoadImages",
			      true).toBool());
  ui.thirdPartyContentCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/blockThirdPartyContent",
			      true).toBool());
  ui.openUserWindowsInNewProcessesCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/openUserWindowsInNewProcesses",
			      false).toBool());
  ui.closeViaMiddleCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/closeViaMiddle", true).toBool());
  ui.suppressHttpRedirectCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/suppressHttpRedirect1", false).
     toBool());
  ui.suppressHttpReferrerCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/suppressHttpReferrer1", false).
     toBool());
  ui.xssAuditingCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/xssAuditingEnabled", true).
     toBool());
  ui.appendNewTabsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/appendNewTabs", false).
     toBool());
  ui.displayIpAddressCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/displayIpAddress", false).
     toBool());
  ui.dntCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/doNotTrack", false).toBool());
  ui.centerChildWindows->setChecked
    (dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool());
  ui.useNativeFileDialogs->setChecked
    (dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			      false).toBool());
  ui.disableAllEncryptedDatabaseWrites->setChecked
    (dooble::s_settings.
     value("settingsWindow/disableAllEncryptedDatabaseWrites",
	   false).toBool());
  ui.httponlycookies->setChecked
    (dooble::s_settings.
     value("settingsWindow/httpOnlyCookies", false).toBool());

  int priority = dooble::s_settings.value("settingsWindow/displaypriority",
					  3).toInt();

  if(priority < 0 || priority > 7)
    priority = 3;

  for(int i = 0; i < ui.displaypriority->count(); i++)
    if(priority == ui.displaypriority->itemData(i).toInt())
      {
	ui.displaypriority->setCurrentIndex(i);
	break;
      }

  QStringList allKeys(dooble::s_settings.keys());
  QMap<QString, QString> itemsMap;

  for(int i = 0; i < allKeys.size(); i++)
    if(allKeys.at(i).contains("iconSet"))
      {
	if(!QFileInfo(dooble::s_settings.value(allKeys.at(i)).
		      toString()).exists())
	  {
	    /*
	    ** Remove the setting from the configuration file.
	    */

	    QSettings settings;

	    settings.remove(allKeys.at(i));
	    dooble::s_settings.remove(allKeys.at(i));
	    continue;
	  }

	QSettings settings
	  (dooble::s_settings.value(allKeys.at(i)).toString(),
	   QSettings::IniFormat);

	settings.beginGroup("Description");

	QString key(settings.value("Name").toString());
	QString value(dooble::s_settings.value(allKeys.at(i)).toString());

	itemsMap[key] = value;
      }

  disconnect(ui.iconSetComboBox,
	     SIGNAL(currentIndexChanged(int)),
	     this,
	     SLOT(slotIconsPreview(void)));
  ui.iconSetComboBox->clear();
  connect(ui.iconSetComboBox,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotIconsPreview(void)));

  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("Description");
  disconnect(ui.iconSetComboBox,
	     SIGNAL(currentIndexChanged(int)),
	     this,
	     SLOT(slotIconsPreview(void)));

  for(int i = 0; i < itemsMap.size(); i++)
    {
      ui.iconSetComboBox->addItem(itemsMap.keys().at(i),
				  itemsMap.values().at(i));

      if(settings.value("Name").toString() == itemsMap.keys().at(i))
	ui.iconSetComboBox->setCurrentIndex(i);
    }

  connect(ui.iconSetComboBox,
	  SIGNAL(currentIndexChanged(int)),
	  this,
	  SLOT(slotIconsPreview(void)));
  ui.javaCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javaEnabled", false).toBool());
  ui.javascriptGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptEnabled", false).
     toBool());
  ui.jsAllowNewWindowsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAllowNewWindows",
			      true).toBool());
  ui.jsAcceptAlertsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAcceptAlerts",
			      true).toBool());
  ui.jsAcceptConfirmationsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAcceptConfirmations",
			      true).toBool());
  ui.jsAcceptPromptsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAcceptPrompts",
			      true).toBool());
  ui.cookieGroupBox->setChecked
    (dooble::s_settings.value("settingsWindow/cookiesEnabled", true).toBool());
  ui.blockPopupsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/blockPopups", true).toBool());
#if QT_VERSION >= 0x040800
  ui.hyperlinkAuditing->setChecked
    (dooble::s_settings.value("settingsWindow/hyperlinkAuditing",
			      false).toBool());
  ui.hyperlinkAuditing->setEnabled(true);
  ui.webglCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/webglEnabled", false).toBool());
  ui.webglCheckBox->setEnabled(true);
#else
  ui.hyperlinkAuditing->setChecked(false);
  ui.hyperlinkAuditing->setEnabled(false);
  ui.webglCheckBox->setChecked(false);
  ui.webglCheckBox->setEnabled(false);
#endif

  QFont font;

  if(font.
     fromString(dooble::s_settings.value("settingsWindow/standardWebFont", "").
		toString()) && !font.family().isEmpty())
    {
      if(ui.defaultFontCombinationBox->findText(font.family()) > -1)
	ui.defaultFontCombinationBox->setCurrentIndex
	  (ui.defaultFontCombinationBox->findText(font.family()));
      else
	ui.defaultFontCombinationBox->setCurrentIndex(0);

      updateFontWidgets(font.family(), ui.defaultFontSizeCombinationBox);

      QString fontSize(QString::number(font.pointSize()));

      if(ui.defaultFontSizeCombinationBox->findText(fontSize) > -1)
	ui.defaultFontSizeCombinationBox->setCurrentIndex
	  (ui.defaultFontSizeCombinationBox->findText(fontSize));
      else
	ui.defaultFontSizeCombinationBox->setCurrentIndex(0);
    }
  else
    {
      QString text("");
      QString fontSize("");

#ifdef Q_OS_MAC
      text = "Times";
      fontSize = "16";
#elif defined(Q_OS_WIN32)
      text = "Serif";
      fontSize = "10";
#else
      text = "Serif";
      fontSize = "16";
#endif

      if(ui.defaultFontCombinationBox->findText(text) > -1)
	ui.defaultFontCombinationBox->setCurrentIndex
	  (ui.defaultFontCombinationBox->findText(text));
      else
	ui.defaultFontCombinationBox->setCurrentIndex(0);

      updateFontWidgets(ui.defaultFontCombinationBox->currentText(),
			ui.defaultFontSizeCombinationBox);

      if(ui.defaultFontSizeCombinationBox->findText(fontSize) > -1)
	ui.defaultFontSizeCombinationBox->setCurrentIndex
	  (ui.defaultFontSizeCombinationBox->findText(fontSize));
      else
	ui.defaultFontSizeCombinationBox->setCurrentIndex(0);
    }

  if(font.fromString(dooble::s_settings.value("settingsWindow/"
					      "cursiveWebFont", "").
		     toString()) && !font.family().isEmpty())
    {
      if(ui.cursiveFontCombinationBox->findText(font.family()) > -1)
	ui.cursiveFontCombinationBox->setCurrentIndex
	  (ui.cursiveFontCombinationBox->findText(font.family()));
      else
	ui.cursiveFontCombinationBox->setCurrentIndex(0);
    }
  else
    {
      if(ui.cursiveFontCombinationBox->
	 findText("Serif") > -1)
	ui.cursiveFontCombinationBox->setCurrentIndex
	  (ui.cursiveFontCombinationBox->
	   findText("Serif"));
      else
	ui.cursiveFontCombinationBox->setCurrentIndex(0);
    }

  if(font.fromString(dooble::s_settings.value("settingsWindow/"
					      "fantasyWebFont", "").
		     toString()) && !font.family().isEmpty())
    {
      if(ui.fantasyFontCombinationBox->findText(font.family()) > -1)
	ui.fantasyFontCombinationBox->setCurrentIndex
	  (ui.fantasyFontCombinationBox->findText(font.family()));
      else
	ui.fantasyFontCombinationBox->setCurrentIndex(0);
    }
  else
    {
      if(ui.fantasyFontCombinationBox->
	 findText("Serif") > -1)
	ui.fantasyFontCombinationBox->setCurrentIndex
	  (ui.fantasyFontCombinationBox->
	   findText("Serif"));
      else
	ui.fantasyFontCombinationBox->setCurrentIndex(0);
    }

  if(font.fromString(dooble::s_settings.value("settingsWindow/"
					      "fixedWebFont", "").
		     toString()) && !font.family().isEmpty())
    {
      if(ui.fixedFontCombinationBox->findText(font.family()) > -1)
	ui.fixedFontCombinationBox->setCurrentIndex
	  (ui.fixedFontCombinationBox->findText(font.family()));
      else
	ui.fixedFontCombinationBox->setCurrentIndex(0);

      updateFontWidgets(font.family(), ui.fixedFontSizeCombinationBox);

      QString fontSize(QString::number(font.pointSize()));

      if(ui.fixedFontSizeCombinationBox->findText(fontSize) > -1)
	ui.fixedFontSizeCombinationBox->setCurrentIndex
	  (ui.fixedFontSizeCombinationBox->findText(fontSize));
      else
	ui.fixedFontSizeCombinationBox->setCurrentIndex(0);
    }
  else
    {
      int index = -1;
      QString text("");
      QString fontSize("");

#ifdef Q_OS_MAC
      text = "Courier";
      fontSize = "13";
#elif defined(Q_OS_WIN32)
      text = "Courier New";
      fontSize = "10";
#else
      text = "Courier";
      fontSize = "10";
#endif

      if((index = ui.fixedFontCombinationBox->
	  findText(text, Qt::MatchContains)) > -1)
	ui.fixedFontCombinationBox->setCurrentIndex(index);
      else
	ui.fixedFontCombinationBox->setCurrentIndex(0);

      updateFontWidgets(ui.fixedFontCombinationBox->currentText(),
			ui.fixedFontSizeCombinationBox);

      if(ui.fixedFontSizeCombinationBox->findText(fontSize) > -1)
	ui.fixedFontSizeCombinationBox->setCurrentIndex
	  (ui.fixedFontSizeCombinationBox->findText(fontSize));
      else
	ui.fixedFontSizeCombinationBox->setCurrentIndex(0);
    }

  if(font.fromString(dooble::s_settings.value("settingsWindow/"
					      "sansSerifWebFont", "").
		     toString()) && !font.family().isEmpty())
    {
      if(ui.sansSerifFontCombinationBox->findText(font.family()) > -1)
	ui.sansSerifFontCombinationBox->setCurrentIndex
	  (ui.sansSerifFontCombinationBox->findText(font.family()));
      else
	ui.sansSerifFontCombinationBox->setCurrentIndex(0);
    }
  else
    {
      QString text("");

#ifdef Q_OS_MAC
      text = "Helvetica";
#elif defined(Q_OS_WIN32)
      text = "Arial";
#else
      text = "Sans Serif";
#endif

      if(ui.sansSerifFontCombinationBox->
	 findText(text) > -1)
	ui.sansSerifFontCombinationBox->setCurrentIndex
	  (ui.sansSerifFontCombinationBox->
	   findText(text));
      else
	ui.sansSerifFontCombinationBox->setCurrentIndex(0);
    }

  if(font.fromString(dooble::s_settings.value("settingsWindow/"
					      "serifWebFont", "").
		     toString()) && !font.family().isEmpty())
    {
      if(ui.serifFontCombinationBox->findText(font.family()) > -1)
	ui.serifFontCombinationBox->setCurrentIndex
	  (ui.serifFontCombinationBox->findText(font.family()));
      else
	ui.serifFontCombinationBox->setCurrentIndex(0);
    }
  else
    {
      QString text("");

#ifdef Q_OS_MAC
      text = "Times";
#elif defined(Q_OS_WIN32)
      text = "Times New Roman";
#else
      text = "Serif";
#endif

      if(ui.serifFontCombinationBox->
	 findText(text) > -1)
	ui.serifFontCombinationBox->setCurrentIndex
	  (ui.serifFontCombinationBox->findText(text));
      else
	ui.serifFontCombinationBox->setCurrentIndex(0);
    }

  QString size = dooble::s_settings.value
    ("settingsWindow/minimumWebFontSize", "12").toString();

  if(ui.minimumFontSizeCombinationBox->findText(size) > -1)
    ui.minimumFontSizeCombinationBox->setCurrentIndex
      (ui.minimumFontSizeCombinationBox->findText(size));
  else
    ui.minimumFontSizeCombinationBox->setCurrentIndex(0);

  QList<int> mibs(QTextCodec::availableMibs());
  QStringList codecs;

  for(int i = 0; i < mibs.size(); i++)
    {
      QString codec = QLatin1String
	(QTextCodec::codecForMib(mibs.at(i))->name());

      codecs.append(codec);
    }

  codecs.sort();
  ui.encodingCombinationBox->clear();
  ui.encodingCombinationBox->addItems(codecs);

  QString text(dooble::s_settings.value("settingsWindow/characterEncoding",
					"").toString());

  if(ui.encodingCombinationBox->findText(text) > -1)
    ui.encodingCombinationBox->setCurrentIndex
      (ui.encodingCombinationBox->findText(text));
  else
    {
      text = QWebSettings::globalSettings()->defaultTextEncoding();

      for(int i = 0; i < ui.encodingCombinationBox->count(); i++)
	{
	  QString str(ui.encodingCombinationBox->itemText(i));

	  if(str.toLower() == text.toLower())
	    {
	      ui.encodingCombinationBox->setCurrentIndex(i);
	      break;
	    }
	}
    }

  ui.jsAcceptGeometryRequestsCheckBox->setChecked
    (dooble::s_settings.value
     ("settingsWindow/javascriptAcceptGeometryChangeRequests",
      false).toBool());
  ui.historyCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/rememberHistory",
			      true).toBool());
  ui.downloadCheckBox->setChecked
    (dooble::s_settings.value
     ("settingsWindow/rememberDownloads", true).toBool());
  ui.historySpinBox->setValue
    (dooble::s_settings.value("settingsWindow/historyDays", 8).toInt());
  ui.historySpinBox->setEnabled(ui.historyCheckBox->isChecked());
  ui.sightSslErrorsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/sightSslErrors",
			      true).toBool());
  ui.sightSslErrorsPushButton->setEnabled
    (ui.sightSslErrorsCheckBox->isChecked());
  ui.sslLevel->setEnabled(ui.sightSslErrorsCheckBox->isChecked());
  ui.sslLevel->setCurrentIndex
    (dooble::s_settings.value("settingsWindow/sslLevel", 1).toInt());
  ui.jsHideMenuBarCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAllowMenuBarHiding",
                              true).toBool());
  ui.jsHideStatusBarCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAllowStatusBarHiding",
			      false).toBool());
  ui.jsHideToolBarCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/javascriptAllowToolBarHiding",
			      false).toBool());
  ui.alwaysShowTabBarCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/alwaysShowTabBar",
			      true).toBool());
  ui.spatialNavigationCheckBox->setEnabled(true);
  ui.spatialNavigationCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/spatialNavigation",
			      false).toBool());
  ui.closeTabWarningCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/warnBeforeClosingModifiedTab",
			      false).toBool());
  ui.leaveModifiedTabCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/warnBeforeLeavingModifiedTab",
			      false).toBool());

  if(dooble::s_settings.value("settingsWindow/cookiesShouldBe",
			      0).toInt() == 0)
    ui.cookieComboBox->setCurrentIndex(0);
  else if(dooble::s_settings.value("settingsWindow/cookiesShouldBe",
				   0).toInt() == 1)
    ui.cookieComboBox->setCurrentIndex(1);
  else if(dooble::s_settings.value("settingsWindow/cookiesShouldBe",
				   0).toInt() == 2)
    ui.cookieComboBox->setCurrentIndex(2);
  else
    ui.cookieComboBox->setCurrentIndex(0);

  QToolButton *panelButton = 0;

  foreach(QToolButton *button,
	  ui.tabScrollArea->findChildren<QToolButton *> ())
    if(button->property("page").toInt() ==
       dooble::s_settings.value("settingsWindow/currentPage", 0).toInt())
      {
	panelButton = button;
	button->click();
	button->setFocus();
	ui.tabScrollArea->ensureWidgetVisible(button);
	break;
      }

  if(dooble::s_settings.value("settingsWindow/cookieTimerEnabled",
			      false).toBool())
    if(!(ui.cookieTimerCheckBox->checkState() == Qt::Checked))
      {
	ui.cookieTimerCheckBox->click();
	ui.cookieTimerCheckBox->setChecked(true);
      }

  ui.cookieTimerSpinBox->setValue
    (dooble::s_settings.value("settingsWindow/cookieTimerInterval",
			      1).toInt());
  ui.cookieTimerComboBox->setCurrentIndex
    (dooble::s_settings.value("settingsWindow/cookieTimerUnit", 0).toInt());
  ui.textSizeMultiplierSpinBox->setValue
    (dooble::s_settings.value("settingsWindow/textSizeMultiplier",
			      1.00).toFloat());
  ui.rememberClosedTabsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/rememberClosedTabs", false).
     toBool());
  ui.rememberClosedTabsSpinBox->setEnabled
    (ui.rememberClosedTabsCheckBox->isChecked());
  ui.rememberClosedTabsSpinBox->setValue
    (dooble::s_settings.value("settingsWindow/rememberClosedTabsCount", 1).
     toInt());
  ui.closeDownloadsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/closeDownloads",
			      false).toBool());
  ui.reencodeBookmarksCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeBookmarks", false).
     toBool());
  ui.reencodeCacheCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeCache", false).
     toBool());
  ui.reencodeCookiesCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeCookies", false).
     toBool());
  ui.reencodeDownloadsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeDownloads", false).
     toBool());
  ui.reencodeExceptionsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeExceptions", false).
     toBool());
  ui.reencodeFaviconsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeFavicons", false).
     toBool());
  ui.reencodeHistoryCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/reencodeHistory", false).
     toBool());

  int index = dooble::s_settings.value
    ("settingsWindow/thirdPartyCookiesPolicy", 1).toInt();

  if(index + 1 > ui.thirdPartyCookiesComboBox->count())
    ui.thirdPartyCookiesComboBox->setCurrentIndex(0);
  else
    ui.thirdPartyCookiesComboBox->setCurrentIndex(index);

  ui.notifyExceptionsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/notifyOfExceptions",
			      true).toBool());
  ui.purgeMemoryCachesCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/purgeMemoryCaches", true).
     toBool());

  QSize s
    (dooble::s_settings.value("settingsWindow/locationToolbarIconSize",
			      QSize(24, 24)).toSize());

  if(s == QSize(16, 16))
    ui.locationToolbarIconSizeComboBox->setCurrentIndex(0);
  else if(s == QSize(24, 24))
    ui.locationToolbarIconSizeComboBox->setCurrentIndex(1);
  else if(s == QSize(32, 32))
    ui.locationToolbarIconSizeComboBox->setCurrentIndex(2);
  else
    ui.locationToolbarIconSizeComboBox->setCurrentIndex(1);

  ui.sessionRestorationCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/sessionRestoration",
			      true).toBool());
  ui.alwaysHttpsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/alwaysHttps", false).toBool());
  ui.loadImagesExceptionsPushButton->setEnabled
    (ui.automaticallyLoadImagesCheckBox->isChecked());
  ui.popupExceptionsPushButton->setEnabled
    (ui.blockPopupsCheckBox->isChecked());
  ui.thirdPartyExceptionsPushButton->setEnabled
    (ui.thirdPartyContentCheckBox->isChecked());
  ui.dntExceptionsPushButton->setEnabled
    (ui.dntCheckBox->isChecked());
  ui.httpRedirectExceptionsPushButton->setEnabled
    (ui.suppressHttpRedirectCheckBox->isChecked());
  ui.httpReferrerExceptionsPushButton->setEnabled
    (ui.suppressHttpReferrerCheckBox->isChecked());
  ui.alwaysHttpsExceptionsPushButton->setEnabled
    (ui.alwaysHttpsCheckBox->isChecked());
  ui.httponlyexceptions->setEnabled
    (ui.httponlycookies->isChecked());
  ui.diskCacheEnableCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			      false).toBool());

  bool ok = true;
  int value = qAbs
    (dooble::s_settings.value("settingsWindow/webDiskCacheSize",
			      50).toInt(&ok));

  if(ok)
    ui.diskWebCacheSpinBox->setValue(value);
  else
    ui.diskWebCacheSpinBox->setValue(50);

  ui.iterationCountSpinBox->setValue
    (qMax(1000, dooble::s_settings.
	  value("settingsWindow/iterationCount", 10000).toInt()));
  ok = true;
  value = qAbs
    (dooble::s_settings.value("settingsWindow/saltLength", 256).
     toInt(&ok));

  if(ok)
    ui.saltLengthSpinBox->setValue(value);
  else
    ui.saltLengthSpinBox->setValue(256);

  ui.recordFaviconsCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/enableFaviconsDatabase",
			      false).toBool());
  ui.localContentMayAccessLocalContent->setChecked
    (dooble::s_settings.
     value("settingsWindow/localContentMayAccessLocalContent", true).
     toBool());
#if QT_VERSION >= 0x050300
  ui.speedyCheckBox->setChecked
    (dooble::s_settings.value("settingsWindow/speedy", false).toBool());
#else
  ui.speedyCheckBox->setChecked(false);
  ui.speedyCheckBox->setEnabled(false);
#endif
  ui.browsingProxyIgnore->setPlainText
    (dooble::s_settings.value("settingsWindow/browsingProxyIgnore",
			      "localhost, 127.0.0.1").toString().trimmed());
  ui.downloadProxyIgnore->setPlainText
    (dooble::s_settings.value("settingsWindow/downloadProxyIgnore",
			      "localhost, 127.0.0.1").toString().trimmed());
  ui.privateBrowsing->setChecked
    (dooble::s_settings.value("settingsWindow/privateBrowsing",
			      true).toBool());
  ui.jsStagnantScripts->setCurrentIndex
    (qBound(0, dooble::s_settings.
	    value("settingsWindow/javascriptStagnantScripts",
		  2).toInt(), 2));

  if(dooble::s_settings.contains("settingsWindow/"
				 "applicationsTableColumnsState"))
    {
      if(!ui.applicationsTable->horizontalHeader()->restoreState
	 (dooble::s_settings.value("settingsWindow/"
				   "applicationsTableColumnsState",
				   "").toByteArray()))
	ui.applicationsTable->horizontalHeader()->setSortIndicator
	  (0, Qt::AscendingOrder);
    }
  else
    ui.applicationsTable->horizontalHeader()->setSortIndicator
      (0, Qt::AscendingOrder);

  if(dmisc::passphraseWasPrepared())
    {
      if(dmisc::passphraseWasAuthenticated())
	{
	  ui.passphraseGroupBox->setEnabled(false);
	  ui.iterationCountSpinBox->setEnabled(false);
	  ui.saltLengthSpinBox->setEnabled(false);
	  ui.cipherTypeComboBox->setEnabled(false);
	  ui.hashTypeComboBox->setEnabled(false);
	  ui.changePassphrasePushButton->setEnabled(true);
	}
      else
	{
	  ui.passphraseGroupBox->setEnabled(false);
	  ui.iterationCountSpinBox->setEnabled(false);
	  ui.saltLengthSpinBox->setEnabled(false);
	  ui.cipherTypeComboBox->setEnabled(false);
	  ui.hashTypeComboBox->setEnabled(false);
	  ui.changePassphrasePushButton->setEnabled(false);
	}
    }
  else
    {
      ui.passphraseGroupBox->setEnabled(true);
#if DOOBLE_MINIMUM_GCRYPT_VERSION >= 0x010500
      ui.iterationCountSpinBox->setEnabled(true);
#else
      ui.iterationCountSpinBox->setEnabled(false);
#endif
      ui.saltLengthSpinBox->setEnabled(true);
      ui.cipherTypeComboBox->setEnabled(true);
      ui.hashTypeComboBox->setEnabled(true);
      ui.changePassphrasePushButton->setEnabled(false);
    }

  QString str1("");

  if(dooble::s_settings.contains("settingsWindow/cipherType"))
    str1 = dooble::s_settings.value("settingsWindow/cipherType", "unknown").
      toString().mid(0, 16).trimmed();
  else
    str1 = "unknown";

  ui.cipherTypeComboBox->clear();
  ui.cipherTypeComboBox->addItems(dmisc::cipherTypes());

  if(ui.cipherTypeComboBox->count() == 0)
    ui.cipherTypeComboBox->addItem("unknown");

  for(int i = 0; i < ui.cipherTypeComboBox->count(); i++)
    if(str1 == ui.cipherTypeComboBox->itemText(i))
      {
	ui.cipherTypeComboBox->setCurrentIndex(i);
	break;
      }

  if(dooble::s_settings.contains("settingsWindow/passphraseHashType"))
    str1 = dooble::s_settings.value("settingsWindow/passphraseHashType",
				    "unknown").
      toString().mid(0, 16).trimmed();
  else
    str1 = tr("unknown");

  ui.hashTypeComboBox->clear();
  ui.hashTypeComboBox->addItems(dmisc::hashTypes());

  if(ui.hashTypeComboBox->count() == 0)
    ui.hashTypeComboBox->addItem("unknown");

  for(int i = 0; i < ui.hashTypeComboBox->count(); i++)
    if(str1 == ui.hashTypeComboBox->itemText(i))
      {
	ui.hashTypeComboBox->setCurrentIndex(i);
	break;
      }

  str1 = dooble::s_settings.value("settingsWindow/browsingProxySetting",
				  "none").
    toString();

  if(str1 == "none")
    ui.browsingProxyNoneRadio->click();
  else if(str1 == "manual")
    ui.browsingProxyManualRadio->click();
  else
    ui.browsingProxySystemRadio->click();

  str1 = dooble::s_settings.value("settingsWindow/downloadProxySetting",
				  "none").
    toString();

  if(str1 == "none")
    ui.downloadProxyNoneRadio->click();
  else if(str1 == "manual")
    ui.downloadProxyManualRadio->click();
  else
    ui.downloadProxySystemRadio->click();

  int l_width = 0;
  int l_height = 0;

  if(m_parentDooble)
    {
      l_width = static_cast<int> (m_parentDooble->width() -
				  0.15 * m_parentDooble->width());
      l_height = static_cast<int> (m_parentDooble->height() -
				   0.05 * m_parentDooble->height());
    }
  else
    {
      l_width = sizeHint().width();
      l_height = sizeHint().height();
    }

  resize(l_width, l_height);

  if(m_parentDooble)
    {
      if(height() == m_parentDooble->height() &&
	 width() == m_parentDooble->width())
	setGeometry(m_parentDooble->geometry());
      else
	{
	  QPoint p(m_parentDooble->pos());
	  int X = 0;
	  int Y = 0;

	  if(m_parentDooble->width() >= width())
	    X = p.x() + (m_parentDooble->width() - width()) / 2;
	  else
	    X = p.x() - (width() - m_parentDooble->width()) / 2;

	  if(m_parentDooble && m_parentDooble->height() >= height())
	    Y = p.y() + (m_parentDooble->height() - height()) / 2;
	  else
	    Y = p.y() - (height() - m_parentDooble->height()) / 2;

	  move(X, Y);
	}
    }
  else
    move(100, 100);

  showNormal();
  raise();

  if(panelButton)
    panelButton->setFocus();
}

void dsettings::slotClicked(QAbstractButton *button)
{
  if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole ||
     ui.buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole ||
     ui.buttonBox->buttonRole(button) == QDialogButtonBox::YesRole)
    {
      bool shouldReencode = false;

      if(ui.passphraseGroupBox->isEnabled())
	{
	  QString pass1(ui.pass1LineEdit->text());
	  QString pass2(ui.pass2LineEdit->text());

	  if(pass1.isEmpty() && pass2.isEmpty())
	    {
	      // Dream.
	    }
	  else if(pass1 != pass2)
	    {
	      foreach(QToolButton *button,
		      ui.tabScrollArea->findChildren<QToolButton *> ())
		if(button->property("page").toInt() == 5)
		  button->click();

	      QSettings settings(dooble::s_settings.value("iconSet").
				 toString(),
				 QSettings::IniFormat);
	      QMessageBox mb(QMessageBox::Critical,
			     tr("Dooble Web Browser: Error"),
			     tr("The passphrases do not match."),
			     QMessageBox::Cancel,
			     this);

#ifdef Q_OS_MAC
	      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
	      mb.setWindowIcon
		(QIcon(settings.value("mainWindow/windowIcon").toString()));

	      for(int i = 0; i < mb.buttons().size(); i++)
		{
		  mb.buttons().at(i)->setIcon
		    (QIcon(settings.value("cancelButtonIcon").toString()));
		  mb.buttons().at(i)->setIconSize(QSize(16, 16));
		}

	      mb.exec();
	      ui.pass1LineEdit->selectAll();
	      ui.pass1LineEdit->setFocus();
	      return;
	    }
	  else if(pass1.length() < 16)
	    {
	      foreach(QToolButton *button,
		      ui.tabScrollArea->findChildren<QToolButton *> ())
		if(button->property("page").toInt() == 5)
		  button->click();

	      QSettings settings(dooble::s_settings.value("iconSet").
				 toString(),
				 QSettings::IniFormat);
	      QMessageBox mb(QMessageBox::Critical,
			     tr("Dooble Web Browser: Error"),
			     tr("The passphrase must be at least "
				"sixteen characters "
				"long."),
			     QMessageBox::Cancel,
			     this);

#ifdef Q_OS_MAC
	      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
	      mb.setWindowIcon
		(QIcon(settings.value("mainWindow/windowIcon").toString()));

	      for(int i = 0; i < mb.buttons().size(); i++)
		{
		  mb.buttons().at(i)->setIcon
		    (QIcon(settings.value("cancelButtonIcon").toString()));
		  mb.buttons().at(i)->setIconSize(QSize(16, 16));
		}

	      mb.exec();
	      ui.pass1LineEdit->selectAll();
	      ui.pass1LineEdit->setFocus();
	      return;
	    }

	  if(!pass1.isEmpty())
	    {
	      if(ui.changePassphrasePushButton->isEnabled())
		shouldReencode = true;

	      if(shouldReencode)
		dmisc::prepareReencodeCrypt();

	      QByteArray salt;

	      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	      salt.resize(ui.saltLengthSpinBox->value());
	      gcry_fast_random_poll();
	      gcry_randomize((void *) salt.data(), salt.length(),
			     GCRY_STRONG_RANDOM);
	      dmisc::setCipherPassphrase
		(pass1,
		 true,
		 ui.hashTypeComboBox->currentText(),
		 ui.cipherTypeComboBox->currentText(),
		 ui.iterationCountSpinBox->value(),
		 /*
		 ** We'll need a new salt.
		 */
		 salt);
	      QApplication::restoreOverrideCursor();
	    }

	  /*
	  ** Clear the passphrase containers immediately.
	  */

	  pass1.clear();
	  pass2.clear();
	  ui.pass1LineEdit->clear();
	  ui.pass2LineEdit->clear();
	}

      m_previousIconSet.clear();

      QSettings settings;

      settings.setValue
	("iconSet",
	 ui.iconSetComboBox->itemData(ui.iconSetComboBox->currentIndex()).
	 toString());
      dooble::s_settings["iconSet"] =
	ui.iconSetComboBox->itemData(ui.iconSetComboBox->currentIndex()).
	toString();
      settings.setValue("settingsWindow/url1",
			ui.url01lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url2",
			ui.url02lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url3",
			ui.url03lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url4",
			ui.url04lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url5",
			ui.url05lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url6",
			ui.url06lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url7",
			ui.url07lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url8",
			ui.url08lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url9",
			ui.url09lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url10",
			ui.url10lineEdit->text().trimmed());
      settings.setValue("settingsWindow/url11",
			ui.url11lineEdit->text().trimmed());
      settings.setValue("settingsWindow/homeUrl",
			ui.homeLineEdit->text().trimmed());
      settings.setValue("settingsWindow/myRetrievedFiles",
			ui.myRetrievedFilesLineEdit->text().trimmed());
      settings.setValue("settingsWindow/spotOnSharedDatabase",
			ui.spotOnSharedDatabaseLineEdit->text().trimmed());
      settings.setValue("settingsWindow/ircChannel",
			ui.ircLineEdit->text().trimmed());
      settings.setValue("settingsWindow/p2pUrl",
			ui.p2pLineEdit->text().trimmed());
      settings.setValue("settingsWindow/javaEnabled",
			ui.javaCheckBox->isChecked());
      settings.setValue("settingsWindow/javascriptEnabled",
			ui.javascriptGroupBox->isChecked());
      settings.setValue("settingsWindow/javascriptAllowNewWindows",
			ui.jsAllowNewWindowsCheckBox->isChecked());
      settings.setValue("settingsWindow/displayDesktopCheckBox",
			ui.displayDesktopCheckBox->isChecked());
      settings.setValue("settingsWindow/proceedToNewTab",
			ui.proceedToNewTabCheckBox->isChecked());
      settings.setValue("settingsWindow/openInNewTab",
			ui.openInNewTabsCheckBox->isChecked());
      settings.setValue("settingsWindow/httpBrowsingProxyEnabled",
			ui.browsingHttpProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/httpBrowsingProxyType",
			ui.browsingHttpProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/httpBrowsingProxyHost",
			ui.browsingHttpProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/httpBrowsingProxyPort",
			ui.browsingHttpProxyPort->value());
      settings.setValue("settingsWindow/httpBrowsingProxyUser",
			ui.browsingHttpProxyUserName->text());
      settings.setValue("settingsWindow/httpBrowsingProxyPassword",
			ui.browsingHttpProxyPassword->text());
      settings.setValue("settingsWindow/i2pBrowsingProxyEnabled",
			ui.browsingI2pProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/i2pBrowsingProxyType",
			ui.browsingI2pProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/i2pBrowsingProxyHost",
			ui.browsingI2pProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/i2pBrowsingProxyPort",
			ui.browsingI2pProxyPort->value());
      settings.setValue("settingsWindow/i2pDownloadProxyEnabled",
			ui.downloadI2pProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/i2pDownloadProxyType",
			ui.downloadI2pProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/i2pDownloadProxyHost",
			ui.downloadI2pProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/i2pDownloadProxyPort",
			ui.downloadI2pProxyPort->value());
      settings.setValue("settingsWindow/ftpDownloadProxyEnabled",
			ui.downloadFtpProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/ftpDownloadProxyHost",
			ui.downloadFtpProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/ftpDownloadProxyPort",
			ui.downloadFtpProxyPort->value());
      settings.setValue("settingsWindow/ftpDownloadProxyType",
			ui.downloadFtpProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/ftpBrowsingProxyEnabled",
			ui.browsingFtpProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/ftpBrowsingProxyHost",
			ui.browsingFtpProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/ftpBrowsingProxyPort",
			ui.browsingFtpProxyPort->value());
      settings.setValue("settingsWindow/ftpBrowsingProxyType",
			ui.browsingFtpProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/httpDownloadProxyEnabled",
			ui.downloadHttpProxyGroupBox->isChecked());
      settings.setValue("settingsWindow/httpDownloadProxyType",
			ui.downloadHttpProxyType->currentText().trimmed());
      settings.setValue("settingsWindow/httpDownloadProxyHost",
			ui.downloadHttpProxyHostName->text().trimmed());
      settings.setValue("settingsWindow/httpDownloadProxyPort",
			ui.downloadHttpProxyPort->value());
      settings.setValue("settingsWindow/httpDownloadProxyUser",
			ui.downloadHttpProxyUserName->text());
      settings.setValue("settingsWindow/httpDownloadProxyPassword",
			ui.downloadHttpProxyPassword->text());
      settings.setValue("settingsWindow/automaticallyLoadImages",
			ui.automaticallyLoadImagesCheckBox->isChecked());
      settings.setValue("settingsWindow/blockThirdPartyContent",
			ui.thirdPartyContentCheckBox->isChecked());
      settings.setValue("settingsWindow/enableWebPlugins",
			ui.webPluginsCheckBox->isChecked());
      settings.setValue("settingsWindow/addTabWithDoubleClick",
			ui.addTabWithDoubleClick->isChecked());
      settings.setValue("settingsWindow/openUserWindowsInNewProcesses",
			ui.openUserWindowsInNewProcessesCheckBox->
			isChecked());
      settings.setValue("settingsWindow/closeViaMiddle",
			ui.closeViaMiddleCheckBox->isChecked());
      settings.setValue("settingsWindow/cookiesEnabled",
			ui.cookieGroupBox->isChecked());
      settings.setValue("settingsWindow/blockPopups",
			ui.blockPopupsCheckBox->isChecked());
      settings.setValue("settingsWindow/cursiveWebFont",
			ui.cursiveFontCombinationBox->
			currentFont().toString());
      settings.setValue("settingsWindow/fantasyWebFont",
			ui.fantasyFontCombinationBox->
			currentFont().toString());
      settings.setValue("settingsWindow/sansSerifWebFont",
			ui.sansSerifFontCombinationBox->
			currentFont().toString());
      settings.setValue("settingsWindow/serifWebFont",
			ui.serifFontCombinationBox->
			currentFont().toString());
      settings.setValue("settingsWindow/minimumWebFontSize",
			ui.minimumFontSizeCombinationBox->
			currentText().toInt());
      settings.setValue("settingsWindow/characterEncoding",
			ui.encodingCombinationBox->
			currentText());
      settings.setValue
	("settingsWindow/javascriptAcceptGeometryChangeRequests",
	 ui.jsAcceptGeometryRequestsCheckBox->isChecked());
      settings.setValue
	("settingsWindow/javascriptAcceptAlerts",
	 ui.jsAcceptAlertsCheckBox->isChecked());
      settings.setValue
	("settingsWindow/javascriptAcceptConfirmations",
	 ui.jsAcceptConfirmationsCheckBox->isChecked());
      settings.setValue
	("settingsWindow/javascriptAcceptPrompts",
	 ui.jsAcceptPromptsCheckBox->isChecked());
      settings.setValue("settingsWindow/rememberHistory",
			ui.historyCheckBox->isChecked());
      settings.setValue("settingsWindow/rememberDownloads",
			ui.downloadCheckBox->isChecked());
      settings.setValue("settingsWindow/historyDays",
			ui.historySpinBox->value());
      settings.setValue("settingsWindow/javascriptAllowMenuBarHiding",
			ui.jsHideMenuBarCheckBox->isChecked());
      settings.setValue("settingsWindow/javascriptAllowStatusBarHiding",
			ui.jsHideStatusBarCheckBox->isChecked());
      settings.setValue("settingsWindow/javascriptAllowToolBarHiding",
			ui.jsHideToolBarCheckBox->isChecked());
      settings.setValue("settingsWindow/alwaysShowTabBar",
			ui.alwaysShowTabBarCheckBox->isChecked());
      settings.setValue("settingsWindow/spatialNavigation",
			ui.spatialNavigationCheckBox->isChecked());
      settings.setValue("settingsWindow/cookiesShouldBe",
			ui.cookieComboBox->currentIndex());
      settings.setValue("settingsWindow/textSizeMultiplier",
			ui.textSizeMultiplierSpinBox->value());

      if(ui.browsingProxyManualRadio->isChecked())
	settings.setValue("settingsWindow/browsingProxySetting", "manual");
      else if(ui.browsingProxySystemRadio->isChecked())
	settings.setValue("settingsWindow/browsingProxySetting", "system");
      else
	settings.setValue("settingsWindow/browsingProxySetting", "none");

      if(ui.downloadProxyManualRadio->isChecked())
	settings.setValue("settingsWindow/downloadProxySetting", "manual");
      else if(ui.downloadProxySystemRadio->isChecked())
	settings.setValue("settingsWindow/downloadProxySetting", "system");
      else
	settings.setValue("settingsWindow/downloadProxySetting", "none");

      QFont font;

      font = ui.fixedFontCombinationBox->currentFont();
      font.setPointSize(ui.fixedFontSizeCombinationBox->
			currentText().toInt());
      font.setWeight(QFont::Normal);
      settings.setValue("settingsWindow/fixedWebFont", font.toString());
      font = ui.defaultFontCombinationBox->currentFont();
      font.setPointSize(ui.defaultFontSizeCombinationBox->
			currentText().toInt());
      font.setWeight(QFont::Normal);
      settings.setValue("settingsWindow/standardWebFont", font.toString());
      settings.setValue("settingsWindow/currentPage",
			ui.stackedWidget->currentIndex());
      settings.setValue("settingsWindow/cookieTimerEnabled",
			ui.cookieTimerCheckBox->isChecked());
      settings.setValue("settingsWindow/cookieTimerInterval",
			ui.cookieTimerSpinBox->value());
      settings.setValue("settingsWindow/cookieTimerUnit",
			ui.cookieTimerComboBox->currentIndex());
      settings.setValue("settingsWindow/closeDownloads",
			ui.closeDownloadsCheckBox->isChecked());
      settings.setValue("settingsWindow/suppressHttpRedirect1",
			ui.suppressHttpRedirectCheckBox->isChecked());
      settings.setValue("settingsWindow/suppressHttpReferrer1",
			ui.suppressHttpReferrerCheckBox->isChecked());
      settings.setValue("settingsWindow/doNotTrack",
			ui.dntCheckBox->isChecked());
      settings.setValue("settingsWindow/xssAuditingEnabled",
			ui.xssAuditingCheckBox->isChecked());
      settings.setValue("settingsWindow/appendNewTabs",
			ui.appendNewTabsCheckBox->isChecked());
      settings.setValue("settingsWindow/displayIpAddress",
			ui.displayIpAddressCheckBox->isChecked());
      settings.setValue("settingsWindow/warnBeforeClosingModifiedTab",
			ui.closeTabWarningCheckBox->isChecked());
      settings.setValue("settingsWindow/warnBeforeLeavingModifiedTab",
			ui.leaveModifiedTabCheckBox->isChecked());
      settings.setValue("settingsWindow/rememberClosedTabs",
			ui.rememberClosedTabsCheckBox->isChecked());
      settings.setValue("settingsWindow/rememberClosedTabsCount",
			ui.rememberClosedTabsSpinBox->value());
      settings.setValue("settingsWindow/reencodeBookmarks",
			ui.reencodeBookmarksCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeCache",
			ui.reencodeCacheCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeCookies",
			ui.reencodeCookiesCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeDownloads",
			ui.reencodeDownloadsCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeExceptions",
			ui.reencodeExceptionsCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeFavicons",
			ui.reencodeFaviconsCheckBox->isChecked());
      settings.setValue("settingsWindow/reencodeHistory",
			ui.reencodeHistoryCheckBox->isChecked());
      settings.setValue("settingsWindow/thirdPartyCookiesPolicy",
			ui.thirdPartyCookiesComboBox->currentIndex());
      settings.setValue("settingsWindow/notifyOfExceptions",
			ui.notifyExceptionsCheckBox->isChecked());
      settings.setValue("settingsWindow/purgeMemoryCaches",
			ui.purgeMemoryCachesCheckBox->isChecked());
      settings.setValue("settingsWindow/applicationsTableColumnsState",
			ui.applicationsTable->horizontalHeader()->
			saveState());
      settings.setValue
	("settingsWindow/locationToolbarIconSize",
	 QSize(ui.locationToolbarIconSizeComboBox->currentText().toInt(),
	       ui.locationToolbarIconSizeComboBox->currentText().toInt()));
      settings.setValue("settingsWindow/sessionRestoration",
			ui.sessionRestorationCheckBox->isChecked());
      settings.setValue("settingsWindow/ftpBrowsingProxyUser",
			ui.browsingFtpProxyUserName->text());
      settings.setValue("settingsWindow/ftpBrowsingProxyPassword",
			ui.browsingFtpProxyPassword->text());
      settings.setValue("settingsWindow/ftpDownloadProxyUser",
			ui.downloadFtpProxyUserName->text());
      settings.setValue("settingsWindow/ftpDownloadProxyPassword",
			ui.downloadFtpProxyPassword->text());
      settings.setValue("settingsWindow/diskCacheEnabled",
			ui.diskCacheEnableCheckBox->isChecked());
      settings.setValue("settingsWindow/webglEnabled",
			ui.webglCheckBox->isChecked());
      settings.setValue("settingsWindow/webDiskCacheSize",
			ui.diskWebCacheSpinBox->value());
      settings.setValue("settingsWindow/enableFaviconsDatabase",
			ui.recordFaviconsCheckBox->isChecked());
      settings.setValue("settingsWindow/alwaysHttps",
			ui.alwaysHttpsCheckBox->isChecked());
      settings.setValue("settingsWindow/showAuthentication",
			ui.showAuthenticationCheckBox->isChecked());
      settings.setValue("settingsWindow/sightSslErrors",
			ui.sightSslErrorsCheckBox->isChecked());
      settings.setValue("settingsWindow/hyperlinkAuditing",
			ui.hyperlinkAuditing->isChecked());
      settings.setValue("settingsWindow/sslLevel",
			ui.sslLevel->currentIndex());
      settings.setValue("settingsWindow/localContentMayAccessLocalContent",
			ui.localContentMayAccessLocalContent->isChecked());
      settings.setValue("settingsWindow/centerChildWindows",
			ui.centerChildWindows->isChecked());
      settings.setValue("settingsWindow/useNativeFileDialogs",
			ui.useNativeFileDialogs->isChecked());
      settings.setValue("settingsWindow/disableAllEncryptedDatabaseWrites",
			ui.disableAllEncryptedDatabaseWrites->isChecked());
      settings.setValue("settingsWindow/speedy",
			ui.speedyCheckBox->isChecked());
      settings.setValue("settingsWindow/browsingProxyIgnore",
			ui.browsingProxyIgnore->toPlainText().trimmed());
      settings.setValue("settingsWindow/downloadProxyIgnore",
			ui.downloadProxyIgnore->toPlainText().trimmed());
      settings.setValue("settingsWindow/privateBrowsing",
			ui.privateBrowsing->isChecked());
      settings.setValue("settingsWindow/httpOnlyCookies",
			ui.httponlycookies->isChecked());
      settings.setValue
	("settingsWindow/displaypriority",
	 ui.displaypriority->itemData(ui.displaypriority->currentIndex()).
	 toInt());
      settings.setValue
	("settingsWindow/javascriptStagnantScripts",
	 ui.jsStagnantScripts->currentIndex());

      QString str("");

      if(ui.tab_bar_position->currentIndex() == 0)
	str = "east";
      else if(ui.tab_bar_position->currentIndex() == 1)
	str = "north";
      else if(ui.tab_bar_position->currentIndex() == 2)
	str = "south";
      else
	str = "west";

      settings.setValue("settingsWindow/tabBarPosition", str);

      QThread *thread = QApplication::instance()->thread();

      if(thread)
	thread->setPriority
	  (QThread::Priority(ui.displaypriority->
			     itemData(ui.displaypriority->
				      currentIndex()).toInt()));

      if(shouldReencode)
	{
	  /*
	  ** Perform re-encoding only if a passphrase was
	  ** previously set.
	  */

	  if(ui.sessionRestorationCheckBox->isChecked())
	    emit reencodeRestorationFile();

	  if(ui.reencodeBookmarksCheckBox->isChecked())
	    dooble::s_bookmarksWindow->reencode(ui.bookmarksProgressBar);

	  if(ui.reencodeCacheCheckBox->isChecked())
	    dooble::s_networkCache->reencode(ui.cacheProgressBar);

	  if(ui.reencodeCookiesCheckBox->isChecked())
	    dooble::s_cookies->reencode(ui.cookiesProgressBar);

	  if(ui.reencodeDownloadsCheckBox->isChecked())
	    dooble::s_downloadWindow->reencode(ui.downloadsProgressBar);

	  if(ui.reencodeExceptionsCheckBox->isChecked())
	    {
	      dooble::s_adBlockWindow->reencode(ui.exceptionsProgressBar);
	      dooble::s_alwaysHttpsExceptionsWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_cacheExceptionsWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_cookiesBlockWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_dntWindow->reencode(ui.exceptionsProgressBar);
	      dooble::s_httpOnlyExceptionsWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_httpRedirectWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_httpReferrerWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_imageBlockWindow->reencode(ui.exceptionsProgressBar);
	      dooble::s_javaScriptExceptionsWindow->reencode
		(ui.exceptionsProgressBar);
	      dooble::s_popupsWindow->reencode(ui.exceptionsProgressBar);
	      dooble::s_sslExceptionsWindow->reencode
		(ui.exceptionsProgressBar);
	    }

	  if(ui.reencodeFaviconsCheckBox->isChecked())
	    dmisc::reencodeFavicons(ui.faviconsProgressBar);

	  if(ui.reencodeHistoryCheckBox->isChecked())
	    dooble::s_historyWindow->reencode(ui.historyProgressBar);

	  /*
	  ** Very important.
	  */

	  dmisc::destroyReencodeCrypt();
	}

      QWebSettings::globalSettings()->setDefaultTextEncoding
	(ui.encodingCombinationBox->currentText().toLower());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::JavaEnabled,
	 ui.javaCheckBox->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::JavascriptEnabled,
	 ui.javascriptGroupBox->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::JavascriptCanOpenWindows,
	 ui.jsAllowNewWindowsCheckBox->isChecked() &&
	 ui.javascriptGroupBox->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::AutoLoadImages,
	 ui.automaticallyLoadImagesCheckBox->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::PluginsEnabled,
	 ui.webPluginsCheckBox->isChecked());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::StandardFont,
	 ui.defaultFontSizeCombinationBox->currentText());
      QWebSettings::globalSettings()->setFontSize
	(QWebSettings::DefaultFontSize,
	 ui.defaultFontSizeCombinationBox->currentText().toInt());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::CursiveFont,
	 ui.cursiveFontCombinationBox->currentFont().family());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::FantasyFont,
	 ui.fantasyFontCombinationBox->currentFont().family());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::FixedFont,
	 ui.fixedFontCombinationBox->currentFont().family());
      QWebSettings::globalSettings()->setFontSize
	(QWebSettings::DefaultFixedFontSize,
	 ui.fixedFontSizeCombinationBox->currentText().toInt());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::SansSerifFont,
	 ui.sansSerifFontCombinationBox->currentFont().family());
      QWebSettings::globalSettings()->setFontFamily
	(QWebSettings::SerifFont,
	 ui.serifFontCombinationBox->currentFont().family());
      QWebSettings::globalSettings()->setFontSize
	(QWebSettings::MinimumFontSize,
	 ui.minimumFontSizeCombinationBox->currentText().toInt());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::SpatialNavigationEnabled,
	 ui.spatialNavigationCheckBox->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::XSSAuditingEnabled,
	 ui.javascriptGroupBox->isChecked() &&
	 ui.xssAuditingCheckBox->isChecked());
#if QT_VERSION >= 0x050000
      QWebSettings::globalSettings()->setThirdPartyCookiePolicy
	(QWebSettings::ThirdPartyCookiePolicy(ui.thirdPartyCookiesComboBox->
					      currentIndex()));
#endif
#if QT_VERSION >= 0x040800
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::HyperlinkAuditingEnabled,
	 ui.hyperlinkAuditing->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::WebGLEnabled, ui.webglCheckBox->isChecked());
#endif
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::LocalContentCanAccessFileUrls,
	 ui.localContentMayAccessLocalContent->isChecked());
      QWebSettings::globalSettings()->setAttribute
	(QWebSettings::PrivateBrowsingEnabled,
	 ui.privateBrowsing->isChecked());

      /*
      ** Populate the dooble::s_settings container.
      */

      for(int i = 0; i < settings.allKeys().size(); i++)
	dooble::s_settings[settings.allKeys().at(i)] =
	  settings.value(settings.allKeys().at(i));

      for(int i = 0; i < ui.iconSetComboBox->count(); i++)
	{
	  settings.setValue(QString("settingsWindow/iconSet%1").arg(i + 1),
			    ui.iconSetComboBox->itemData(i));
	  dooble::s_settings[QString("settingsWindow/iconSet%1").arg(i + 1)] =
	    ui.iconSetComboBox->itemData(i);
	}

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      QHash<int, int> httpStatusCodes;

      for(int i = 0; i < ui.httpStatusCodes->rowCount(); i++)
	{
	  QTableWidgetItem *item = ui.httpStatusCodes->item(i, 0);

	  if(item)
	    httpStatusCodes[item->text().toInt()] =
	      (item->checkState() == Qt::Checked);
	}

      dmisc::prepareProxyIgnoreLists();
      dmisc::updateHttpStatusCodes(httpStatusCodes);
      QApplication::restoreOverrideCursor();

      /*
      ** Some of these signals should have been combined.
      */

      emit cookieTimerChanged();
      emit iconsChanged();
      emit settingsChanged();
      emit showIpAddress(ui.displayIpAddressCheckBox->isChecked());
      emit showTabBar(ui.alwaysShowTabBarCheckBox->isChecked());
      emit textSizeMultiplierChanged(ui.textSizeMultiplierSpinBox->value());

      if(ui.purgeMemoryCachesCheckBox->isChecked())
	{
	  if(!m_purgeMemoryCachesTimer.isActive())
	    m_purgeMemoryCachesTimer.start();
	}
      else
	m_purgeMemoryCachesTimer.stop();

      if(ui.sessionRestorationCheckBox->isChecked())
	dooble::s_makeCrashFile();
      else
	dooble::s_removeCrashFile();

      slotSetIcons();
    }
  else if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
      QMessageBox mb(this);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setIcon(QMessageBox::Question);
      mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
      mb.setWindowModality(Qt::WindowModal);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("You are about to reset all of your configuration "
		    "settings. Dooble will be restarted in order to "
		    "complete the reset process. Do you wish to "
                    "continue?"));

      QSettings settings(dooble::s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);

      for(int i = 0; i < mb.buttons().size(); i++)
	if(mb.buttonRole(mb.buttons().at(i)) == QMessageBox::AcceptRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::ApplyRole ||
	   mb.buttonRole(mb.buttons().at(i)) == QMessageBox::YesRole)
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("okButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }
	else
	  {
	    mb.buttons().at(i)->setIcon
	      (QIcon(settings.value("cancelButtonIcon").toString()));
	    mb.buttons().at(i)->setIconSize(QSize(16, 16));
	  }

      mb.setWindowIcon
	(QIcon(settings.value("settingsWindow/windowIcon").toString()));

      if(mb.exec() == QMessageBox::Yes)
	{
	  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	  QSettings settings;

	  QFile::remove(settings.fileName());
	  dooble::s_settings.clear();

	  QStringList list;

	  list << "adblockexceptions.db"
	       << "allowedsslciphers.db"
	       << "alwayshttpsexceptions.db"
	       << "applications.db"
	       << "autoloadedimagesexceptions.db"
	       << "bookmarks.db"
	       << "cacheexceptions.db"
	       << "cookies.db"
	       << "cookiesexceptions.db"
	       << "dntexceptions.db"
	       << "downloads.db"
	       << "favicons.db"
	       << "history.db"
	       << "javascriptexceptions.db"
	       << "popupsexceptions.db"
	       << "sslexceptions.db"
	       << "suppresshttpredirectexceptions.db"
	       << "suppresshttpreferrerexceptions.db";

	  while(!list.isEmpty())
	    QFile::remove
	      (dooble::s_homePath + QDir::separator() +
	       list.takeFirst());

	  dmisc::removeRestorationFiles();
	  dooble::s_networkCache->clear();
	  QApplication::restoreOverrideCursor();
	  emit settingsReset();
	}

      return;
    }

  close();
}

void dsettings::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dsettings::slotSelectIconCfgFile(void)
{
  QFileDialog fileDialog(this);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setWindowTitle(tr("Dooble Web Browser: Theme "
			       "Selection"));

  if(!dooble::s_settings.value("settingsWindow/themesPath", "").toString().
     trimmed().isEmpty())
    {
      if(QFile::exists(dooble::s_settings.value("settingsWindow/themesPath").
		       toString()))
	fileDialog.setDirectory
	  (dooble::s_settings.value("settingsWindow/themesPath").toString());
      else
	fileDialog.setDirectory(QDir::currentPath());
    }
  else
    fileDialog.setDirectory(QDir::currentPath());

  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setLabelText(QFileDialog::Accept, tr("Select"));
  fileDialog.setNameFilter(tr("Theme Configuration File (*.cfg)"));

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QStringList list(fileDialog.selectedFiles());

      if(!list.isEmpty())
	{
	  QDir dir(fileDialog.directory());
	  QSettings settings;

	  if(dir.cdUp())
	    {
	      settings.setValue("settingsWindow/themesPath",
				dir.absolutePath());
	      dooble::s_settings["settingsWindow/themesPath"] =
		dir.absolutePath();
	    }
	  else
	    {
	      settings.setValue("settingsWindow/themesPath",
				dooble::s_homePath);
	      dooble::s_settings["settingsWindow/themesPath"] =
		dooble::s_homePath;
	    }

	  bool itemInserted = false;
	  QSettings cfgSettings(list.at(0),
				QSettings::IniFormat);

	  cfgSettings.beginGroup("Description");
	  disconnect(ui.iconSetComboBox,
		     SIGNAL(currentIndexChanged(int)),
		     this,
		     SLOT(slotIconsPreview(void)));

	  for(int i = 0; i < ui.iconSetComboBox->count(); i++)
	    if(cfgSettings.value("Name").toString() <
	       ui.iconSetComboBox->itemText(i))
	      {
		itemInserted = true;
		ui.iconSetComboBox->insertItem
		  (i, cfgSettings.value("Name").toString(),
		   list.at(0));
		ui.iconSetComboBox->setCurrentIndex(i);
		break;
	      }
	    else if(cfgSettings.value("Name").toString() ==
		    ui.iconSetComboBox->itemText(i))
	      {
		itemInserted = true;
		ui.iconSetComboBox->setCurrentIndex(i);
		break;
	      }

	  if(!itemInserted)
	    {
	      ui.iconSetComboBox->addItem
		(cfgSettings.value("Name").toString(),
		 list.at(0));
	      ui.iconSetComboBox->setCurrentIndex
		(ui.iconSetComboBox->count() - 1);
	    }

	  connect(ui.iconSetComboBox,
		  SIGNAL(currentIndexChanged(int)),
		  this,
		  SLOT(slotIconsPreview(void)));
	  slotIconsPreview();
	}
    }
}

void dsettings::slotChooseMyRetrievedFilesDirectory(void)
{
  QFileDialog fileDialog(this);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setWindowTitle
    (tr("Dooble Web Browser: My Retrieved Files Directory Selection"));
  fileDialog.setFileMode(QFileDialog::Directory);
  fileDialog.setDirectory(QDir::homePath());
  fileDialog.setLabelText(QFileDialog::Accept, tr("Open"));
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QStringList list(fileDialog.selectedFiles());

      if(!list.isEmpty())
	ui.myRetrievedFilesLineEdit->setText(list.at(0));
    }
}

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
void dsettings::slotChooseSpotOnSharedDatabaseFile(void)
{
  QFileDialog fileDialog(this);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setWindowTitle
    (tr("Dooble Web Browser: Spot-on Shared Database Selection"));
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setDirectory(QDir::homePath());
  fileDialog.setLabelText(QFileDialog::Accept, tr("Open"));
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QStringList list(fileDialog.selectedFiles());

      if(!list.isEmpty())
	ui.spotOnSharedDatabaseLineEdit->setText(list.at(0));
    }
}
#endif

void dsettings::updateFontWidgets(const QString &fontName,
				  QComboBox *fontSizeWidget)
{
  if(fontName.isEmpty() || !fontSizeWidget)
    return;

  disconnect(fontSizeWidget,
	     SIGNAL(currentIndexChanged(const QString &)),
	     this, SLOT(slotWebFontChanged(const QString &)));
  fontSizeWidget->clear();
  connect(fontSizeWidget,
	  SIGNAL(currentIndexChanged(const QString &)),
	  this, SLOT(slotWebFontChanged(const QString &)));

  QList<int> fontSizes;
  QFontDatabase fontDatabase;

  fontSizes = fontDatabase.pointSizes(fontName);

  /*
  ** The minimum Web font size shall be 6.
  */

  disconnect(fontSizeWidget,
	     SIGNAL(currentIndexChanged(const QString &)),
	     this, SLOT(slotWebFontChanged(const QString &)));

  for(int i = 0; i < fontSizes.size(); i++)
    if(fontSizes.at(i) > 5)
      fontSizeWidget->addItem(QString::number(fontSizes.at(i)));

  if(fontSizes.isEmpty())
    fontSizeWidget->addItem("6");

  connect(fontSizeWidget,
	  SIGNAL(currentIndexChanged(const QString &)),
	  this, SLOT(slotWebFontChanged(const QString &)));
}

void dsettings::slotWebFontChanged(const QString &text)
{
  if(text.isEmpty())
    return;

  QComboBox *comboBox = qobject_cast<QComboBox *> (sender());

  if(!comboBox)
    return;

  if(comboBox == ui.defaultFontCombinationBox)
    updateFontWidgets(text, ui.defaultFontSizeCombinationBox);
  else if(comboBox == ui.fixedFontCombinationBox)
    updateFontWidgets(text, ui.fixedFontSizeCombinationBox);
}

void dsettings::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(),
     QSettings::IniFormat);

  foreach(QPushButton *button,
	  ui.buttonBox->findChildren<QPushButton *>())
    {
      button->setIconSize(QSize(16, 16));

      if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole ||
	 ui.buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole ||
	 ui.buttonBox->buttonRole(button) == QDialogButtonBox::YesRole)
	button->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
      else if(ui.buttonBox->buttonRole(button) == QDialogButtonBox::RejectRole)
	button->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
      else
	button->setIcon
	  (QIcon(settings.value("settingsWindow/resetButtonIcon").toString()));
    }

  ui.applicationsButton->setIcon
    (QIcon(settings.value("settingsWindow/applicationsButtonIcon").
	   toString()));
  ui.displayButton->setIcon
    (QIcon(settings.value("settingsWindow/displayButtonIcon").toString()));
  ui.historyButton->setIcon
    (QIcon(settings.value("settingsWindow/historyButtonIcon").toString()));
  ui.homeButton->setIcon
    (QIcon(settings.value("settingsWindow/homeButtonIcon").toString()));
  ui.proxyButton->setIcon
    (QIcon(settings.value("settingsWindow/proxyButtonIcon").toString()));
  ui.securityButton->setIcon
    (QIcon(settings.value("settingsWindow/securityButtonIcon").toString()));
  ui.tabsButton->setIcon
    (QIcon(settings.value("settingsWindow/tabsButtonIcon").toString()));
  ui.webkitButton->setIcon
    (QIcon(settings.value("settingsWindow/webkitButtonIcon").toString()));
  ui.windowsButton->setIcon
    (QIcon(settings.value("settingsWindow/windowsButtonIcon").toString()));
  ui.safeButton->setIcon
    (QIcon(settings.value("settingsWindow/safeButtonIcon").toString()));

  int h = 32; // Based on the icon size. See UI/settings.ui.
  int w = 0;

  foreach(QToolButton *button,
	  ui.tabScrollArea->findChildren<QToolButton *> ())
    {
      if(button->sizeHint().width() > w)
	w = button->sizeHint().width();

      if(button->sizeHint().height() > h)
	h = button->sizeHint().height();
    }

  foreach(QToolButton *button,
	  ui.tabScrollArea->findChildren<QToolButton *> ())
    button->setMinimumSize(w, h);

  ui.tabScrollArea->setMinimumWidth(w + 30);
}

void dsettings::slotChangePage(bool checked)
{
  QToolButton *button = qobject_cast<QToolButton *> (sender());

  if(!button)
    return;
  else if(!checked)
    {
      button->setChecked(true);
      button->setFocus();
      return;
    }

  foreach(QToolButton *btn, ui.tabScrollArea->findChildren<QToolButton *> ())
    if(btn != button)
      btn->setChecked(!checked);
    else
      btn->setChecked(checked);

  button->setFocus();
  ui.stackedWidget->setCurrentIndex(button->property("page").toInt());
}

void dsettings::slotCookieTimerTimeChanged(int index)
{
  int value = ui.cookieTimerSpinBox->value();

  if(index == 0)
    {
      ui.cookieTimerSpinBox->setMinimum(1);
      ui.cookieTimerSpinBox->setMaximum(60);
    }
  else
    {
      ui.cookieTimerSpinBox->setMinimum(1);
      ui.cookieTimerSpinBox->setMaximum(3600);
    }

  ui.cookieTimerSpinBox->setValue(value);
}

void dsettings::closeEvent(QCloseEvent *event)
{
  m_updateLabelTimer.stop();
  ui.pass1LineEdit->clear();
  ui.pass2LineEdit->clear();

  if(!m_previousIconSet.isEmpty())
    {
      dooble::s_settings["iconSet"] = m_previousIconSet;
      emit iconsChanged();
    }

  m_previousIconSet.clear();
  QMainWindow::closeEvent(event);
}

void dsettings::slotEnablePassphrase(void)
{
  ui.passphraseGroupBox->setEnabled(true);
#if DOOBLE_MINIMUM_GCRYPT_VERSION >= 0x010500
  ui.iterationCountSpinBox->setEnabled
    (ui.changePassphrasePushButton->isEnabled());
#else
  ui.iterationCountSpinBox->setEnabled(false);
#endif
  ui.saltLengthSpinBox->setEnabled
    (ui.changePassphrasePushButton->isEnabled());
  ui.cipherTypeComboBox->setEnabled
    (ui.changePassphrasePushButton->isEnabled());
  ui.hashTypeComboBox->setEnabled
    (ui.changePassphrasePushButton->isEnabled());
  ui.pass1LineEdit->setFocus();
}

void dsettings::slotGroupBoxClicked(bool checked)
{
  QGroupBox *groupBox = qobject_cast<QGroupBox *> (sender());

  if(groupBox && groupBox == ui.cookieGroupBox)
    {
      ui.cookieTimerSpinBox->setEnabled
	(checked && ui.cookieTimerCheckBox->isChecked());
      ui.cookieTimerComboBox->setEnabled
	(checked && ui.cookieTimerCheckBox->isChecked());
    }
}

void dsettings::slotPassphraseWasAuthenticated(const bool state)
{
  /*
  ** This slot is connected to dooble's passphraseWasAuthenticated()
  ** signal.
  */

  ui.diskCacheEnableCheckBox->setEnabled(state);
  ui.clearDiskCachePushButton->setEnabled(state);
  ui.passphraseGroupBox->setEnabled(false);
  ui.changePassphrasePushButton->setEnabled(state);
}

void dsettings::slotIconsPreview(void)
{
  if(m_previousIconSet.isEmpty())
    m_previousIconSet = dooble::s_settings["iconSet"].toString();

  dooble::s_settings["iconSet"] =
    ui.iconSetComboBox->itemData(ui.iconSetComboBox->currentIndex()).
    toString();
  emit iconsChanged();
}

dooble *dsettings::parentDooble(void) const
{
  return m_parentDooble;
}

void dsettings::slotPurgeMemoryCaches(void)
{
  QWebSettings::globalSettings()->clearMemoryCaches();
}

void dsettings::slotPopulateApplications
(const QMap<QString, QString> &suffixes)
{
  ui.applicationsTable->setSortingEnabled(false);

  for(int i = 0; i < suffixes.keys().size(); i++)
    {
      QComboBox *comboBox = 0;
      QTableWidgetItem *item = 0;

      item = new QTableWidgetItem(suffixes.keys().at(i));
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item->setIcon(dmisc::iconForFileSuffix(suffixes.keys().at(i)));
      comboBox = new QComboBox(this);
      comboBox->addItem(tr("Prompt"));
      comboBox->setItemData(0, item->text());
      comboBox->insertSeparator(1);

      QString action(suffixes[suffixes.keys().at(i)]);

      if(action == "prompt")
	comboBox->setCurrentIndex(0);
      else
	{
	  QFileInfo fileInfo(action);

	  comboBox->addItem(fileInfo.fileName());
	  comboBox->setItemData(2, action);
	  comboBox->setCurrentIndex(2);
	}

      comboBox->addItem(tr("Use other..."));
      connect(comboBox,
	      SIGNAL(activated(int)),
	      this,
	      SLOT(slotApplicationPulldownActivated(int)));
      ui.applicationsTable->setRowCount
	(ui.applicationsTable->rowCount() + 1);
      ui.applicationsTable->setItem
	(ui.applicationsTable->rowCount() - 1, 0, item);
      ui.applicationsTable->setCellWidget
	(ui.applicationsTable->rowCount() - 1, 1, comboBox);
    }

  ui.applicationsTable->setSortingEnabled(true);
}

void dsettings::slotApplicationPulldownActivated(int index)
{
  /*
  ** 0 - Prompt
  ** 1 - Separator
  ** 2 - Use Other
  **
  ** 0 - Prompt
  ** 1 - Separator
  ** 2 - Application
  ** 3 - Use Other
  */

  QComboBox *comboBox = qobject_cast<QComboBox *> (sender());

  if(comboBox)
    {
      QString action("");
      QString suffix(comboBox->itemData(0).toString());
      QFileDialog *fileDialog = 0;

      if(comboBox->count() == 3)
	{
	  /*
	  ** An application has not been selected.
	  */

	  if(index == 2)
	    fileDialog = new QFileDialog(this);
	}
      else
	{
	  /*
	  ** An application was previously selected.
	  */

	  if(index == 3)
	    fileDialog = new QFileDialog(this);
	}

      if(fileDialog)
	{
	  fileDialog->setFilter(QDir::AllDirs | QDir::Files
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	                        | QDir::Readable | QDir::Executable);
#else
	                       );
#endif
	  fileDialog->setWindowTitle
	    (tr("Dooble Web Browser: Select Application"));
	  fileDialog->setFileMode(QFileDialog::ExistingFile);
	  fileDialog->setDirectory(QDir::homePath());
	  fileDialog->setLabelText(QFileDialog::Accept, tr("Select"));
	  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
#ifdef Q_OS_MAC
	  fileDialog->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
	  fileDialog->setOption
	    (QFileDialog::DontUseNativeDialog,
	     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
				       false).toBool());

	  if(fileDialog->exec() == QDialog::Accepted)
	    {
	      action = fileDialog->selectedFiles().value(0);

	      if(action.isEmpty())
		action = "prompt";

	      dooble::s_applicationsActions[suffix] = action;

	      if(action == "prompt")
		comboBox->setCurrentIndex(0);
	      else
		{
		  if(comboBox->count() == 3)
		    comboBox->insertItem
		      (2, QFileInfo(fileDialog->selectedFiles().value(0)).
		       fileName());
		  else
		    comboBox->setItemText
		      (2, QFileInfo(fileDialog->selectedFiles().value(0)).
		       fileName());

		  comboBox->setItemData
		    (2, fileDialog->selectedFiles().value(0));
		  comboBox->setCurrentIndex(2);
		}
	    }
	  else
	    {
	      /*
	      ** The user rejected a selection. Let's set
	      ** the combination box's value to its previous setting.
	      */

	      if(dooble::s_applicationsActions[suffix] == "prompt")
		comboBox->setCurrentIndex(0);
	      else
		comboBox->setCurrentIndex(2);
	    }

	  fileDialog->deleteLater();
	}
      else if(index == 0)
	{
	  action = "prompt";
	  dooble::s_applicationsActions[suffix] = action;
	}
      else if(index == 2)
	{
	  action = comboBox->itemData(index).toString();
	  dooble::s_applicationsActions[suffix] = action;
	}

      if(!action.isEmpty())
	dmisc::setActionForFileSuffix(suffix, action);
    }
}

void dsettings::slotUpdateApplication(const QString &suffix,
				      const QString &action)
{
  QList<QTableWidgetItem *> list(ui.applicationsTable->
				 findItems(suffix, Qt::MatchExactly));

  if(!list.isEmpty())
    {
      list.at(0)->setIcon(dmisc::iconForFileSuffix(suffix));

      QComboBox *comboBox = qobject_cast<QComboBox *>
	(ui.applicationsTable->cellWidget(list.at(0)->row(), 1));

      if(comboBox)
	{
	  if(comboBox->count() == 3)
	    {
	      if(action != "prompt")
		comboBox->insertItem(2, QFileInfo(action).fileName());
	    }
	  else
	    {
	      if(action != "prompt")
		comboBox->setItemText(2, QFileInfo(action).fileName());
	    }

	  if(action != "prompt")
	    {
	      comboBox->setItemData(2, action);
	      comboBox->setCurrentIndex(2);
	    }
	}
    }
}

void dsettings::slotCustomContextMenuRequested(const QPoint &point)
{
  if(sender() == ui.applicationsTable &&
     ui.applicationsTable->currentRow() != -1)
    {
      QMenu menu(this);

      menu.addAction(tr("&Delete File Suffix"),
		     this, SLOT(slotDeleteSuffix(void)));
      menu.exec(ui.applicationsTable->mapToGlobal(point));
    }
}

void dsettings::slotDeleteSuffix(void)
{
  int row = ui.applicationsTable->currentRow();

  if(row < 0)
    return;
  else if(!ui.applicationsTable->item(row, 0))
    return;

  QString suffix(ui.applicationsTable->item(row, 0)->text());

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "applications");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "applications.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("DELETE FROM applications WHERE file_suffix = ?");
	query.bindValue(0, suffix);
	
	if(query.exec())
	  {
	    dooble::s_applicationsActions.remove(suffix);
	    ui.applicationsTable->removeRow(row);
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("applications");
}

void dsettings::slotClearDiskCache(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dooble::s_networkCache->clear();
  QApplication::restoreOverrideCursor();
}

void dsettings::slotUpdateLabels(void)
{
  ui.bookmarksSizeLabel->setText
    (dmisc::formattedSize(dooble::s_bookmarksWindow->size()));

  qint64 diskCacheSize = 0;

  if(dooble::s_networkCache)
    diskCacheSize = dooble::s_networkCache->cacheSize();

  ui.diskCacheLabel->setText
    (tr("%1 MiB of content is cached.").
     arg(QString::number(static_cast<double> (diskCacheSize) / 1048576.0,
			 'f', 1)));
  ui.cacheSizeLabel->setText(dmisc::formattedSize(diskCacheSize));
  ui.cookiesSizeLabel->setText
    (dmisc::formattedSize(dooble::s_cookies->size()));
  ui.downloadsSizeLabel->setText
    (dmisc::formattedSize(dooble::s_downloadWindow->size()));

  qint64 exceptionsSize = 0;

  exceptionsSize += dooble::s_dntWindow->size() +
    dooble::s_popupsWindow->size() +
    dooble::s_adBlockWindow->size() + dooble::s_imageBlockWindow->size() +
    dooble::s_cookiesBlockWindow->size() +
    dooble::s_httpRedirectWindow->size() +
    dooble::s_cacheExceptionsWindow->size() +
    dooble::s_javaScriptExceptionsWindow->size() +
    dooble::s_alwaysHttpsExceptionsWindow->size() +
    dooble::s_sslExceptionsWindow->size();
  ui.exceptionsSizeLabel->setText(dmisc::formattedSize(exceptionsSize));
  ui.faviconsSizeLabel->setText(dmisc::formattedSize(dmisc::faviconsSize()));
  ui.historySizeLabel->setText
    (dmisc::formattedSize(dooble::s_historyWindow->size()));
}

void dsettings::slotClearFavicons(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dmisc::clearFavicons();
  QApplication::restoreOverrideCursor();
}

Ui_settingsWindow dsettings::UI(void) const
{
  return ui;
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool dsettings::event(QEvent *event)
{
  if(event)
    if(event->type() == QEvent::WindowStateChange)
      if(windowState() == Qt::WindowNoState)
	{
	  /*
	  ** Minimizing the window on OS 10.6.8 and Qt 5.x will cause
	  ** the window to become stale once it has resurfaced.
	  */

	  hide();
	  show();
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool dsettings::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool dsettings::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
