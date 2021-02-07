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

#ifndef dooble_about_h
#define dooble_about_h

#include <QFuture>
#include <QMainWindow>

#include "ui_dooble_about.h"

#ifndef dooble_architecture_h
#define dooble_architecture_h
#if defined(__alpha_ev4__)
#define DOOBLE_ARCHITECTURE_STR "__alpha_ev4__"
#elif defined(__alpha_ev5__)
#define DOOBLE_ARCHITECTURE_STR "__alpha_ev5__"
#elif defined(__alpha_ev6__)
#define DOOBLE_ARCHITECTURE_STR "__alpha_ev6__"
#elif defined(__alpha__)
#define DOOBLE_ARCHITECTURE_STR "__alpha__"
#elif defined(__amd64__)
#define DOOBLE_ARCHITECTURE_STR "__amd64__"
#elif defined(__amd64)
#define DOOBLE_ARCHITECTURE_STR "__amd64"
#elif defined(__x86_64__)
#define DOOBLE_ARCHITECTURE_STR "__x86_64__"
#elif defined(__x86_64)
#define DOOBLE_ARCHITECTURE_STR "__x86_64"
#elif defined(__ARM_ARCH_6J__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6J__"
#elif defined(__ARM_ARCH_6K__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6K__"
#elif defined(__ARM_ARCH_6T2__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6T2__"
#elif defined(__ARM_ARCH_6ZK__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6ZK__"
#elif defined(__ARM_ARCH_6Z__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6Z__"
#elif defined(__ARM_ARCH_6__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_6__"
#elif defined(__ARM_ARCH_7A__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_7A__"
#elif defined(__ARM_ARCH_7M__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_7M__"
#elif defined(__ARM_ARCH_7R__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_7R__"
#elif defined(__ARM_ARCH_7S__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_7S__"
#elif defined(__ARM_ARCH_7__)
#define DOOBLE_ARCHITECTURE_STR "__ARM_ARCH_7__"
#elif defined(i386)
#define DOOBLE_ARCHITECTURE_STR "i386"
#elif defined(__i386)
#define DOOBLE_ARCHITECTURE_STR "__i386"
#elif defined(__i386__)
#define DOOBLE_ARCHITECTURE_STR "__i386__"
#elif defined(__i486__)
#define DOOBLE_ARCHITECTURE_STR "__i486__"
#elif defined(__i586__)
#define DOOBLE_ARCHITECTURE_STR "__i586__"
#elif defined(__i686__)
#define DOOBLE_ARCHITECTURE_STR "__i686__"
#elif defined(_X86_)
#define DOOBLE_ARCHITECTURE_STR "_X86_"
#elif defined(__ia64__)
#define DOOBLE_ARCHITECTURE_STR "__ia64__"
#elif defined(_IA64)
#define DOOBLE_ARCHITECTURE_STR "_IA64"
#elif defined(__IA64__)
#define DOOBLE_ARCHITECTURE_STR "__IA64__"
#elif defined(__powerpc64__)
#define DOOBLE_ARCHITECTURE_STR "__powerpc64__"
#elif defined(__ppc64__)
#define DOOBLE_ARCHITECTURE_STR "__ppc64__"
#elif defined(__PPC64__)
#define DOOBLE_ARCHITECTURE_STR "__PPC64__"
#elif defined(_ARCH_PPC64)
#define DOOBLE_ARCHITECTURE_STR "_ARCH_PPC64"
#elif defined(__powerpc)
#define DOOBLE_ARCHITECTURE_STR "__powerpc"
#elif defined(__powerpc__)
#define DOOBLE_ARCHITECTURE_STR "__powerpc__"
#elif defined(__POWERPC__)
#define DOOBLE_ARCHITECTURE_STR "__POWERPC__"
#elif defined(__ppc__)
#define DOOBLE_ARCHITECTURE_STR "__ppc__"
#elif defined(__PPC__)
#define DOOBLE_ARCHITECTURE_STR "__PPC__"
#elif defined(_ARCH_PPC)
#define DOOBLE_ARCHITECTURE_STR "_ARCH_PPC"
#elif defined(__sparc_v8__)
#define DOOBLE_ARCHITECTURE_STR "__sparc_v8__"
#elif defined(__sparc_v9__)
#define DOOBLE_ARCHITECTURE_STR "__sparc_v9__"
#elif defined(__sparcv8)
#define DOOBLE_ARCHITECTURE_STR "__sparcv8"
#elif defined(__sparcv9)
#define DOOBLE_ARCHITECTURE_STR "__sparcv9"
#elif defined(__sparc__)
#define DOOBLE_ARCHITECTURE_STR "__sparc__"
#elif defined(__sparc)
#define DOOBLE_ARCHITECTURE_STR "__sparc"
#else
#define DOOBLE_ARCHITECTURE_STR "gingerjitter"
#endif
#endif

class dooble_about: public QMainWindow
{
  Q_OBJECT

 public:
  dooble_about(void);
  ~dooble_about();

 protected:
  void keyPressEvent(QKeyEvent *event);

 private:
  QFuture<void> m_future;
  Ui_dooble_about m_ui;
  void compute_self_digest(void);
  void compute_self_digest_task(const QString &file_path);

 private slots:
  void slot_file_digest_computed(const QByteArray &digest);
  void slot_link_activated(const QString &url);

 signals:
  void file_digest_computed(const QByteArray &digest);
  void link_activated(const QUrl &url);
};

#endif
