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

#include <QApplication>

extern "C"
{
#include <fcntl.h>
#include <signal.h>
#if defined(Q_OS_MAC)
#include <sys/resource.h>
#endif
#if defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
#include <unistd.h>
#endif
}

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
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
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

#ifdef Q_OS_MAC
  struct rlimit rlim = {0, 0};

  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = OPEN_MAX;
  setrlimit(RLIMIT_NOFILE, &rlim);
#endif

  /*
  ** Prepare signal handlers. Clearly, this does not account for
  ** the user's desire to restore sessions.
  */

  QList<int> list;
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
  struct sigaction signal_action;
#endif

  list << SIGABRT
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

  while(!list.isEmpty())
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
      signal_action.sa_handler = signal_handler;
      sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = 0;
      sigaction(list.takeFirst(), &signal_action, (struct sigaction *) 0);
#else
      signal(list.takeFirst(), signal_handler);
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
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
  QApplication::setStyle("fusion");
#endif

  QApplication qapplication(argc, argv);

  return qapplication.exec();
}
