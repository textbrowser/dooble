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

#include "dooble.h"
#include "dspoton.h"

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
#ifdef Q_OS_WIN32
#define LIBSPOTON_OS_WINDOWS 1
#endif

extern "C"
{
#include "libSpotOn/libspoton.h"
}
#endif

dspoton::dspoton(void):QObject()
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(&m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotTimeout(void)));
  m_timer.start(5000);
#endif
}

dspoton::~dspoton()
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  m_timer.stop();
#endif
}

bool dspoton::isKernelRegistered(void)
{
  bool registered = false;
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  if(dmisc::passphraseWasPrepared())
    if(dmisc::s_crypt)
      registered = true;
#endif
  return registered;
}

void dspoton::registerWidget(QWidget *widget)
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(this,
	  SIGNAL(spotonKernelRegistered(bool)),
	  widget,
	  SLOT(setEnabled(bool)));
#else
  Q_UNUSED(widget);
#endif
}

void dspoton::share(const QUrl &url,
		    const QString &title,
		    const QString &description)
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  if(dmisc::passphraseWasPrepared())
    if(dmisc::s_crypt)
      {
	libspoton_error_t err = LIBSPOTON_ERROR_NONE;
	libspoton_handle_t libspotonHandle;

	if((err = libspoton_init_a(dooble::s_settings.
				   value("settingsWindow/"
					 "spotOnSharedDatabase").toString().
				   toStdString().c_str(),
				   dmisc::s_crypt->cipherType().toStdString().
				   c_str(),
				   dmisc::s_crypt->encryptionKey(),
				   dmisc::s_crypt->encryptionKeyLength(),
				   &libspotonHandle,
				   16384)) == LIBSPOTON_ERROR_NONE)
	  err = libspoton_save_url
	    (url.toEncoded(QUrl::StripTrailingSlash).constData(),
	     url.toEncoded(QUrl::StripTrailingSlash).length(),
	     title.toUtf8().constData(),
	     title.toUtf8().length(),
	     description.toUtf8().constData(),
	     description.toUtf8().length(),
	     &libspotonHandle);

	libspoton_close(&libspotonHandle);
      }
#else
  Q_UNUSED(description);
  Q_UNUSED(title);
  Q_UNUSED(url);
#endif
}

#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
void dspoton::slotTimeout(void)
{
  emit spotonKernelRegistered(isKernelRegistered());
}
#endif
