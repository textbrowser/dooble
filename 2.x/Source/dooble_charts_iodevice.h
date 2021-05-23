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

#ifndef dooble_charts_iodevice_h
#define dooble_charts_iodevice_h

#include <QIODevice>
#include <QReadWriteLock>
#include <QTimer>

class dooble_charts_iodevice: public QIODevice
{
  Q_OBJECT

 public:
  dooble_charts_iodevice(QObject *parent):QIODevice(parent)
  {
    m_read_interval = 256;
    m_read_size = 1024;
    m_read_timer.setInterval(m_read_interval);
  }

  virtual ~dooble_charts_iodevice()
  {
    m_read_timer.stop();
  }

  virtual void start(void) = 0;
  virtual void stop(void) = 0;

  void set_address(const QString &address)
  {
    QWriteLocker locker(&m_address_mutex);

    m_address = address;
  }

  void set_read_interval(const int interval)
  {
    m_read_interval = qMax(1, interval);
    m_read_timer.setInterval(m_read_interval);
  }

  void set_read_size(const int size)
  {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    m_read_size.store(qMax(1, size));
#else
    m_read_size.storeRelaxed(qMax(1, size));
#endif
  }

 protected:
  QAtomicInteger<int> m_read_size;
  QReadWriteLock m_address_mutex;
  QString m_address;
  QTimer m_read_timer;
  int m_read_interval;
};

#endif
