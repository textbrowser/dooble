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

#include "dooble_charts_file.h"

#include <QtConcurrent>

dooble_charts_file::dooble_charts_file(QObject *parent):
  dooble_charts_iodevice(parent)
{
  connect(&m_read_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_timeout(void)));
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  m_finished.store(0);
#else
  m_finished.storeRelaxed(0);
#endif
  m_read_offset = 0;
}

dooble_charts_file::~dooble_charts_file()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void dooble_charts_file::run(void)
{
}

void dooble_charts_file::slot_timeout(void)
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  if(m_finished.load())
#else
  if(m_finished.loadRelaxed())
#endif
    {
      m_read_timer.stop();
      return;
    }

  if(!m_future.isFinished())
    m_future = QtConcurrent::run(this, &dooble_charts_file::run);
}

void dooble_charts_file::start(void)
{
  m_read_timer.start();
}

void dooble_charts_file::stop(void)
{
  m_read_timer.stop();
}
