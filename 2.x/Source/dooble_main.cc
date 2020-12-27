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
#include <QElapsedTimer>
#include <QSplashScreen>
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
#include <QStyleFactory>
#endif
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#include <QWebEngineUrlScheme>
#endif

extern "C"
{
#if defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#endif
#if defined(Q_OS_MACOS)
#include <sys/resource.h>
#endif
}

#ifdef Q_OS_MACOS
#include "CocoaInitializer.h"
#endif
#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_random.h"
#include "dooble_search_engines_popup.h"
#include "dooble_style_sheet.h"

#include <csignal>
#include <iostream>

static void signal_handler(int signal_number)
{
  /*
  ** _Exit() and _exit() may be safely called from signal handlers.
  */

  static int fatal_error = 0;

  if(fatal_error)
    _Exit(signal_number);

  fatal_error = 1;
  _Exit(signal_number);
}

int main(int argc, char *argv[])
{
  qputenv("QT_ENABLE_REGEXP_JIT", "0");
  qputenv("QV4_FORCE_INTERPRETER", "1");

#ifdef Q_OS_MACOS
  struct rlimit rlim = {0, 0};

  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = OPEN_MAX;
  setrlimit(RLIMIT_NOFILE, &rlim);
#endif

  QList<int> list;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
  struct sigaction signal_action = {};
#endif

  list << SIGABRT
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
       << SIGBUS
#endif
       << SIGFPE
       << SIGILL
       << SIGINT
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
       << SIGQUIT
#endif
       << SIGSEGV
       << SIGTERM;

  for(const auto i : list)
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
      memset(&signal_action, 0, sizeof(struct sigaction));
      signal_action.sa_handler = signal_handler;
      sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = 0;

      if(sigaction(i, &signal_action, (struct sigaction *) nullptr))
	std::cerr << "sigaction() failure on " << i << std::endl;
#else
      signal(list.at(i), signal_handler);
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
  /*
  ** Ignore SIGPIPE.
  */

  memset(&signal_action, 0, sizeof(struct sigaction));
  signal_action.sa_handler = SIG_IGN;
  sigemptyset(&signal_action.sa_mask);
  signal_action.sa_flags = 0;
  sigaction(SIGPIPE, &signal_action, (struct sigaction *) nullptr);
#endif
  qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>
    ("QAbstractItemModel::LayoutChangeHint");
  qRegisterMetaType<QList<QPersistentModelIndex> >
    ("QListQPersistentModelIndex");
  qRegisterMetaType<QListPairIconString> ("QListPairIconString");
  qRegisterMetaType<QListUrl> ("QListUrl");
  qRegisterMetaType<QListVectorByteArray> ("QListVectorByteArray");
  qRegisterMetaType<Qt::SortOrder> ("Qt::SortOrder");
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#ifdef Q_OS_WIN
  QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
#endif
#else
  QCoreApplication::setAttribute
    (Qt::AA_EnableHighDpiScaling,
     QVariant(qgetenv("AA_ENABLEHIGHDPISCALING")).toBool());
  QCoreApplication::setAttribute
    (Qt::AA_UseHighDpiPixmaps,
     QVariant(qgetenv("AA_USEHIGHDPIPIXMAPS")).toBool());
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  QWebEngineUrlScheme scheme("gopher");

  scheme.setDefaultPort(70);
  scheme.setFlags(QWebEngineUrlScheme::ViewSourceAllowed);
  scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
  QWebEngineUrlScheme::registerScheme(scheme);
#endif

  dooble::s_application = new dooble_application(argc, argv);

#ifdef Q_OS_MACOS
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
  QApplication::setStyle(QStyleFactory::create("Macintosh"));
#endif
  QDir::setCurrent("/Applications/Dooble.d");
#elif defined(Q_OS_WIN)
  QApplication::addLibraryPath("plugins");
  QApplication::setStyle(QStyleFactory::create("Windows"));
#endif
#if defined(Q_OS_WIN)
  QByteArray tmp(qgetenv("USERNAME").mid(0, 32));
  QDir home_dir(QDir::current());
  QFileInfo file_info(home_dir.absolutePath());
  QString username(tmp);

  if(!(file_info.isReadable() && file_info.isWritable()))
    home_dir = QDir::home();

  if(username.isEmpty())
    home_dir.mkdir(".dooble_v2");
  else
    home_dir.mkdir(username + QDir::separator() + ".dooble_v2");

  if(username.isEmpty())
    dooble_settings::set_setting
      ("home_path", home_dir.absolutePath() + QDir::separator() + ".dooble_v2");
  else
    dooble_settings::set_setting("home_path",
				 home_dir.absolutePath() +
				 QDir::separator() +
				 username + QDir::separator() +
				 ".dooble_v2");
#else
  QDir home_dir(QDir::home());

  home_dir.mkdir(".dooble_v2");
  dooble_settings::set_setting
    ("home_path", home_dir.absolutePath() + QDir::separator() + ".dooble_v2");
#endif

  /*
  ** Create a splash screen.
  */

  QElapsedTimer t;
  QSplashScreen splash(QPixmap(":/Miscellaneous/splash.png"));

  splash.setEnabled(false);
  splash.show();
  splash.showMessage
    (QObject::tr("Initializing Dooble."), Qt::AlignHCenter | Qt::AlignBottom);
  t.start();

  while(t.elapsed() < 1000)
    splash.repaint();

  dooble_random::initialize();
  dooble::s_application->processEvents();

#ifdef Q_OS_MACOS
  /*
  ** Eliminate pool errors on OS X.
  */

  CocoaInitializer cocoa_initializer;
#endif
  dooble::s_application->install_translator();
  splash.showMessage
    (QObject::tr("Purging temporary database entries."),
     Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();
  dooble_certificate_exceptions_menu_widget::purge_temporary();
  dooble_favicons::purge_temporary();
  splash.showMessage
    (QObject::tr("Preparing QWebEngine."), Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();
  QWebEngineProfile::defaultProfile()->setCachePath
    (dooble_settings::setting("home_path").toString() +
     QDir::separator() +
     "WebEngineCache");
  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(0);
  QWebEngineProfile::defaultProfile()->setHttpCacheType
    (QWebEngineProfile::MemoryHttpCache);
  QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy
    (QWebEngineProfile::NoPersistentCookies);
  QWebEngineProfile::defaultProfile()->setPersistentStoragePath
    (dooble_settings::setting("home_path").toString() +
     QDir::separator() +
     "WebEnginePersistentStorage");
  QWebEngineProfile::defaultProfile()->setSpellCheckEnabled(true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ErrorPageEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::FocusOnNavigationEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::FullScreenSupportEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScreenCaptureEnabled, false);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
#endif
#endif
  splash.showMessage(QObject::tr("Preparing Dooble objects."),
		     Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();
  dooble::s_settings = new dooble_settings();

  QStringList arguments(QCoreApplication::arguments());
  auto *d = new dooble
    (QUrl(), arguments.contains("--private") ||
             dooble::s_settings->setting("private_mode").toBool());

  QObject::connect(QWebEngineProfile::defaultProfile()->cookieStore(),
		   SIGNAL(cookieAdded(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_cookie_added(const QNetworkCookie &)));
  QObject::connect(QWebEngineProfile::defaultProfile()->cookieStore(),
		   SIGNAL(cookieRemoved(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_cookie_removed(const QNetworkCookie &)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_accepted_or_blocked_domains,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_certificate_exceptions,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_cookies,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_downloads,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_history,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_search_engines_window,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_settings,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_style_sheet,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(address_widget_populated(void)),
		   d,
		   SLOT(slot_populated(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookies_added(const QList<QNetworkCookie> &,
					const QList<bool> &)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookies_added(const QList<QNetworkCookie> &,
					   const QList<bool> &)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookie_removed(const QNetworkCookie &)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookie_removed(const QNetworkCookie &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_cookie(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_cookie(const QNetworkCookie &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_domain(const QString &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_domain(const QString &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_items(const QList<QNetworkCookie> &,
				       const QStringList &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_items(const QList<QNetworkCookie> &,
					  const QStringList &)));
  QObject::connect(dooble::s_settings,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)));
  QObject::connect(dooble::s_settings,
		   SIGNAL(populated(void)),
		   d,
		   SLOT(slot_populated(void)));
  splash.showMessage(QObject::tr("Populating containers."),
		     Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();

  while(!d->initialized())
    splash.repaint();

  splash.finish(nullptr);
  QTimer::singleShot(1500, d, SLOT(show(void)));
  return dooble::s_application->exec();
}
