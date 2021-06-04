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
  m_read_offset = 0;
  m_type = tr("Text File");
}

dooble_charts_file::~dooble_charts_file()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void dooble_charts_file::run(void)
{
  QReadLocker lock(&m_address_mutex);
  auto address(m_address);

  lock.unlock();

  QFile file(address);

  if(file.open(QIODevice::ReadOnly))
    {
      QReadLocker lock(&m_read_offset_mutex);
      auto read_offset = m_read_offset;

      lock.unlock();

      if(file.seek(read_offset))
	{
	  QByteArray bytes;
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
	  auto read_size = static_cast<qint64> (m_read_size.load());
#else
	  auto read_size = static_cast<qint64> (m_read_size.loadRelaxed());
#endif
	  qint64 rc = 0;

	  bytes.resize(static_cast<int> (read_size));

	  if((rc = file.read(bytes.data(), read_size)) > 0)
	    {
	      QWriteLocker lock(&m_read_offset_mutex);

	      m_read_offset += rc;
	      lock.unlock();
	      emit bytes_read(bytes);
	    }
	  else if(rc == 0)
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
	    m_finished.store(1);
#else
	    m_finished.storeRelaxed(1);
#endif
	}
    }
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
