/*
** Copyright (c) 2008 - present, Alexis Megas, Bernd Stramm.
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

#include <iostream>
#include <string>

#include <QApplication>
#include <QAuthenticator>
#include <QBuffer>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPaintEngine>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextCodec>
#include <QTimer>
#include <QTranslator>
#include <QWebEngineHistoryItem>
#include <QWidgetAction>
#include <QtConcurrent>
#include <QtCore>

extern "C"
{
#include <fcntl.h>
#include <signal.h>
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
#include <unistd.h>
#endif
#if defined(Q_OS_MAC)
#include <sys/resource.h>
#endif
#if defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#endif
}

#ifdef Q_OS_MAC
#include "CocoaInitializer.h"
#endif
#include "architecture.h"
#include "dbookmarkspopup.h"
#include "dbookmarkswindow.h"
#include "dclearcontainers.h"
#include "derrorlog.h"
#include "dhistory.h"
#include "dmisc.h"
#include "dnetworkaccessmanager.h"
#include "dnetworkcache.h"
#include "dooble.h"
#include "dpagesourcewindow.h"
#include "dprintfromcommandprompt.h"
#include "dreinstatedooble.h"
#include "dwebpage.h"
#include "dwebview.h"
#include "ui_dpassphrasePrompt.h"
#include "ui_dpasswordPrompt.h"

QPointer<QMenu> dooble::s_bookmarksPopupMenu = 0;
QUuid dooble::s_id = QUuid::createUuid();
QMutex dooble::s_saveHistoryMutex;
QString dooble::s_homePath = "";
QPointer<dspoton> dooble::s_spoton = 0;
QPointer<dcookies> dooble::s_cookies = 0;
QPointer<dhistory> dooble::s_historyWindow = 0;
QPointer<derrorlog> dooble::s_errorLog = 0;
QPointer<dsettings> dooble::s_settingsWindow = 0;
QPointer<dcookiewindow> dooble::s_cookieWindow = 0;
QPointer<dhistorymodel> dooble::s_historyModel = 0;
QPointer<dnetworkcache> dooble::s_networkCache = 0;
QPointer<ddownloadwindow> dooble::s_downloadWindow = 0;
QPointer<dbookmarkspopup> dooble::s_bookmarksPopup = 0;
QPointer<dbookmarkswindow> dooble::s_bookmarksWindow = 0;
QPointer<dclearcontainers> dooble::s_clearContainersWindow = 0;
QPointer<dexceptionswindow> dooble::s_dntWindow = 0;
QPointer<dexceptionswindow> dooble::s_popupsWindow = 0;
QPointer<dexceptionswindow> dooble::s_adBlockWindow = 0;
QPointer<dexceptionswindow> dooble::s_httpOnlyExceptionsWindow = 0;
QPointer<dexceptionswindow> dooble::s_httpReferrerWindow = 0;
QPointer<dexceptionswindow> dooble::s_imageBlockWindow = 0;
QPointer<dexceptionswindow> dooble::s_cookiesBlockWindow = 0;
QPointer<dexceptionswindow> dooble::s_httpRedirectWindow = 0;
QPointer<dexceptionswindow> dooble::s_cacheExceptionsWindow = 0;
QPointer<dexceptionswindow> dooble::s_javaScriptExceptionsWindow = 0;
QPointer<dexceptionswindow> dooble::s_alwaysHttpsExceptionsWindow = 0;
QPointer<dexceptionswindow> dooble::s_sslExceptionsWindow = 0;
QPointer<dsslcipherswindow> dooble::s_sslCiphersWindow = 0;
QPointer<QStandardItemModel> dooble::s_bookmarksFolderModel = 0;
QHash<QString, qint64> dooble::s_mostVisitedHosts;
QMap<QString, QString> dooble::s_applicationsActions;
QHash<QString, QVariant> dooble::s_settings;
int dprintfromcommandprompt::s_count = 0;
qint64 dooble::s_instances = 0;

static char *s_crashFileName = 0;

static void sig_handler(int signum)
{
  /*
  ** _Exit() and _exit() may be safely called from signal handlers.
  */

  static int fatal_error = 0;

  if(fatal_error)
    _Exit(signum);

  fatal_error = 1;
  dcrypt::terminate(); // Safe.

  if(signum == SIGTERM)
    dmisc::removeRestorationFiles(dooble::s_id); // Not safe.
  else
    {
      /*
      ** We shall create a .crashed file in the user's Dooble directory.
      ** Upon restart of Dooble, Dooble will query the Histories directory and
      ** adjust itself accordingly if the .crashed file exists.
      */

      if(s_crashFileName)
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	close(open(s_crashFileName, O_CREAT, S_IRUSR | S_IWUSR)); // Safe.
#else
        close(creat(s_crashFileName, O_CREAT)); // Safe.
#endif
    }

  _Exit(signum);
}

static void qt_message_handler(QtMsgType type,
			       const QMessageLogContext &context,
			       const QString &msg)
{
  Q_UNUSED(type);
  Q_UNUSED(context);
  dmisc::logError(msg);
}

int main(int argc, char *argv[])
{
  qputenv("QT_ENABLE_REGEXP_JIT", "0");
  qputenv("QV4_FORCE_INTERPRETER", "1");

#ifdef Q_OS_MAC
  struct rlimit rlim = {0, 0};

  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = OPEN_MAX;
  setrlimit(RLIMIT_NOFILE, &rlim);
#endif
  /*
  ** See http://trac.webkit.org/wiki/Fingerprinting#vii.DateObject.
  */

  qputenv("TZ", ":UTC");

  /*
  ** Prepare signal handlers. Clearly, this does not account for
  ** the user's desire to restore sessions.
  */

  QList<int> l_signals;
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
  struct sigaction act;
#endif
  l_signals << SIGABRT
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	    << SIGBUS
#endif
	    << SIGFPE
	    << SIGILL
	    << SIGINT
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	    << SIGQUIT
#endif
	    << SIGSEGV
	    << SIGTERM;

  while(!l_signals.isEmpty())
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
      act.sa_handler = sig_handler;
      sigemptyset(&act.sa_mask);
      act.sa_flags = 0;
      sigaction(l_signals.takeFirst(), &act, (struct sigaction *) 0);
#else
      signal(l_signals.takeFirst(), sig_handler);
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
  /*
  ** Ignore SIGPIPE. Some plugins cause Dooble to die.
  */

  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGPIPE, &act, (struct sigaction *) 0);
#endif

  /*
  ** Prepare the style before creating a QApplication object, per Qt.
  */

#ifdef Q_OS_WIN32
  QApplication::addLibraryPath("plugins");
  QApplication::setStyle("fusion");
#endif

  /*
  ** Proceed to Qt initialization.
  */

  qInstallMessageHandler(qt_message_handler);

  QApplication qapp(argc, argv);

  if(argc > 1)
    {
      QList<QUrl> urls;
      QString usage("");
      int rc = 0;

      usage.append("Usage: Dooble --print-url URL\n\n");

      for(int i = 1; i < argc; i++)
	{
	  if(!argv || !argv[i])
	    continue;

	  QString option(argv[i]);

	  option = option.toLower().trimmed();

	  if(option == "--print-url")
	    {
	      i += 1;

	      if(i < argc && argv[i])
		{
		  if(QUrl::fromUserInput(argv[i]).isValid())
		    {
		      QUrl url(QUrl::fromUserInput(argv[i]));

		      urls.append(url.toString(QUrl::StripTrailingSlash));
		    }
		  else
		    fprintf(stderr, "Invalid URL: %s. Skipping.\n", argv[i]);
		}
	      else
		rc = 1;
	    }
	}

      if(rc > 0)
	{
	  fprintf(stdout, "%s\n", usage.toStdString().data());
	  return rc;
	}
      else if(!urls.isEmpty())
	{
	  dprintfromcommandprompt::s_count = urls.size();

	  for(int i = 0; i < urls.size(); i++)
	    Q_UNUSED(new dprintfromcommandprompt(urls.at(i), i + 1));

	  return qapp.exec();
	}
    }

  qRegisterMetaType<dnetworkblockreply *> ("dnetworkblockreply *");
  qRegisterMetaType<dnetworkerrorreply *> ("dnetworkerrorreply *");
  qRegisterMetaType<dnetworksslerrorreply *> ("dnetworksslerrorreply *");
#ifdef Q_OS_MAC
  /*
  ** Eliminate pool errors on OS X.
  */

  CocoaInitializer ci;
#endif

  /*
  ** Set the maximum threadpool count.
  */

  int idealThreadCount = QThread::idealThreadCount();

  if(idealThreadCount > 0)
    if(QThreadPool::globalInstance())
      QThreadPool::globalInstance()->setMaxThreadCount
	(qCeil(1.5 * idealThreadCount));

  /*
  ** Set application attributes.
  */

#ifdef Q_OS_WIN32
  QByteArray tmp(qgetenv("USERNAME").mid(0, 32));
  QDir homeDir(QDir::current());
  QFileInfo fileInfo(homeDir.absolutePath());
  QString username(tmp);

  if(!(fileInfo.isReadable() && fileInfo.isWritable()))
    homeDir = QDir::home();

  if(username.isEmpty())
    homeDir.mkdir(".dooble_v2");
  else
    homeDir.mkdir(username + QDir::separator() + ".dooble_v2");

  if(username.isEmpty())
    dooble::s_homePath = homeDir.absolutePath() +
      QDir::separator() + ".dooble_v2";
  else
    dooble::s_homePath = homeDir.absolutePath() + QDir::separator() +
      username + QDir::separator() + ".dooble_v2";
#else
  QDir homeDir(QDir::home());

  homeDir.mkdir(".dooble_v2");
  dooble::s_homePath = homeDir.absolutePath() +
    QDir::separator() + ".dooble_v2";
#endif
  QCoreApplication::setApplicationName("Dooble");
  QCoreApplication::setOrganizationName("Dooble");
  QCoreApplication::setOrganizationDomain("dooble.sf.net");
  QCoreApplication::setApplicationVersion(DOOBLE_VERSION_STR);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
		     dooble::s_homePath);
  QSettings::setDefaultFormat(QSettings::IniFormat);

  /*
  ** The Error Information window is quite important. It must be
  ** created ight after the QApplication object.
  */

  dooble::s_errorLog = new derrorlog();
  dmisc::initializeBlockedHosts();

  /*
  ** The initializeCrypt() method must be called as soon as possible.
  */

  dmisc::initializeCrypt();

  /*
  ** We may need to clear the cache during important updates.
  */

  dooble::s_networkCache = new dnetworkcache();

  /*
  ** Process command-line arguments.
  */

  QStringList urls;
  QHash<QString, QVariant> argumentsHash;

  if(argc > 1)
    {
      int rc = 0;
      QString usage("");

      usage.append("Usage: Dooble [OPTION] --load-url URL\n\n");
      usage.append("Options:\n");

      for(int i = 1; i < argc; i++)
	{
	  if(!argv || !argv[i])
	    continue;

	  QString option(argv[i]);

	  option = option.toLower().trimmed();

	  if(option == "--load-url")
	    {
	      i += 1;

	      if(i < argc && argv[i])
		{
		  if(QUrl::fromUserInput(argv[i]).isValid())
		    {
		      QUrl url(QUrl::fromUserInput(argv[i]));

		      urls.append(url.toString(QUrl::StripTrailingSlash));
		    }
		  else
		    fprintf(stderr, "Invalid URL: %s. Skipping.\n", argv[i]);
		}
	      else
		rc = 1;
	    }
	  else
	    fprintf(stderr, "Unknown option %s. Ignoring.\n", argv[i]);
	}

      if(rc > 0)
	{
	  fprintf(stdout, "%s\n", usage.toStdString().data());
	  return rc;
	}
    }

  /*
  ** Configure translations.
  */

  QByteArray env(qgetenv("DOOBLE_SYSTEM_NAME").mid(0, 6));
  QTranslator qtTranslator;

  if(env.isEmpty())
    env = QLocale::system().name().toLatin1();

  qtTranslator.load("qt_" + env, "Translations");
  qapp.installTranslator(&qtTranslator);

  QTranslator myappTranslator;

  myappTranslator.load("dooble_" + env, "Translations");
  qapp.installTranslator(&myappTranslator);

  /*
  ** Remove old configuration settings.
  */

  QSettings settings;
  int priority = dooble::s_settings.value("settingsWindow/displaypriority",
					  3).toInt();

  if(priority < 0 || priority > 7)
    priority = 3;

  QThread *thread = qapp.thread();

  if(!thread)
    thread = QThread::currentThread();

  if(thread)
    thread->setPriority(QThread::Priority(priority));

  settings.remove("mainWindow/showLocationToolBar");
  settings.remove("mainWindow/zoomTextOnly");
  settings.remove("settingsWindow/javaEnabled");
  settings.remove("settingsWindow/purgeMemoryCaches");
  settings.remove("vidalia/hostName");
  settings.remove("vidalia/isConnected");
  settings.remove("vidalia/port");
  settings.remove("vidalia/userName");
  settings.remove("vidalia/userPwd");

  /*
  ** We need to set these before creating some of the support windows.
  */

  if(!settings.contains("iconSet"))
    settings.setValue("iconSet",
		      QString("%1/%2").arg(QDir::currentPath()).
		      arg("Icons/nuovext/configuration.cfg"));

  if(!QFileInfo(settings.value("iconSet").toString()).exists())
    settings.setValue("iconSet",
		      QString("%1/%2").arg(QDir::currentPath()).
		      arg("Icons/nuovext/configuration.cfg"));

  if(!settings.contains("settingsWindow/iconSet1"))
    settings.setValue("settingsWindow/iconSet1", settings.value("iconSet"));

  if(!QFileInfo(settings.value("settingsWindow/iconSet1").toString()).exists())
    settings.setValue("settingsWindow/iconSet1",
		      QString("%1/%2").arg(QDir::currentPath()).
		      arg("Icons/nuovext/configuration.cfg"));

  dooble::s_settings.clear();

  for(int i = 0; i < settings.allKeys().size(); i++)
    dooble::s_settings[settings.allKeys().at(i)] =
      settings.value(settings.allKeys().at(i));

  dmisc::prepareProxyIgnoreLists();

  /*
  ** QWebSettings changes have to be performed after dooble::s_settings
  ** gets populated!
  */

  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::JavascriptEnabled,
     dooble::s_settings.value("settingsWindow/javascriptEnabled",
			      false).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows,
     dooble::s_settings.value("settingsWindow/javascriptEnabled",
			      false).toBool() &&
     dooble::s_settings.value("settingsWindow/javascriptAllowNewWindows",
			      true).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::AutoLoadImages,
     dooble::s_settings.value("settingsWindow/automaticallyLoadImages",
			      true).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::SpatialNavigationEnabled,
     dooble::s_settings.value("settingsWindow/spatialNavigation",
			      false).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::XSSAuditingEnabled,
     dooble::s_settings.value("settingsWindow/javascriptEnabled",
			      false).toBool() &&
     dooble::s_settings.value("settingsWindow/xssAuditingEnabled",
			      true).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled, true);
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::HyperlinkAuditingEnabled,
     dooble::s_settings.
     value("settingsWindow/"
	   "hyperlinkAuditing", false).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::LocalContentCanAccessFileUrls,
     dooble::s_settings.
     value("settingsWindow/localContentMayAccessLocalContent",
	   true).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::FullScreenSupportEnabled,
     dooble::s_settings.
     value("settingsWindow/fullScreenSupport", true).toBool());
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::ScrollAnimatorEnabled,
     dooble::s_settings.
     value("settingsWindow/scrollingAnimation", true).toBool());

  QString str(dooble::s_settings.value("settingsWindow/characterEncoding",
				       "").toString().toLower());
  QTextCodec *codec = 0;

  if((codec = QTextCodec::codecForName(str.toUtf8().constData())))
    QWebEngineSettings::globalSettings()->
      setDefaultTextEncoding(codec->name());

  QFont font;

  if(!font.fromString
     (dooble::s_settings.value("settingsWindow/standardWebFont", "").
      toString()) || font.family().isEmpty())
#ifdef Q_OS_MAC
    font = QFont("Times", 16);
#elif defined(Q_OS_WIN32)
    font = QFont("Serif", 10);
#else
    font = QFont("Serif", 16);
#endif

  if(font.pointSize() <= 0)
#if defined(Q_OS_WIN32)
    font.setPointSize(10);
#else
    font.setPointSize(16);
#endif

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::StandardFont, font.family());
  QWebEngineSettings::globalSettings()->setFontSize
    (QWebEngineSettings::DefaultFontSize, font.pointSize());

  if(!font.fromString(dooble::s_settings.value
		      ("settingsWindow/cursiveWebFont", "").
		      toString()) || font.family().isEmpty())
    font = QFont("Serif");

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::CursiveFont,
     font.family());

  if(!font.fromString(dooble::s_settings.value
		      ("settingsWindow/fantasyWebFont", "").
		      toString()) || font.family().isEmpty())
    font = QFont("Serif");

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::FantasyFont,
     font.family());

  if(!font.fromString(dooble::s_settings.value
		      ("settingsWindow/fixedWebFont", "").
		      toString()) || font.family().isEmpty())
#ifdef Q_OS_MAC
    font = QFont("Courier", 13);
#elif defined(Q_OS_WIN32)
    font = QFont("Courier New", 10);
#else
    font = QFont("Courier", 10);
#endif

  if(font.pointSize() <= 0)
#ifdef Q_OS_MAC
    font.setPointSize(13);
#elif defined(Q_OS_WIN32)
    font.setPointSize(10);
#else
    font.setPointSize(10);
#endif

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::FixedFont,
     font.family());
  QWebEngineSettings::globalSettings()->setFontSize
    (QWebEngineSettings::DefaultFixedFontSize,
     font.pointSize());

  if(!font.fromString(dooble::s_settings.value
		      ("settingsWindow/sansSerifWebFont", "").
		      toString()) || font.family().isEmpty())
#ifdef Q_OS_MAC
    font = QFont("Helvetica");
#elif defined(Q_OS_WIN32)
    font = QFont("Arial");
#else
    font = QFont("Sans Serif");
#endif

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::SansSerifFont,
     font.family());

  if(!font.fromString(dooble::s_settings.value
		      ("settingsWindow/serifWebFont", "").
		      toString()) || font.family().isEmpty())
#ifdef Q_OS_MAC
    font = QFont("Times");
#elif defined(Q_OS_WIN32)
    font = QFont("Times New Roman");
#else
    font = QFont("Serif");
#endif

  font.setWeight(QFont::Normal);
  QWebEngineSettings::globalSettings()->setFontFamily
    (QWebEngineSettings::SerifFont,
     font.family());

  int fontSize = dooble::s_settings.value("settingsWindow/minimumWebFontSize",
					  12).toInt();

  if(fontSize <= 0 || fontSize > 25)
    fontSize = 12;

  QWebEngineSettings::globalSettings()->setFontSize
    (QWebEngineSettings::MinimumFontSize, fontSize);

  /*
  ** Initialize static members.
  */

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  dooble::s_spoton = new dspoton();
#endif
  dooble::s_historyModel = new dhistorymodel();
  dooble::s_bookmarksFolderModel = new QStandardItemModel();
  dooble::s_dntWindow = new dexceptionswindow
    (new dexceptionsmodel("dntexceptions"));
  dooble::s_dntWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "DNT (Do Not Track) Exceptions"));
  dooble::s_popupsWindow = new dexceptionswindow
    (new dexceptionsmodel("popupsexceptions"));
  dooble::s_popupsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "JavaScript Pop-ups Exceptions"));
  dooble::s_adBlockWindow = new dexceptionswindow
    (new dexceptionsmodel("adblockexceptions"));
  dooble::s_adBlockWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Third-Party Blocking Exceptions"));
  dooble::s_cookiesBlockWindow = new dexceptionswindow
    (new dexceptionsmodel("cookiesexceptions"));
  dooble::s_cookiesBlockWindow->enableApproachRadioButtons(true);
  dooble::s_cookiesBlockWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Cookies Exceptions"));
  dooble::s_httpOnlyExceptionsWindow = new dexceptionswindow
    (new dexceptionsmodel("httponlyexceptions"));
  dooble::s_httpOnlyExceptionsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "HTTP-Only Exceptions"));
  dooble::s_httpReferrerWindow = new dexceptionswindow
    (new dexceptionsmodel("suppresshttpreferrerexceptions"));
  dooble::s_httpReferrerWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Suppress HTTP Referrer Exceptions"));
  dooble::s_httpRedirectWindow = new dexceptionswindow
    (new dexceptionsmodel("suppresshttpredirectexceptions"));
  dooble::s_httpRedirectWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Suppress HTTP Redirect Exceptions"));
  dooble::s_javaScriptExceptionsWindow = new dexceptionswindow
    (new dexceptionsmodel("javascriptexceptions"));
  dooble::s_javaScriptExceptionsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "JavaScript Exceptions"));
  dooble::s_imageBlockWindow = new dexceptionswindow
    (new dexceptionsmodel("autoloadedimagesexceptions"));
  dooble::s_imageBlockWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Automatically-Loaded Images Exceptions"));
  dooble::s_cacheExceptionsWindow = new dexceptionswindow
    (new dexceptionsmodel("cacheexceptions"));
  dooble::s_cacheExceptionsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Cache Exceptions"));
  dooble::s_alwaysHttpsExceptionsWindow = new dexceptionswindow
    (new dexceptionsmodel("alwayshttpsexceptions"));
  dooble::s_alwaysHttpsExceptionsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "Always HTTPS Exceptions"));
  dooble::s_sslExceptionsWindow = new dexceptionswindow
    (new dexceptionsmodel("sslexceptions"));
  dooble::s_sslExceptionsWindow->setWindowTitle
    (QObject::tr("Dooble Web Browser: "
		 "SSL Errors Exceptions"));
  dooble::s_sslCiphersWindow = new dsslcipherswindow();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "applications");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "applications.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT file_suffix, action FROM applications"))
	  while(query.next())
	    {
	      QString action("");
	      QString suffix(query.value(0).toString());

	      if(query.isNull(1))
		action = "prompt";
	      else
		{
		  action = query.value(1).toString();

		  QFileInfo fileInfo(action);

		  if(!fileInfo.isExecutable() || !fileInfo.isReadable())
		    {
		      action = "prompt";

		      /*
		      ** Correct damaged entries.
		      */

		      QSqlQuery updateQuery(db);

		      updateQuery.prepare("UPDATE applications "
					  "SET action = NULL "
					  "WHERE file_suffix = ?");
		      updateQuery.bindValue(0, query.value(0));
		      updateQuery.exec();
		    }
		}

	      dooble::s_applicationsActions[suffix] = action;
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("applications");
  dooble::s_historyWindow = new dhistory();
  dooble::s_downloadWindow = new ddownloadwindow();
  dooble::s_settingsWindow = new dsettings();
  dooble::s_settingsWindow->slotPopulateApplications
    (dooble::s_applicationsActions);

  if(dfilemanager::tableModel)
    {
      QObject::connect
	(dfilemanager::tableModel,
	 SIGNAL(suffixesAdded(const QMap<QString, QString> &)),
	 dooble::s_settingsWindow,
	 SLOT(slotPopulateApplications(const QMap<QString, QString> &)));
      QObject::connect(dfilemanager::tableModel,
		       SIGNAL(suffixUpdated(const QString &, const QString &)),
		       dooble::s_settingsWindow,
		       SLOT(slotUpdateApplication(const QString &,
						  const QString &)));
    }

  dooble::s_bookmarksPopup = new dbookmarkspopup(); /*
						    ** The object
						    ** dooble::
						    ** s_bookmarksPopup
						    ** must be created
						    ** before dooble::
						    ** s_bookmarksWindow.
						    */
  dooble::s_bookmarksPopupMenu = new QMenu(0);

  QWidgetAction *action = new QWidgetAction(0);

  action->setDefaultWidget(dooble::s_bookmarksPopup);
  dooble::s_bookmarksPopupMenu->addAction(action);
  QObject::connect(dooble::s_bookmarksPopup,
		   SIGNAL(closed(void)),
		   dooble::s_bookmarksPopupMenu,
		   SLOT(close(void)));
  dooble::s_bookmarksWindow = new dbookmarkswindow();
  dooble::s_cookies = new dcookies(false); /*
					   ** The object dooble::s_cookies
					   ** must be created after
					   ** dooble::s_cookiesBlockWindow.
					   */
  dooble::s_cookieWindow = new dcookiewindow(dooble::s_cookies);
  dooble::s_clearContainersWindow = new dclearcontainers();
  QObject::connect(dooble::s_clearContainersWindow,
		   SIGNAL(clearCookies(void)),
		   dooble::s_cookies,
		   SLOT(slotClear(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(changed(void)),
		   dooble::s_cookieWindow,
		   SLOT(slotCookiesChanged(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(domainsRemoved(const QStringList &)),
		   dooble::s_cookieWindow,
		   SLOT(slotDomainsRemoved(const QStringList &)));
  QObject::connect(dooble::s_settingsWindow,
		   SIGNAL(cookieTimerChanged(void)),
		   dooble::s_cookies,
		   SLOT(slotCookieTimerChanged(void)));
  QObject::connect(dooble::s_historyWindow,
		   SIGNAL(bookmark(const QUrl &,
				   const QIcon &,
				   const QString &,
				   const QString &,
				   const QDateTime &,
				   const QDateTime &)),
		   dooble::s_bookmarksWindow,
		   SLOT(slotAddBookmark(const QUrl &,
					const QIcon &,
					const QString &,
					const QString &,
					const QDateTime &,
					const QDateTime &)));
  QObject::connect(dooble::s_bookmarksPopup,
		   SIGNAL(changed(void)),
		   dooble::s_bookmarksWindow,
		   SLOT(slotRefresh(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(httpCookieReceived(const QString &,
					     const QUrl &,
					     const QDateTime &)),
		   dooble::s_httpOnlyExceptionsWindow,
		   SLOT(slotAdd(const QString &,
				const QUrl &,
				const QDateTime &)));

  QUrl url;
  QString urlText(dooble::s_settings.value("settingsWindow/homeUrl", "").
		  toString());

  url = QUrl::fromUserInput(urlText);

  if(argc > 1)
    {
      urls.append(url.toString(QUrl::StripTrailingSlash));
      argumentsHash["urls"] = urls;
      Q_UNUSED(new dooble(argumentsHash, 0));
    }
  else
    {
      QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

      webAttributes[QWebEngineSettings::JavascriptEnabled] =
	dooble::s_settings.value("settingsWindow/javascriptEnabled",
				 false).toBool();
      Q_UNUSED(new dooble(url.toString(QUrl::StripTrailingSlash), 0, 0,
			  webAttributes));
    }

  /*
  ** If the last Dooble window is closed, Dooble will exit.
  ** So, it's not necessary to connect QApplication's
  ** lastWindowClosed() signal.
  */

  int rc = qapp.exec();

  dmisc::purgeTemporaryData();
  dmisc::destroyCrypt();
  return rc;
}

void dooble::init_dooble(const bool isJavaScriptWindow)
{
  /*
  ** This method is used whenever a new Dooble window
  ** is about to be created.
  */

  dmisc::createPreferencesDatabase();
  setUrlHandler(this);
  m_isJavaScriptWindow = isJavaScriptWindow;
  showFindFrame = false;
  s_instances += 1;
  m_id = QDateTime::currentMSecsSinceEpoch() + s_instances;
  ui.setupUi(this);
  ui.historyFrame->setLayout(new QHBoxLayout(ui.historyFrame));
  ui.historyFrame->layout()->setContentsMargins(1, 0, 1, 0);
#ifdef DOOBLE_URLFRAME_LAYOUT_SPACING
  ui.urlFrame->layout()->setSpacing(DOOBLE_URLFRAME_LAYOUT_SPACING);
#endif
  m_historySideBar = new dhistorysidebar(this);
  ui.historyFrame->layout()->addWidget(m_historySideBar);
  ui.actionShow_HistorySideBar->setChecked
    (s_settings.value("mainWindow/showHistorySideBar", false).
     toBool());
  ui.actionSearch_Widget->setChecked
    (s_settings.value("mainWindow/showSearchWidget", true).toBool());
  ui.actionOffline->setChecked
    (s_settings.value("mainWindow/offlineMode", false).toBool());
  ui.historyFrame->setVisible(false);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);

  /*
  ** Fixes shuffling.
  */

  statusBar()->setSizeGripEnabled(false);
#endif
  initializeBookmarksMenu();
  ui.action_Desktop_Button->setChecked
    (s_settings.value("mainWindow/showDesktopButton", true).toBool());
  ui.action_Home_Button->setChecked
    (s_settings.value("mainWindow/showHomeButton", true).toBool());
  ui.desktopToolButton->setVisible
    (ui.action_Desktop_Button->isChecked());
  ui.homeToolButton->setVisible
    (ui.action_Home_Button->isChecked());
  ui.urlFrame->setParent(0);
  ui.locationLineEdit->setPlaceholderText(tr("Address"));
  ui.locationToolBar->setVisible(false);
  ui.locationToolBar->addWidget(ui.urlFrame);
  ui.locationToolBar->setVisible(true);
  ui.favoritesFrame->setParent(0);
  ui.favoritesToolBar->setVisible(false);
  ui.favoritesToolBar->addWidget(ui.favoritesFrame);
  ui.favoritesToolBar->setVisible(true);
  sbWidget = new QWidget(this);
  m_desktopWidget = 0;
  sb.setupUi(sbWidget);
  sb.progressBar->setValue(0);
  sb.progressBar->setVisible(false);
  sb.statusLabel->setVisible(true);
  sb.exceptionsToolButton->setVisible(false);
  sb.errorLogToolButton->setVisible(false);
#ifdef Q_OS_MAC
  sbWidget->layout()->setContentsMargins(0, 5, 15, 5);
#endif
  ui.findFrame->setVisible(false);
  ui.findLineEdit->setPlaceholderText(tr("Search Page"));
  ui.backToolButton->setMenu(new QMenu(this));
#ifdef Q_OS_MAC
  ui.action_Hide_Menubar->setEnabled(false);
  ui.menuToolButton->setVisible(false);
#else
  ui.menuToolButton->setMenu(new QMenu(this));
#endif
  ui.searchLineEdit->setVisible(ui.actionSearch_Widget->isChecked());
  ui.action_Web_Inspector->setEnabled(false);
  ui.action_Web_Inspector->setToolTip(tr("WebEngine does not yet "
					 "support Web inspectors."));
  ui.actionShow_Hidden_Files->setEnabled(false);
  ui.actionShow_Hidden_Files->setToolTip(tr("WebEngine supports the browsing "
					    "of directories."));

  if(dooble::s_spoton)
    ui.action_Clear_Spot_On_Shared_Links->setEnabled
      (dooble::s_spoton->isKernelRegistered());
  else
    ui.action_Clear_Spot_On_Shared_Links->setEnabled(false);

  connect(ui.action_Clear_Spot_On_Shared_Links,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotClearSpotOnSharedLinks(void)));
  connect(ui.action_Hide_Menubar,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotHideMainMenus(void)));
  connect(ui.actionNew_Private_Tab, SIGNAL(triggered(void)), this,
	  SLOT(slotNewPrivateTab(void)));
  connect(ui.tabWidget, SIGNAL(createPrivateTab(void)), this,
	  SLOT(slotNewPrivateTab(void)));
  connect(ui.action_Web_Inspector,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotShowWebInspector(void)));
  connect(ui.actionOffline,
	  SIGNAL(triggered(bool)),
	  this,
	  SLOT(slotOffline(bool)));
  connect(ui.backToolButton->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slotAboutToShowBackForwardMenu(void)));
  connect(ui.bookmarksMenu,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slotAboutToShowBookmarksMenu(void)));
  ui.forwardToolButton->setMenu(new QMenu(this));
  connect(ui.forwardToolButton->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slotAboutToShowBackForwardMenu(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_historyWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_downloadWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_cookieWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_bookmarksPopup,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_bookmarksWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_errorLog,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_sslCiphersWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(passphraseWasAuthenticated(const bool)),
	  s_settingsWindow,
	  SLOT(slotPassphraseWasAuthenticated(const bool)));
  connect(s_settingsWindow,
	  SIGNAL(iconsChanged(void)),
	  this,
	  SLOT(slotSetIcons(void)));
  connect(s_errorLog,
	  SIGNAL(iconsChanged(void)),
	  this,
	  SLOT(slotSetIcons(void)));
  connect(s_sslCiphersWindow,
	  SIGNAL(iconsChanged(void)),
	  this,
	  SLOT(slotSetIcons(void)));  
  connect(s_settingsWindow,
	  SIGNAL(settingsChanged(void)),
	  ui.tabWidget,
	  SLOT(slotSetPosition(void)));
  connect(s_settingsWindow,
	  SIGNAL(settingsChanged(void)),
	  this,
	  SLOT(slotSettingsChanged(void)));
  connect(s_settingsWindow,
	  SIGNAL(showTabBar(const bool)),
	  this,
	  SLOT(slotSetTabBarVisible(const bool)));
  connect(s_settingsWindow,
	  SIGNAL(settingsReset(void)),
	  this,
	  SLOT(slotQuitAndRestart(void)));
  connect(s_settingsWindow,
	  SIGNAL(showIpAddress(const bool)),
	  this,
	  SLOT(slotShowIpAddress(const bool)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.searchLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.locationLineEdit,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  ui.tabWidget,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  m_historySideBar,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_dntWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_popupsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_adBlockWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_cookieWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_httpOnlyExceptionsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_httpRedirectWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_httpReferrerWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_javaScriptExceptionsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_imageBlockWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_cacheExceptionsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_alwaysHttpsExceptionsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_sslExceptionsWindow,
	  SLOT(slotSetIcons(void)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  s_clearContainersWindow,
	  SLOT(slotSetIcons(void)));

  /*
  ** Invoke the method when control has returned to the main
  ** thread.
  */

  connect(this,
	  SIGNAL(bookmarkUpdated(void)),
	  s_bookmarksWindow,
	  SLOT(slotRefresh(void)),
	  Qt::QueuedConnection);
  connect(ui.splitter,
	  SIGNAL(splitterMoved(int, int)),
	  this,
	  SLOT(slotLocationSplitterMoved(int, int)));
  connect(ui.historyAndTabSplitter,
	  SIGNAL(splitterMoved(int, int)),
	  this,
	  SLOT(slotHistoryTabSplitterMoved(int, int)));
  connect(ui.locationLineEdit,
	  SIGNAL(loadPage(const QUrl &)),
	  this,
	  SLOT(slotLoadPage(const QUrl &)));
  connect(s_downloadWindow, SIGNAL(saveUrl(const QUrl &, const int)), this,
	  SLOT(slotSaveUrl(const QUrl &, const int)));
  connect(m_historySideBar, SIGNAL(open(const QUrl &)),
	  this, SLOT(slotLoadPage(const QUrl &)));
  connect(m_historySideBar, SIGNAL(createTab(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(m_historySideBar, SIGNAL(openInNewWindow(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
  connect(m_historySideBar, SIGNAL(closed(void)),
	  this, SLOT(slotHistorySideBarClosed(void)));
  connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
	  SLOT(slotTabSelected(int)));
  connect(ui.actionNew_Tab, SIGNAL(triggered(void)), this,
	  SLOT(slotNewTab(void)));
  connect(ui.tabWidget, SIGNAL(createTab(void)), this,
	  SLOT(slotNewTab(void)));
  connect(ui.tabWidget, SIGNAL(openInNewWindow(const int)), this,
	  SLOT(slotOpenPageInNewWindow(const int)));
  connect(ui.actionNew_Window, SIGNAL(triggered(void)), this,
	  SLOT(slotNewWindow(void)));
  connect(ui.actionOpen_URL, SIGNAL(triggered(void)), this,
	  SLOT(slotOpenUrl(void)));
  connect(ui.actionClose_Tab, SIGNAL(triggered(void)), this,
	  SLOT(slotCloseTab(void)));
  connect(ui.tabWidget, SIGNAL(closeTab(const int)), this,
	  SLOT(slotCloseTab(const int)));
  connect(ui.tabWidget, SIGNAL(tabMoved(int, int)), this,
	  SLOT(slotTabMoved(int, int)));
  connect(ui.actionClose_Window, SIGNAL(triggered(void)), this,
	  SLOT(slotClose(void)));
  connect(ui.actionQuit, SIGNAL(triggered(void)), this,
	  SLOT(slotQuit(void)));
  connect(ui.locationLineEdit, SIGNAL(textEdited(const QString &)),
	  this, SLOT(slotTextChanged(const QString &)));
  connect(ui.locationLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotLoadPage(void)));
  connect(ui.locationLineEdit, SIGNAL(selectionChanged(void)),
	  this, SLOT(slotSelectionChanged(void)));
  connect(ui.locationLineEdit, SIGNAL(openLinkInNewTab(const QUrl &)), this,
	  SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(ui.locationLineEdit, SIGNAL(resetUrl(void)), this,
	  SLOT(slotResetUrl(void)));
  connect(ui.locationLineEdit, SIGNAL(bookmark(void)),
	  this, SLOT(slotBookmark(void)));
  connect(ui.locationLineEdit, SIGNAL(iconToolButtonClicked(void)),
	  this, SLOT(slotIconToolButtonClicked(void)));
  connect(ui.locationLineEdit, SIGNAL(submitUrlToSpotOn(void)),
	  this, SLOT(slotSubmitUrlToSpotOn(void)));
  connect(ui.backToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotBack(void)));
  connect(ui.forwardToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotForward(void)));
  connect(ui.reloadToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotReload(void)));
  connect(ui.actionReload, SIGNAL(triggered(void)), this,
	  SLOT(slotReload(void)));
  connect(ui.stopToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotStop(void)));
  connect(ui.actionStop, SIGNAL(triggered(void)), this,
	  SLOT(slotStop(void)));
  connect(ui.homeToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotGoHome(void)));
  connect(ui.action_Desktop_Button, SIGNAL(toggled(bool)), this,
	  SLOT(slotShowLocationBarButton(bool)));
  connect(ui.action_Home_Button, SIGNAL(toggled(bool)), this,
	  SLOT(slotShowLocationBarButton(bool)));
  connect(ui.searchLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotSearch(void)));
  connect(ui.searchLineEdit->findButton(), SIGNAL(clicked(void)), this,
	  SLOT(slotSearch(void)));
  connect(ui.searchLineEdit, SIGNAL(selectionChanged(void)),
	  this, SLOT(slotSelectionChanged(void)));
  connect(ui.actionAbout_Dooble, SIGNAL(triggered(void)), this,
	  SLOT(slotAbout(void)));
  connect(ui.actionSave_Page, SIGNAL(triggered(void)), this,
	  SLOT(slotSavePage(void)));
  connect(ui.actionSave_Page_as_Data_URI, SIGNAL(triggered(void)), this,
	  SLOT(slotSavePage(void)));
  connect(ui.actionDownloads, SIGNAL(triggered(void)), this,
	  SLOT(slotDisplayDownloadWindow(void)));
  connect(ui.actionFind, SIGNAL(triggered(void)), this,
	  SLOT(slotShowFind(void)));
  connect(ui.actionPrint, SIGNAL(triggered(void)), this,
	  SLOT(slotPrint(void)));
  connect(ui.actionPrint_Preview, SIGNAL(triggered(void)), this,
	  SLOT(slotPrintPreview(void)));
  connect(ui.actionPage_Source, SIGNAL(triggered(void)), this,
	  SLOT(slotShowPageSource(void)));
  connect(ui.actionSettings, SIGNAL(triggered(void)), this,
	  SLOT(slotShowSettingsWindow(void)));
  connect(ui.hideFindToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotHideFind(void)));
  connect(ui.findLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotFind(void)));
  connect(ui.findLineEdit, SIGNAL(textEdited(const QString &)), this,
	  SLOT(slotFind(void)));
  connect(ui.findLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotFindNext(void)));
  connect(ui.findLineEdit, SIGNAL(textEdited(const QString &)), this,
	  SLOT(slotFindNext(void)));
  connect(ui.highlightAllCheckBox, SIGNAL(clicked(bool)), this,
	  SLOT(slotFind(void)));
  connect(ui.nextToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotFindNext(void)));
  connect(ui.previousToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotFindPrevious(void)));
  connect(ui.desktopToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotShowDesktopTab(void)));
  connect(ui.actionStatusbar, SIGNAL(triggered(bool)), this,
	  SLOT(slotStatusBarDisplay(bool)));
  connect(ui.actionShow_Hidden_Files, SIGNAL(triggered(bool)), this,
	  SLOT(slotShowHiddenFiles(bool)));
  connect(ui.actionSearch_Widget, SIGNAL(triggered(bool)), this,
	  SLOT(slotShowSearchWidget(bool)));
  connect(ui.actionOpen_Directory, SIGNAL(triggered(void)), this,
	  SLOT(slotOpenDirectory(void)));
  connect(ui.actionZoom_In, SIGNAL(triggered(void)), this,
	  SLOT(slotViewZoomIn(void)));
  connect(ui.actionZoom_Out, SIGNAL(triggered(void)), this,
	  SLOT(slotViewZoomOut(void)));
  connect(ui.actionReset_Zoom, SIGNAL(triggered(void)), this,
	  SLOT(slotViewResetZoom(void)));
  connect(ui.actionMy_Retrieved_Files, SIGNAL(triggered(void)), this,
	  SLOT(slotOpenMyRetrievedFiles(void)));
  connect(ui.actionP2P_Email, SIGNAL(triggered(void)), this,
	  SLOT(slotOpenP2PEmail(void)));
  connect(ui.action_IRC_Channel, SIGNAL(triggered(void)), this,
	  SLOT(slotOpenIrcChannel(void)));
  connect(ui.actionCopy, SIGNAL(triggered(void)), this,
	  SLOT(slotCopy(void)));
  connect(ui.actionPaste, SIGNAL(triggered(void)), this,
	  SLOT(slotPaste(void)));
  connect(ui.actionSelect_All_Content, SIGNAL(triggered(void)), this,
	  SLOT(slotSelectAllContent(void)));
  connect(ui.actionApplication_Cookies, SIGNAL(triggered(void)), this,
	  SLOT(slotShowApplicationCookies(void)));
  connect(ui.actionShow_FavoritesToolBar, SIGNAL(toggled(bool)),
	  this, SLOT(slotShowFavoritesToolBar(bool)));
  connect(ui.actionShow_HistorySideBar, SIGNAL(toggled(bool)),
	  this, SLOT(slotShowHistorySideBar(bool)));
  connect(ui.action_Authenticate, SIGNAL(triggered(void)),
	  this, SLOT(slotAuthenticate(void)));
  connect(ui.menuToolButton, SIGNAL(clicked(void)),
	  ui.menuToolButton, SLOT(showMenu(void)));
  connect(ui.tabWidget, SIGNAL(urlsReceivedViaDrop(const QList<QUrl> &)),
	  this, SLOT(slotOpenUrlsFromDrop(const QList<QUrl> &)));
  connect(ui.tabWidget, SIGNAL(openLinkInNewTab(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(ui.tabWidget, SIGNAL(bookmark(const int)),
	  this, SLOT(slotBookmark(const int)));
  connect(ui.tabWidget, SIGNAL(reloadTab(const int)),
	  this, SLOT(slotReloadTab(const int)));
  connect(ui.tabWidget, SIGNAL(stopTab(const int)),
	  this, SLOT(slotStopTab(const int)));
  connect(ui.action_Cookies, SIGNAL(triggered(void)),
	  s_cookiesBlockWindow, SLOT(slotShow(void)));
  connect(ui.action_HTTP_Cookies, SIGNAL(triggered(void)),
	  s_httpOnlyExceptionsWindow, SLOT(slotShow(void)));
  connect(ui.action_HTTP_Redirect, SIGNAL(triggered(void)),
	  s_httpRedirectWindow, SLOT(slotShow(void)));
  connect(ui.action_HTTP_Referrer, SIGNAL(triggered(void)),
	  s_httpReferrerWindow, SLOT(slotShow(void)));
  connect(ui.action_JavaScript, SIGNAL(triggered(void)),
	  s_javaScriptExceptionsWindow, SLOT(slotShow(void)));
  connect(ui.action_JavaScript_Popups, SIGNAL(triggered(void)),
	  s_popupsWindow, SLOT(slotShow(void)));
  connect(ui.action_Third_Party_Frame_Content, SIGNAL(triggered(void)),
	  s_adBlockWindow, SLOT(slotShow(void)));
  connect(ui.action_DNT, SIGNAL(triggered(void)),
	  s_dntWindow, SLOT(slotShow(void)));
  connect(ui.action_Automatically_Loaded_Images, SIGNAL(triggered(void)),
	  s_imageBlockWindow, SLOT(slotShow(void)));
  connect(ui.action_Cache, SIGNAL(triggered(void)),
	  s_cacheExceptionsWindow, SLOT(slotShow(void)));
  connect(ui.action_AlwaysHttps, SIGNAL(triggered(void)),
	  s_alwaysHttpsExceptionsWindow, SLOT(slotShow(void)));
  connect(ui.actionSSLErrors, SIGNAL(triggered(void)),
	  s_sslExceptionsWindow, SLOT(slotShow(void)));
  connect(ui.actionError_Log, SIGNAL(triggered(void)),
	  s_errorLog, SLOT(slotShow(void)));
  connect(ui.action_SSL_Ciphers, SIGNAL(triggered(void)),
	  s_sslCiphersWindow, SLOT(slotShow(void)));
  connect(ui.actionError_Log, SIGNAL(triggered(void)),
	  sb.errorLogToolButton, SLOT(hide(void)));
  connect(ui.favoritesToolBar,
	  SIGNAL(visibilityChanged(bool)),
	  ui.actionShow_FavoritesToolBar,
	  SLOT(setChecked(bool)));
  connect(s_cookies,
	  SIGNAL(exceptionRaised(dexceptionswindow *,
				 const QUrl &)),
	  this,
	  SLOT(slotExceptionRaised(dexceptionswindow *,
				   const QUrl &)));
  connect(s_bookmarksPopup,
	  SIGNAL(changed(void)),
	  this,
	  SLOT(slotBookmarksChanged(void)));
  connect(s_bookmarksWindow,
	  SIGNAL(changed(void)),
	  this,
	  SLOT(slotBookmarksChanged(void)));
  connect(s_errorLog,
	  SIGNAL(errorLogged(void)),
	  this,
	  SLOT(slotErrorLogged(void)));
  connect(sb.authenticate, SIGNAL(clicked(void)),
	  this, SLOT(slotAuthenticate(void)));
  connect(sb.errorLogToolButton,
	  SIGNAL(clicked(void)),
	  s_errorLog,
	  SLOT(slotShow(void)));
  connect(sb.errorLogToolButton,
	  SIGNAL(clicked(void)),
	  sb.errorLogToolButton,
	  SLOT(hide(void)));
  connect(ui.action_Clear_Containers,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotClearContainers(void)));
  ui.viewMenu->insertSeparator(ui.zoomMenu->menuAction());

  if(ui.historyMenu->actions().isEmpty())
    {
      QAction *action = 0;
      QSettings settings
	(s_settings.value("iconSet").toString(), QSettings::IniFormat);

      action = ui.historyMenu->addAction
	(QIcon(settings.value("mainWindow/historyMenu").toString()),
	 tr("&Clear History"));
      action->setEnabled(false);
      connect(action, SIGNAL(triggered(void)), this,
	      SLOT(slotClearHistory(void)));
      action = ui.historyMenu->addAction
	(QIcon(settings.value("mainWindow/viewHistory").toString()),
	 tr("Show &History..."));
      action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_H));
      action->setEnabled(true);
      connect(action, SIGNAL(triggered(void)), this,
	      SLOT(slotShowHistory(void)));
      ui.historyMenu->addSeparator();
      action = ui.historyMenu->addAction(tr("&Recently-Closed Tabs"));
      action->setEnabled(false);
    }

  ui.actionPrint->setEnabled(false);
  ui.actionPrint_Preview->setEnabled(false);
  ui.actionOpen_URL->setEnabled(false);
  ui.actionShow_Hidden_Files->setEnabled(false);
  ui.actionSave_Page->setEnabled(false);
  ui.actionSave_Page_as_Data_URI->setEnabled(false);

  for(int i = 0; i < ui.tabWidget->count(); i++)
    if(qobject_cast<ddesktopwidget *> (ui.tabWidget->widget(i)))
      {
	ui.desktopToolButton->setEnabled(false);
	break;
      }

  /*
  ** According to Qt documentation, resize() followed by move() should
  ** be used to restore a window's geometry. Actual results may vary.
  */

  /*
  ** I don't know what to do about JavaScript windows that do not resize
  ** themselves.
  */

  if(s_settings.contains("mainWindow/geometry"))
    {
      if(!m_isJavaScriptWindow)
	{
	  if(dmisc::isGnome())
	    setGeometry(s_settings.
			value("mainWindow/geometry",
			      QRect(100, 100, 1024, 768)).toRect());
	  else
	    {
	      QByteArray g(s_settings.value("mainWindow/geometry").
			   toByteArray());

	      if(!restoreGeometry(g))
		setGeometry(100, 100, 1024, 768);
	    }
	}
      else
	{
	  if(m_parentWindow)
	    {
	      if(m_parentWindow->geometry().isValid())
		setGeometry
		  (dmisc::balancedGeometry(QRect(m_parentWindow->
						 geometry().x(),
						 m_parentWindow->
						 geometry().y(),
						 800, 600), this));
	      else
		setGeometry(dmisc::balancedGeometry(QRect(100, 100, 800, 600),
						    this));
	    }
	  else
	    setGeometry(dmisc::balancedGeometry(QRect(100, 100, 800, 600),
						this));
	}
    }
  else if(QApplication::desktop()->screenGeometry().isValid())
    {
      if(!m_isJavaScriptWindow)
	setGeometry
	  (dmisc::balancedGeometry(QRect(100, 100, 1024, 768), this));
      else if(m_parentWindow)
	{
	  if(m_parentWindow->geometry().isValid())
	    setGeometry(dmisc::balancedGeometry(QRect(m_parentWindow->
						      geometry().x(),
						      m_parentWindow->
						      geometry().y(),
						      800, 600), this));
	  else
	    setGeometry(dmisc::balancedGeometry(QRect(100, 100, 800, 600),
						this));
	}
      else
	/*
	** Instead of sizeHint(), use a fixed value. sizeHint() may
	** return an invalid size.
	*/

	setGeometry(dmisc::balancedGeometry(QRect(100, 100, 1024, 768),
					    this));
    }
  else if(m_parentWindow)
    {
      if(m_parentWindow->geometry().isValid())
	setGeometry(dmisc::balancedGeometry(QRect(m_parentWindow->
						  geometry().x(),
						  m_parentWindow->
						  geometry().y(),
						  1024, 768), this));
      else
	setGeometry(dmisc::balancedGeometry(QRect(100, 100, 1024, 768),
					    this));
    }
  else
    /*
    ** Please read the above comment.
    */

    setGeometry(dmisc::balancedGeometry(QRect(100, 100, 1024, 768), this));

  if(s_settings.contains("mainWindow/state2"))
    restoreState(s_settings["mainWindow/state2"].toByteArray());

  if(s_settings.contains("mainWindow/splitterState"))
    ui.splitter->restoreState
      (s_settings.value("mainWindow/splitterState", "").toByteArray());
  else
    {
      ui.splitter->setStretchFactor(0, 1);
      ui.splitter->setStretchFactor(1, 0);
    }

  if(s_settings.contains("mainWindow/historyTabSplitterState"))
    {
      if(!ui.historyAndTabSplitter->restoreState
	 (s_settings.value("mainWindow/historyTabSplitterState",
			   "").toByteArray()))
	{
	  ui.historyAndTabSplitter->setStretchFactor(0, 0);
	  ui.historyAndTabSplitter->setStretchFactor(1, 100);
	}
    }
  else
    {
      ui.historyAndTabSplitter->setStretchFactor(0, 0);
      ui.historyAndTabSplitter->setStretchFactor(1, 100);
    }

  statusBar()->addPermanentWidget(sbWidget, 100);
  statusBar()->setStyleSheet("QStatusBar::item {"
			     "border: none; "
			     "}");
  statusBar()->setMaximumHeight(sbWidget->height());
  statusBar()->setVisible
    (s_settings.value("mainWindow/statusbarDisplay", true).toBool());
  ui.actionStatusbar->setChecked(!statusBar()->isHidden());
  ui.actionShow_Hidden_Files->setChecked
    (s_settings.value("mainWindow/showHiddenFiles", true).toBool());
  ui.favoritesToolBar->setVisible
    (s_settings.value("mainWindow/showFavoritesToolBar", false).
     toBool());
  ui.actionShow_FavoritesToolBar->setChecked
    (s_settings.value("mainWindow/showFavoritesToolBar", false).
     toBool());
  sb.authenticate->setEnabled
    (!dmisc::passphraseWasAuthenticated() && dmisc::passphraseWasPrepared());
  ui.action_Authenticate->setEnabled
    (!dmisc::passphraseWasAuthenticated() && dmisc::passphraseWasPrepared());
  slotSetIcons();
#ifdef Q_OS_MAC
  foreach(QToolButton *toolButton, findChildren<QToolButton *> ())
    if(toolButton == ui.backToolButton ||
       toolButton == ui.forwardToolButton)
      toolButton->setStyleSheet
	("QToolButton {border: none; padding-right: 10px}"
	 "QToolButton::menu-button {border: none;}");
    else if(toolButton == ui.stopToolButton ||
	    toolButton == ui.reloadToolButton ||
	    toolButton == ui.homeToolButton ||
	    toolButton == ui.desktopToolButton ||
	    toolButton == sb.authenticate ||
	    toolButton == sb.exceptionsToolButton ||
	    toolButton == sb.errorLogToolButton)
      toolButton->setStyleSheet
	("QToolButton {border: none;}"
	 "QToolButton::menu-button {border: none;}");
#endif
#if (defined (Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
  setWindowRole("browser");
#endif
#ifndef Q_OS_MAC
  prepareMenuBar(s_settings.value("mainWindow/hideMenuBar", false).toBool());
#endif
}

dooble::dooble
(const bool isJavaScriptWindow, dooble *d):QMainWindow()
{
  /*
  ** m_parentWindow is used for positioning JavaScript windows.
  */

  if(m_isJavaScriptWindow)
    m_parentWindow = d;
  else
    m_parentWindow = 0;

  init_dooble(isJavaScriptWindow);
  copyDooble(d);
  prepareMostVisited();

  if(!isJavaScriptWindow)
    if(s_settings.value("settingsWindow/displayDesktopCheckBox",
			false).toBool())
      slotShowDesktopTab(false);

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, d);

  show();
  ui.historyFrame->setVisible
    (ui.actionShow_HistorySideBar->isChecked());
  update();

  if(s_instances <= 1)
    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
      QMessageBox::critical
	(this, tr("Dooble Web Browser: Error"),
	 tr("The SQLite database driver QSQLITE is not available. "
	    "This is a fatal flaw."));

  if(!dmisc::passphraseWasPrepared() && s_instances <= 1)
    remindUserToSetPassphrase();
}

dooble::dooble
(const QUrl &url, dooble *d, dcookies *cookies,
 const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes):
  QMainWindow()
{
  m_parentWindow = 0;
  init_dooble(false);
  ui.tabWidget->setVisible(false);
  s_sslCiphersWindow->populate();

  if(promptForPassphrase())
    {
      /*
      ** We're not going to populate the History model and window.
      */

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      s_cookies->populate();
      s_cookieWindow->populate();
      s_downloadWindow->populate();
      s_bookmarksWindow->populate();
      s_popupsWindow->populate();
      s_adBlockWindow->populate();
      s_cookiesBlockWindow->populate();
      s_httpOnlyExceptionsWindow->populate();
      s_httpRedirectWindow->populate();
      s_httpReferrerWindow->populate();
      s_javaScriptExceptionsWindow->populate();
      s_dntWindow->populate();
      s_imageBlockWindow->populate();
      s_cacheExceptionsWindow->populate();
      s_alwaysHttpsExceptionsWindow->populate();
      s_sslExceptionsWindow->populate();
      s_networkCache->populate();
      QApplication::restoreOverrideCursor();
    }

  copyDooble(d);
  prepareMostVisited();
  ui.tabWidget->setVisible(true);
  newTab(url, cookies, webAttributes);

  if(s_settings.value("settingsWindow/displayDesktopCheckBox",
		      false).toBool())
    slotShowDesktopTab(false);

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, d);

  show();
  ui.tabWidget->update();
  ui.historyFrame->setVisible
    (ui.actionShow_HistorySideBar->isChecked());
  reinstate();
  update();

  if(s_instances <= 1)
    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
      QMessageBox::critical
	(this, tr("Dooble Web Browser: Error"),
	 tr("The SQLite database driver QSQLITE is not available. "
	    "This is a fatal flaw."));

  if(!dmisc::passphraseWasPrepared() && s_instances <= 1)
    remindUserToSetPassphrase();
}

dooble::dooble(dview *p, dooble *d):QMainWindow()
{
  /*
  ** This method is called when the user has decided to
  ** change a tab into a window.
  */

  m_parentWindow = 0;
  disconnectPageSignals(p, d);
  init_dooble(false);
  copyDooble(d);
  prepareMostVisited();
  newTab(p);

  if(s_settings.value("settingsWindow/displayDesktopCheckBox",
		      false).toBool())
    slotShowDesktopTab(false);

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, d);

  show();
  ui.tabWidget->update();
  ui.historyFrame->setVisible(ui.actionShow_HistorySideBar->isChecked());
  update();

  if(ui.tabWidget->indexOf(p) > -1)
    ui.tabWidget->setTabText
      (ui.tabWidget->indexOf(p),
       ui.tabWidget->tabText(ui.tabWidget->indexOf(p)));

  if(s_instances <= 1)
    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
      QMessageBox::critical
	(this, tr("Dooble Web Browser: Error"),
	 tr("The SQLite database driver QSQLITE is not available. "
	    "This is a fatal flaw."));

  if(!dmisc::passphraseWasPrepared() && s_instances <= 1)
    remindUserToSetPassphrase();
}

dooble::dooble(const QByteArray &history, dooble *d):QMainWindow()
{
  m_parentWindow = 0;
  init_dooble(false);
  copyDooble(d);
  prepareMostVisited();
  newTab(history);

  if(s_settings.value("settingsWindow/displayDesktopCheckBox",
		      false).toBool())
    slotShowDesktopTab(false);

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, d);

  show();
  ui.tabWidget->update();
  ui.historyFrame->setVisible
    (ui.actionShow_HistorySideBar->isChecked());
  update();

  if(s_instances <= 1)
    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
      QMessageBox::critical
	(this, tr("Dooble Web Browser: Error"),
	 tr("The SQLite database driver QSQLITE is not available. "
	    "This is a fatal flaw."));

  if(!dmisc::passphraseWasPrepared() && s_instances <= 1)
    remindUserToSetPassphrase();
}

dooble::dooble(const QHash<QString, QVariant> &hash, dooble *d):QMainWindow()
{
  m_parentWindow = 0;
  init_dooble(false);
  ui.tabWidget->setVisible(false);
  s_sslCiphersWindow->populate();

  if(promptForPassphrase())
    {
      /*
      ** We're not going to populate the History model and window.
      */

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      s_cookies->populate();
      s_cookieWindow->populate();
      s_downloadWindow->populate();
      s_bookmarksWindow->populate();
      s_popupsWindow->populate();
      s_adBlockWindow->populate();
      s_cookiesBlockWindow->populate();
      s_httpOnlyExceptionsWindow->populate();
      s_httpRedirectWindow->populate();
      s_httpReferrerWindow->populate();
      s_javaScriptExceptionsWindow->populate();
      s_dntWindow->populate();
      s_imageBlockWindow->populate();
      s_cacheExceptionsWindow->populate();
      s_alwaysHttpsExceptionsWindow->populate();
      s_sslExceptionsWindow->populate();
      s_networkCache->populate();
      QApplication::restoreOverrideCursor();
    }

  copyDooble(d);
  prepareMostVisited();
  ui.tabWidget->setVisible(true);

  QStringList urls(hash["urls"].toStringList());

  while(!urls.isEmpty())
    {
      QUrl url(QUrl::fromUserInput(urls.takeFirst()));

      if(url.isValid())
	{
	  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

	  webAttributes[QWebEngineSettings::JavascriptEnabled] =
	    s_settings.value("settingsWindow/javascriptEnabled",
			     false).toBool();
	  newTab
	    (QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash)), 0,
	     webAttributes);
	}
    }

  if(s_settings.value("settingsWindow/displayDesktopCheckBox",
		      false).toBool())
    slotShowDesktopTab(false);

  if(dooble::s_settings.value("settingsWindow/centerChildWindows",
			      false).toBool())
    dmisc::centerChildWithParent(this, d);

  show();
  ui.tabWidget->update();
  ui.historyFrame->setVisible
    (ui.actionShow_HistorySideBar->isChecked());
  reinstate();
  update();

  if(s_instances <= 1)
    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
      QMessageBox::critical
	(this, tr("Dooble Web Browser: Error"),
	 tr("The SQLite database driver QSQLITE is not available. "
	    "This is a fatal flaw."));

  if(!dmisc::passphraseWasPrepared() && s_instances <= 1)
    remindUserToSetPassphrase();
}

dooble::~dooble()
{
  if(s_instances > 0)
    s_instances -= 1;

  m_closedTabs.clear();
}

void dooble::slotSetIcons(void)
{
  QSize size;
  QSettings settings(s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  size = s_settings.value("settingsWindow/locationToolbarIconSize",
			  QSize(24, 24)).toSize();

  if(!(size == QSize(16, 16) ||
       size == QSize(24, 24) ||
       size == QSize(32, 32)))
    size = QSize(24, 24);

  ui.backToolButton->setIconSize(size);
  ui.forwardToolButton->setIconSize(size);
  ui.menuToolButton->setIconSize(size);
  ui.reloadToolButton->setIconSize(size);
  ui.stopToolButton->setIconSize(size);
  ui.homeToolButton->setIconSize(size);
  ui.desktopToolButton->setIconSize(size);
  ui.backToolButton->setIcon
    (QIcon(settings.value("mainWindow/backToolButton").toString()));
  ui.forwardToolButton->setIcon
    (QIcon(settings.value("mainWindow/forwardToolButton").toString()));
  ui.menuToolButton->setIcon
    (QIcon(settings.value("mainWindow/menuToolButton").toString()));
  ui.reloadToolButton->setIcon
    (QIcon(settings.value("mainWindow/reloadToolButton").toString()));
  ui.stopToolButton->setIcon
    (QIcon(settings.value("mainWindow/stopToolButton").toString()));
  ui.homeToolButton->setIcon
    (QIcon(settings.value("mainWindow/homeToolButton").toString()));
  ui.hideFindToolButton->setIcon
    (QIcon(settings.value("mainWindow/hideFindToolButton").toString()));
  ui.actionNew_Tab->setIcon
    (QIcon(settings.value("mainWindow/actionNew_Tab").toString()));
  ui.actionNew_Window->setIcon
    (QIcon(settings.value("mainWindow/actionNew_Window").toString()));
  ui.actionOpen_URL->setIcon
    (QIcon(settings.value("mainWindow/actionOpen_URL").toString()));
  ui.actionClose_Tab->setIcon
    (QIcon(settings.value("mainWindow/actionClose_Tab").toString()));
  ui.actionClose_Window->setIcon
    (QIcon(settings.value("mainWindow/actionClose_Window").toString()));
  ui.actionSave_Page->setIcon
    (QIcon(settings.value("mainWindow/actionSave_Page").toString()));
  ui.actionSave_Page_as_Data_URI->setIcon
    (QIcon(settings.value("mainWindow/actionSave_Page_as_Data_URI").
	   toString()));
  ui.actionPrint->setIcon
    (QIcon(settings.value("mainWindow/actionPrint").toString()));
  ui.actionPrint_Preview->setIcon
    (QIcon(settings.value("mainWindow/actionPrint_Preview").toString()));
  ui.actionQuit->setIcon
    (QIcon(settings.value("mainWindow/actionQuit").toString()));
  ui.actionFind->setIcon
    (QIcon(settings.value("mainWindow/actionFind").toString()));
  ui.actionReload->setIcon
    (QIcon(settings.value("mainWindow/actionReload").toString()));
  ui.actionStop->setIcon
    (QIcon(settings.value("mainWindow/actionStop").toString()));
  ui.actionPage_Source->setIcon
    (QIcon(settings.value("mainWindow/actionPage_Source").toString()));
  ui.actionDownloads->
    setIcon(QIcon(settings.value("mainWindow/actionDownloads").toString()));
  ui.actionSettings->setIcon
    (QIcon(settings.value("mainWindow/actionSettings").toString()));
  ui.actionAbout_Dooble->
    setIcon(QIcon(settings.value("mainWindow/actionAbout_Dooble").toString()));
  ui.desktopToolButton->
    setIcon(QIcon(settings.value("mainWindow/desktopToolButton").toString()));
  ui.actionOpen_Directory->setIcon
    (QIcon(settings.value("mainWindow/actionOpen_Directory").toString()));
  ui.actionMy_Retrieved_Files->setIcon
    (QIcon(settings.value("mainWindow/actionMy_Retrieved_Files").toString()));
  ui.actionCopy->setIcon
    (QIcon(settings.value("mainWindow/actionCopy").toString()));
  ui.actionPaste->setIcon
    (QIcon(settings.value("mainWindow/actionPaste").toString()));
  ui.actionSelect_All_Content->setIcon
    (QIcon(settings.value("mainWindow/actionSelect_All_Content").toString()));
  ui.zoomMenu->setIcon
    (QIcon(settings.value("mainWindow/zoomMenu").toString()));
  ui.nextToolButton->setIcon
    (QIcon(settings.value("mainWindow/nextToolButton").toString()));
  ui.previousToolButton->setIcon
    (QIcon(settings.value("mainWindow/previousToolButton").toString()));
  ui.actionApplication_Cookies->setIcon
    (QIcon(settings.value
	   ("mainWindow/actionApplication_Cookies").toString()));
  ui.actionP2P_Email->setIcon
    (QIcon(settings.value("mainWindow/actionP2P_Email").toString()));
  ui.action_IRC_Channel->setIcon
    (QIcon(settings.value("windowIcon").toString()));
  ui.action_Authenticate->setIcon
    (QIcon(settings.value("mainWindow/authenticate_Action").toString()));
  ui.actionError_Log->setIcon
    (QIcon(settings.value("mainWindow/actionError_Log").toString()));
  ui.action_Clear_Containers->setIcon
    (QIcon(settings.value("mainWindow/actionClear_Containers").toString()));
  ui.action_SSL_Ciphers->setIcon
    (QIcon(settings.value("mainWindow/authenticate_Action").toString()));
  sb.authenticate->setIcon
    (QIcon(settings.value("mainWindow/authenticate_Action").toString()));
  sb.exceptionsToolButton->setIcon
    (QIcon(settings.value("mainWindow/exceptionToolButton").toString()));
  sb.errorLogToolButton->setIcon
    (QIcon(settings.value("mainWindow/errorLogToolButton").toString()));

  if(ui.historyMenu->actions().size() > 0)
    {
      ui.historyMenu->actions().at(0)->setIcon
	(QIcon(settings.value("mainWindow/historyMenu").toString()));

      if(ui.historyMenu->actions().size() >= 4 &&
	 ui.historyMenu->actions().at(3)->menu() &&
	 ui.historyMenu->actions().at(3)->menu()->actions().size() > 0)
	ui.historyMenu->actions().at(3)->menu()->actions().at(0)->setIcon
	  (QIcon(settings.value("mainWindow/historyMenu").toString()));
    }

  if(ui.historyMenu->actions().size() >= 2)
    ui.historyMenu->actions().at(1)->setIcon
      (QIcon(settings.value("mainWindow/viewHistory").toString()));

  if(ui.bookmarksMenu->actions().size() >= 3)
    {
      ui.bookmarksMenu->actions().at(0)->setIcon
	(QIcon(settings.value("mainWindow/actionBookmarks").toString()));
      ui.bookmarksMenu->actions().at(2)->setIcon
	(QIcon(settings.value("mainWindow/actionBookmarks").toString()));
    }

  QMessageBox *mb = findChild<QMessageBox *> ("about");

  if(mb)
    {
      for(int i = 0; i < mb->buttons().size(); i++)
	if(mb->buttonRole(mb->buttons().at(i)) == QMessageBox::AcceptRole ||
	   mb->buttonRole(mb->buttons().at(i)) == QMessageBox::ApplyRole ||
	   mb->buttonRole(mb->buttons().at(i)) == QMessageBox::YesRole)
	  mb->buttons().at(i)->setIcon
	    (QIcon(settings.value("okButtonIcon").toString()));

      mb->setWindowIcon
	(QIcon(settings.value("mainWindow/windowIcon").toString()));
    }

  /*
  ** The 0th tab is not necessarily the Desktop.
  */

  for(int i = 0; i < ui.tabWidget->count(); i++)
    if(qobject_cast<ddesktopwidget *> (ui.tabWidget->widget(i)))
      {
	ui.tabWidget->setTabIcon
	  (i, QIcon(settings.value("mainWindow/tabWidget").toString()));
	break;
      }

  setWindowIcon(QIcon(settings.value("mainWindow/windowIcon").toString()));
  emit iconsChanged();
}

void dooble::newTabInit(dview *p)
{
  if(!p)
    return;

  connect(p, SIGNAL(destroyed(QObject *)),
	  this, SLOT(slotObjectDestroyed(QObject *)));
  connect(p, SIGNAL(urlChanged(const QUrl &)), this,
	  SLOT(slotUrlChanged(const QUrl &)));
  connect(p, SIGNAL(titleChanged(const QString &)), this,
	  SLOT(slotTitleChanged(const QString &)));
  connect(p, SIGNAL(loadFinished(bool)), this,
	  SLOT(slotLoadFinished(bool)));
  connect(p, SIGNAL(loadProgress(int)), this,
	  SLOT(slotLoadProgress(int)));
  connect(p, SIGNAL(iconChanged(void)), this,
	  SLOT(slotIconChanged(void)));
  connect(p, SIGNAL(loadStarted(void)), this,
	  SLOT(slotLoadStarted(void)));
  connect
    (p,
     SIGNAL(openLinkInNewTab(const QUrl &, dcookies *,
			     const QHash<QWebEngineSettings::WebAttribute,
			     bool> &)),
     this,
     SLOT(slotOpenLinkInNewTab(const QUrl &, dcookies *,
			       const QHash<QWebEngineSettings::WebAttribute,
			       bool> &)));
  connect
    (p,
     SIGNAL(openLinkInNewWindow(const QUrl &, dcookies *,
				const QHash<QWebEngineSettings::WebAttribute,
				bool> &)),
     this,
     SLOT(slotOpenLinkInNewWindow(const QUrl &, dcookies *,
				  const QHash<QWebEngineSettings::WebAttribute,
				  bool> &)));
  connect(p, SIGNAL(copyLink(const QUrl &)), this,
	  SLOT(slotCopyLink(const QUrl &)));
  connect(p, SIGNAL(saveUrl(const QUrl &, const int)), this,
	  SLOT(slotSaveUrl(const QUrl &, const int)));
  connect(p, SIGNAL(saveFile(const QString &, const QUrl &, const int)),
	  this, SLOT(slotSaveFile(const QString &, const QUrl &, const int)));
  connect(p, SIGNAL(viewImage(const QUrl &)), this,
	  SLOT(slotLoadPage(const QUrl &)));
  connect(p, SIGNAL(ipAddressChanged(const QString &)), this,
	  SLOT(slotIpAddressChanged(const QString &)));
  connect(p->page(),
	  SIGNAL(linkHovered(const QString &)),
	  this,
	  SLOT(slotLinkHovered(const QString &)));
  connect(p->page(),
	  SIGNAL(authenticationRequired(const QUrl &, QAuthenticator *)),
	  this,
	  SLOT(slotAuthenticationRequired(const QUrl &, QAuthenticator *)));
  connect(p->page(),
	  SIGNAL(proxyAuthenticationRequired(const QUrl &,
					     QAuthenticator *,
					     const QString &)),
	  this,
	  SLOT(slotProxyAuthenticationRequired(const QUrl &,
					       QAuthenticator *,
					       const QString &)));
  connect(p->page(), SIGNAL(windowCloseRequested(void)),
	  this, SLOT(slotCloseWindow(void)));
  connect(p->page(), SIGNAL(geometryChangeRequested(const QRect &)),
	  this, SLOT(slotGeometryChangeRequested(const QRect &)));
  connect(this,
	  SIGNAL(clearHistory(void)),
	  p,
	  SLOT(slotClearHistory(void)));
  connect(p, SIGNAL(selectionChanged(const QString &)),
	  this, SLOT(slotSelectionChanged(const QString &)));
  connect(p, SIGNAL(viewPageSource(void)),
	  this, SLOT(slotShowPageSource(void)));
  connect(p, SIGNAL(goBack(void)),
	  this, SLOT(slotBack(void)));
  connect(p, SIGNAL(goReload(void)),
	  this, SLOT(slotReload(void)));
  connect(p, SIGNAL(goForward(void)),
	  this, SLOT(slotForward(void)));
  connect(p,
	  SIGNAL(exceptionRaised(dexceptionswindow *,
				 const QUrl &)),
	  this,
	  SLOT(slotExceptionRaised(dexceptionswindow *,
				   const QUrl &)));
  connect(p->page(),
	  SIGNAL(exceptionRaised(dexceptionswindow *,
				 const QUrl &)),
	  this,
	  SLOT(slotExceptionRaised(dexceptionswindow *,
				   const QUrl &)));
  connect(p->page(),
	  SIGNAL(exceptionRaised(dexceptionswindow *,
				 const QUrl &)),
	  this,
	  SLOT(slotExceptionRaised(dexceptionswindow *,
				   const QUrl &)));
  connect(s_clearContainersWindow,
	  SIGNAL(clearCookies(void)),
	  p,
	  SLOT(slotClearCookies(void)));

  int index = 0;
  QString title(p->title());

  if(title.isEmpty())
    title = p->url().toString(QUrl::StripTrailingSlash);

  if(title.isEmpty())
    title = tr("(Untitled)");

  if(s_settings.value("settingsWindow/appendNewTabs", false).toBool())
    {
      index = ui.tabWidget->addTab(p, title.replace("&", "&&"));
      ui.tabWidget->setTabToolTip(index, title);
    }
  else
    {
      index = ui.tabWidget->currentIndex() + 1;
      index = ui.tabWidget->insertTab(index, p, title.replace("&", "&&"));
      ui.tabWidget->setTabToolTip(index, title);
    }

  ui.tabWidget->setTabButton(index);

  /*
  ** If p came from another window, its animation icon will
  ** appear distorted because of the new tab
  ** having a "selected" background color (stylesheet).
  */

  ui.tabWidget->animateIndex(index, !p->isLoaded(), p->webviewIcon(),
			     p->progress(), !statusBar()->isHidden());

  if(p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    sb.statusLabel->clear();

  ui.tabWidget->setTabsClosable(ui.tabWidget->count() > 1);

  if(ui.tabWidget->count() == 1)
    ui.tabWidget->setBarVisible
      (s_settings.value("settingsWindow/alwaysShowTabBar",
			true).toBool());
  else
    ui.tabWidget->setBarVisible(true);
}

dview *dooble::newTab(dview *p)
{
  if(p)
    {
      p->removeRestorationFiles();
      p->setParent(this);
      newTabInit(p);

      if(p->url().isEmpty() || !p->url().isValid())
	slotOpenUrl();

      QString title(p->title());

      if(title.isEmpty())
	title = p->url().toString(QUrl::StripTrailingSlash);

      title = dmisc::elidedTitleText(title);

      if(title.isEmpty())
	title = tr("(Untitled)");

      QAction *action = new QAction(p->icon(), title, this);

      action->setData(p->url());
      connect(action, SIGNAL(triggered(void)), this,
	      SLOT(slotLinkActionTriggered(void)));
      p->setTabAction(action);
      prepareTabsMenu();

      /*
      ** Record the restoration file after p has been assigned to
      ** this window.
      */

      p->recordRestorationHistory();
    }

  return p;
}

dview *dooble::newTab
(const QUrl &url, dcookies *cookies,
 const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes)
{
  dview *p = new dview(this, QByteArray(), cookies, webAttributes);

  newTabInit(p);

  if(p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      ui.locationLineEdit->setText(url.toString(QUrl::StripTrailingSlash));

      if(!ui.locationLineEdit->text().isEmpty())
	ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());

      if(url.isEmpty() || !url.isValid())
	slotOpenUrl();
    }

  QAction *action = new QAction(p->icon(), tr("(Untitled)"), this);

  action->setData(url);
  connect(action, SIGNAL(triggered(void)), this,
	  SLOT(slotLinkActionTriggered(void)));
  p->setTabAction(action);
  p->load(url);
  prepareTabsMenu();
  prepareWidgetsBasedOnView(p);
  return p;
}

dview *dooble::newTab(const QByteArray &history)
{
  QUrl url;
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;
  QIcon icon;
  bool shouldLoad = true;
  dview *p = 0;

  webAttributes[QWebEngineSettings::JavascriptEnabled] =
    s_settings.value("settingsWindow/javascriptEnabled",
		     false).toBool();
  p = new dview(this, history, 0, webAttributes);
  newTabInit(p);

  if(!history.isEmpty())
    {
      QByteArray h(history);
      QDataStream in(&h, QIODevice::ReadOnly);

      if(in.status() == QDataStream::Ok)
	{
	  in >> *p->page()->history();

	  if(p->page()->history() &&
	     p->page()->history()->currentItem().isValid())
	    {
	      url = p->page()->history()->currentItem().url();
	      icon = dmisc::iconForUrl(url);
	      shouldLoad = false;
	    }
	  else if(p->page()->history())
	    p->page()->history()->clear();
	}
    }

  if(p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      ui.locationLineEdit->setText(url.toString(QUrl::StripTrailingSlash));

      if(!ui.locationLineEdit->text().isEmpty())
	ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());

      if(url.isEmpty() || !url.isValid())
	slotOpenUrl();
    }

  if(icon.isNull())
    icon = dmisc::iconForUrl(url);

  QAction *action = new QAction(icon, tr("(Untitled)"), this);

  action->setData(url);
  connect(action, SIGNAL(triggered(void)), this,
	  SLOT(slotLinkActionTriggered(void)));
  p->setTabAction(action);

  if(shouldLoad)
    p->load(url);
  else
    p->reload();

  prepareTabsMenu();
  prepareWidgetsBasedOnView(p);
  return p;
}

void dooble::cleanupBeforeExit(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dmisc::removeRestorationFiles(s_id);
  s_historyWindow->deleteLater();
  s_bookmarksPopup->deleteLater();
  s_downloadWindow->deleteLater();
  s_bookmarksWindow->deleteLater();
  s_cookies->deleteLater();
  s_cookieWindow->deleteLater();
  s_historyModel->deleteLater();
  s_popupsWindow->deleteLater();
  s_adBlockWindow->deleteLater();
  s_cookiesBlockWindow->deleteLater();
  s_httpOnlyExceptionsWindow->deleteLater();
  s_httpRedirectWindow->deleteLater();
  s_httpReferrerWindow->deleteLater();
  s_javaScriptExceptionsWindow->deleteLater();
  s_dntWindow->deleteLater();
  s_errorLog->deleteLater();
  s_imageBlockWindow->deleteLater();
  s_cacheExceptionsWindow->deleteLater();
  s_alwaysHttpsExceptionsWindow->deleteLater();
  s_sslExceptionsWindow->deleteLater();
  s_networkCache->deleteLater();
  s_clearContainersWindow->deleteLater();

  if(s_spoton)
    s_spoton->deleteLater();

  if(dfilemanager::tableModel)
    dfilemanager::tableModel->deleteLater();

  if(dfilemanager::treeModel)
    dfilemanager::treeModel->deleteLater();

  QApplication::restoreOverrideCursor();
}

void dooble::slotQuit(void)
{
  /*
  ** closeEvent() may call this slot.
  */

  if(sender() != 0)
    {
      if(!warnAboutDownloads())
	return;

      if(!warnAboutTabs(QApplication::instance()))
	return;
    }

  s_downloadWindow->abort();
  cleanupBeforeExit();
  saveSettings();
  QApplication::instance()->exit(0);
}

void dooble::slotQuitAndRestart(void)
{
  cleanupBeforeExit();
  QApplication::instance()->exit(0);

#ifdef Q_OS_WIN32
  QString program(QCoreApplication::applicationDirPath() +
		  QDir::separator() +
		  QCoreApplication::applicationName());

  int rc = (int)
    (::ShellExecuteA(0, "open", program.toUtf8().constData(),
		     0, 0, SW_SHOWNORMAL));

  if(rc == SE_ERR_ACCESSDENIED)
    /*
    ** Elevated?
    */

    ::ShellExecuteA(0, "runas", program.toUtf8().constData(),
		    0, 0, SW_SHOWNORMAL);
#else
  QProcess::startDetached(QCoreApplication::applicationDirPath() +
			  QDir::separator() +
			  QCoreApplication::applicationName());
#endif
}

void dooble::saveSettings(void)
{
  if(!m_isJavaScriptWindow)
    {
      QSettings settings;

      settings.setValue("mainWindow/historyTabSplitterState",
			ui.historyAndTabSplitter->saveState());
      settings.setValue("mainWindow/splitterState",
			ui.splitter->saveState());
      settings.setValue("mainWindow/state2", saveState());

      if(!isFullScreen())
	{
	  if(dmisc::isGnome())
	    settings.setValue("mainWindow/geometry", geometry());
	  else
	    settings.setValue("mainWindow/geometry", saveGeometry());
	}

      settings.setValue
	("mainWindow/ftpManagerColumnsState1",
	 s_settings.value
	 ("mainWindow/ftpManagerColumnsState1").toByteArray());
      settings.setValue
	("mainWindow/fileManagerColumnsState1",
	 s_settings.value
	 ("mainWindow/fileManagerColumnsState1").toByteArray());
    }
}

void dooble::loadPage(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    {
      /*
      ** Prevent the user from loading an invalid URL.
      */

      slotOpenUrl();
      return;
    }

  QUrl l_url(dmisc::correctedUrlPath(url));
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      sb.statusLabel->clear();

      QString str(l_url.toString(QUrl::StripTrailingSlash).trimmed());

      ui.locationLineEdit->setBookmarkButtonEnabled(false);
      ui.locationLineEdit->setBookmarkColor(false);
      ui.locationLineEdit->setIconButtonEnabled(false);

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled(false);

      ui.locationLineEdit->setIcon(dmisc::iconForUrl(QUrl()));
      ui.locationLineEdit->setSecureColor(false);
      ui.locationLineEdit->setSpotOnButtonEnabled(false);
      ui.locationLineEdit->setSpotOnColor(false);
      ui.locationLineEdit->setText(str);

      if(!ui.locationLineEdit->text().isEmpty())
	ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());

      if(p->tabAction())
	p->tabAction()->setIcon(dmisc::iconForUrl(QUrl()));

      p->load(l_url);
      ui.zoomMenu->setEnabled(true);
      ui.actionSelect_All_Content->setEnabled(true);
      ui.actionFind->setEnabled(true);
      ui.findFrame->setVisible(showFindFrame);
    }
  else
    {
      QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

      webAttributes[QWebEngineSettings::JavascriptEnabled] =
	s_settings.value("settingsWindow/javascriptEnabled",
			 false).toBool();
      newTab(l_url, 0, webAttributes);
      ui.tabWidget->update();
    }
}

void dooble::slotLoadPage(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->isModified() && !warnAboutLeavingModifiedTab())
    return;

  QString str(ui.locationLineEdit->text().trimmed());

  loadPage(QUrl::fromEncoded(QUrl::fromUserInput(str).
			     toEncoded(QUrl::StripTrailingSlash)));

  if(ui.tabWidget->currentWidget())
    ui.tabWidget->currentWidget()->setFocus();
}

void dooble::slotLoadPage(const QUrl &url)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->isModified() && !warnAboutLeavingModifiedTab())
    {
      if(sender() == ui.locationLineEdit)
	{
	  ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
	  ui.locationLineEdit->setBookmarkButtonEnabled(p->isBookmarked());
	  ui.locationLineEdit->setIcon(p->icon());
	  ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
	  ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
	  ui.locationLineEdit->setSpotOnColor(p->isLoaded());
	  ui.locationLineEdit->setText
	    (p->url().toString(QUrl::StripTrailingSlash));

	  if(!ui.locationLineEdit->text().isEmpty())
	    ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());
	}

      return;
    }

  loadPage(url);

  if(ui.tabWidget->currentWidget())
    ui.tabWidget->currentWidget()->setFocus();
}

void dooble::slotNewTab(void)
{
  QUrl url;
  dview *p = 0;

  url = QUrl::fromUserInput(s_settings.value("settingsWindow/homeUrl",
					     "").toString());

  if(!url.isValid())
    url = QUrl();
  else
    url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

  webAttributes[QWebEngineSettings::JavascriptEnabled] =
    s_settings.value("settingsWindow/javascriptEnabled",
		     false).toBool();
  p = newTab(url, 0, webAttributes);

  if(p)
    {
      ui.tabWidget->setCurrentWidget(p);
      ui.tabWidget->update();

      if(url.isEmpty() || !url.isValid())
	/*
	** p's url may be empty at this point.
	*/

	slotOpenUrl();
    }
}

void dooble::slotNewWindow(void)
{
  QUrl url;

  url = QUrl::fromUserInput(s_settings.value("settingsWindow/homeUrl", "").
			    toString());

  if(!url.isValid())
    url = QUrl();
  else
    url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

  if(s_settings.value("settingsWindow/openUserWindowsInNewProcesses",
		      false).toBool())
    launchDooble(url);
  else
    {
      QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

      webAttributes[QWebEngineSettings::JavascriptEnabled] =
	s_settings.value("settingsWindow/javascriptEnabled",
			 false).toBool();
      Q_UNUSED(new dooble(url, this, 0, webAttributes));
    }
}

void dooble::slotClose(void)
{
  close();
}

bool dooble::warnAboutTabs(QObject *object)
{
  /*
  ** Returns true if the warning was accepted or
  ** if the setting is disabled.
  */

  if(s_settings.value("settingsWindow/warnBeforeClosingModifiedTab", false).
     toBool())
    {
      bool ask = false;

      if(qobject_cast<QApplication *> (object))
	{
	  foreach(QWidget *widget, QApplication::allWidgets())
	    {
	      dview *p = qobject_cast<dview *> (widget);

	      if(p && p->isModified())
		{
		  ask = true;
		  break;
		}
	    }
	}
      else
	{
	  dooble *d = qobject_cast<dooble *> (object);

	  if(d)
	    for(int i = 0; i < d->ui.tabWidget->count(); i++)
	      {
		dview *p = qobject_cast<dview *> (d->ui.tabWidget->widget(i));

		if(p && p->isModified())
		  {
		    ask = true;
		    break;
		  }
	      }
	}

      if(ask)
	{
	  QMessageBox mb(this);

	  mb.setIcon(QMessageBox::Question);
	  mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
	  mb.setWindowModality(Qt::WindowModal);
	  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);

	  if(s_instances <= 1 || qobject_cast<QApplication *> (object))
	    mb.setText(tr("You have tabs with modified content. "
			  "Are you sure that you wish to exit?"));
	  else
	    mb.setText(tr("You have tabs with modified content. "
			  "Are you sure that you wish to close?"));

	  QSettings settings(s_settings.value("iconSet").toString(),
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
	    (QIcon(settings.value("mainWindow/windowIcon").toString()));

	  if(mb.exec() == QMessageBox::No)
	    return false;
	}
    }

  return true;
}

bool dooble::warnAboutDownloads(void)
{
  /*
  ** Returns true if the warning was accepted.
  */

  if(s_downloadWindow && s_downloadWindow->isActive())
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
      mb.setWindowModality(Qt::WindowModal);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Terminating Dooble "
		    "will cause existing downloads to be interrupted. "
		    "Are you sure that you wish to continue?"));

      QSettings settings(s_settings.value("iconSet").toString(),
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
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      if(mb.exec() == QMessageBox::No)
	return false;
    }

  return true;
}

void dooble::closeEvent(QCloseEvent *event)
{
  if(s_instances <= 1 && !warnAboutDownloads())
    {
      if(event)
	event->ignore();

      return;
    }

  if(!warnAboutTabs(this))
    {
      if(event)
	event->ignore();

      return;
    }

  unsetUrlHandler();

  if(s_instances > 1)
    deleteLater();

  if(s_instances <= 1)
    slotQuit();
}

void dooble::slotStop(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    p->stop();

  sb.progressBar->setValue(0);
  sb.progressBar->setVisible(false);
  ui.reloadStopWidget->setCurrentIndex(1);
  ui.stopToolButton->setEnabled(false);
  ui.actionStop->setEnabled(false);
}

void dooble::slotReload(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      if(p->isModified() && !warnAboutLeavingModifiedTab())
	return;

      p->reload();
    }
}

void dooble::slotStopTab(const int index)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p)
    p->stop();
}

void dooble::slotBack(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      if(p->isModified() && !warnAboutLeavingModifiedTab())
	return;

      p->back();
      ui.forwardToolButton->setEnabled(p->canGoForward());
    }
}

void dooble::slotForward(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      if(p->isModified() && !warnAboutLeavingModifiedTab())
	return;

      p->forward();
      ui.backToolButton->setEnabled(p->canGoBack());
    }
}

void dooble::slotTabSelected(const int index)
{
  for(int i = 0; i < ui.tabWidget->count(); i++)
    if(i != index)
      {
	dview *p = qobject_cast<dview *> (ui.tabWidget->widget(i));

	if(p)
	  ui.tabWidget->animateIndex
	    (i, !p->isLoaded(), p->webviewIcon(), p->progress(),
	     !statusBar()->isHidden());
      }

  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p)
    {
      ui.tabWidget->animateIndex
	(ui.tabWidget->indexOf(p), !p->isLoaded(), p->webviewIcon(),
	 p->progress(), !statusBar()->isHidden());
      ui.homeToolButton->setEnabled(true);
      ui.locationLineEdit->setVisible(true);
      ui.reloadToolButton->setEnabled(true);
      ui.searchLineEdit->setEnabled(true);
      ui.editMenu->setEnabled(true);
      ui.viewMenu->setEnabled(true);
      ui.actionPrint->setEnabled(true);
      ui.actionPrint_Preview->setEnabled(true);
      ui.actionOpen_URL->setEnabled(true);
      ui.actionSave_Page->setEnabled(true);
      ui.actionSave_Page_as_Data_URI->setEnabled(true);

      if(p->title().isEmpty())
	{
	  if(p->ipAddress().isEmpty() ||
	     !s_settings.value("settingsWindow/displayIpAddress",
			       false).toBool())
	    setWindowTitle(tr("Dooble Web Browser"));
	  else
	    setWindowTitle(QString(tr("Dooble Web Browser (%1)")).
			   arg(p->ipAddress()));
	}
      else
	{
	  if(p->ipAddress().isEmpty() ||
	     !s_settings.value("settingsWindow/displayIpAddress",
			       false).toBool())
	    setWindowTitle(p->title() + tr(" - Dooble Web Browser"));
	  else
            setWindowTitle(p->title() + QString(" (%1)").
			   arg(p->ipAddress()) + tr(" - Dooble Web Browser"));
	}

      /*
      ** Bound the progress bar's value. Some styles (GTK+) issue warnings
      ** if the value is outside of the acceptable range.
      */

      sb.progressBar->setValue(qBound(sb.progressBar->minimum(),
				      p->progress(),
				      sb.progressBar->maximum()));
      sb.progressBar->setVisible(!p->isLoaded());
      ui.reloadStopWidget->setCurrentIndex(p->isLoaded() ? 1 : 0);
      ui.stopToolButton->setEnabled(!p->isLoaded());
      ui.actionStop->setEnabled(!p->isLoaded());
      ui.backToolButton->setEnabled(p->canGoBack());
      ui.forwardToolButton->setEnabled(p->canGoForward());

      if(p->url().toString(QUrl::StripTrailingSlash) !=
	 p->webviewUrl().toString(QUrl::StripTrailingSlash))
	ui.locationLineEdit->setIcon(dmisc::iconForUrl(QUrl()));
      else
	ui.locationLineEdit->setIcon(p->icon());

      ui.locationLineEdit->setText
	(p->url().toString(QUrl::StripTrailingSlash));

      if(!ui.locationLineEdit->text().isEmpty())
	ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());

      ui.locationLineEdit->setSecureColor(p->hasSecureConnection());

      bool isBookmarkWorthy = p->isBookmarkWorthy();

      ui.locationLineEdit->setBookmarkButtonEnabled(isBookmarkWorthy);
      ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
      ui.locationLineEdit->setIconButtonEnabled(isBookmarkWorthy);
      ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
      ui.locationLineEdit->setSpotOnColor(p->isLoaded());

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled(isBookmarkWorthy);

      sb.statusLabel->setText(p->statusMessage());
      statusBar()->setVisible(ui.actionStatusbar->isChecked());
      prepareWidgetsBasedOnView(p);

      if(p->url().isEmpty() || !p->url().isValid() || p->isDir())
	slotOpenUrl();
      else
	p->setFocus();

      p->update();
    }
  else
    {
      setWindowTitle(tr("Dooble Web Browser"));
      ui.forwardToolButton->setEnabled(false);
      ui.backToolButton->setEnabled(false);
      ui.stopToolButton->setEnabled(false);
      ui.reloadToolButton->setEnabled(false);
      ui.homeToolButton->setEnabled(false);
      ui.searchLineEdit->setVisible(false);
      ui.locationLineEdit->setVisible(false);
      ui.editMenu->setEnabled(false);
      ui.viewMenu->setEnabled(false);
      ui.actionPrint->setEnabled(false);
      ui.actionPrint_Preview->setEnabled(false);
      ui.actionOpen_URL->setEnabled(false);
      ui.actionSave_Page->setEnabled(false);
      ui.actionSave_Page_as_Data_URI->setEnabled(false);
      ui.locationLineEdit->setBookmarkButtonEnabled(false);
      ui.locationLineEdit->setBookmarkColor(false);
      ui.locationLineEdit->setIconButtonEnabled(false);
      ui.locationLineEdit->setSpotOnButtonEnabled(false);
      ui.locationLineEdit->setSpotOnColor(false);

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled(false);

      ui.searchLineEdit->clear();
      ui.findFrame->setVisible(false);
      sb.statusLabel->clear();
      sb.progressBar->setValue(0);
      sb.progressBar->setVisible(false);
      statusBar()->setVisible(false);

      QWidget *widget = ui.tabWidget->widget(index);

      if(widget)
	widget->setFocus();
    }
}

void dooble::slotTextChanged(const QString &text)
{
  Q_UNUSED(text);

  QIcon icon;
  QSettings settings
    (s_settings.value("iconSet").toString(), QSettings::IniFormat);

  icon = QIcon(settings.value("urlWidget/emptyIcon").toString());
  ui.locationLineEdit->setBookmarkButtonEnabled(false);
  ui.locationLineEdit->setIcon(icon);
  ui.locationLineEdit->setIconButtonEnabled(false);
  ui.locationLineEdit->setSecureColor(false);
  ui.locationLineEdit->setBookmarkColor(false);
  ui.locationLineEdit->setSpotOnButtonEnabled(false);
  ui.locationLineEdit->setSpotOnColor(false);

  if(ui.bookmarksMenu->actions().size() > 0)
    ui.bookmarksMenu->actions().at(0)->setEnabled(false);
}

void dooble::slotUrlChanged(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  ui.locationLineEdit->addItem(url.toString(QUrl::StripTrailingSlash));

  dview *p = qobject_cast<dview *> (sender());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      prepareWidgetsBasedOnView(p);
      ui.backToolButton->setEnabled(p->canGoBack());
      ui.forwardToolButton->setEnabled(p->canGoForward());

      bool isBookmarkWorthy = p->isBookmarkWorthy();

      ui.locationLineEdit->setBookmarkButtonEnabled(isBookmarkWorthy);
      ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
      ui.locationLineEdit->setIconButtonEnabled(isBookmarkWorthy);
      ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
      ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
      ui.locationLineEdit->setSpotOnColor(p->isLoaded());

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled(isBookmarkWorthy);
    }

  /*
  ** The URL widget's text should not be changed if the
  ** user has edited its contents. However, how will
  ** the widget reflect the current URL?
  */

  if(!ui.locationLineEdit->isModified())
    if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
      {
	ui.locationLineEdit->setText(url.toString(QUrl::StripTrailingSlash));

	if(!ui.locationLineEdit->text().isEmpty())
	  ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());
      }
}

void dooble::slotTitleChanged(const QString &titleArg)
{
  dview *p = qobject_cast<dview *> (sender());
  QString title(titleArg.trimmed());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      if(title.isEmpty())
	{
	  if(p->ipAddress().isEmpty() ||
	     !s_settings.value("settingsWindow/displayIpAddress",
			       false).toBool())
	    setWindowTitle(tr("Dooble Web Browser"));
	  else
	    setWindowTitle(QString(tr("Dooble Web Browser (%1)")).
			   arg(p->ipAddress()));
	}
      else
	{
	  if(p->ipAddress().isEmpty() ||
	     !s_settings.value("settingsWindow/displayIpAddress",
			       false).toBool())
	    setWindowTitle(title + tr(" - Dooble Web Browser"));
	  else
	    setWindowTitle(title + QString(" (%1)").arg(p->ipAddress()) +
			   tr(" - Dooble Web Browser"));
	}
    }

  if(p)
    {
      title = p->title();

      if(title.isEmpty())
	/*
	** The tab's title will be set to the URL.
	*/

	title = p->url().toString(QUrl::StripTrailingSlash);

      if(title.isEmpty())
	title = tr("(Untitled)");

      if(ui.tabWidget->indexOf(p) > -1)
	{
	  ui.tabWidget->setTabText(ui.tabWidget->indexOf(p),
				   title.replace("&", "&&"));
	  ui.tabWidget->setTabToolTip(ui.tabWidget->indexOf(p), title);
	}
    }
}

void dooble::slotIconChanged(void)
{
  dview *p = qobject_cast<dview *> (sender());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      ui.locationLineEdit->setIcon(p->icon());
      ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
    }

  if(p)
    {
      if(ui.tabWidget->indexOf(p) > -1)
	ui.tabWidget->animateIndex
	  (ui.tabWidget->indexOf(p), !p->isLoaded(), p->icon(),
	   p->progress(), !statusBar()->isHidden());

      int index = ui.locationLineEdit->findText
	(p->url().toString(QUrl::StripTrailingSlash));

      if(index > -1)
	ui.locationLineEdit->setItemIcon(index, p->icon());

      QList<QAction *> actions = ui.historyMenu->actions();

      if(actions.size() >= 6)
	for(int i = 5; i < actions.size(); i++)
	  if(actions.at(i)->data().toUrl().
	     toString(QUrl::StripTrailingSlash) ==
	     p->url().toString(QUrl::StripTrailingSlash))
	    {
	      actions.at(i)->setIcon(p->icon());
	      break;
	    }

      if(ui.favoritesToolBar->isVisible())
	foreach(QToolButton *toolButton,
		ui.favoritesToolButtonsFrame->findChildren<QToolButton *> ())
	  if(toolButton->property("url").toUrl().
	     toString(QUrl::StripTrailingSlash) ==
	     p->url().toString(QUrl::StripTrailingSlash))
	    {
	      toolButton->setIcon(p->icon());
	      break;
	    }
    }
}

void dooble::slotLoadProgress(int progress)
{
  dview *p = qobject_cast<dview *> (sender());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      sb.progressBar->setMaximum(100);
      sb.progressBar->setVisible(!p->isLoaded());
      ui.tabWidget->animateIndex
	(ui.tabWidget->indexOf(p), !p->isLoaded(), p->webviewIcon(),
	 p->progress(), !statusBar()->isHidden());
      ui.reloadStopWidget->setCurrentIndex(p->isLoaded() ? 1 : 0);
      ui.stopToolButton->setEnabled(!p->isLoaded());
      ui.actionStop->setEnabled(!p->isLoaded());
      ui.backToolButton->setEnabled(p->canGoBack());
      ui.forwardToolButton->setEnabled(p->canGoForward());
      sb.progressBar->setValue(qBound(sb.progressBar->minimum(),
				      progress,
				      sb.progressBar->maximum()));
      sb.statusLabel->setText(p->statusMessage());
    }
  else if(p)
    ui.tabWidget->animateIndex
      (ui.tabWidget->indexOf(p), !p->isLoaded(), p->webviewIcon(),
       p->progress(), !statusBar()->isHidden());
}

void dooble::slotLoadFinished(bool ok)
{
  dview *p = qobject_cast<dview *> (sender());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      sb.progressBar->setValue(0);
      sb.progressBar->setVisible(false);
      ui.reloadStopWidget->setCurrentIndex(1);
      ui.stopToolButton->setEnabled(false);
      ui.actionStop->setEnabled(false);
      ui.backToolButton->setEnabled(p->canGoBack());
      ui.forwardToolButton->setEnabled(p->canGoForward());

      if(ok)
	{
	  if(!ui.locationLineEdit->isModified())
	    {
	      ui.locationLineEdit->
		setText(p->url().toString(QUrl::StripTrailingSlash));

	      if(!ui.locationLineEdit->text().isEmpty())
		ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());
	    }
	}
      else if(p->url().toString(QUrl::StripTrailingSlash) ==
	      p->webviewUrl().toString(QUrl::StripTrailingSlash))
	{
	  ui.locationLineEdit->setIcon(p->icon());
	  ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
	  ui.locationLineEdit->setBookmarkButtonEnabled
	    (p->isBookmarkWorthy());
	  ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
	  ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
	  ui.locationLineEdit->setSpotOnColor(p->isLoaded());
	  sb.statusLabel->setText(p->statusMessage());
	}
    }

  if(!ok)
    {
      if(p)
	{
	  int index = ui.tabWidget->indexOf(p);

	  if(index > -1)
	    ui.tabWidget->animateIndex
	      (index, false, p->webviewIcon(),
	       p->progress(), !statusBar()->isHidden());

	  if(p->tabAction())
	    p->tabAction()->setIcon(p->webviewIcon());

	  if(p->isDir() &&
	     p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
	    slotOpenUrl();
	}

      return;
    }

  if(p)
    {
      if(p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
	{
	  ui.locationLineEdit->setBookmarkButtonEnabled
	    (p->isBookmarkWorthy());
	  ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
	  ui.locationLineEdit->setIconButtonEnabled(true);

	  if(ui.bookmarksMenu->actions().size() > 0)
	    ui.bookmarksMenu->actions().at(0)->setEnabled(true);

	  ui.locationLineEdit->setIcon(p->icon());
	  ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
	  ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
	  ui.locationLineEdit->setSpotOnColor(p->isLoaded());
	  sb.statusLabel->setText(p->statusMessage());
	}

      int index = ui.locationLineEdit->findText
	(p->url().toString(QUrl::StripTrailingSlash));

      if(index > -1)
	ui.locationLineEdit->setItemIcon(index, p->icon());

      if(ui.tabWidget->indexOf(p) > -1)
	ui.tabWidget->animateIndex
	  (ui.tabWidget->indexOf(p), false, p->icon(), p->progress(),
	   !statusBar()->isHidden());

      QString title(p->title());

      if(title.isEmpty())
	title = p->url().toString(QUrl::StripTrailingSlash);

      if(title.isEmpty())
	title = tr("(Untitled)");

      if(ui.tabWidget->indexOf(p) > -1)
	{
	  ui.tabWidget->setTabText(ui.tabWidget->indexOf(p),
				   title.replace("&", "&&"));
	  ui.tabWidget->setTabToolTip(ui.tabWidget->indexOf(p), title);
	}

      QAction *action = p->tabAction();

      if(action)
	{
	  action->setData(p->url());
	  action->setIcon(p->icon());

	  title = p->title();

	  if(title.isEmpty())
	    title = p->url().toString(QUrl::StripTrailingSlash);

	  title = dmisc::elidedTitleText(title);

	  if(title.isEmpty())
	    title = tr("(Untitled)");

	  action->setText(title);
	}

      QWebEngineHistoryItem item(p->page()->history()->currentItem());

      if(item.isValid() &&
	 dmisc::isSchemeAcceptedByDooble(item.url().scheme()))
	{
	  QBuffer buffer;
	  QByteArray bytes;

	  buffer.setBuffer(&bytes);

	  if(buffer.open(QIODevice::WriteOnly))
	    {
	      QDataStream out(&buffer);

	      out << dmisc::iconForUrl(item.url());

	      if(out.status() != QDataStream::Ok)
		bytes.clear();
	    }
	  else
	    bytes.clear();

	  buffer.close();

	  if(!s_settings.value("settingsWindow/"
			       "disableAllEncryptedDatabaseWrites", false).
	     toBool())
	    {
	      QList<QVariant> list;

	      list.append(item.url());
	      list.append(item.title());
	      list.append(bytes);
	      list.append(bytes);
	      list.append(dmisc::passphraseWasAuthenticated() ? 0 : 1);
	      list.append
		(dooble::s_settings.value("settingsWindow/rememberHistory",
					  true));
	      QtConcurrent::run
		(this, &dooble::saveHistoryThread, list);
	    }

	  if(!p->url().host().isEmpty())
	    s_mostVisitedHosts[p->url().host()] += 1;
	  else
	    s_mostVisitedHosts
	      [p->url().toString(QUrl::StripTrailingSlash)] += 1;

	  prepareMostVisited();
	}
    }

  /*
  ** Add items to the history menu.
  */

  if(p && dmisc::isSchemeAcceptedByDooble(p->url().scheme()) &&
     s_settings.value("settingsWindow/rememberHistory",
		      true).toBool())
    {
      int index = -1;
      bool actionExists = false;
      QString title(p->title());
      QList<QAction *> actions = ui.historyMenu->actions();

      if(title.isEmpty())
	title = p->url().toString(QUrl::StripTrailingSlash);

      title = dmisc::elidedTitleText(title);

      if(title.isEmpty())
	title = tr("Dooble Web Browser");

      if(actions.size() >= 6)
	for(int i = 5; i < actions.size(); i++)
	  if(actions.at(i)->data().toUrl().
	     toString(QUrl::StripTrailingSlash) ==
	     p->url().toString(QUrl::StripTrailingSlash))
	    {
	      index = i;
	      actions.at(i)->setIcon(p->icon());
	      actions.at(i)->setText(title);
	      actionExists = true;
	      break;
	    }

      if(!actionExists)
	{
	  if(ui.historyMenu->actions().size() == 4)
	    {
	      ui.historyMenu->addSeparator();
	      ui.historyMenu->actions().at(0)->setEnabled
		(ui.historyMenu->isEnabled());
	      ui.historyMenu->actions().at(1)->setEnabled(true);
	    }

	  /*
	  ** 5 = Clear History, Show History, a separator,
	  **     Recently-Closed Tabs, and a separator.
	  */

	  QAction *action = 0;

	  if(ui.historyMenu->actions().size() >= 5 + MAX_HISTORY_ITEMS)
	    {
	      action = ui.historyMenu->actions().value(5); /*
							   ** Notice use of
							   ** value().
							   */

	      for(int i = ui.historyMenu->actions().size() - 1; i >= 6; i--)
		{
		  ui.historyMenu->actions().at(i)->setData
		    (ui.historyMenu->actions().at(i - 1)->data());
		  ui.historyMenu->actions().at(i)->setIcon
		    (ui.historyMenu->actions().at(i - 1)->icon());
		  ui.historyMenu->actions().at(i)->setText
		    (ui.historyMenu->actions().at(i - 1)->text());
		}
	    }
	  else
	    {
	      action = new QAction(this);
	      connect(action, SIGNAL(triggered(void)), this,
		      SLOT(slotLinkActionTriggered(void)));

	      if(ui.historyMenu->actions().size() == 5)
		ui.historyMenu->addAction(action);
	      else if(ui.historyMenu->actions().size() >= 6)
		ui.historyMenu->insertAction
		  (ui.historyMenu->actions().at(5), action);
	    }

	  if(action)
	    {
	      action->setEnabled(ui.historyMenu->isEnabled());
	      action->setData(p->url());
	      action->setIcon(p->icon());
	      action->setText(title);
	    }
	}
      else
	{
	  /*
	  ** We need to promote the action's index.
	  */

	  QAction *action = ui.historyMenu->actions().value(index);

	  if(action)
	    {
	      ui.historyMenu->removeAction(action);
	      ui.historyMenu->insertAction
		(ui.historyMenu->actions().value(5), action); /*
							      ** Notice use
							      ** of value().
							      */
	    }
	}
    }

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      if(!ui.findLineEdit->hasFocus() &&
	 !ui.locationLineEdit->hasFocus() &&
	 !ui.locationLineEdit->isModified() &&
	 !ui.searchLineEdit->hasFocus() &&
	 !p->isDir() &&
	 !p->url().isEmpty() && p->url().isValid())
	p->setFocus();

      if(p->isDir())
	slotOpenUrl();
    }
}

void dooble::prepareNavigationButtonMenus(dview *p, QMenu *menu)
{
  if(!p || !menu)
    return;

  if(!s_settings.value("settingsWindow/rememberHistory", true).toBool())
    return;

  if(menu == ui.backToolButton->menu())
    {
      ui.backToolButton->menu()->clear();

      QList<QWebEngineHistoryItem> list(p->backItems(MAX_HISTORY_ITEMS));

      for(int i = list.size() - 1; i >= 0; i--)
	{
	  QUrl url(list.at(i).url());
	  QString title(list.at(i).title());
	  QString scheme(url.scheme().toLower().trimmed());

	  if(scheme.startsWith("dooble-ssl-"))
	    url.setScheme
	      (scheme.mid(static_cast<int> (qstrlen("dooble-ssl-"))));
	  else if(scheme.startsWith("dooble-"))
	    url.setScheme
	      (scheme.mid(static_cast<int> (qstrlen("dooble-"))));

	  title = dmisc::elidedTitleText(title);

	  if(title.isEmpty())
	    title = dmisc::elidedTitleText
	      (url.toString(QUrl::StripTrailingSlash));

	  if(title.isEmpty())
	    title = tr("Dooble Web Browser");

	  QIcon icon(dmisc::iconForUrl(url));
	  QAction *action = ui.backToolButton->menu()->addAction
	    (icon, title, this, SLOT(slotGoToBackHistoryItem(void)));

	  if(action)
	    action->setData(i);
	}
    }

  if(menu == ui.forwardToolButton->menu())
    {
      ui.forwardToolButton->menu()->clear();

      QList<QWebEngineHistoryItem> list(p->forwardItems(MAX_HISTORY_ITEMS));

      for(int i = 0; i < list.size(); i++)
	{
	  QUrl url(list.at(i).url());
	  QString title(list.at(i).title());
	  QString scheme(url.scheme().toLower().trimmed());

	  if(scheme.startsWith("dooble-ssl-"))
	    url.setScheme
	      (scheme.mid(static_cast<int> (qstrlen("dooble-ssl-"))));
	  else if(scheme.startsWith("dooble-"))
	    url.setScheme
	      (scheme.mid(static_cast<int> (qstrlen("dooble-"))));

	  if(title.isEmpty())
	    title = url.toString(QUrl::StripTrailingSlash);

	  title = dmisc::elidedTitleText(title);

	  if(title.isEmpty())
	    title = dmisc::elidedTitleText
	      (url.toString(QUrl::StripTrailingSlash));

	  if(title.isEmpty())
	    title = tr("Dooble Web Browser");

	  QIcon icon(dmisc::iconForUrl(url));
	  QAction *action = ui.forwardToolButton->menu()->addAction
	    (icon, title, this, SLOT(slotGoToForwardHistoryItem(void)));

	  if(action)
	    action->setData(i);
	}
    }
}

void dooble::slotGoHome(void)
{
  if(QApplication::keyboardModifiers() == Qt::ControlModifier)
    {
      QStringList allKeys = s_settings.keys();

      for(int i = 0; i < allKeys.size(); i++)
	{
	  if(!(allKeys.at(i).startsWith("settingsWindow/url") ||
	       allKeys.at(i).startsWith("settingsWindow/myRetrievedFiles")))
	    continue;

	  bool urlFound = false;
	  QUrl url
	    (QUrl::fromUserInput(s_settings.value(allKeys.at(i),
						  "").toString().trimmed()));

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

	  for(int j = 0; j < ui.tabWidget->count(); j++)
	    {
	      dview *p = qobject_cast<dview *> (ui.tabWidget->widget(j));

	      if(p && p->url().toString(QUrl::StripTrailingSlash) ==
		 url.toString(QUrl::StripTrailingSlash))
		{
		  urlFound = true;
		  break;
		}
	    }

	  if(!urlFound)
	    {
	      QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

	      webAttributes[QWebEngineSettings::JavascriptEnabled] =
		s_settings.value("settingsWindow/javascriptEnabled",
				 false).toBool();
	      newTab(url, 0, webAttributes);
	      ui.tabWidget->update();
	    }
	}
    }
  else
    {
      bool urlFound = false;
      QUrl url
	(QUrl::fromUserInput(s_settings.value("settingsWindow/homeUrl"
					      "").toString().trimmed()));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

      for(int j = 0; j < ui.tabWidget->count(); j++)
	{
	  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(j));

	  if(p && p->url().toString(QUrl::StripTrailingSlash) ==
	     url.toString(QUrl::StripTrailingSlash))
	    {
	      urlFound = true;
	      ui.tabWidget->setCurrentWidget(p);
	      break;
	    }
	}

      if(!urlFound)
	{
	  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;
	  dview *p = 0;

	  webAttributes[QWebEngineSettings::JavascriptEnabled] =
	    s_settings.value("settingsWindow/javascriptEnabled",
			     false).toBool();
	  p = newTab(url, 0, webAttributes);
	  ui.tabWidget->update();
	  ui.tabWidget->setCurrentWidget(p);
	}
    }
}

void dooble::closeTab(const int index)
{
  int count = ui.tabWidget->count();
  bool createNewTabAfter = false;

  if(count == 1 && s_instances <= 1)
    {
      if(qobject_cast<dreinstatedooble *> (ui.tabWidget->widget(index)))
	/*
	** We need to create a new tab, otherwise Dooble will exit.
	*/

	createNewTabAfter = true;
      else
	{
	  slotQuit();
	  return;
	}
    }
  else if(count == 1)
    {
      if(qobject_cast<dreinstatedooble *> (ui.tabWidget->widget(index)))
	/*
	** We need to create a new tab, otherwise Dooble will exit.
	*/

	createNewTabAfter = true;
      else
	{
	  close();
	  return;
	}
    }

  if(index < 0)
    return;

  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p && p->isModified() &&
     s_settings.value("settingsWindow/warnBeforeClosingModifiedTab",
		      false).toBool())
    {
      ui.tabWidget->setCurrentWidget(p);

      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
      mb.setWindowModality(Qt::WindowModal);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to close this modified "
		    "tab?"));

      QSettings settings(s_settings.value("iconSet").toString(),
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
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      if(mb.exec() == QMessageBox::No)
	return;
    }

  if(count > 1 || createNewTabAfter)
    {
      if(p)
	{
	  if(!p->url().isEmpty() && p->url().isValid() &&
	     s_settings.value("settingsWindow/rememberClosedTabs",
			      false).toBool())
	    {
	      m_closedTabs.prepend(p->history());

	      QString title(p->title());

	      if(title.isEmpty())
		title = p->url().toString(QUrl::StripTrailingSlash);

	      title = dmisc::elidedTitleText(title);

	      if(title.isEmpty())
		title = tr("Dooble Web Browser");

	      QAction *action = new QAction(p->icon(), title, this);

	      connect(action, SIGNAL(triggered(void)), this,
		      SLOT(slotReopenClosedTab(void)));

	      if(ui.historyMenu->actions().size() >= 4 &&
		 !ui.historyMenu->actions().at(3)->menu())
		ui.historyMenu->actions().at(3)->setMenu(new QMenu(this));

	      if(ui.historyMenu->actions().size() >= 4)
		{
		  if(ui.historyMenu->actions().at(3)->menu()->
		     actions().isEmpty())
		    {
		      QSettings settings
			(s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);

		      ui.historyMenu->actions().at(3)->menu()->addAction
			(QIcon(settings.value("mainWindow/historyMenu").
			       toString()), tr("&Clear"), this,
			 SLOT(slotClearRecentlyClosedTabs(void)));
		      ui.historyMenu->actions().at(3)->menu()->addSeparator();
		    }

		  ui.historyMenu->actions().at(3)->menu()->insertAction
		    (ui.historyMenu->actions().at(3)->menu()->actions().
		     value(2), action);
		}

	      /*
	      ** Enable the Clear History action.
	      */

	      if(ui.historyMenu->actions().size() >= 1)
		ui.historyMenu->actions().at(0)->setEnabled(true);

	      /*
	      ** Enable the Recently-Closed Tabs menu.
	      */

	      if(ui.historyMenu->actions().size() >= 4)
		ui.historyMenu->actions().at(3)->setEnabled(true);

	      /*
	      ** Remove outliers.
	      */

	      int number = s_settings.value
		("settingsWindow/rememberClosedTabsCount", 1).toInt();

	      if(number < 1 || number > MAX_HISTORY_ITEMS)
		number = 1;

	      for(int i = m_closedTabs.size() - 1; i >= number; i--)
		{
		  m_closedTabs.removeAt(i);

		  if(ui.historyMenu->actions().size() >= 4 &&
		     ui.historyMenu->actions().at(3)->menu() &&
		     ui.historyMenu->actions().at(3)->menu()->actions().
		     size() > 2)
		    ui.historyMenu->actions().at(3)->menu()->actions().
		      value(i + 2)->deleteLater();
		}
	    }

	  /*
	  ** The status bar should be reset if the user closed
	  ** a tab while the mouse cursor was on a link. We should only
	  ** clear the text if the tab that was closed was the current tab.
	  */

	  if(p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
	    sb.statusLabel->clear();

	  p->deleteLater();
	  ui.tabWidget->removeTab(index);
	}
      else if(qobject_cast<ddesktopwidget *> (ui.tabWidget->widget(index)))
	closeDesktop();
      else if(qobject_cast<dreinstatedooble *> (ui.tabWidget->widget(index)))
	{
	  dreinstatedooble *reinstateWidget = qobject_cast
	    <dreinstatedooble *> (ui.tabWidget->widget(index));

	  if(reinstateWidget)
	    reinstateWidget->deleteLater();
	}

      count -= 1;
    }

  if(createNewTabAfter)
    {
      slotNewTab();
      count += 1;
    }

  if(count == 1)
    ui.tabWidget->setBarVisible
      (s_settings.value("settingsWindow/alwaysShowTabBar",
			true).toBool());
  else
    ui.tabWidget->setBarVisible(true);

  ui.tabWidget->update();
}

void dooble::slotCloseTab(void)
{
  closeTab(ui.tabWidget->currentIndex());
}

void dooble::slotCloseTab(const int index)
{
  closeTab(index);
}

void dooble::slotOpenUrl(void)
{
  ui.locationLineEdit->setFocus();
  ui.locationLineEdit->selectAll();
  update();
}

void dooble::slotGoToBackHistoryItem(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p)
	{
	  int index = action->data().toInt();
	  QList<QWebEngineHistoryItem> list(p->backItems(MAX_HISTORY_ITEMS));

	  if(index >= 0 && index < list.size())
	    p->goToItem(list.at(index));
	}
    }
}

void dooble::slotGoToForwardHistoryItem(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p)
	{
	  int index = action->data().toInt();
	  QList<QWebEngineHistoryItem> list
	    (p->forwardItems(MAX_HISTORY_ITEMS));

	  if(index >= 0 && index < list.size())
	    p->goToItem(list.at(index));
	}
    }
}

void dooble::slotLinkActionTriggered(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      bool doLoad = true;
      QList<QWidget *> list(action->associatedWidgets());

      for(int i = 0; i < list.size(); i++)
	if(qobject_cast<dview *> (list.at(i)))
	  {
	    doLoad = false;
	    ui.tabWidget->setCurrentWidget(list.at(i));
	    break;
	  }

      if(doLoad)
	{
	  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

	  if(p && p->isModified() && !warnAboutLeavingModifiedTab())
	    return;

	  loadPage(action->data().toUrl());

	  if(ui.tabWidget->currentWidget())
	    ui.tabWidget->currentWidget()->setFocus();
	}
    }
}

void dooble::slotLoadStarted(void)
{
  dview *p = qobject_cast<dview *> (sender());

  if(p)
    if(ui.tabWidget->indexOf(p) > -1)
      ui.tabWidget->animateIndex(ui.tabWidget->indexOf(p), true, QIcon(),
				 p->progress(), !statusBar()->isHidden());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      ui.locationLineEdit->setIcon(dmisc::iconForUrl(QUrl()));
      ui.locationLineEdit->setIconButtonEnabled(false);

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled(false);

      if(p->tabAction())
	p->tabAction()->setIcon(dmisc::iconForUrl(QUrl()));

      sb.progressBar->setMaximum(100);
      sb.statusLabel->clear();
      sb.progressBar->setValue(0);
      sb.progressBar->setVisible(true);
      ui.reloadStopWidget->setCurrentIndex(0);
      ui.stopToolButton->setEnabled(true);
      ui.actionStop->setEnabled(true);
      ui.backToolButton->setEnabled(p->canGoBack());
      ui.forwardToolButton->setEnabled(p->canGoForward());
      ui.locationLineEdit->setSecureColor(false);
      ui.locationLineEdit->setBookmarkButtonEnabled
	(p->isBookmarkWorthy());
      ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
      ui.locationLineEdit->setSpotOnButtonEnabled(false);
      ui.locationLineEdit->setSpotOnColor(false);
    }
}

void dooble::slotSearch(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      bool post = false;
      QString urlText("");
      QString postText("");
      QString searchName(ui.searchLineEdit->type());

      if(searchName == "blekko")
	urlText = QString("https://www.blekko.com/ws/%1").arg
	  (ui.searchLineEdit->text().trimmed().replace(" ", "+"));
      else if(searchName == "dogpile")
	urlText = QString("https://www.dogpile.com/dogpile_other/ws/results/"
			  "Web/%1/1/417/TopNavigation/Relevance/iq=true/"
			  "zoom=off/_iceUrlFlag=7?_IceUrl=true").arg
	  (ui.searchLineEdit->text().trimmed().replace(" ", "+"));
      else if(searchName == "duckduckgo")
	/*
	** kd=1: Redirect (Prevent Information Sharing)
	** kg=p: Post
	** kh=1: HTTPS
	** kp=1: Safe Search
	*/

	urlText = QString("https://www.duckduckgo.com?q=%1&"
			  "kd=1&kg=p&kh=1&kp=1").
	  arg(ui.searchLineEdit->text().trimmed());
      else if(searchName == "google")
        urlText = QString("https://www.google.com/search?q=%1").arg
	  (ui.searchLineEdit->text().trimmed().replace(" ", "+"));
      else if(searchName == "ixquick")
	{
	  post = true;
	  urlText = "https://www.ixquick.com/do/metasearch.pl";
	  postText = QString("query=%1").
	    arg(ui.searchLineEdit->text().trimmed());
	}
      else if(searchName == "history")
	{
	  ui.actionShow_HistorySideBar->setChecked(true);
	  m_historySideBar->search(ui.searchLineEdit->text().toLower().
				   trimmed());
	  return;
	}
      else if(searchName == "metager")
	urlText = QString("https://www.metager.de/meta/cgi-bin/"
			  "meta.ger1?Enter=Search&wikipedia=on&"
			  "eingabe=%1").
	  arg(ui.searchLineEdit->text().trimmed());
      else if(searchName == "startpage")
	{
	  post = true;
	  urlText = "https://startpage.com/do/search";
	  postText = QString("query=%1").
	    arg(ui.searchLineEdit->text().trimmed());
	}
      else if(searchName == "wikibooks")
	{
	  QLocale locale;

	  urlText = QString
	    ("https://%1.wikibooks.org/w/index.php?"
	     "search=%2&title=Special%3ASearch&go=Go").
	    arg(locale.name().left(2)).
	    arg(ui.searchLineEdit->text().trimmed().replace(" ", "+"));
	}
      else if(searchName == "wikinews")
	{
	  QLocale locale;

	  urlText = QString
	    ("https://secure.wikimedia.org/wikinews/%1/w/index.php?"
	     "search=%2").
	    arg(locale.name().left(2)).
	    arg(ui.searchLineEdit->text().trimmed().replace(" ", "+"));
	}
      else if(searchName == "wikipedia")
	{
	  QLocale locale;

	  urlText = QString
            ("https://%1.wikipedia.org/wiki/Special:"
	     "Search?search=%2").
	    arg(locale.name().left(2)).
	    arg(ui.searchLineEdit->text().trimmed().replace(" ", "+"));
	}
      else if(searchName == "wolframalpha")
	urlText = QString("https://www.wolframalpha.com/input/?i=%1").
	  arg(ui.searchLineEdit->text().trimmed().replace(" ", "+"));
      else if(searchName == "yacy")
        urlText = QString
	  ("http://localhost:8080/yacysearch.html?display=2&count=100"
	   "&resource=global&query=%1").arg
	  (ui.searchLineEdit->text().trimmed().replace(" ", "+"));
      else
	/*
	** Unknown!
	*/

	return;

      QUrl url(QUrl::fromUserInput(urlText));

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

      if(post)
	p->post(url, postText);
      else
	p->load(url);
    }
}

void dooble::slotCopyLink(const QUrl &url)
{
  QApplication::clipboard()->setText(url.toString(QUrl::StripTrailingSlash));
}

void dooble::slotOpenLinkInNewTab(const QUrl &url)
{
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

  webAttributes[QWebEngineSettings::JavascriptEnabled] =
    s_settings.value("settingsWindow/javascriptEnabled",
		     false).toBool();
  slotOpenLinkInNewTab(url, 0, webAttributes);
}

void dooble::slotOpenLinkInNewTab
(const QUrl &url, dcookies *cookies,
 const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes)
{
  dview *p = newTab(url, cookies, webAttributes);

  if(p)
    {
      if(s_settings.value("settingsWindow/proceedToNewTab",
			  true).toBool())
	ui.tabWidget->setCurrentWidget(p);

      ui.tabWidget->update();
    }
}

void dooble::slotOpenLinkInNewWindow(const QUrl &url)
{
  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

  webAttributes[QWebEngineSettings::JavascriptEnabled] =
    s_settings.value("settingsWindow/javascriptEnabled",
		     false).toBool();
  slotOpenLinkInNewWindow(url, 0, webAttributes);
}

void dooble::slotOpenLinkInNewWindow
(const QUrl &url, dcookies *cookies,
 const QHash<QWebEngineSettings::WebAttribute, bool> &webAttributes)
{
  if(s_settings.value("settingsWindow/openUserWindowsInNewProcesses",
		      false).toBool())
    launchDooble(url);
  else
    {
      Q_UNUSED(new dooble(url, this, cookies, webAttributes));
    }
}

void dooble::slotOpenPageInNewWindow(const int index)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p)
    {
      int count = ui.tabWidget->count() - 1;

      if(s_settings.value("settingsWindow/openUserWindowsInNewProcesses",
			  false).toBool())
	{
	  launchDooble(p->url());
	  p->deleteLater();
	}
      else
	{
	  Q_UNUSED(new dooble(p, this));
	}

      ui.tabWidget->setTabsClosable(count > 1);
    }
}

void dooble::slotLinkHovered(const QString &link)
{
  if(link.isEmpty())
    sb.statusLabel->clear();
  else
    sb.statusLabel->setText(link.trimmed().mid(0, 100));
}

void dooble::slotAbout(void)
{
  setUrlHandler(this);

  QMessageBox *mb = findChild<QMessageBox *> ("about");

  if(mb)
    mb->deleteLater();

  mb = new QMessageBox(this);
  mb->setObjectName("about");
  mb->setWindowModality(Qt::NonModal);
  mb->setStyleSheet("QMessageBox {background: white;}");
  mb->setWindowTitle(tr("Dooble Web Browser: About"));
  mb->setTextFormat(Qt::RichText);
  mb->setText
    (QString("<html>"
             "Dooble Web Browser<br><br>"
             "Version %1, Violet Moonbeams.<br>"
	     "Copyright (c) 2008 - present.<br>"
	     "Qt version %3, architecture %4.<br>"
	     "libgcrypt version %5."
	     "<hr>"
	     "Please visit <a href=\"http://dooble.sf.net\">"
	     "http://dooble.sf.net</a> for more information."
	     "<hr>"
	     "Please visit <a href=\"http://spot-on.sf.net\">"
	     "http://spot-on.sf.net</a> for information regarding "
	     "the Spot-On project."
	     "<hr>"
	     "Are you interested in the latest "
	     "<a href=\"qrc:/Documentation/RELEASE-NOTES.html\">"
	     "release notes</a>?"
	     "</html>").
     arg(DOOBLE_VERSION_STR).
     arg(QT_VERSION_STR).
     arg(ARCHITECTURE_STR).
     arg(GCRYPT_VERSION));
  mb->setStandardButtons(QMessageBox::Ok);

  QSettings settings
    (s_settings.value("iconSet").toString(), QSettings::IniFormat);

  mb->setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < mb->buttons().size(); i++)
    if(mb->buttonRole(mb->buttons().at(i)) == QMessageBox::AcceptRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::ApplyRole ||
       mb->buttonRole(mb->buttons().at(i)) == QMessageBox::YesRole)
      {
	mb->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	mb->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  mb->setIconPixmap(QPixmap("Icons/AxB/dooble.png"));
  mb->show();
}

void dooble::slotSavePage(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      QAction *action = qobject_cast<QAction *> (sender());

      if(action == ui.actionSave_Page_as_Data_URI ||
	 p->isDir() || p->isFtp() ||
	 p->url().scheme().toLower().trimmed() == "qrc")
	{
	  QByteArray html;

	  if(action == ui.actionSave_Page_as_Data_URI)
	    {
	      html.append("data:text/html;charset=utf-8;base64,");
	      html.append(p->html().toUtf8().toBase64());
	    }
	  else
	    html.append(p->html());

	  saveHandler(p->url(), html.constData(), "", 0);
	}
      else
	saveHandler(p->url(), "", "", 0);
    }
}

void dooble::slotSaveUrl(const QUrl &url, const int choice)
{
  saveHandler(url, "", "", choice);
}

void dooble::slotSaveFile(const QString &fileName, const QUrl &url,
			  const int choice)
{
  saveHandler(url, "", fileName, choice);
}

void dooble::saveHandler(const QUrl &url, const QString &html,
			 const QString &fileName,
			 const int choice)
{
  QString path
    (s_settings.value("settingsWindow/myRetrievedFiles", "").toString());
  QWidget *parent = qobject_cast<QWidget *> (sender());
  QFileInfo fileInfo(path);

  if(!fileInfo.isReadable() || !fileInfo.isWritable())
    path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

  if(!parent)
    parent = this;

  QString l_fileName(fileName);

  /*
  ** This fileDialog business is troublesome. At times,
  ** when the dialog is closed, Dooble will terminate.
  */

  QFileDialog *fileDialog = new QFileDialog(parent);

#ifdef Q_OS_MAC
  fileDialog->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog->setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog->setWindowTitle(tr("Dooble Web Browser: Save As"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setDirectory(path);
  fileDialog->setLabelText(QFileDialog::Accept, tr("Save"));
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);

  if(url.path().isEmpty() || url.path() == "/")
    {
      if(l_fileName.isEmpty())
	l_fileName = url.host();

      if(l_fileName.isEmpty())
	l_fileName = "dooble.download";

      l_fileName = dmisc::findUniqueFileName
	(l_fileName, fileDialog->directory());
      fileDialog->selectFile(l_fileName);
    }
  else if(url.path().contains("/"))
    {
      if(l_fileName.isEmpty())
	l_fileName = QFileInfo(url.path()).fileName();

      if(l_fileName.isEmpty())
	l_fileName = url.host();

      if(l_fileName.isEmpty())
	l_fileName = "dooble.download";

      l_fileName = dmisc::findUniqueFileName
	(l_fileName, fileDialog->directory());
      fileDialog->selectFile(l_fileName);
    }

  /*
  ** We sometimes terminate on this very spot.
  ** Are we not allowed to start an event loop
  ** from a function that is called by a slot?
  */

  /*
  ** Now for a workaround.
  */

  connect(fileDialog,
	  SIGNAL(finished(int)),
	  this,
	  SLOT(slotHandleQuirkySaveDialog(int)));
  m_saveDialogHackContainer.url = url;
  m_saveDialogHackContainer.html = html;
  m_saveDialogHackContainer.choice = choice;
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setWindowModality(Qt::WindowModal);
  fileDialog->show();
}

void dooble::slotHandleQuirkySaveDialog(int result)
{
  QFileDialog *fileDialog = qobject_cast<QFileDialog *> (sender());

  if(!fileDialog)
    return;

  /*
  ** You should not be here when their parents are out!
  */

  if(result == QDialog::Accepted)
    {
      QStringList list(fileDialog->selectedFiles());

      if(!list.isEmpty())
	{
	  s_downloadWindow->show(this);

	  QString fileName(QFileInfo(list.value(0)).absoluteFilePath());

	  if(!m_saveDialogHackContainer.html.isEmpty())
	    s_downloadWindow->addHtmlItem
	      (m_saveDialogHackContainer.html, fileName);
	  else if(!m_saveDialogHackContainer.url.scheme().toLower().trimmed().
		  startsWith("file"))
	    s_downloadWindow->addUrlItem
	      (m_saveDialogHackContainer.url, fileName, true,
	       QDateTime::currentDateTime(),
	       m_saveDialogHackContainer.choice);
	  else
	    s_downloadWindow->addFileItem
	      (m_saveDialogHackContainer.url.toLocalFile(), fileName,
	       true, QDateTime::currentDateTime());
	}
    }

  fileDialog->deleteLater();
}

void dooble::slotDisplayDownloadWindow(void)
{
  s_downloadWindow->show(this);
}

void dooble::slotShowFind(void)
{
  if(qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      showFindFrame = true;
      ui.findFrame->setVisible(true);
      ui.actionFind->setEnabled(true);

      if(ui.tabWidget->isVisible())
	{
	  ui.findLineEdit->setFocus();
	  ui.findLineEdit->selectAll();

#ifdef Q_OS_MAC
	  static int fixed = 0;

	  if(!fixed)
	    {
	      QColor color(240, 128, 128); // Light Coral!
	      QPalette palette(ui.findLineEdit->palette());

	      palette.setColor(ui.findLineEdit->backgroundRole(), color);
	      ui.findLineEdit->setPalette(palette);
	      fixed = 1;
	    }
#endif
	  update();
	}
    }
}

void dooble::slotHideFind(void)
{
  showFindFrame = false;
  ui.findFrame->setVisible(false);
}

void dooble::slotFindNext(void)
{
  if(ui.matchCaseCheckBox->isChecked())
    find(QWebEnginePage::FindCaseSensitively);
  else
    find(QWebEnginePage::FindCaseSensitively);
}

void dooble::slotFindPrevious(void)
{
  if(ui.matchCaseCheckBox->isChecked())
    find(QWebEnginePage::FindFlag(QWebEnginePage::FindBackward |
				  QWebEnginePage::FindCaseSensitively));
  else
    find(QWebEnginePage::FindBackward);
}

void dooble::slotFind(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      p->page()->findText(QString(""), 0);

      if(ui.highlightAllCheckBox->isChecked())
	find(0);
    }
}

void dooble::find(QWebEnginePage::FindFlags flags)
{
  QString text(ui.findLineEdit->text());
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    p->page()->findText(text, flags);
}

void dooble::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      QKeySequence userKeys(event->modifiers() + event->key());
      bool processed = false;

      if(userKeys == QKeySequence(Qt::ControlModifier + Qt::Key_B))
	{
	  processed = true;
	  slotShowBookmarks();
	}
      else if(event->key() == Qt::Key_Escape && ui.findFrame->isVisible())
	{
	  processed = true;
	  showFindFrame = false;
	  ui.findFrame->setVisible(false);
	}
      else if(event->key() == Qt::Key_Escape)
	{
	  processed = true;
	  slotStop();
	}
      else if(userKeys ==
	      QKeySequence(Qt::ControlModifier + Qt::ShiftModifier +
			   Qt::Key_Plus))
	{
	  processed = true;
	  ui.actionZoom_In->trigger();
	}
      else
	{
	  int index = 0;
	  QList<QKeySequence> list;

	  list << QKeySequence(Qt::AltModifier + Qt::Key_0)
	       << QKeySequence(Qt::AltModifier + Qt::Key_1)
	       << QKeySequence(Qt::AltModifier + Qt::Key_2)
	       << QKeySequence(Qt::AltModifier + Qt::Key_3)
	       << QKeySequence(Qt::AltModifier + Qt::Key_4)
	       << QKeySequence(Qt::AltModifier + Qt::Key_5)
	       << QKeySequence(Qt::AltModifier + Qt::Key_6)
	       << QKeySequence(Qt::AltModifier + Qt::Key_7)
	       << QKeySequence(Qt::AltModifier + Qt::Key_8)
	       << QKeySequence(Qt::AltModifier + Qt::Key_9);
	  index = list.indexOf(userKeys);

	  if(index == 0)
	    {
	      processed = true;
	      ui.tabWidget->setCurrentIndex(ui.tabWidget->count() - 1);
	    }
	  else if(index > 0)
	    {
	      processed = true;
	      ui.tabWidget->setCurrentIndex(index - 1);
	    }
	}

      if(!processed && (isFullScreen() || m_isJavaScriptWindow))
	{
	  /*
	  ** OK, this is going to be tricky.
	  ** Grab all of the actions.
	  ** For every action that is enabled, determine
	  ** its shortcuts. If a shortcut matches the event's key
	  ** and the event's modifier, issue the action's trigger() method.
	  */

#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
	  if(QKeySequence(event->modifiers() + event->key()) ==
	     QKeySequence(Qt::ControlModifier + Qt::Key_Equal) ||
	     QKeySequence(event->modifiers() + event->key()) ==
	     QKeySequence(Qt::ControlModifier + Qt::ShiftModifier +
			  Qt::Key_Plus))
	    {
	      ui.actionZoom_In->trigger();
	      goto done_label;
	    }
#endif

	  QKeySequence shortcut(event->modifiers() + event->key());

	  foreach(QAction *action, findChildren<QAction *> ())
	    if(!action->isSeparator() && action->isEnabled())
	      {
		QList<QKeySequence> shortcuts(action->shortcuts());

		for(int i = 0; i < shortcuts.size(); i++)
		  if(shortcut == shortcuts.at(i))
		    {
		      action->trigger();
		      goto done_label;
		    }
	      }
	}
    }

 done_label:
  QMainWindow::keyPressEvent(event);
}

void dooble::slotPrint(void)
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog printDialog(&printer, this);

#ifdef Q_OS_MAC
  printDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif

  if(printDialog.exec() == QDialog::Accepted)
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p)
	p->slotPrint(&printer);
    }
}

void dooble::slotPrintRequested(QWebEnginePage *frame)
{
  Q_UNUSED(frame);
}

void dooble::slotPrintPreview(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      QPrinter printer(QPrinter::HighResolution);
      QPrintPreviewDialog printDialog(&printer, this);

#ifdef Q_OS_MAC
      printDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      printDialog.setWindowModality(Qt::WindowModal);
      connect(&printDialog,
	      SIGNAL(paintRequested(QPrinter *)),
	      p,
	      SLOT(slotPrint(QPrinter *)));
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      printDialog.show();
      QApplication::restoreOverrideCursor();
      printDialog.exec();
    }
}

void dooble::slotShowDesktopTab(const bool state)
{
  if(!m_desktopWidget)
    {
      m_desktopWidget = new ddesktopwidget(this);
      connect(m_desktopWidget,
	      SIGNAL(backgroundImageChanged(void)),
	      this,
	      SLOT(slotChangeDesktopBackgrounds(void)));

      QSettings settings
	(s_settings.value("iconSet").toString(), QSettings::IniFormat);

      settings.beginGroup("mainWindow");

      int index = -1;

      if(s_settings.value("settingsWindow/appendNewTabs",
			  false).toBool())
	index = ui.tabWidget->addTab
	  (m_desktopWidget,
	   QIcon(settings.value("tabWidget").toString()),
	   tr("Dooble Desktop"));
      else
	index = ui.tabWidget->insertTab
	  (ui.tabWidget->currentIndex() + 1,
	   m_desktopWidget,
	   QIcon(settings.value("tabWidget").toString()),
	   tr("Dooble Desktop"));

      ui.tabWidget->setTabToolTip(index, tr("Dooble Desktop"));
      ui.desktopToolButton->setEnabled(false);
      ui.tabWidget->setTabsClosable(ui.tabWidget->count() > 1);

      QAction *action = new QAction(tr("Dooble Desktop"), this);

      action->setData("Desktop");
      action->setIcon
	(QIcon(settings.value("actionDesktop").toString()));
      connect(action, SIGNAL(triggered(void)), this,
	      SLOT(slotShowDesktopTab(void)));
      m_desktopWidget->setTabAction(action);
      prepareTabsMenu();

      if(state)
	ui.tabWidget->setCurrentWidget(m_desktopWidget);

      if(ui.tabWidget->count() == 1)
	ui.tabWidget->setBarVisible
	  (s_settings.value("settingsWindow/alwaysShowTabBar",
			    true).toBool());
      else
	ui.tabWidget->setBarVisible(true);
    }
  else
    ui.tabWidget->setCurrentWidget(m_desktopWidget);
}

void dooble::slotShowPageSource(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->currentFrame())
    {
      dpagesourcewindow *ps = new dpagesourcewindow
	(this,
	 p->url(),
	 p->html());

      connect(this,
	      SIGNAL(iconsChanged(void)),
	      ps,
	      SLOT(slotSetIcons(void)));

      /*
      ** The dpagesourcewindow object automatically destroys
      ** itself whenever it is closed.
      */
    }
}

QList<QVariantList> dooble::tabUrls(void) const
{
  QList<QVariantList> urls;

  for(int i = 0; i < ui.tabWidget->count(); i++)
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->widget(i));

      if(p)
	{
	  QString title(p->title());

	  if(title.isEmpty())
	    title = p->url().toString(QUrl::StripTrailingSlash);

	  title = dmisc::elidedTitleText(title);

	  if(title.isEmpty())
	    title = tr("(Untitled)");

	  QVariantList list;

	  list.append(title);
	  list.append(p->url());
	  list.append(p->icon());
	  urls.append(list);
	}
    }

  return urls;
}

void dooble::slotShowSettingsWindow(void)
{
  s_settingsWindow->exec(this);
}

void dooble::slotViewZoomIn(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    p->setZoomFactor(p->zoomFactor() + 0.1);
}

void dooble::slotViewZoomOut(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());
	
  if(p)
    p->setZoomFactor(p->zoomFactor() - 0.1);
}

void dooble::slotViewResetZoom(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    p->setZoomFactor(1.0);
}

void dooble::slotStatusBarDisplay(bool state)
{
  if(isFullScreen())
    return;

  statusBar()->setVisible(state);

  QSettings settings;

  settings.setValue("mainWindow/statusbarDisplay", state);
  s_settings["mainWindow/statusbarDisplay"] = state;
}

void dooble::slotShowHiddenFiles(bool state)
{
  QSettings settings;

  settings.setValue("mainWindow/showHiddenFiles", state);
  s_settings["mainWindow/showHiddenFiles"] = state;

  if(state)
    {
      if(dfilemanager::treeModel)
	dfilemanager::treeModel->setFilter
	  (dfilemanager::treeModel->filter() | QDir::Hidden);

      if(dfilemanager::tableModel)
	dfilemanager::tableModel->setFilter
	  (dfilemanager::tableModel->filter() | QDir::Hidden);
    }
  else
    {
      if(dfilemanager::treeModel)
	dfilemanager::treeModel->setFilter
	  (dfilemanager::treeModel->filter() & ~QDir::Hidden);

      if(dfilemanager::tableModel)
	dfilemanager::tableModel->setFilter
	  (dfilemanager::tableModel->filter() & ~QDir::Hidden);
    }
}

void dooble::closeDesktop(void)
{
  /*
  ** Destroy the widget that is housing the Desktop.
  */

  for(int i = 0; i < ui.tabWidget->count(); i++)
    if(qobject_cast<ddesktopwidget *> (ui.tabWidget->widget(i)))
      {
	ui.tabWidget->removeTab(i);
	ui.desktopToolButton->setEnabled(true);
	break;
      }

  for(int i = 0; i < ui.menu_Tabs->actions().size(); i++)
    if(ui.menu_Tabs->actions().at(i)->data().toString() == "Desktop")
      {
	ui.menu_Tabs->removeAction(ui.menu_Tabs->actions().at(i));
	break;
      }

  ui.tabWidget->setTabsClosable(ui.tabWidget->count() > 1);

  if(m_desktopWidget)
    m_desktopWidget->deleteLater();
}

void dooble::slotOpenDirectory(void)
{
  QFileDialog fileDialog(this);

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setWindowModality(Qt::WindowModal);
  fileDialog.setWindowTitle(tr("Dooble Web Browser: Open Directory"));
  fileDialog.setFileMode(QFileDialog::Directory);
  fileDialog.setDirectory(QDir::homePath());
  fileDialog.setLabelText(QFileDialog::Accept, tr("Open"));
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QStringList list(fileDialog.selectedFiles());

      if(!list.isEmpty())
	{
	  if(qobject_cast<ddesktopwidget *> (ui.tabWidget->currentWidget()))
	    {
	      if(m_desktopWidget)
		{
		  QUrl url(QUrl::fromLocalFile(list.at(0)));

		  url = QUrl::fromEncoded
		    (url.toEncoded(QUrl::StripTrailingSlash));
		  m_desktopWidget->showFileManagerWindow(url);
		}
	    }
	  else
	    {
	      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

	      if(p && p->isModified() && !warnAboutLeavingModifiedTab())
		return;

	      QUrl url(QUrl::fromLocalFile(list.at(0)));

	      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	      loadPage(url);

	      if(ui.tabWidget->currentWidget())
		ui.tabWidget->currentWidget()->setFocus();
	    }
	}
    }
}

void dooble::slotOpenMyRetrievedFiles(void)
{
  if(qobject_cast<ddesktopwidget *> (ui.tabWidget->currentWidget()))
    {
      if(m_desktopWidget)
	m_desktopWidget->showFileManagerWindow();
    }
  else
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p && p->isModified() && !warnAboutLeavingModifiedTab())
	return;

      QUrl url;
      QString path(s_settings.value("settingsWindow/myRetrievedFiles", "").
		   toString());

      url = QUrl::fromLocalFile(path);
      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      loadPage(url);

      if(ui.tabWidget->currentWidget())
	ui.tabWidget->currentWidget()->setFocus();
    }
}

void dooble::slotChangeDesktopBackgrounds(void)
{
  foreach(QWidget *widget, QApplication::topLevelWidgets())
    if(qobject_cast<dooble *> (widget))
      {
	dooble *d = qobject_cast<dooble *> (widget);

	if(d)
	  emit d->updateDesktopBackground();
      }
}

void dooble::slotCopy(void)
{
  if(ui.searchLineEdit->hasFocus() &&
     !ui.searchLineEdit->selectedText().isEmpty())
    ui.searchLineEdit->copy();
  else if(ui.locationLineEdit->hasFocus() &&
	  !ui.locationLineEdit->selectedText().isEmpty())
    ui.locationLineEdit->copy();
  else
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p)
	p->page()->triggerAction(QWebEnginePage::Copy);
    }
}

void dooble::slotPaste(void)
{
  if(ui.searchLineEdit->hasFocus())
    ui.searchLineEdit->paste();
  else if(ui.locationLineEdit->hasFocus())
    ui.locationLineEdit->paste();
  else
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

      if(p)
	p->page()->triggerAction(QWebEnginePage::Paste);
    }
}

void dooble::slotSelectAllContent(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    p->page()->triggerAction(QWebEnginePage::SelectAll);
}

void dooble::slotAuthenticationRequired(const QUrl &url,
					QAuthenticator *authenticator)
{
  if(!authenticator)
    return;

  QDialog dialog(this);
  Ui_passwordDialog ui_p;

  ui_p.setupUi(&dialog);
#ifdef Q_OS_MAC
  dialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  ui_p.messageLabel->setText
    (QString(tr("The site %1 is requesting "
		"credentials.").
	     arg(url.
		 toString(QUrl::RemovePath | QUrl::RemovePassword |
			  QUrl::RemoveQuery | QUrl::StripTrailingSlash))));

  QSettings settings(s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  dialog.setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < ui_p.buttonBox->buttons().size(); i++)
    if(ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::AcceptRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::ApplyRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::YesRole)
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(true);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(false);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setUser(ui_p.usernameLineEdit->text());
      authenticator->setPassword(ui_p.passwordLineEdit->text());
    }
  else
    *authenticator = QAuthenticator(); // Cancel repetitions.
}

void dooble::slotProxyAuthenticationRequired(const QUrl &url,
					     QAuthenticator *authenticator,
					     const QString &proxyHost)
{
  if(!authenticator)
    return;

  Q_UNUSED(url);
  QDialog dialog(this);
  Ui_passwordDialog ui_p;

  ui_p.setupUi(&dialog);
#ifdef Q_OS_MAC
  dialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  ui_p.messageLabel->setText
    (QString(tr("The proxy %1 is requesting "
		"credentials.").
	     arg(proxyHost)));

  QSettings settings(s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  dialog.setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < ui_p.buttonBox->buttons().size(); i++)
    if(ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::AcceptRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::ApplyRole ||
       ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::YesRole)
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(true);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i)))
	  qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().at(i))->
	    setDefault(false);

	ui_p.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setUser(ui_p.usernameLineEdit->text());
      authenticator->setPassword(ui_p.passwordLineEdit->text());
    }
  else
    *authenticator = QAuthenticator(); // Cancel repetitions.
}

void dooble::slotSelectionChanged(const QString &text)
{
  Q_UNUSED(text);
}

void dooble::slotSelectionChanged(void)
{
}

void dooble::slotEnablePaste(void)
{
}

void dooble::slotIpAddressChanged(const QString &ipAddress)
{
  dview *p = qobject_cast<dview *> (sender());

  if(p && p == qobject_cast<dview *> (ui.tabWidget->currentWidget()))
    {
      if(p->title().isEmpty())
       {
         if(ipAddress.isEmpty() ||
            !s_settings.value("settingsWindow/displayIpAddress",
                              false).toBool())
           setWindowTitle(tr("Dooble Web Browser"));
         else
           setWindowTitle(QString(tr("Dooble Web Browser (%1)")).
                          arg(ipAddress));
       }
      else
       {
         if(ipAddress.isEmpty() ||
            !s_settings.value("settingsWindow/displayIpAddress",
                              false).toBool())
           setWindowTitle(p->title() + tr(" - Dooble Web Browser"));
         else
           setWindowTitle(p->title() + QString(" (%1)").arg(ipAddress) +
                          tr(" - Dooble Web Browser"));
       }
    }
}

void dooble::slotShowHistory(void)
{
  /*
  ** The current window should be the artificial parent.
  */

  disconnect(s_historyWindow, SIGNAL(open(const QUrl &)),
	     this, SLOT(slotLoadPage(const QUrl &)));
  disconnect(s_historyWindow, SIGNAL(createTab(const QUrl &)),
	     this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  disconnect(s_historyWindow, SIGNAL(openInNewWindow(const QUrl &)),
	     this, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
  connect(s_historyWindow, SIGNAL(open(const QUrl &)),
	  this, SLOT(slotLoadPage(const QUrl &)));
  connect(s_historyWindow, SIGNAL(createTab(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(s_historyWindow, SIGNAL(openInNewWindow(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
  s_historyWindow->show(this);
}

void dooble::slotShowBookmarks(void)
{
  /*
  ** The current window should be the artificial parent.
  */

  disconnect(s_bookmarksWindow, SIGNAL(open(const QUrl &)),
	     this, SLOT(slotLoadPage(const QUrl &)));
  disconnect(s_bookmarksWindow, SIGNAL(createTab(const QUrl &)),
	     this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  disconnect(s_bookmarksWindow, SIGNAL(openInNewWindow(const QUrl &)),
	     this, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
  connect(s_bookmarksWindow, SIGNAL(open(const QUrl &)),
	  this, SLOT(slotLoadPage(const QUrl &)));
  connect(s_bookmarksWindow, SIGNAL(createTab(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewTab(const QUrl &)));
  connect(s_bookmarksWindow, SIGNAL(openInNewWindow(const QUrl &)),
	  this, SLOT(slotOpenLinkInNewWindow(const QUrl &)));
  s_bookmarksWindow->show(this);
}

void dooble::slotResetUrl(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      QString title(p->title());

      if(title.isEmpty())
	/*
	** The tab's title will be set to the URL.
	*/

	title = p->url().toString(QUrl::StripTrailingSlash);

      if(title.isEmpty())
	title = tr("(Untitled)");

      if(ui.tabWidget->indexOf(p) > -1)
	{
	  ui.tabWidget->setTabText(ui.tabWidget->indexOf(p),
				   title.replace("&", "&&"));
	  ui.tabWidget->setTabToolTip(ui.tabWidget->indexOf(p), title);
	  ui.tabWidget->animateIndex
	    (ui.tabWidget->indexOf(p), !p->isLoaded(), p->webviewIcon(),
	     p->progress(), !statusBar()->isHidden());
	}

      ui.locationLineEdit->setText
	(p->url().toString(QUrl::StripTrailingSlash));

      if(!ui.locationLineEdit->text().isEmpty())
	ui.locationLineEdit->setToolTip(ui.locationLineEdit->text());

      ui.locationLineEdit->setBookmarkButtonEnabled
	(dmisc::isSchemeAcceptedByDooble(p->url().scheme()));
      ui.locationLineEdit->setIcon(p->icon());
      ui.locationLineEdit->setIconButtonEnabled
	(dmisc::isSchemeAcceptedByDooble(p->url().scheme()));
      ui.locationLineEdit->setSecureColor(p->hasSecureConnection());
      ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
      ui.locationLineEdit->setSpotOnButtonEnabled(p->isLoaded());
      ui.locationLineEdit->setSpotOnColor(p->isLoaded());

      if(ui.bookmarksMenu->actions().size() > 0)
	ui.bookmarksMenu->actions().at(0)->setEnabled
	  (dmisc::isSchemeAcceptedByDooble(p->url().scheme()));

      slotOpenUrl();
    }
}

void dooble::slotCloseWindow(void)
{
  /*
  ** This method is connected to the windowCloseRequested() signal.
  */

  if(sender() && sender()->parent() && ui.tabWidget->count() > 1)
    {
      dview *p = qobject_cast<dview *> (sender()->parent()->parent());

      if(p)
	if(ui.tabWidget->indexOf(p) > -1)
	  closeTab(ui.tabWidget->indexOf(p));
    }
  else
    close();
}

void dooble::launchDooble(const QUrl &url)
{
  QStringList arguments;

  if(!url.isEmpty() && url.isValid())
    arguments << url.toString(QUrl::StripTrailingSlash);

  QProcess::startDetached
    (QCoreApplication::applicationDirPath() +
     QDir::separator() + QCoreApplication::applicationName(),
     arguments);
}

void dooble::slotTabMoved(int from, int to)
{
  if(from < 0 || to < 0)
    return;

  if(dmisc::passphraseWasAuthenticated() &&
     s_settings.value("settingsWindow/sessionRestoration", true).toBool())
    {
      dmisc::removeRestorationFiles(s_id, m_id);

      for(int i = 0; i < ui.tabWidget->count(); i++)
	{
	  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(i));

	  if(p)
	    p->recordRestorationHistory();
	}
    }

  if(from < ui.menu_Tabs->actions().size() &&
     to < ui.menu_Tabs->actions().size())
    {
      QAction *toAction = ui.menu_Tabs->actions().at(to);
      QAction *fromAction = ui.menu_Tabs->actions().at(from);

      if(to < from)
	{
	  ui.menu_Tabs->removeAction(fromAction);
	  ui.menu_Tabs->insertAction(toAction, fromAction);
	}
      else
	{
	  ui.menu_Tabs->removeAction(toAction);
	  ui.menu_Tabs->insertAction(fromAction, toAction);
	}
    }
}

void dooble::slotGeometryChangeRequested(const QRect &geometry)
{
  if(geometry.isValid() && m_isJavaScriptWindow)
    {
      bool isJavaScriptEnabled =
	s_settings.value("settingsWindow/javascriptEnabled", false).toBool();
      dwebpage *p = qobject_cast<dwebpage *> (sender());

      if(p)
	isJavaScriptEnabled = p->isJavaScriptEnabled();

      if(isJavaScriptEnabled &&
	 s_settings.value("settingsWindow/"
			  "javascriptAcceptGeometryChangeRequests", false).
	 toBool())
	setGeometry(dmisc::balancedGeometry(geometry, this));
    }
}

void dooble::slotHideMenuBar(void)
{
  if(m_isJavaScriptWindow)
    {
      bool isJavaScriptEnabled =
	s_settings.value("settingsWindow/javascriptEnabled", false).toBool();
      dwebpage *p = qobject_cast<dwebpage *> (sender());

      if(p)
	isJavaScriptEnabled = p->isJavaScriptEnabled();

      if(isJavaScriptEnabled &&
	 s_settings.value("settingsWindow/javascriptAllowMenuBarHiding",
			  true).toBool())
	{
	  menuBar()->setVisible(false);
	  ui.historyFrame->setVisible(false);
#ifndef Q_OS_MAC
	  ui.locationToolBar->setVisible(true);
	  ui.menuToolButton->setVisible(true);
#endif
	}
    }
}

void dooble::slotHideStatusBar(void)
{
  if(m_isJavaScriptWindow)
    {
      bool isJavaScriptEnabled =
	s_settings.value("settingsWindow/javascriptEnabled", false).toBool();
      dwebpage *p = qobject_cast<dwebpage *> (sender());

      if(p)
	isJavaScriptEnabled = p->isJavaScriptEnabled();

      if(isJavaScriptEnabled &&
	 s_settings.value("settingsWindow/javascriptAllowStatusBarHiding",
			  false).toBool() &&
	 !isFullScreen())
	statusBar()->setVisible(false);
    }
}

void dooble::slotHideToolBar(void)
{
  if(m_isJavaScriptWindow)
    {
      bool isJavaScriptEnabled =
	s_settings.value("settingsWindow/javascriptEnabled", false).toBool();
      dwebpage *p = qobject_cast<dwebpage *> (sender());

      if(p)
	isJavaScriptEnabled = p->isJavaScriptEnabled();

      if(isJavaScriptEnabled &&
	 s_settings.value("settingsWindow/javascriptAllowToolBarHiding",
			  false).toBool())
	{
	  ui.backToolButton->setVisible(false);
	  ui.forwardToolButton->setVisible(false);
	  ui.homeToolButton->setVisible(false);
	  ui.menuToolButton->setVisible(false);
	  ui.reloadStopWidget->setVisible(false);
	  ui.desktopToolButton->setVisible(false);
	  ui.searchLineEdit->setVisible(false);
	}
    }
}

void dooble::slotSetTabBarVisible(const bool state)
{
  if(ui.tabWidget->count() == 1)
    ui.tabWidget->setBarVisible(state);
}

void dooble::slotShowApplicationCookies(void)
{
  s_cookieWindow->show(this);
}

void dooble::prepareWidgetsBasedOnView(dview *p)
{
  if(!p)
    return;

  ui.zoomMenu->setEnabled(true);
  ui.actionFind->setEnabled(true);
  ui.actionSelect_All_Content->setEnabled(true);
  ui.actionShow_Hidden_Files->setEnabled(false);
  ui.findFrame->setVisible(showFindFrame);
}

void dooble::slotLocationSplitterMoved(int pos, int index)
{
  Q_UNUSED(pos);
  Q_UNUSED(index);

  if(!m_isJavaScriptWindow)
    s_settings["mainWindow/splitterState"] =
      ui.splitter->saveState();
}

void dooble::slotHistoryTabSplitterMoved(int pos, int index)
{
  Q_UNUSED(pos);
  Q_UNUSED(index);

  if(!m_isJavaScriptWindow)
    s_settings["mainWindow/historyTabSplitterState"] =
      ui.historyAndTabSplitter->saveState();
}

void dooble::setCurrentPage(dview *p)
{
  if(p)
    ui.tabWidget->setCurrentWidget(p);
}

bool dooble::promptForPassphrase(const bool override)
{
  /*
  ** This methods returns true if the user provided
  ** the correct passphrase.
  */

  if(dmisc::passphraseWasAuthenticated())
    return false;

  int iterationCount = qMax(1000,
			    s_settings.value("settingsWindow/iterationCount",
					     10000).toInt());
  bool showAuthentication = s_settings.value
    ("settingsWindow/showAuthentication", true).toBool();
  QString hashType(s_settings.
		   value("settingsWindow/passphraseHashType",
			 "unknown").toString());
  QString cipherMode(s_settings.
		     value("settingsWindow/cipherMode", "CBC").toString());
  QString cipherType(s_settings.
		     value("settingsWindow/cipherType",
			   "unknown").toString());

  if((override || (showAuthentication && s_instances == 1)) &&
     dmisc::passphraseWasPrepared())
    {
      show();
#ifndef Q_OS_MAC
      /*
      ** This will help center the passphrase dialog.
      */

      update();
      QApplication::processEvents();
#endif

      QDialog dialog(this);
      Ui_passphrasePrompt ui_p;

      ui_p.setupUi(&dialog);
#ifdef Q_OS_MAC
      dialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      QPixmap pixmap;
      QPixmap scaledPixmap;
      QSettings settings(s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);

      pixmap.load(settings.value("settingsWindow/safeButtonIcon").
		  toString());

      if(!pixmap.isNull())
	scaledPixmap = pixmap.scaled(QSize(48, 48),
				     Qt::KeepAspectRatio,
				     Qt::SmoothTransformation);

      if(scaledPixmap.isNull())
	ui_p.label->setPixmap(pixmap);
      else
	ui_p.label->setPixmap(scaledPixmap);

      dialog.setWindowIcon
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      for(int i = 0; i < ui_p.buttonBox->buttons().size(); i++)
	if(ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
	   QDialogButtonBox::AcceptRole ||
	   ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
	   QDialogButtonBox::ApplyRole ||
	   ui_p.buttonBox->buttonRole(ui_p.buttonBox->buttons().at(i)) ==
	   QDialogButtonBox::YesRole)
	  {
	    if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().
					    at(i)))
	      qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().
					   at(i))->setDefault(true);

	    ui_p.buttonBox->buttons().at(i)->setIcon
	      (QIcon(settings.value("okButtonIcon").toString()));
	    ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
	  }
	else
	  {
	    if(qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().
					    at(i)))
	      qobject_cast<QPushButton *> (ui_p.buttonBox->buttons().
					   at(i))->setDefault(false);

	    ui_p.buttonBox->buttons().at(i)->setIcon
	      (QIcon(settings.value("cancelButtonIcon").toString()));
	    ui_p.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
	  }

#ifndef Q_OS_MAC
      dialog.setWindowModality(Qt::ApplicationModal);
      dialog.show();
#endif
      dmisc::centerChildWithParent(&dialog, this);

      if(dialog.exec() == QDialog::Accepted)
	{
	  QByteArray hash;
	  QByteArray salt
	    (s_settings.
	     value("settingsWindow/passphraseSalt", "").toByteArray());
	  QByteArray storedHash
	    (s_settings.value("settingsWindow/passphraseHash").
	     toByteArray());

	  /*
	  ** Validate the passphrase.
	  */

	  hash = dmisc::passphraseHash
	    (ui_p.passphraseLineEdit->text(), salt, hashType);

	  if(!hash.isEmpty() && !storedHash.isEmpty() &&
	     dmisc::compareByteArrays(hash, storedHash))
	    {
	      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	      dmisc::setCipherPassphrase
		(ui_p.passphraseLineEdit->text(), false, hashType, cipherType,
		 iterationCount, salt, cipherMode);
	      QApplication::restoreOverrideCursor();
	      ui_p.passphraseLineEdit->clear();
	      emit passphraseWasAuthenticated
		(dmisc::passphraseWasAuthenticated());

	      foreach(QWidget *widget, QApplication::topLevelWidgets())
		if(qobject_cast<dooble *> (widget))
		  {
		    dooble *d = qobject_cast<dooble *> (widget);

		    if(d)
		      {
			d->sb.authenticate->setEnabled
			  (!dmisc::passphraseWasAuthenticated());
			d->ui.action_Authenticate->setEnabled
			  (!dmisc::passphraseWasAuthenticated());
		      }
		  }

	      reinstate();
	      s_makeCrashFile();
	      return true;
	    }
	  else
	    {
	      ui_p.passphraseLineEdit->clear();
#ifdef Q_OS_MAC
	      dialog.hide();
	      QApplication::processEvents();
#endif
	      return promptForPassphrase(override);
	    }
	}
      else
	{
	  ui_p.passphraseLineEdit->clear();

	  if(!dmisc::s_crypt)
	    {
	      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	      dmisc::setCipherPassphrase
		(QString(""), false, hashType, cipherType,
		 iterationCount, QByteArray(), cipherMode);
	      QApplication::restoreOverrideCursor();
	    }
	}
    }
  else if(!dmisc::s_crypt && !override && s_instances == 1)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      dmisc::setCipherPassphrase(QString(""), false, hashType, cipherType,
				 iterationCount, QByteArray(), cipherMode);
      QApplication::restoreOverrideCursor();
    }

  return false;
}

void dooble::initializeHistory(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Add history menu actions, but only if the user wishes
  ** to have a history. The entries will be date-ordered.
  */

  if(s_settings.value("settingsWindow/rememberHistory",
		      true).toBool())
    {
      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history");

	db.setDatabaseName(s_homePath +
			   QDir::separator() + "history.db");

	if(db.open())
	  {
	    int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	    QSqlQuery query(db);
	    QPair<QString, QIcon> pair;
	    QMultiMap<QDateTime, QVariantList> items;

	    query.setForwardOnly(true);
	    query.prepare("SELECT url, title, icon, "
			  "last_visited, visits "
			  "FROM history WHERE "
			  "temporary = ?");
	    query.bindValue(0, temporary);

	    if(query.exec())
	      while(query.next())
		{
		  bool ok = true;
		  QUrl url
		    (QUrl::fromEncoded
		     (dmisc::daa
		      (QByteArray::fromBase64
		       (query.value(0).toByteArray()), &ok),
		      QUrl::StrictMode));

		  /*
		  ** Simple test to determine if the URL
		  ** can be decoded. If it can't be,
		  ** ignore the history entry.
		  */

		  if(!ok || !url.isValid())
		    {
		      QSqlQuery deleteQuery(db);

		      deleteQuery.exec("PRAGMA secure_delete = ON");
		      deleteQuery.prepare("DELETE FROM history WHERE "
					  "url = ? AND temporary = ?");
		      deleteQuery.bindValue(0, query.value(0));
		      deleteQuery.bindValue(1, temporary);
		      deleteQuery.exec();
		      continue;
		    }

		  if(!dmisc::isSchemeAcceptedByDooble(url.scheme()))
		    continue;

		  if(ui.historyMenu->actions().size() == 4)
		    {
		      ui.historyMenu->actions().at(0)->setEnabled(true);
		      ui.historyMenu->actions().at(1)->setEnabled(true);
		      ui.historyMenu->addSeparator();
		    }

		  QIcon icon;

		  if(!query.isNull(2))
		    {
		      QBuffer buffer;
		      QByteArray bytes
			(query.value(2).toByteArray());

		      bytes = dmisc::daa(bytes, &ok);

		      if(ok)
			buffer.setBuffer(&bytes);

		      if(ok && buffer.open(QIODevice::ReadOnly))
			{
			  QDataStream in(&buffer);

			  in >> icon;

			  if(in.status() != QDataStream::Ok)
			    icon = QIcon();

			  buffer.close();
			}
		      else
			icon = QIcon();
		    }

		  if(icon.isNull())
		    icon = dmisc::iconForUrl(url);

		  QString title("");
		  QDateTime dateTime;

		  if(ok)
		    dateTime = QDateTime::fromString
		      (QString::fromUtf8
		       (dmisc::daa
			(QByteArray::fromBase64
			 (query.value(3).toByteArray()), &ok)),
		       Qt::ISODate);

		  if(ok)
		    title = QString::fromUtf8
		      (dmisc::daa
		       (QByteArray::fromBase64
			(query.value(1).toByteArray()), &ok));

		  if(title.isEmpty())
		    title = url.toString(QUrl::StripTrailingSlash);

		  title = dmisc::elidedTitleText(title);

		  if(title.isEmpty())
		    title = tr("Dooble Web Browser");

		  QVariantList list;

		  list.append(url);
		  list.append(icon);
		  list.append(title);
		  items.insert(dateTime, list);

		  QString host(url.host());
		  QString visits;

		  if(ok)
		    visits = dmisc::daa
		      (QByteArray::fromBase64
		       (query.value(4).toByteArray()), &ok);

		  if(ok && !host.isEmpty())
		    s_mostVisitedHosts[host] += visits.toLongLong();
		  else
		    {
		      QSqlQuery deleteQuery(db);

		      deleteQuery.exec("PRAGMA secure_delete = ON");
		      deleteQuery.prepare("DELETE FROM history WHERE "
					  "url = ? AND temporary = ?");
		      deleteQuery.bindValue(0, query.value(0));
		      deleteQuery.bindValue(1, temporary);
		      deleteQuery.exec();
		      continue;
		    }
		}

	    QMapIterator<QDateTime, QVariantList> i(items);

	    i.toBack();

	    while(i.hasPrevious())
	      {
		i.previous();

		if(i.value().size() < 3)
		  continue;

		QIcon icon(i.value().at(1).value<QIcon> ());

		if(icon.isNull())
		  icon = dmisc::iconForUrl(i.value().at(0).toUrl());

		QAction *action = new QAction(icon,
					      i.value().at(2).toString(),
					      this);

		action->setData(i.value().at(0));
		connect(action, SIGNAL(triggered(void)), this,
			SLOT(slotLinkActionTriggered(void)));
		ui.historyMenu->addAction(action);

		if(ui.historyMenu->actions().size() >= 5 + MAX_HISTORY_ITEMS)
		  break;
	      }

	    /*
	    ** The history menu should have at least three entries
	    ** at this point.
	    */

	    ui.locationLineEdit->appendItems(items);
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase("history");
    }

  QApplication::restoreOverrideCursor();
}

void dooble::initializeBookmarksMenu(void)
{
  ui.bookmarksMenu->clear();
  ui.bookmarksMenu->addActions(s_bookmarksWindow->actions(this));

  if(ui.bookmarksMenu->actions().size() >= 3)
    {
      ui.bookmarksMenu->actions().at(0)->setEnabled
	(ui.locationLineEdit->isBookmarkButtonEnabled());

      QSettings settings(s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);

      ui.bookmarksMenu->actions().at(0)->setIcon
	(QIcon(settings.value("mainWindow/actionBookmarks").toString()));
      ui.bookmarksMenu->actions().at(2)->setIcon
	(QIcon(settings.value("mainWindow/actionBookmarks").toString()));
      connect(ui.bookmarksMenu->actions().at(0),
	      SIGNAL(triggered(void)),
	      this,
	      SLOT(slotBookmark(void)));
      connect(ui.bookmarksMenu->actions().at(2),
	      SIGNAL(triggered(void)),
	      this,
	      SLOT(slotShowBookmarks(void)));
    }

  ui.bookmarksMenu->update();
}

void dooble::slotOpenP2PEmail(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->isModified() && !warnAboutLeavingModifiedTab())
    return;

  QUrl url(QUrl::fromUserInput(s_settings.value("settingsWindow/p2pUrl",
						"about: blank").toString()));

  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
  loadPage(url);

  if(ui.tabWidget->currentWidget())
    ui.tabWidget->currentWidget()->setFocus();
}

void dooble::slotOpenIrcChannel(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->isModified() && !warnAboutLeavingModifiedTab())
    return;

  QUrl url
    (QUrl::fromUserInput(s_settings.
			 value("settingsWindow/ircChannel",
			       "https://webchat.freenode.net?channels=dooble").
			 toString()));

  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
  loadPage(url);

  if(ui.tabWidget->currentWidget())
    ui.tabWidget->currentWidget()->setFocus();
}

void dooble::slotRepaintRequested(const QRect &dirtyRect)
{
  Q_UNUSED(dirtyRect);
}

void dooble::slotShowFavoritesToolBar(bool checked)
{
  if(!isVisible())
    return;

  disconnect(ui.favoritesToolBar,
	     SIGNAL(visibilityChanged(bool)),
	     ui.actionShow_FavoritesToolBar,
	     SLOT(setChecked(bool)));
  ui.favoritesToolBar->setVisible(checked);
  connect(ui.favoritesToolBar,
	  SIGNAL(visibilityChanged(bool)),
	  ui.actionShow_FavoritesToolBar,
	  SLOT(setChecked(bool)));

  QSettings settings;

  settings.setValue("mainWindow/showFavoritesToolBar", checked);
  s_settings["mainWindow/showFavoritesToolBar"] = checked;

  if(checked)
    prepareMostVisited();
}

void dooble::prepareMostVisited(void)
{
  if(!ui.favoritesToolBar->isVisible())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(QToolButton *toolButton,
	  ui.favoritesToolButtonsFrame->findChildren<QToolButton *> ())
    toolButton->deleteLater();

  QList<qint64> list(s_mostVisitedHosts.values());
  QStringList addedHosts;

  qSort(list);

  for(int i = list.size() - 1, j = 0; i >= 0; i--)
    {
      QList<QString> keys(s_mostVisitedHosts.keys(list.at(i)));

      /*
      ** We should have something like:
      ** Count (C1 >= C2 >= ... >= CN): Host
      ** (H1 >= H2 >= ... >= HI >= ... >= HK)
      ** C1: H1
      ** C1: H2
      ** ...
      ** C1: HI
      ** ...
      ** CN: HK
      */

      qSort(keys);

      /*
      ** Multiple hosts may enjoy identical counts.
      */

      for(int k = 0; k < keys.size(); k++)
	if(j < MAX_MOST_VISITED_ITEMS)
	  {
	    if(!keys.at(k).isEmpty() &&
	       !addedHosts.contains(keys.at(k)))
	      {
		QUrl url(QUrl::fromUserInput(keys.at(k)));
		QIcon icon(dmisc::iconForUrl(url));
		QToolButton *toolButton = new QToolButton
		  (ui.favoritesToolButtonsFrame);

		toolButton->setIcon(icon);
		toolButton->setToolTip
		  (url.toString(QUrl::StripTrailingSlash));
		toolButton->setIconSize(QSize(16, 16));
#ifdef Q_OS_MAC
		toolButton->setStyleSheet
		  ("QToolButton {border: none;}"
		   "QToolButton::menu-button {border: none;}");
#else
		toolButton->setAutoRaise(true);
#endif
		ui.favoritesToolButtonsFrame->layout()->
		  addWidget(toolButton);
		toolButton->setProperty("url", url);
		connect(toolButton,
			SIGNAL(clicked(void)),
			this,
			SLOT(slotFavoritesToolButtonClicked(void)));
		j += 1;
		addedHosts.append(keys.at(k));
	      }
	  }
	else
	  break;

      if(j >= MAX_MOST_VISITED_ITEMS)
	break;
    }

  QApplication::restoreOverrideCursor();
}

void dooble::slotFavoritesToolButtonClicked(void)
{
  QToolButton *toolButton = qobject_cast<QToolButton *> (sender());

  if(toolButton)
    {
      QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;
      dview *p = 0;

      webAttributes[QWebEngineSettings::JavascriptEnabled] =
	s_settings.value("settingsWindow/javascriptEnabled",
			 false).toBool();
      p = newTab(toolButton->property("url").toUrl(), 0, webAttributes);

      if(p)
	{
	  ui.tabWidget->setCurrentWidget(p);
	  ui.tabWidget->update();
	}
    }
}

void dooble::slotShowIpAddress(const bool state)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(!p)
    return;

  if(state)
    p->findIpAddress(p->url());
  else
    {
      if(p->title().isEmpty())
	setWindowTitle(tr("Dooble Web Browser"));
      else
	setWindowTitle(p->title() + tr(" - Dooble Web Browser"));
    }
}

void dooble::slotShowHistorySideBar(bool state)
{
  ui.historyFrame->setVisible(state);
  dooble::s_settings["mainWindow/showHistorySideBar"] = state;

  QSettings settings;

  settings.setValue("mainWindow/showHistorySideBar", state);
}

void dooble::slotHistorySideBarClosed(void)
{
  ui.actionShow_HistorySideBar->setChecked(false);
  slotShowHistorySideBar(false);
}

void dooble::slotOpenUrlsFromDrop(const QList<QUrl> &list)
{
  if(list.size() > 5)
    {
      QApplication::restoreOverrideCursor();

      QMessageBox mb(this);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setIcon(QMessageBox::Question);
      mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
      mb.setWindowModality(Qt::WindowModal);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(QString(tr("Are you sure that you wish to open "
			    "%1 pages?")).arg(list.size()));

      QSettings settings(s_settings.value("iconSet").toString(),
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
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      if(mb.exec() == QMessageBox::No)
	return;
    }

  for(int i = 0; i < list.size(); i++)
    if(i == 0)
      {
	dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

	if(p && p->isModified() && !warnAboutLeavingModifiedTab())
	  return;

	loadPage(list.at(i));
      }
    else
      {
	QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

	webAttributes[QWebEngineSettings::JavascriptEnabled] =
	  s_settings.value("settingsWindow/javascriptEnabled",
			   false).toBool();
	newTab(list.at(i), 0, webAttributes);
      }

  ui.tabWidget->update();

  if(ui.tabWidget->currentWidget())
    ui.tabWidget->currentWidget()->setFocus();
}

void dooble::copyDooble(dooble *d)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(d)
    {
      for(int i = 0; i < d->ui.historyMenu->actions().size(); i++)
	if(i == 0 || i == 1)
	  ui.historyMenu->actions().at(i)->setEnabled
	    (d->ui.historyMenu->actions().at(i)->isEnabled());
	else if(i == 2)
	  ui.historyMenu->addSeparator();
	else if(i == 3 || i == 4)
	  continue;
	else
	  {
	    QAction *a = new QAction
	      (d->ui.historyMenu->actions().at(i)->icon(),
	       d->ui.historyMenu->actions().at(i)->text(),
	       this);

	    a->setData(d->ui.historyMenu->actions().at(i)->data());
	    connect(a, SIGNAL(triggered(void)), this,
		    SLOT(slotLinkActionTriggered(void)));
	    ui.historyMenu->addAction(a);
	  }

      ui.locationLineEdit->copyContentsOf(d->ui.locationLineEdit);
    }
  else
    initializeHistory();

  QApplication::restoreOverrideCursor();
}

void dooble::slotStatusBarMessage(const QString &text)
{
  /*
  ** As of version 1.32, this method is not connected to the
  ** statusBarMessage() signal. If the signal is connected to
  ** this method, dfilemanager and dftpbrowser must be considered.
  */

  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());
  dwebpage *d = qobject_cast<dwebpage *> (sender());

  if(d && p && d == p->page())
    {
      sb.statusLabel->setText(text);
      QTimer::singleShot(1000, sb.statusLabel, SLOT(clear(void)));
    }
}

void dooble::slotObjectDestroyed(QObject *object)
{
  QWidget *w = qobject_cast<QWidget *> (object);

  if(w)
    {
      if(ui.tabWidget->indexOf(w) != -1)
	ui.tabWidget->removeTab(ui.tabWidget->indexOf(w));

      if(ui.tabWidget->count() == 1)
	{
	  ui.tabWidget->setTabsClosable(false);
	  ui.tabWidget->setBarVisible
	    (s_settings.value("settingsWindow/alwaysShowTabBar",
			      true).toBool());
	}
    }
}

bool dooble::warnAboutLeavingModifiedTab(void)
{
  /*
  ** Returns true if the warning was accepted or
  ** if the setting is disabled.
  */

  if(s_settings.value("settingsWindow/warnBeforeLeavingModifiedTab",
		      false).toBool())
    {
      QMessageBox mb(this);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setIcon(QMessageBox::Question);
      mb.setWindowTitle(tr("Dooble Web Browser: Confirmation"));
      mb.setWindowModality(Qt::WindowModal);
      mb.setStandardButtons(QMessageBox::Cancel |
			    QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Are you sure that you wish to leave the modified "
		    "page?"));

      QSettings settings(s_settings.value("iconSet").toString(),
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
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      if(mb.exec() == QMessageBox::Yes)
	return true;
      else
	return false;
    }

  return true;
}

void dooble::slotAuthenticate(void)
{
  if(promptForPassphrase(true))
    {
      clearHistoryWidgets();
      initializeHistory();

      /*
      ** We're not going to populate the History model and window.
      */

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      s_cookies->populate();
      s_cookieWindow->populate();
      s_downloadWindow->populate();
      s_bookmarksWindow->populate();
      s_popupsWindow->populate();
      s_adBlockWindow->populate();
      s_cookiesBlockWindow->populate();
      s_httpOnlyExceptionsWindow->populate();
      s_httpRedirectWindow->populate();
      s_httpReferrerWindow->populate();
      s_javaScriptExceptionsWindow->populate();
      s_dntWindow->populate();
      s_imageBlockWindow->populate();
      s_cacheExceptionsWindow->populate();
      s_alwaysHttpsExceptionsWindow->populate();
      s_sslExceptionsWindow->populate();
      s_networkCache->populate();
      QApplication::restoreOverrideCursor();

      foreach(QWidget *widget, QApplication::topLevelWidgets())
	if(qobject_cast<dooble *> (widget))
	  {
	    dooble *d = qobject_cast<dooble *> (widget);

	    if(d)
	      {
		d->prepareMostVisited();

		/*
		** Copy this window's important attributes.
		*/

		if(d != this)
		  d->copyDooble(this);
	      }
	  }
    }
}

void dooble::setUrlHandler(dooble *d)
{
  QDesktopServices::setUrlHandler("data",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("dooble-ftp",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("dooble-http",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("dooble-https",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("file",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("ftp",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("http",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("https",
				  d,
				  "slotOpenLinkInNewTab");
  QDesktopServices::setUrlHandler("qrc",
				  d,
				  "slotOpenLinkInNewTab");
}

void dooble::unsetUrlHandler(void)
{
  QDesktopServices::unsetUrlHandler("data");
  QDesktopServices::unsetUrlHandler("dooble-ftp");
  QDesktopServices::unsetUrlHandler("dooble-http");
  QDesktopServices::unsetUrlHandler("dooble-https");
  QDesktopServices::unsetUrlHandler("file");
  QDesktopServices::unsetUrlHandler("ftp");
  QDesktopServices::unsetUrlHandler("http");
  QDesktopServices::unsetUrlHandler("https");
  QDesktopServices::unsetUrlHandler("qrc");

  foreach(QWidget *widget, QApplication::topLevelWidgets())
    if(qobject_cast<dooble *> (widget))
      {
	dooble *d = qobject_cast<dooble *> (widget);

	if(d && d != this)
	  {
	    setUrlHandler(d);
	    break;
	  }
      }
}

void dooble::slotReopenClosedTab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      int index = -1;

      /*
      ** First, delete the correct action from the Recently-Closed Tabs
      ** menu.
      */

      if(ui.historyMenu->actions().size() >= 4)
	if(ui.historyMenu->actions().at(3)->menu())
	  for(int i = 0;
	      i < ui.historyMenu->actions().at(3)->menu()->actions().size();
	      i++)
	    if(action == ui.historyMenu->actions().at(3)->menu()->
	       actions().at(i))
	      {
		index = i;
		action->deleteLater();
		break;
	      }

      if(index > -1)
	/*
	** Subtract 2 (Clear action and separator)
	*/

	index -= 2;

      /*
      ** Create a view having the old view's history.
      */

      for(int i = 0; i < m_closedTabs.count(); i++)
	if(i == index)
	  {
	    dview *p = 0;
	    QByteArray history = m_closedTabs.at(i);

	    if((p = newTab(history)))
	      {
		ui.tabWidget->setCurrentWidget(p);
		ui.tabWidget->update();
	      }

	    m_closedTabs.removeAt(i);
	    prepareTabsMenu();
	    break;
	  }

      if(m_closedTabs.isEmpty() &&
	 ui.historyMenu->actions().size() >= 4)
	{
	  ui.historyMenu->actions().at(3)->setEnabled(false);

	  if(ui.historyMenu->actions().at(3)->menu())
	    {
	      while(!ui.historyMenu->actions().at(3)->menu()->actions().
		    isEmpty())
		{
		  QAction *action = ui.historyMenu->actions().at(3)->menu()->
		    actions().last();

		  ui.historyMenu->actions().at(3)->menu()->
		    removeAction(action);
		  action->deleteLater();
		}

	      ui.historyMenu->actions().at(3)->menu()->deleteLater();
#ifdef Q_OS_MAC
	      ui.historyMenu->actions().at(3)->setMenu(0);
#endif
	    }
	}
    }
}

void dooble::prepareTabsMenu(void)
{
  ui.menu_Tabs->clear();

  for(int i = 0; i < ui.tabWidget->count(); i++)
    {
      dview *p = qobject_cast<dview *> (ui.tabWidget->widget(i));
      QAction *action = 0;

      if(p)
	action = p->tabAction();
      else
	{
	  dreinstatedooble *reinstateWidget =
	    qobject_cast<dreinstatedooble *> (ui.tabWidget->widget(i));

	  if(reinstateWidget)
	    action = reinstateWidget->tabAction();
	  else
	    {
	      if(m_desktopWidget)
		action = m_desktopWidget->tabAction();
	    }
	}

      if(action)
	ui.menu_Tabs->addAction(action);
    }

  ui.menu_Tabs->setEnabled(!ui.menu_Tabs->actions().isEmpty());
}

void dooble::slotClearRecentlyClosedTabs(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_closedTabs.clear();

  if(ui.historyMenu->actions().size() >= 4 &&
     ui.historyMenu->actions().at(3)->menu())
    {
      while(!ui.historyMenu->actions().at(3)->menu()->actions().
	    isEmpty())
	{
	  QAction *action = ui.historyMenu->actions().at(3)->menu()->
	    actions().last();

	  ui.historyMenu->actions().at(3)->menu()->removeAction(action);
	  action->deleteLater();
	}

      ui.historyMenu->actions().at(3)->menu()->deleteLater();
#ifdef Q_OS_MAC
      ui.historyMenu->actions().at(3)->setMenu(0);
#endif
    }

  if(ui.historyMenu->actions().size() >= 4)
    ui.historyMenu->actions().at(3)->setEnabled(false);

  QApplication::restoreOverrideCursor();
}

void dooble::slotExceptionRaised(dexceptionswindow *window,
				 const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;
  else if(!s_settings.value("settingsWindow/notifyOfExceptions",
			    true).toBool())
    return;

  if(window == s_cookiesBlockWindow || (window && !window->contains(url.
								    host())))
    {
      if(url.host().isEmpty())
	sb.exceptionsToolButton->setToolTip
	  (QString(tr("The site %1 caused an exception. "
		      "Please click to review.").
		   arg(url.toString(QUrl::StripTrailingSlash))));
      else
	sb.exceptionsToolButton->setToolTip
	  (QString(tr("The site %1 caused an exception. "
		      "Please click to review.").
		   arg(url.host())));

      sb.exceptionsToolButton->setVisible(true);
      sb.exceptionsToolButton->disconnect(window);
      connect(sb.exceptionsToolButton,
	      SIGNAL(clicked(void)),
	      window,
	      SLOT(slotShow(void)));
      connect(sb.exceptionsToolButton,
	      SIGNAL(clicked(void)),
	      sb.exceptionsToolButton,
	      SLOT(hide(void)));
    }
}

void dooble::slotClearContainers(void)
{
  s_clearContainersWindow->show(this);
}

void dooble::slotClearHistory(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  s_mostVisitedHosts.clear();
  dmisc::removeRestorationFiles();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history");

    db.setDatabaseName(s_homePath +
		       QDir::separator() + "history.db");

    if(db.open())
      {
	int temporary = dmisc::passphraseWasAuthenticated() ? 0 : 1;
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");

	if(!temporary)
	  query.exec("DELETE FROM history");
	else
	  query.exec("DELETE FROM history WHERE temporary = 1");

	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history");
  QApplication::restoreOverrideCursor();
  clearHistoryWidgets();
}

void dooble::clearHistoryWidgets(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  /*
  ** Clears every window's history widgets, including recently-closed tabs.
  */

  foreach(QWidget *widget, QApplication::topLevelWidgets())
    {
      dooble *d = qobject_cast<dooble *> (widget);

      if(!d)
	continue;

      emit d->clearHistory();
      d->ui.locationLineEdit->clearHistory();
      d->m_closedTabs.clear();

      if(d->ui.historyMenu->actions().size() >= 4 &&
	 d->ui.historyMenu->actions().at(3)->menu())
	{
	  while(!d->ui.historyMenu->actions().at(3)->menu()->actions().
		isEmpty())
	    {
	      QAction *action = d->ui.historyMenu->actions().at(3)->menu()->
		actions().last();

	      d->ui.historyMenu->actions().at(3)->menu()->removeAction(action);
	      action->deleteLater();
	    }

	  d->ui.historyMenu->actions().at(3)->menu()->deleteLater();
#ifdef Q_OS_MAC
	  d->ui.historyMenu->actions().at(3)->setMenu(0);
#endif
	}

      while(d->ui.historyMenu->actions().size() > 4)
	{
	  QAction *action = d->ui.historyMenu->actions().last();

	  d->ui.historyMenu->removeAction(action);
	  action->deleteLater();
	}

      if(d->ui.historyMenu->actions().size() >= 4)
	{
	  d->ui.historyMenu->actions().at(0)->setEnabled(false);
	  d->ui.historyMenu->actions().at(1)->setEnabled(true);
	  d->ui.historyMenu->actions().at(3)->setEnabled(false);
	}

      foreach(QToolButton *toolButton,
	      d->ui.favoritesToolButtonsFrame->findChildren<QToolButton *> ())
	toolButton->deleteLater();

      d->ui.backToolButton->menu()->clear();
      d->ui.forwardToolButton->menu()->clear();
      d->ui.backToolButton->setEnabled(false);
      d->ui.forwardToolButton->setEnabled(false);
    }

  QApplication::restoreOverrideCursor();
}

void dooble::slotBookmark(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      if(p->isBookmarked())
	{
	  s_bookmarksPopup->populate(p->url());
	  s_bookmarksPopupMenu->exec
	    (ui.locationLineEdit->mapToGlobal(ui.locationLineEdit->
					      bookmarkButtonPopupPosition()));
	}
      else
	{
	  QDateTime now(QDateTime::currentDateTime());

	  s_bookmarksWindow->slotAddBookmark(p->webviewUrl(),
					     p->webviewIcon(),
					     p->title(),
					     p->description(),
					     now,
					     now);
	}
    }
}

void dooble::slotBookmark(const int index)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p)
    {
      if(p->isBookmarked())
	{
	  s_bookmarksPopup->populate(p->url());
	  s_bookmarksPopupMenu->exec
	    (ui.locationLineEdit->mapToGlobal(ui.locationLineEdit->
					      bookmarkButtonPopupPosition()));
	}
      else
	{
	  QDateTime now(QDateTime::currentDateTime());

	  s_bookmarksWindow->slotAddBookmark(p->webviewUrl(),
					     p->webviewIcon(),
					     p->title(),
					     p->description(),
					     now,
					     now);
	}
    }
}

void dooble::highlightBookmarkButton(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    ui.locationLineEdit->setBookmarkColor(p->isBookmarked());
  else
    ui.locationLineEdit->setBookmarkColor(false);
}

void dooble::slotBookmarksChanged(void)
{
  highlightBookmarkButton();
}

qint64 dooble::id(void) const
{
  return m_id;
}

void dooble::reinstate(void)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;
  else if(!s_settings.value("settingsWindow/sessionRestoration", true).
	  toBool())
    return;
  else if(s_instances > 1)
    return;
  else if(findChild<dreinstatedooble *> ())
    return;

  QFileInfo fileInfo(dooble::s_homePath + QDir::separator() + ".crashed");

  if(!fileInfo.exists())
    return;

  dreinstatedooble *reinstateWidget = new dreinstatedooble(this);

  if(reinstateWidget->isEmpty())
    {
      reinstateWidget->deleteLater();
      QFile::remove(dooble::s_homePath + QDir::separator() + ".crashed");
      return;
    }

  connect(reinstateWidget,
	  SIGNAL(close(void)),
	  this,
	  SLOT(slotCloseTab(void)));

  QSettings settings
    (s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("mainWindow");

  int index = ui.tabWidget->addTab
    (reinstateWidget,
     QIcon(settings.value("actionRestore").toString()),
     tr("Restore Session"));

  ui.tabWidget->setTabToolTip(index, tr("Restore Session"));
  ui.tabWidget->setTabsClosable(ui.tabWidget->count() > 1);
  ui.tabWidget->setCurrentWidget(reinstateWidget);

  QAction *action = new QAction(tr("Restore Session"), this);

  action->setData("Restore Session");
  action->setIcon
    (QIcon(settings.value("actionRestore").toString()));
  connect(action, SIGNAL(triggered(void)), this,
	  SLOT(slotShowRestoreSessionTab(void)));
  reinstateWidget->setTabAction(action);
  prepareTabsMenu();
}

void dooble::slotShowRestoreSessionTab(void)
{
  dreinstatedooble *reinstateWidget = qobject_cast<dreinstatedooble *>
    (findChild<dreinstatedooble *> ());

  if(reinstateWidget)
    ui.tabWidget->setCurrentWidget(reinstateWidget);
}

void dooble::remindUserToSetPassphrase(void)
{
  bool showAuthentication = s_settings.value
    ("settingsWindow/showAuthentication", true).toBool();

  if(!showAuthentication)
    return;

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);
  QMessageBox mb(this);

#ifdef Q_OS_MAC
  mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  mb.setIcon(QMessageBox::Information);
  mb.setStandardButtons(QMessageBox::Ok);
  mb.setWindowTitle(tr("Dooble Web Browser: Reminder"));
  mb.setText(tr("A passphrase has not been prepared. "
		"Please visit the Safe panel in the Settings window and "
		"choose a passphrase. "
		"Once a passphrase is selected, bookmarks, cookies, "
		"and all other essential information will be available "
		"in future sessions. You may disable this reminder via "
		"the Safe panel."));

  for(int i = 0; i < mb.buttons().size(); i++)
    if(mb.buttonRole(mb.buttons().at(i)) == QMessageBox::AcceptRole ||
       mb.buttonRole(mb.buttons().at(i)) == QMessageBox::ApplyRole ||
       mb.buttonRole(mb.buttons().at(i)) == QMessageBox::YesRole)
      mb.buttons().at(i)->setIcon
	(QIcon(settings.value("okButtonIcon").toString()));

  mb.setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));
  mb.exec();
}

void dooble::slotReloadTab(const int index)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->widget(index));

  if(p)
    {
      if(p->isModified() && !warnAboutLeavingModifiedTab())
	return;

      p->reload();
    }
}

void dooble::s_makeCrashFile(void)
{
  if(s_crashFileName == 0 &&
     dmisc::passphraseWasAuthenticated() &&
     s_settings.value("settingsWindow/sessionRestoration", true).toBool())
    {
      QString str(dooble::s_homePath);

      str.append(QDir::separator());
      str.append(".crashed");
      s_crashFileName = new char[str.length() + 1];
      memset(s_crashFileName, 0, str.length() + 1);
      strncpy(s_crashFileName, str.toStdString().c_str(), str.length());
    }
}

void dooble::s_removeCrashFile(void)
{
  if(s_crashFileName &&
     dmisc::passphraseWasAuthenticated())
    {
      QFile::remove(s_crashFileName);
      delete []s_crashFileName;
      s_crashFileName = 0;
    }
}

void dooble::slotErrorLogged(void)
{
  sb.errorLogToolButton->setVisible(true);
}

void dooble::slotAboutToShowBackForwardMenu(void)
{
  prepareNavigationButtonMenus
    (qobject_cast<dview *> (ui.tabWidget->currentWidget()),
     qobject_cast<QMenu *> (sender()));
}

void dooble::slotAboutToShowBookmarksMenu(void)
{
  initializeBookmarksMenu();
}

void dooble::slotOffline(bool state)
{
  QSettings settings;

  settings.setValue("mainWindow/offlineMode", state);
  s_settings["mainWindow/offlineMode"] = state;
}

void dooble::showEvent(QShowEvent *event)
{
  QMainWindow::showEvent(event);
}

bool dooble::event(QEvent *event)
{
  return QMainWindow::event(event);
}

void dooble::disconnectPageSignals(dview *p, dooble *d)
{
  if(!p)
    return;

  disconnect(p, SIGNAL(destroyed(QObject *)),
	     d, SLOT(slotObjectDestroyed(QObject *)));
  disconnect(p, SIGNAL(urlChanged(const QUrl &)), d,
	     SLOT(slotUrlChanged(const QUrl &)));
  disconnect(p, SIGNAL(titleChanged(const QString &)), d,
	     SLOT(slotTitleChanged(const QString &)));
  disconnect(p, SIGNAL(loadFinished(bool)), d,
	     SLOT(slotLoadFinished(bool)));
  disconnect(p, SIGNAL(loadProgress(int)), d,
	     SLOT(slotLoadProgress(int)));
  disconnect(p, SIGNAL(iconChanged(void)), d,
	     SLOT(slotIconChanged(void)));
  disconnect(p, SIGNAL(loadStarted(void)), d,
	     SLOT(slotLoadStarted(void)));
  disconnect
    (p,
     SIGNAL(openLinkInNewTab(const QUrl &, dcookies *,
			     const QHash<QWebEngineSettings::WebAttribute,
			     bool> &)),
     d,
     SLOT(slotOpenLinkInNewTab(const QUrl &, dcookies *,
			       const QHash<QWebEngineSettings::WebAttribute,
			       bool> &)));
  disconnect
    (p,
     SIGNAL(openLinkInNewWindow(const QUrl &, dcookies *,
				const QHash<QWebEngineSettings::WebAttribute,
				bool> &)),
     d,
     SLOT
     (slotOpenLinkInNewWindow(const QUrl &, dcookies *,
			      const QHash<QWebEngineSettings::WebAttribute,
					  bool> &)));
  disconnect(p, SIGNAL(copyLink(const QUrl &)), d,
	     SLOT(slotCopyLink(const QUrl &)));
  disconnect(p, SIGNAL(saveUrl(const QUrl &, const int)), d,
	     SLOT(slotSaveUrl(const QUrl &, const int)));
  disconnect
    (p, SIGNAL(saveFile(const QString &, const QUrl &, const int)),
     d, SLOT(slotSaveFile(const QString &, const QUrl &, const int)));
  disconnect(p, SIGNAL(viewImage(const QUrl &)), d,
	     SLOT(slotLoadPage(const QUrl &)));
  disconnect(p, SIGNAL(ipAddressChanged(const QString &)), d,
	     SLOT(slotIpAddressChanged(const QString &)));
  connect(p->page(),
	  SIGNAL(linkHovered(const QString &)),
	  d,
	  SLOT(slotLinkHovered(const QString &)));
  disconnect
    (p->page(),
     SIGNAL(authenticationRequired(const QUrl &, QAuthenticator *)),
     d,
     SLOT(slotAuthenticationRequired(const QUrl &, QAuthenticator *)));
  disconnect(p->page(),
	     SIGNAL(proxyAuthenticationRequired(const QUrl &,
						QAuthenticator *,
						const QString &)),
	     d,
	     SLOT(slotProxyAuthenticationRequired(const QUrl &,
						  QAuthenticator *,
						  const QString &)));
  disconnect(p->page(), SIGNAL(windowCloseRequested(void)),
	     d, SLOT(slotCloseWindow(void)));
  disconnect(p->page(), SIGNAL(geometryChangeRequested(const QRect &)),
	     d, SLOT(slotGeometryChangeRequested(const QRect &)));
  disconnect(d,
	     SIGNAL(clearHistory(void)),
	     p,
	     SLOT(slotClearHistory(void)));
  disconnect(p, SIGNAL(selectionChanged(const QString &)),
	     d, SLOT(slotSelectionChanged(const QString &)));
  disconnect(p, SIGNAL(viewPageSource(void)),
	     d, SLOT(slotShowPageSource(void)));
  disconnect(p, SIGNAL(goBack(void)),
	     d, SLOT(slotBack(void)));
  disconnect(p, SIGNAL(goReload(void)),
	     d, SLOT(slotReload(void)));
  disconnect(p, SIGNAL(goForward(void)),
	     d, SLOT(slotForward(void)));
  disconnect(p,
	     SIGNAL(exceptionRaised(dexceptionswindow *,
				    const QUrl &)),
	     d,
	     SLOT(slotExceptionRaised(dexceptionswindow *,
				      const QUrl &)));
  disconnect(p->page(),
	     SIGNAL(exceptionRaised(dexceptionswindow *,
				    const QUrl &)),
	     d,
	     SLOT(slotExceptionRaised(dexceptionswindow *,
				      const QUrl &)));
  disconnect(p->page(),
	     SIGNAL(exceptionRaised(dexceptionswindow *,
				    const QUrl &)),
	     d,
	     SLOT(slotExceptionRaised(dexceptionswindow *,
				      const QUrl &)));
  disconnect(s_clearContainersWindow,
	     SIGNAL(clearCookies(void)),
	     p,
	     SLOT(slotClearCookies(void)));
}

void dooble::slotIconToolButtonClicked(void)
{
  QMenu menu(this);
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      QString text(p->url().host().trimmed());

      if(!text.isEmpty())
	menu.addAction(tr("View %1 &Cookies...").arg(text), this,
		       SLOT(slotViewSiteCookies(void)));
      else
	menu.addAction(tr("View Site &Cookies..."), this,
		       SLOT(slotViewSiteCookies(void)));
    }
  else

    menu.addAction(tr("View Site &Cookies..."), this,
		   SLOT(slotViewSiteCookies(void)));

  menu.exec(ui.locationLineEdit->mapToGlobal(ui.locationLineEdit->
					     iconButtonPopupPosition()));
}

void dooble::slotViewSiteCookies(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p)
    {
      QString text(p->url().host().trimmed());

      if(!text.isEmpty())
	s_cookieWindow->find(text);
    }

  s_cookieWindow->show(this);
}

void dooble::saveHistoryThread(const QList<QVariant> &list)
{
  QByteArray p_bytes = list.value(3).toByteArray();
  QString p_title = list.value(1).toString();
  QVariant p_userData = list.value(2);
  QUrl p_url = list.value(0).toUrl();
  bool p_saveHistory = list.value(5).toBool();
  int p_temporary = list.value(4).toInt();

  /*
  ** Do not disable database synchronization. Other methods require
  ** valid QIcon objects. Qt is sensitive.
  */

  QMutexLocker locker(&s_saveHistoryMutex);

  /*
  ** If the item is bookmarked, update the bookmark's attributes.
  */

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "bookmarks_thread");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "bookmarks.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.setForwardOnly(true);
	query.prepare("SELECT visits FROM bookmarks WHERE "
		      "temporary = ? AND url_hash = ?");
	query.bindValue(0, p_temporary);
	query.bindValue
	  (1,
	   dmisc::hashedString(p_url.
			       toEncoded(QUrl::StripTrailingSlash), &ok).
	   toBase64());

	if(ok && query.exec())
	  if(query.next())
	    {
	      qint64 count = 0;

	      count = dmisc::daa
		(QByteArray::fromBase64(query.value(0).
					toByteArray()),
		 &ok).toLongLong() + 1;

	      if(!ok)
		{
		  QSqlQuery deleteQuery(db);
		  bool ok = true;

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare
		    ("DELETE FROM bookmarks WHERE temporary = ? AND "
		     "url_hash = ?");
		  deleteQuery.bindValue(0, p_temporary);
		  deleteQuery.bindValue
		    (1,
		     dmisc::hashedString(p_url.
					 toEncoded(QUrl::StripTrailingSlash),
					 &ok).
		     toBase64());

		  if(ok)
		    deleteQuery.exec();
		}
	      else
		{
		  QSqlQuery updateQuery(db);

		  updateQuery.prepare
		    ("UPDATE bookmarks SET visits = ?, "
		     "last_visited = ?, "
		     "icon = ? "
		     "WHERE url_hash = ? AND "
		     "temporary = ?");
		  updateQuery.bindValue
		    (0,
		     dmisc::etm(QString::number(count).toLatin1(),
				true, &ok).toBase64());

		  if(ok)
		    updateQuery.bindValue
		      (1,
		       dmisc::etm(QDateTime::currentDateTime().
				  toString(Qt::ISODate).
				  toUtf8(), true, &ok).toBase64());

		  if(ok)
		    updateQuery.bindValue
		      (2,
		       dmisc::etm(p_bytes, true, &ok));

		  if(ok)
		    updateQuery.bindValue
		      (3,
		       dmisc::hashedString(p_url.
					   toEncoded(QUrl::
						     StripTrailingSlash),
					   &ok).toBase64());

		  updateQuery.bindValue(4, p_temporary);

		  if(ok)
		    if(updateQuery.exec())
		      emit bookmarkUpdated();
		}
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("bookmarks_thread");

  if(!p_saveHistory)
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history_thread");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "history.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	qint64 count = 1;

	query.setForwardOnly(true);
	query.exec("CREATE TABLE IF NOT EXISTS history ("
		   "title TEXT, "
		   "url TEXT NOT NULL, "
		   "url_hash TEXT NOT NULL, "
		   "last_visited TEXT NOT NULL, "
		   "icon BLOB DEFAULT NULL, "
		   "visits TEXT NOT NULL, "
		   "temporary INTEGER NOT NULL, "
		   "description TEXT, "
		   "PRIMARY KEY (url_hash, temporary))");
	query.prepare("SELECT visits FROM history WHERE "
		      "temporary = ? AND url_hash = ?");
	query.bindValue(0, p_temporary);
	query.bindValue
	  (1,
	   dmisc::hashedString(p_url.
			       toEncoded(QUrl::StripTrailingSlash), &ok).
	   toBase64());

	if(ok && query.exec())
	  if(query.next())
	    {
	      count = dmisc::daa
		(QByteArray::fromBase64
		 (query.value(0).toByteArray()), &ok).toLongLong() + 1;

	      if(!ok)
		{
		  QSqlQuery deleteQuery(db);
		  bool ok = true;

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare
		    ("DELETE FROM history WHERE temporary = ? AND "
		     "url_hash = ?");
		  deleteQuery.bindValue(0, p_temporary);
		  deleteQuery.bindValue
		    (1,
		     dmisc::hashedString(p_url.
					 toEncoded(QUrl::
						   StripTrailingSlash),
					 &ok).toBase64());

		  if(ok)
		    deleteQuery.exec();
		}
	    }

	ok = true;
	query.prepare
	  ("INSERT OR REPLACE INTO history "
	   "(url, icon, last_visited, title, visits, temporary, "
	   "description, url_hash) "
	   "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
	query.bindValue
	  (0,
	   dmisc::etm(p_url.toEncoded(QUrl::StripTrailingSlash),
		      true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (1,
	     dmisc::etm(p_bytes, true, &ok));

	if(ok)
	  query.bindValue
	    (2,
	     dmisc::etm(QDateTime::currentDateTime().
			toString(Qt::ISODate).
			toUtf8(), true, &ok).toBase64());

	if(ok)
	  {
	    if(p_title.trimmed().isEmpty())
	      query.bindValue
		(3,
		 dmisc::etm(p_url.toEncoded(QUrl::
					    StripTrailingSlash),
			    true, &ok).toBase64());
	    else
	      query.bindValue
		(3,
		 dmisc::etm(p_title.toUtf8(), true, &ok).
		 toBase64());
	  }

	if(ok)
	  query.bindValue
	    (4,
	     dmisc::etm(QString::number(count).toLatin1(),
			true, &ok).toBase64());

	query.bindValue(5, p_temporary);

	if(ok)
	  query.bindValue
	    (6,
	     dmisc::etm(p_userData.toString().toUtf8(),
			true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (7,
	     dmisc::hashedString(p_url.
				 toEncoded(QUrl::
					   StripTrailingSlash),
				 &ok).toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("history_thread");
}

void dooble::slotShowWebInspector(void)
{
  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(p && p->page())
    p->page()->showWebInspector();
}

void dooble::slotSubmitUrlToSpotOn(void)
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  if(!dooble::s_spoton)
    return;

  dview *p = qobject_cast<dview *> (ui.tabWidget->currentWidget());

  if(!p)
    return;
  else if(!p->isLoaded())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dooble::s_spoton->share(p->url(), p->title(), p->description(), p->html());
  QApplication::restoreOverrideCursor();
#endif
}

void dooble::slotHideMainMenus(void)
{
  bool state = true;

  if(s_settings.value("mainWindow/hideMenuBar", false).toBool())
    state = false;

  QSettings settings;

  settings.setValue("mainWindow/hideMenuBar", state);
  s_settings["mainWindow/hideMenuBar"] = state;
  prepareMenuBar(state);
}

void dooble::prepareMenuBar(const bool state)
{
  menuBar()->setVisible(!state);
  ui.locationToolBar->setVisible(true);
  ui.menuToolButton->setVisible(state);

  if(state)
    {
      /*
      ** findChildren() does not return the menus in the
      ** desired order.
      */

      ui.menuToolButton->menu()->clear();
      ui.menuToolButton->menu()->addMenu(ui.menu_File);
      ui.menuToolButton->menu()->addMenu(ui.editMenu);
      ui.menuToolButton->menu()->addMenu(ui.bookmarksMenu);
      ui.menuToolButton->menu()->addMenu(ui.historyMenu);
      ui.menuToolButton->menu()->addMenu(ui.menu_Locations);
      ui.menuToolButton->menu()->addMenu(ui.menu_Tabs);
      ui.menuToolButton->menu()->addMenu(ui.viewMenu);
      ui.menuToolButton->menu()->addMenu(ui.menu_Windows);
      ui.menuToolButton->menu()->addMenu(ui.menu_About);
    }

  if(state)
    ui.action_Hide_Menubar->setText(tr("&Show Menu Bar"));
  else
    ui.action_Hide_Menubar->setText(tr("&Hide Menu Bar"));
}

void dooble::slotShowLocationBarButton(bool state)
{
  if(sender() == ui.action_Desktop_Button)
    {
      QSettings settings;

      settings.setValue("mainWindow/showDesktopButton", state);
      s_settings["mainWindow/showDesktopButton"] = state;
      ui.desktopToolButton->setVisible(state);
    }
  else if(sender() == ui.action_Home_Button)
    {
      QSettings settings;

      settings.setValue("mainWindow/showHomeButton", state);
      s_settings["mainWindow/showHomeButton"] = state;
      ui.homeToolButton->setVisible(state);
    }
}

void dooble::slotClearSpotOnSharedLinks(void)
{
  if(dooble::s_spoton)
    {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      dooble::s_spoton->clear();
      QApplication::restoreOverrideCursor();
    }
}

void dooble::slotShowSearchWidget(bool state)
{
  QSettings settings;

  settings.setValue("mainWindow/showSearchWidget", state);
  s_settings["mainWindow/showSearchWidget"] = state;
  ui.searchLineEdit->setVisible(state);
}

void dooble::slotSettingsChanged(void)
{
  ui.locationLineEdit->updateToolTips();
}

void dooble::slotNewPrivateTab(void)
{
  QUrl url;
  dview *p = 0;

  url = QUrl::fromUserInput
    (s_settings.value("settingsWindow/homeUrl", "").toString());

  if(!url.isValid())
    url = QUrl();
  else
    url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));

  QHash<QWebEngineSettings::WebAttribute, bool> webAttributes;

  webAttributes[QWebEngineSettings::JavascriptEnabled] = false;
  webAttributes[QWebEngineSettings::PluginsEnabled] = false;
  p = newTab(url, 0, webAttributes);

  if(p)
    {
      p->setPrivateCookies(true);
      ui.tabWidget->setCurrentWidget(p);
      ui.tabWidget->update();

      if(url.isEmpty() || !url.isValid())
	/*
	** p's url may be empty at this point.
	*/

	slotOpenUrl();
    }
}
