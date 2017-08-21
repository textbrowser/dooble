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
#include <QSplashScreen>
#ifdef Q_OS_WIN32
#include <QStyleFactory>
#endif
#include <QThread>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

extern "C"
{
#include <fcntl.h>
#include <signal.h>
#if defined(Q_OS_MACOS)
#include <sys/resource.h>
#endif
#if defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
#include <unistd.h>
#endif
}

#ifdef Q_OS_MACOS
#include "CocoaInitializer.h"
#endif
#include "dooble.h"
#include "dooble_application.h"
#include "dooble_blocked_domains.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_settings.h"

class dooble_thread: public QThread
{
 public:
  static void msleep(unsigned long secs)
  {
    QThread::msleep(secs);
  }
};

static char *s_doobleAbortedFileName = 0;

static void signal_handler(int signal_number)
{
  /*
  ** _Exit() and _exit() may be safely called from signal handlers.
  */

  static int fatal_error = 0;

  if(fatal_error)
    _Exit(signal_number);

  fatal_error = 1;

  if(signal_number == SIGTERM)
    {
      /*
      ** Remove restoration files.
      */
    }
  else
    {
      /*
      ** Create a special file.
      */

      if(s_doobleAbortedFileName)
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
	close(open(s_doobleAbortedFileName, O_CREAT, S_IRUSR | S_IWUSR));
#else
        close(creat(s_doobleAbortedFileName, O_CREAT));
#endif
    }

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
  struct sigaction signal_action;
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

  while(!list.isEmpty())
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
      signal_action.sa_handler = signal_handler;
      sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = 0;
      sigaction(list.takeFirst(), &signal_action, (struct sigaction *) 0);
#else
      signal(list.takeFirst(), signal_handler);
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
  /*
  ** Ignore SIGPIPE.
  */

  signal_action.sa_handler = SIG_IGN;
  sigemptyset(&signal_action.sa_mask);
  signal_action.sa_flags = 0;
  sigaction(SIGPIPE, &signal_action, (struct sigaction *) 0);
#endif

#ifdef Q_OS_WIN32
  QApplication::addLibraryPath("plugins");
  QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif
  dooble::s_application = new dooble_application(argc, argv);

  /*
  ** Create a splash screen.
  */

  QSplashScreen splash(QPixmap(":/splash.png"));

  splash.show();
  splash.showMessage
    (QObject::tr("Initializing Dooble."), Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();

#ifdef Q_OS_MACOS
  /*
  ** Eliminate pool errors on OS X.
  */

  CocoaInitializer cocoa_initializer;
#endif

#ifdef Q_OS_WIN32
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

  splash.showMessage
    (QObject::tr("Purging temporary favicons."),
     Qt::AlignHCenter | Qt::AlignBottom);
  splash.repaint();
  dooble::s_application->processEvents();
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
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::FullScreenSupportEnabled, true);
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  QWebEngineSettings::globalSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, false);
  dooble_thread::msleep(750);
  splash.finish(0);
  dooble::s_settings = new dooble_settings();

  dooble *d = new dooble();

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
		   dooble::s_blocked_domains,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_cookies,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_history,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookie_added(const QNetworkCookie &, bool)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookie_added(const QNetworkCookie &, bool)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookie_removed(const QNetworkCookie &)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookie_removed(const QNetworkCookie &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_cookie(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_cookie(const QNetworkCookie &)));
  QObject::connect(dooble::s_settings,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)));
  d->show();

  int rc = dooble::s_application->exec();

  dooble_favicons::purge_temporary();
  dooble::s_application->deleteLater();
  return rc;
}
