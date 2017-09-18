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

#ifndef _dnetworkcache_h_
#define _dnetworkcache_h_

#include <QAbstractNetworkCache>
#include <QDir>
#include <QFuture>
#include <QReadWriteLock>
#include <QTimer>

class QProgressBar;

class dnetworkcache: public QAbstractNetworkCache
{
  Q_OBJECT

 public:
  dnetworkcache(void);
  ~dnetworkcache();
  qint64 cacheSize(void) const;
  QIODevice *data(const QUrl &url);
  void insert(QIODevice *device);
  QNetworkCacheMetaData metaData(const QUrl &url);
  void populate(void);
  QIODevice *prepare(const QNetworkCacheMetaData &metaData);
  void reencode(QProgressBar *progress);
  bool remove(const QUrl &url);
  void updateMetaData(const QNetworkCacheMetaData &metaData);

 private:
  QDir m_dir;
  QFuture<void> m_future;
  QFuture<void> m_clearFuture;
  QTimer m_timer;
  mutable QReadWriteLock m_cacheSizeMutex;
  qint64 m_cacheSize;
  bool prepareEntry(const QNetworkCacheMetaData &metaData);
  void clearTemp(void);
  void clearInThread(const QDir &dir);
  void computeCacheSizeAndCleanDirectory(const int cacheSizeDesired,
					 const QString &path);
  void insertEntry(const QByteArray &data, const QUrl &url);

 public slots:
  void clear(void);

 private slots:
  void slotTimeout(void);
};

#endif
