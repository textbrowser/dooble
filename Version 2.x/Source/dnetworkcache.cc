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

#include <QBuffer>
#include <QWriteLocker>
#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#endif
#include <QtCore>

#include "dooble.h"
#include "dnetworkcache.h"

dnetworkcache::dnetworkcache(void):QAbstractNetworkCache()
{
  m_cacheSize = 0;
  QDir().mkpath(dooble::s_homePath + QDir::separator() + "Cache");
  m_dir.setPath(dooble::s_homePath + QDir::separator() + "Cache");
  m_timer.setInterval(10000);
  connect(&m_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slotTimeout(void)));
  m_timer.start();
}

dnetworkcache::~dnetworkcache()
{
  m_timer.stop();
  clearTemp();
}

void dnetworkcache::populate(void)
{
  if(dmisc::passphraseWasAuthenticated())
    clearTemp();
}

qint64 dnetworkcache::cacheSize(void) const
{
  m_cacheSizeMutex.lockForRead();

  qint64 size = m_cacheSize;

  m_cacheSizeMutex.unlock();
  return size;
}

QIODevice *dnetworkcache::data(const QUrl &url)
{
  if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			       false).toBool())
    return 0;
  else if(url.isEmpty() || !url.isValid())
    return 0;
  else if(dooble::s_cacheExceptionsWindow->allowed(url.host()))
    return 0;

  QString hash;
  bool ok = true;

  hash = dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash),
			     &ok).toHex();

  if(!ok)
    return 0;

  QFile file;
  QString path(hash.mid(0, 2));
  QByteArray bytes;

  if(path.isEmpty())
    path = "xy";

  file.setFileName
    (m_dir.absolutePath() + QDir::separator() + path + QDir::separator() +
     hash + "_data_" + QString::number(!dmisc::passphraseWasAuthenticated()));

  if((ok = file.open(QIODevice::ReadOnly)))
    if(!(bytes = file.readAll()).isEmpty())
      bytes = dmisc::daa(bytes, &ok);

  if(bytes.isEmpty() || !ok)
    {
      file.close();
      file.remove();
      return 0;
    }

  file.close();

  QBuffer *buffer = new QBuffer();

  buffer->setData(bytes);

  if(!buffer->open(QIODevice::ReadOnly))
    {
      buffer->close();
      buffer->deleteLater();
      return 0;
    }

  return buffer;
}

void dnetworkcache::insert(QIODevice *device)
{
  if(!device)
    return;
  else if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
				    false).toBool())
    {
      device->deleteLater();
      return;
    }

  QBuffer *buffer = qobject_cast<QBuffer *> (device);

  if(!buffer || !buffer->isOpen())
    {
      device->deleteLater();
      return;
    }

  QUrl url(buffer->property("dooble-url").toUrl());

  if(url.isEmpty() || !url.isValid())
    {
      buffer->close();
      buffer->deleteLater();
      return;
    }
  else if(dooble::s_cacheExceptionsWindow->allowed(url.host()))
    {
      buffer->close();
      buffer->deleteLater();
      return;
    }

  insertEntry(buffer->data(), url);
  buffer->close();
  buffer->deleteLater();
}

QNetworkCacheMetaData dnetworkcache::metaData(const QUrl &url)
{
  QNetworkCacheMetaData metaData;

  if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			       false).toBool())
    return metaData;
  else if(url.isEmpty() || !url.isValid())
    return metaData;
  else if(dooble::s_cacheExceptionsWindow->allowed(url.host()))
    return metaData;

  QString hash;
  bool ok = true;

  hash = dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash),
			     &ok).toHex();

  if(!ok)
    return metaData;

  QFile file;
  QString path(hash.mid(0, 2));

  if(path.isEmpty())
    path = "xy";

  file.setFileName
    (m_dir.absolutePath() + QDir::separator() + path + QDir::separator() +
     hash + "_metadata_" +
     QString::number(!dmisc::passphraseWasAuthenticated()));

  if((ok = file.open(QIODevice::ReadOnly)))
    {
      QByteArray bytes;

      if((ok = !(bytes = file.readAll()).isEmpty()))
	{
	  bytes = dmisc::daa(bytes, &ok);

	  if(ok)
	    {
	      QDataStream in(&bytes, QIODevice::ReadOnly);

	      in >> metaData;

	      if(in.status() != QDataStream::Ok)
		{
		  metaData = QNetworkCacheMetaData();
		  ok = false;
		}
	    }
	}
    }

  file.close();

  if(!ok)
    file.remove();

  return metaData;
}

QIODevice *dnetworkcache::prepare(const QNetworkCacheMetaData &metaData)
{
  if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			       false).toBool())
    return 0;
  else if(!metaData.isValid() || metaData.url().isEmpty() ||
	  !metaData.url().isValid() || !metaData.saveToDisk())
    return 0;
  else if(dooble::s_cacheExceptionsWindow->allowed(metaData.url().host()))
    return 0;

  bool ok = true;
  int cacheSize = 0;
  int value = qAbs
    (dooble::s_settings.value("settingsWindow/webDiskCacheSize", 50).
     toInt(&ok));
  qint64 size = 0;

  if(!ok)
    value = 50;

  cacheSize = 1048576 * value;

  foreach(QNetworkCacheMetaData::RawHeader header, metaData.rawHeaders())
    if(header.first.toLower() == "content-length")
      {
	size = header.second.toInt();

	/*
	** Be careful not to cache large objects.
	*/

	if(size > (cacheSize * 3) / 4)
	  return 0;
      }
    else if(header.first.toLower() == "content-type" &&
	    header.second.toLower().contains("application"))
      return 0;
    else if(header.first.toLower() == "content-type" &&
	    header.second.toLower().contains("ecmascript"))
      return 0;
    else if(header.first.toLower() == "content-type" &&
	    header.second.toLower().contains("javascript"))
      return 0;

  if(!prepareEntry(metaData))
    return 0;

  QBuffer *buffer = new QBuffer(this);

  buffer->setProperty("dooble-url", metaData.url());

  if(!buffer->open(QIODevice::ReadWrite))
    {
      buffer->close();
      buffer->deleteLater();
      return 0;
    }

  return buffer;
}

bool dnetworkcache::remove(const QUrl &url)
{
  if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			       false).toBool())
    return false;
  else if(url.isEmpty() || !url.isValid())
    return false;

  QString hash;
  bool ok = true;

  hash = dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash),
			     &ok).toHex();

  if(!ok)
    return false;

  QString path(hash.mid(0, 2));
  int bit = 1;

  if(path.isEmpty())
    path = "xy";

  bit &= static_cast<int>
    (QFile::remove(m_dir.absolutePath() + QDir::separator() +
		   path + QDir::separator() +
		   hash + "_data_" +
		   QString::
		   number(static_cast<int> (!dmisc::
					    passphraseWasAuthenticated()))));
  bit &= static_cast<int>
    (QFile::remove(m_dir.absolutePath() + QDir::separator() +
		   path + QDir::separator() +
		   hash + "_metadata_" +
		   QString::
		   number(static_cast<int> (!dmisc::
					    passphraseWasAuthenticated()))));

  QDir dir(m_dir);

  dir.rmdir(path);
  return static_cast<bool> (bit);
}

void dnetworkcache::updateMetaData(const QNetworkCacheMetaData &metaData)
{
  if(!dooble::s_settings.value("settingsWindow/diskCacheEnabled",
			       false).toBool())
    return;

  QUrl url(metaData.url());

  if(url.isEmpty() || !url.isValid())
    return;
  else if(dooble::s_cacheExceptionsWindow->allowed(url.host()))
    return;

  prepareEntry(metaData);
}

void dnetworkcache::clear(void)
{
  if(m_clearFuture.isFinished())
    m_clearFuture = QtConcurrent::run
      (this, &dnetworkcache::clearInThread, m_dir);
}

void dnetworkcache::clearInThread(const QDir &dir)
{
  QStringList list(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));

  while(!list.isEmpty())
    {
      QDir l_dir(dir);

      l_dir.cd(list.takeFirst());

      QStringList files(l_dir.entryList(QDir::Files));

      while(!files.isEmpty())
	l_dir.remove(files.takeFirst());

      dir.rmdir(l_dir.absolutePath());
    }
}

void dnetworkcache::clearTemp(void)
{
  QStringList list(m_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));

  while(!list.isEmpty())
    {
      QDir dir(m_dir);

      dir.cd(list.takeFirst());

      QStringList files(dir.entryList(QStringList("*_1"), QDir::Files));

      while(!files.isEmpty())
	dir.remove(files.takeFirst());

      m_dir.rmdir(dir.absolutePath());
    }
}

void dnetworkcache::reencode(QProgressBar *progress)
{
  if(!dmisc::passphraseWasAuthenticated())
    return;

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  m_timer.stop();
  m_future.waitForFinished();
  m_clearFuture.waitForFinished();
  clearTemp();

  QStringList list(m_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));

  while(!list.isEmpty())
    {
      QDir dir(m_dir);
      QString directoryName(list.takeFirst());

      dir.cd(directoryName);

      QStringList files(dir.entryList(QStringList("*_data_0"), QDir::Files));

      while(!files.isEmpty())
	{
	  QFile file;
	  QString fileName(files.takeFirst());

	  file.setFileName
	    (dir.absolutePath() + QDir::separator() + fileName);

	  QByteArray data;

	  if(file.open(QIODevice::ReadOnly))
	    if(!(data = file.readAll()).isEmpty())
	      {
		bool ok = true;

		data = dmisc::daa
		  (dmisc::s_reencodeCrypt, data, &ok);

		if(!ok)
		  data.clear();
	      }

	  file.close();
	  file.remove();

	  QNetworkCacheMetaData metaData;

	  file.setFileName
	    (dir.absolutePath() + QDir::separator() +
	     fileName.replace("_data", "_metadata"));

	  if(file.open(QIODevice::ReadOnly))
	    {
	      QByteArray bytes;

	      if(!(bytes = file.readAll()).isEmpty())
		{
		  bool ok = true;

		  bytes = dmisc::daa
		    (dmisc::s_reencodeCrypt, bytes, &ok);

		  if(ok)
		    {
		      QDataStream in(&bytes, QIODevice::ReadOnly);

		      in >> metaData;

		      if(in.status() != QDataStream::Ok)
			metaData = QNetworkCacheMetaData();
		    }
		}
	    }

	  file.close();
	  file.remove();

	  if(!data.isEmpty() && metaData.isValid())
	    if(prepareEntry(metaData))
	      insertEntry(data, metaData.url());
	}

      /*
      ** Remove empty directories.
      */

      dir.rmdir(dir.absolutePath());
    }

  if(progress)
    progress->setVisible(false);

  m_timer.start();
}

void dnetworkcache::slotTimeout(void)
{
  if(m_future.isFinished())
    {
      int diskCacheSize = 1048576 * dooble::s_settings.value
	("settingsWindow/webDiskCacheSize", 50).toInt();

      m_future = QtConcurrent::run
	(this, &dnetworkcache::computeCacheSizeAndCleanDirectory,
	 diskCacheSize, m_dir.absolutePath());
    }
}

void dnetworkcache::insertEntry(const QByteArray &data,
				const QUrl &url)
{
  QString hash;
  bool ok = true;

  hash = dmisc::hashedString(url.toEncoded(QUrl::StripTrailingSlash), &ok).
    toHex();

  if(!ok)
    return;

  QFile file;
  QString path(hash.mid(0, 2));

  if(path.isEmpty())
    path = "xy";

  m_dir.mkpath(path);
  file.setFileName
    (m_dir.path() + QDir::separator() + path + QDir::separator() +
     hash + "_data_" + QString::number(!dmisc::passphraseWasAuthenticated()));

  if((ok = file.open(QIODevice::Truncate | QIODevice::WriteOnly)))
    {
      QByteArray bytes(dmisc::etm(data, true, &ok));

      if(ok)
	if(file.write(bytes) != bytes.size())
	  ok = false;
    }

  if(ok)
    file.flush();

  file.close();

  if(!ok)
    file.remove();
}

bool dnetworkcache::prepareEntry(const QNetworkCacheMetaData &metaData)
{
  QString hash;
  bool ok = true;

  hash = dmisc::hashedString(metaData.url().
			     toEncoded(QUrl::StripTrailingSlash), &ok).
    toHex();

  if(!ok)
    return false;

  QFile file;
  QString path(hash.mid(0, 2));

  if(path.isEmpty())
    path = "xy";

  m_dir.mkpath(path);
  file.setFileName
    (m_dir.path() + QDir::separator() + path + QDir::separator() +
     hash + "_metadata_" +
     QString::number(!dmisc::passphraseWasAuthenticated()));

  if((ok = file.open(QIODevice::Truncate | QIODevice::WriteOnly)))
    {
      QByteArray bytes;
      QDataStream out(&bytes, QIODevice::WriteOnly);

      out << metaData;

      if(out.status() != QDataStream::Ok)
	ok = false;

      if(ok)
	bytes = dmisc::etm(bytes, true, &ok);

      if(ok)
	if(file.write(bytes) != bytes.size())
	  ok = false;
    }

  if(ok)
    file.flush();

  file.close();

  if(!ok)
    file.remove();

  return ok;
}

void dnetworkcache::computeCacheSizeAndCleanDirectory
(const int cacheSizeDesired, const QString &path)
{
  /*
  ** Please note that this method modifies the contents of the
  ** m_cacheSize variable.
  */

  QDir dir1(path);
  qint64 size = 0;

  {
    QFileInfoList list(dir1.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));

    while(!list.isEmpty())
      {
	QDir dir2(dir1);

	dir2.cd(list.takeFirst().fileName());

	QFileInfoList files(dir2.entryInfoList(QDir::Files));

	while(!files.isEmpty())
	  size += files.takeFirst().size();
      }
  }

  double p = static_cast<double> (size) /
    static_cast<double> (qMax(1, cacheSizeDesired));

  if(size <= 0 || p <= 0.80)
    {
      QWriteLocker locker(&m_cacheSizeMutex);

      m_cacheSize = size;
      return;
    }

  dir1.setPath(path);

  {
    QStringList list(dir1.entryList(QDir::Dirs | QDir::NoDotAndDotDot));

    while(!list.isEmpty())
      {
	QDir dir2(dir1);

	dir2.cd(list.takeFirst());

	QFileInfoList files
	  (dir2.entryInfoList(QDir::Files, QDir::Reversed | QDir::Time));

	while(!files.isEmpty())
	  {
	    QFileInfo fileInfo(files.takeFirst());

	    size -= fileInfo.size();
	    dir2.remove(fileInfo.fileName());
	    fileInfo = QFileInfo
	      (fileInfo.fileName().replace("_data", "_metadata"));
	    size -= fileInfo.size();
	    dir2.remove(fileInfo.fileName());
	    dir1.rmdir(dir2.absolutePath());
	    p = static_cast<double> (size) /
	      static_cast<double> (qMax(1, cacheSizeDesired));

	    if(size <= 0 || p <= 0.80)
	      {
		QWriteLocker locker(&m_cacheSizeMutex);

		m_cacheSize = size;
		return;
	      }
	  }
      }
  }

  QWriteLocker locker(&m_cacheSizeMutex);

  m_cacheSize = size;
}
