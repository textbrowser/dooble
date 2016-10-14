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
#include <QCryptographicHash>
#include <QDesktopWidget>
#include <QDir>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtCore/qmath.h>

#include <bitset>

#include "derrorlog.h"
#include "dmisc.h"
#include "dooble.h"

/*
** One of the below proxy methods (a static one)
** returns a list of proxies. How does one detect which proxy is valid?
*/

/*
** The method systemProxyForQuery() may take several
** seconds to execute on Windows systems.
*/

extern "C"
{
#include <errno.h>
#ifdef DOOBLE_USE_PTHREADS
#include <pthread.h>
#if !defined(GCRYPT_VERSION_NUMBER) || GCRYPT_VERSION_NUMBER < 0x010600
  GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif
#endif
}

#ifndef DOOBLE_USE_PTHREADS
#include <QMutex>
extern "C"
{
  int gcry_qthread_init(void)
  {
    return 0;
  }

  int gcry_qmutex_init(void **mutex)
  {
    *mutex = static_cast<void *> (new (std::nothrow) QMutex());

    if(*mutex)
      return 0;
    else
      return -1;
  }

  int gcry_qmutex_destroy(void **mutex)
  {
    delete static_cast<QMutex *> (*mutex);
    return 0;
  }

  int gcry_qmutex_lock(void **mutex)
  {
    QMutex *m = static_cast<QMutex *> (*mutex);

    if(m)
      {
	m->lock();
	return 0;
      }
    else
      return -1;
  }

  int gcry_qmutex_unlock(void **mutex)
  {
    QMutex *m = static_cast<QMutex *> (*mutex);

    if(m)
      {
	m->unlock();
	return 0;
      }
    else
      return -1;
  }
}

struct gcry_thread_cbs gcry_threads_qt =
  {
    GCRY_THREAD_OPTION_USER, gcry_qthread_init, gcry_qmutex_init,
    gcry_qmutex_destroy, gcry_qmutex_lock, gcry_qmutex_unlock,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
  };
#endif

QHash<QString, char> dmisc::s_blockedhosts;
QHash<int, int> dmisc::s_httpStatusCodes;
QList<QString> dmisc::s_browsingProxyIgnoreList;
QList<QString> dmisc::s_downloadProxyIgnoreList;
QStringList dmisc::s_blockedhostswildcards;
bool dmisc::s_passphraseWasAuthenticated = false;
dcrypt *dmisc::s_crypt = 0;
dcrypt *dmisc::s_reencodeCrypt = 0;
static bool gcryctl_set_thread_cbs_set = false;

void dmisc::destroyCrypt(void)
{
  if(s_crypt)
    delete s_crypt;

  if(s_reencodeCrypt)
    delete s_reencodeCrypt;

  s_crypt = 0;
  s_reencodeCrypt = 0;
  dcrypt::terminate();
}

void dmisc::initializeCrypt(void)
{
  if(!gcryctl_set_thread_cbs_set)
    {
      gcry_error_t err = 0;

#ifdef DOOBLE_USE_PTHREADS
#if !defined(GCRYPT_VERSION_NUMBER) || GCRYPT_VERSION_NUMBER < 0x010600
      err = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread, 0);
#endif
#else
      logError
	("dmisc::initializeCrypt(): Using gcry_threads_qt's "
	 "address as the second parameter to "
	 "gcry_control().");
      err = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_qt, 0);
#endif

      if(err == 0)
	gcryctl_set_thread_cbs_set = true;
      else
	logError("dmisc::initializeCrypt(): gcry_control() failure.");
    }

  if(!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
    {
      gcry_control(GCRYCTL_ENABLE_M_GUARD);

      if(!gcry_check_version(GCRYPT_VERSION))
	logError("dmisc::initializeCrypt(): "
		 "gcry_check_version() failure. Secure memory "
		 "was not explicitly initialized!");
      else
	{
	  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
	  gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
	  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
	  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	}
    }
  else
    logError("dmisc::initializeCrypt(): It appears that "
	     "the gcrypt library is already initialized.");
}

void dmisc::setCipherPassphrase(const QString &passphrase,
				const bool save,
				const QString &hashType,
				const QString &cipherType,
				const int iterationCount,
				const QByteArray &salt,
				const QString &cipherMode)
{
  if(s_crypt)
    delete s_crypt;

  s_crypt = new dcrypt
    (salt, cipherMode, cipherType, hashType, passphrase, iterationCount);

  if(!s_crypt->initialized())
    {
      delete s_crypt;
      s_crypt = 0;
      s_passphraseWasAuthenticated = false;
      return;
    }

  if(!passphrase.isEmpty())
    s_passphraseWasAuthenticated = true;

  if(save && s_passphraseWasAuthenticated)
    {
      QByteArray hash(passphraseHash(passphrase, s_crypt->salt(), hashType));
      QSettings settings;

      settings.setValue("settingsWindow/cipherMode", cipherMode);
      settings.setValue("settingsWindow/cipherType", cipherType);
      settings.setValue("settingsWindow/iterationCount",
			static_cast<int> (s_crypt->iterationCount()));
      settings.setValue("settingsWindow/passphraseHash", hash);
      settings.setValue("settingsWindow/passphraseHashType", hashType);
      settings.setValue("settingsWindow/passphraseSalt",
			s_crypt->salt());
      settings.setValue("settingsWindow/saltLength",
			s_crypt->salt().length());
      dooble::s_settings["settingsWindow/cipherMode"] = cipherMode;
      dooble::s_settings["settingsWindow/cipherType"] = cipherType;
      dooble::s_settings["settingsWindow/iterationCount"] =
	static_cast<int> (s_crypt->iterationCount());
      dooble::s_settings["settingsWindow/passphraseHash"] = hash;
      dooble::s_settings["settingsWindow/passphraseHashType"] = hashType;
      dooble::s_settings["settingsWindow/passphraseSalt"] =
	s_crypt->salt();
      dooble::s_settings["settingsWindow/saltLength"] =
	s_crypt->salt().length();
    }
}

QNetworkProxy dmisc::proxyByUrl(const QUrl &url)
{
  QString str(dooble::s_settings.value("settingsWindow/browsingProxySetting",
				       "none").toString());
  QString scheme(url.scheme().toLower().trimmed());
  QNetworkProxy proxy;

  proxy.setType(QNetworkProxy::NoProxy);

  if(shouldIgnoreProxyFor(url.host().trimmed(), "browsing"))
    return proxy;

  if(str == "system")
    {
      QNetworkProxyQuery query(url);
      QList<QNetworkProxy> list
	(QNetworkProxyFactory::systemProxyForQuery(query));

      if(!list.isEmpty())
	proxy = list.at(0);
    }
  else if(str == "manual")
    {
      if(scheme == "ftp")
	if(dooble::s_settings.value("settingsWindow/ftpBrowsingProxyEnabled",
				    false).toBool())
	  {
	    proxy.setType(QNetworkProxy::Socks5Proxy);
	    proxy.setHostName
	      (dooble::s_settings.value("settingsWindow/"
					"ftpBrowsingProxyHost",
					"").toString().trimmed());
	    proxy.setPort
	      (dooble::s_settings.value("settingsWindow/ftpBrowsingProxyPort",
					1080).toInt());
	    proxy.setUser
	      (dooble::s_settings.value("settingsWindow/"
					"ftpBrowsingProxyUser",
					"").toString());
	    proxy.setPassword
	      (dooble::s_settings.value("settingsWindow/"
					"ftpBrowsingProxyPassword",
					"").toString());
	  }

      if(scheme == "http" || scheme == "https")
	if(dooble::s_settings.value("settingsWindow/httpBrowsingProxyEnabled",
				    false).toBool())
	  {
	    QString str("");

	    str = dooble::s_settings.value
	      ("settingsWindow/httpBrowsingProxyType",
	       "Socks5").toString().trimmed().toLower();

	    if(str == "http")
	      proxy.setType(QNetworkProxy::HttpProxy);
	    else
	      proxy.setType(QNetworkProxy::Socks5Proxy);

	    proxy.setHostName
	      (dooble::s_settings.value("settingsWindow/httpBrowsingProxyHost",
					"").toString().trimmed());
	    proxy.setPort
	      (dooble::s_settings.value("settingsWindow/httpBrowsingProxyPort",
					1080).toInt());
	    proxy.setUser
	      (dooble::s_settings.value("settingsWindow/httpBrowsingProxyUser",
					"").toString());
	    proxy.setPassword(dooble::s_settings.value
			      ("settingsWindow/httpBrowsingProxyPassword",
			       "").toString());
	  }
    }

  /*
  ** Override!
  */

  if(url.host().toLower().trimmed().endsWith(".i2p") &&
     dooble::s_settings.value("settingsWindow/i2pBrowsingProxyEnabled",
			      true).toBool())
    {
      QString str("");

      str = dooble::s_settings.value("settingsWindow/i2pBrowsingProxyType",
				     "Http").toString().trimmed().
	toLower();

      if(str == "socks5")
	proxy.setType(QNetworkProxy::Socks5Proxy);
      else if(str == "http")
	proxy.setType(QNetworkProxy::HttpProxy);
      else
	proxy.setType(QNetworkProxy::HttpProxy);

      proxy.setHostName(dooble::s_settings.value
			("settingsWindow/i2pBrowsingProxyHost",
			 "127.0.0.1").toString().trimmed());
      proxy.setPort
	(dooble::s_settings.value("settingsWindow/i2pBrowsingProxyPort",
				  4444).toInt());
    }

  return proxy;
}

QNetworkProxy dmisc::proxyByFunctionAndUrl
(const DoobleDownloadType::DoobleDownloadTypeEnum functionType,
 const QUrl &url)
{
  QString str(dooble::s_settings.value("settingsWindow/browsingProxySetting",
				       "none").toString());
  QNetworkProxy proxy;

  proxy.setType(QNetworkProxy::NoProxy);

  if(shouldIgnoreProxyFor(url.host().trimmed(), "download"))
    return proxy;

  switch(functionType)
    {
    case DoobleDownloadType::Ftp:
      {
	if(str == "system")
	  {
	    QNetworkProxyQuery query(url);
	    QList<QNetworkProxy> list
	      (QNetworkProxyFactory::systemProxyForQuery(query));

	    if(!list.isEmpty())
	      proxy = list.at(0);
	  }
	else if(str == "manual" &&
		dooble::s_settings.value("settingsWindow/ftpDownloadProxy"
					 "Enabled",false).toBool())
	  {
	    proxy.setType(QNetworkProxy::Socks5Proxy);
	    proxy.setHostName(dooble::s_settings.value("settingsWindow/"
						       "ftpDownloadProxyHost",
						       "").toString().
			      trimmed());
	    proxy.setPort
	      (dooble::s_settings.value("settingsWindow/ftpDownloadProxyPort",
					1080).toInt());
	    proxy.setUser(dooble::s_settings.value("settingsWindow/"
						   "ftpDownloadProxyUser",
						   "").toString());
	    proxy.setPassword
	      (dooble::s_settings.value("settingsWindow/"
					"ftpDownloadProxyPassword",
					"").toString());
	  }

	break;
      }
    case DoobleDownloadType::Http:
      {
	if(str == "system")
	  {
	    QNetworkProxyQuery query(url);
	    QList<QNetworkProxy> list
	      (QNetworkProxyFactory::systemProxyForQuery(query));

	    if(!list.isEmpty())
	      proxy = list.at(0);
	  }
	else if(str == "manual" &&
		dooble::s_settings.value("settingsWindow/httpDownloadProxy"
					 "Enabled", false).toBool())
	  {
	    QString str("");

	    str = dooble::s_settings.value
	      ("settingsWindow/httpDownloadProxyType",
	       "Socks5").toString().trimmed().toLower();

	    if(str == "http")
	      proxy.setType(QNetworkProxy::HttpProxy);
	    else
	      proxy.setType(QNetworkProxy::Socks5Proxy);

	    proxy.setHostName(dooble::s_settings.value
			      ("settingsWindow/httpDownloadProxyHost",
			       "").toString().trimmed());
	    proxy.setPort
	      (dooble::s_settings.value("settingsWindow/httpDownloadProxyPort",
					1080).toInt());
	    proxy.setUser(dooble::s_settings.value
			  ("settingsWindow/httpDownloadProxyUser",
			   "").toString());
	    proxy.setPassword
	      (dooble::s_settings.value
	       ("settingsWindow/httpDownloadProxyPassword",
		"").toString());
	  }

	break;
      }
    default:
      break;
    }

  /*
  ** Override!
  */

  if(url.host().toLower().trimmed().endsWith(".i2p") &&
     dooble::s_settings.value("settingsWindow/i2pDownloadProxyEnabled",
			      true).toBool())
    {
      QString str("");

      str = dooble::s_settings.value("settingsWindow/i2pDownloadProxyType",
				     "Http").toString().trimmed().
	toLower();

      if(str == "socks5")
	proxy.setType(QNetworkProxy::Socks5Proxy);
      else if(str == "http")
	proxy.setType(QNetworkProxy::HttpProxy);
      else
	proxy.setType(QNetworkProxy::HttpProxy);

      proxy.setHostName(dooble::s_settings.value
			("settingsWindow/i2pDownloadProxyHost",
			 "127.0.0.1").toString().trimmed());
      proxy.setPort
	(dooble::s_settings.value("settingsWindow/i2pDownloadProxyPort",
				  4444).toInt());
    }

  return proxy;
}

void dmisc::saveIconForUrl(const QIcon &icon, const QUrl &url)
{
  if(dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    return;
  else if(!dooble::s_settings.value("settingsWindow/enableFaviconsDatabase",
				    false).toBool())
    return;
  else if(icon.isNull() || url.isEmpty() || !url.isValid())
    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "favicons");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;
	int temporary = passphraseWasAuthenticated() ? 0 : 1;

	query.exec("CREATE TABLE IF NOT EXISTS favicons ("
		   "url TEXT NOT NULL, "
		   "url_hash TEXT PRIMARY KEY NOT NULL, "
		   "favicon BLOB DEFAULT NULL, "
		   "temporary INTEGER NOT NULL)");
	query.exec("CREATE INDEX IF NOT EXISTS url_hash_index ON "
		   "favicons (url_hash)");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO favicons "
	   "(url, url_hash, favicon, temporary) "
	   "VALUES (?, ?, ?, ?)");
	query.bindValue
	  (0,
	   etm(url.toEncoded(QUrl::StripTrailingSlash),
	       true, &ok).toBase64());

	if(ok)
	  query.bindValue
	    (1,
	     hashedString(url.toEncoded(QUrl::StripTrailingSlash), &ok).
	     toBase64());

	QBuffer buffer;
	QByteArray bytes;

	if(ok)
	  buffer.setBuffer(&bytes);

	if(ok && buffer.open(QIODevice::WriteOnly))
	  {
	    QDataStream out(&buffer);

	    if(url.scheme().toLower().trimmed() == "ftp" ||
	       url.scheme().toLower().trimmed() == "gopher")
	      out << icon.pixmap(16, QIcon::Normal, QIcon::On);
	    else
	      out << icon;

	    if(out.status() != QDataStream::Ok)
	      bytes.clear();
	  }
	else
	  bytes.clear();

	if(ok)
	  query.bindValue
	    (2, etm(bytes, true, &ok));

	buffer.close();
	query.bindValue(3, temporary);

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("favicons");
}

QIcon dmisc::iconForUrl(const QUrl &url)
{
  QString scheme(url.scheme().toLower().trimmed());
  QFileIconProvider iconProvider;

  if(scheme == "ftp" ||
     scheme == "gopher" ||
     scheme == "http" || scheme == "https" ||
     scheme == "qrc")
    {
      QIcon icon;
      QPixmap pixmap;

      {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						    "favicons");

	db.setDatabaseName(dooble::s_homePath +
			   QDir::separator() + "favicons.db");

	if(db.open())
	  {
	    QByteArray bytes(url.toEncoded(QUrl::StripTrailingSlash));
	    QSqlQuery query(db);
	    bool ok = true;
	    int temporary = passphraseWasAuthenticated() ? 0 : 1;

	    query.setForwardOnly(true);
	    query.prepare("SELECT favicon FROM favicons "
			  "INDEXED BY url_hash_index "
			  "WHERE "
			  "temporary = ? AND "
			  "url_hash IN (?, ?, ?)");
	    query.bindValue(0, temporary);
	    query.bindValue
	      (1, hashedString(bytes, &ok).toBase64());
	    query.bindValue
	      (2, hashedString(bytes.lastIndexOf('/') ==
			       bytes.length() - 1 ?
			       bytes.mid(0, bytes.length() - 1) :
			       bytes, &ok).toBase64());
	    query.bindValue
	      (3, hashedString(bytes.mid(bytes.length() - 1) == "/" ?
			       bytes : bytes.append('/'), &ok).toBase64());

	    if(ok && query.exec())
	      if(query.next())
		if(!query.isNull(0))
		  {
		    QBuffer buffer;
		    QByteArray bytes(query.value(0).toByteArray());
		    bool ok = true;

		    bytes = daa(bytes, &ok);

		    if(ok)
		      buffer.setBuffer(&bytes);

		    if(ok && buffer.open(QIODevice::ReadOnly))
		      {
			QDataStream in(&buffer);

			if(scheme == "ftp" || scheme == "gopher")
			  in >> pixmap;
			else
			  in >> icon;

			if(in.status() != QDataStream::Ok)
			  {
			    icon = QIcon();
			    pixmap = QPixmap();
			  }

			buffer.close();
		      }
		    else
		      {
			icon = QIcon();
			pixmap = QPixmap();
		      }

		    if(!ok)
		      {
			QSqlQuery deleteQuery(db);
			bool ok = true;

			deleteQuery.exec("PRAGMA secure_delete = ON");
			deleteQuery.prepare("DELETE FROM favicons WHERE "
					    "temporary = ? AND url_hash = ?");
			deleteQuery.bindValue(0, temporary);
			deleteQuery.bindValue
			  (1,
			   hashedString(url.
					toEncoded(QUrl::StripTrailingSlash),
					&ok).toBase64());

			if(ok)
			  deleteQuery.exec();
		      }
		  }
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase("favicons");

      if(scheme == "ftp" || scheme == "gopher")
	icon = QIcon(pixmap);

      if(!icon.isNull())
	return icon;
    }
  else if(scheme == "file")
    {
      QFileInfo fileInfo(url.toLocalFile());

      if(fileInfo.isFile())
	return iconProvider.icon(QFileIconProvider::File);
      else if(fileInfo.isDir())
	return iconProvider.icon(QFileIconProvider::Folder);
    }

  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  return QIcon(settings.value("mainWindow/emptyIcon").toString());
}

QByteArray dmisc::daa(dcrypt *crypt, const QByteArray &byteArray, bool *ok)
{
  /*
  ** Decrypt after authenticating.
  */

  if(crypt)
    return crypt->daa(byteArray, ok);
  else if(ok)
    *ok = false;

  return byteArray;
}

QByteArray dmisc::daa(const QByteArray &byteArray, bool *ok)
{
  /*
  ** Decrypt after authenticating.
  */

  if(s_crypt)
    return s_crypt->daa(byteArray, ok);
  else if(ok)
    *ok = false;

  return byteArray;
}

QByteArray dmisc::etm(const QByteArray &byteArray,
		      const bool shouldEncode,
		      bool *ok)
{
  /*
  ** Encrypt then authenticate.
  */

  if(s_crypt && shouldEncode)
    return s_crypt->etm(byteArray, ok);
  else if(ok)
    *ok = false;

  return byteArray;
}

QStringList dmisc::hashTypes(void)
{
  return dcrypt::hashTypes();
}

QStringList dmisc::cipherTypes(void)
{
  return dcrypt::cipherTypes();
}

QByteArray dmisc::passphraseHash(const QString &passphrase,
				 const QByteArray &salt,
				 const QString &hashType)
{
  int algorithm = -1;
  QByteArray hash;
  QByteArray saltedPassphrase;

  saltedPassphrase.append(passphrase.toUtf8()).append(salt);

  if(hashType == "sha512")
    algorithm = GCRY_MD_SHA512;
  else if(hashType == "sha384")
    algorithm = GCRY_MD_SHA384;
  else if(hashType == "sha256")
    algorithm = GCRY_MD_SHA256;
  else if(hashType == "sha224")
    algorithm = GCRY_MD_SHA224;
  else if(hashType == "tiger")
    algorithm = GCRY_MD_TIGER;

  if(algorithm != -1 && isHashTypeSupported(hashType))
    {
      unsigned int length = gcry_md_get_algo_dlen(algorithm);

      if(length > 0)
	{
	  QByteArray byteArray(length, 0);

	  gcry_md_hash_buffer
	    (algorithm,
	     byteArray.data(),
	     saltedPassphrase.constData(),
	     saltedPassphrase.length());
	  hash = byteArray;
	}
      else
	{
	  logError("dmisc::passphraseHash(): "
		   "gcry_md_get_algo_dlen() "
		   "returned zero. Using "
		   "Qt's SHA-1 "
		   "implementation.");
	  hash = QCryptographicHash::hash(saltedPassphrase,
					  QCryptographicHash::Sha1);
	}
    }
  else
    {
      logError(QString("dmisc::passphraseHash(): Unsupported "
		       "hash type %1 (%2). Using "
		       "Qt's SHA-1 "
		       "implementation.").arg(hashType).
	       arg(algorithm));
      hash = QCryptographicHash::hash
	(saltedPassphrase, QCryptographicHash::Sha1);
    }

  return hash;
}

bool dmisc::isHashTypeSupported(const QString &hashType)
{
  int algorithm = gcry_md_map_name(hashType.toLatin1().constData());

  if(algorithm != 0 && gcry_md_test_algo(algorithm) == 0)
    return true;
  else
    return false;
}

int dmisc::levenshteinDistance(const QString &str1,
			       const QString &str2)
{
  if(str1.isEmpty())
    return str2.length();
  else if(str2.isEmpty())
    return str1.length();

  int cost = 0;
  QChar str1_c = 0;
  QChar str2_c = 0;
  QVector<QVector<int> > matrix(str1.length() + 1,
				QVector<int> (str2.length() + 1));

  for(int i = 0; i <= str1.length(); i++)
    matrix[i][0] = i;

  for(int i = 0; i <= str2.length(); i++)
    matrix[0][i] = i;

  for(int i = 1; i <= str1.length(); i++)
    {
      str1_c = str1.at(i - 1);

      for(int j = 1; j <= str2.length(); j++)
	{
	  str2_c = str2.at(j - 1);

	  if(str1_c == str2_c)
	    cost = 0;
	  else
	    cost = 1;

	  matrix[i][j] = qMin(qMin(matrix[i - 1][j] + 1,
				   matrix[i][j - 1] + 1),
			      matrix[i - 1][j - 1] + cost);
	}
    }

  return matrix[str1.length()][str2.length()];
}

bool dmisc::passphraseWasAuthenticated(void)
{
  return s_passphraseWasAuthenticated;
}

QString dmisc::findUniqueFileName(const QString &fileName,
				  const QDir &path)
{
  QFileInfo fileInfo;

  fileInfo.setFile(path, fileName);

  if(fileInfo.exists())
    {
      QFileInfo info(fileName);

      if(info.suffix().trimmed().isEmpty())
	fileInfo.setFile(path, QString("%1(%2)").arg(fileName).
			 arg(dcrypt::weakRandomBytes(8).toHex().
			     constData()));
      else
	fileInfo.setFile
	  (path,
	   QString("%1(%2).%3").arg(info.completeBaseName()).
	   arg(dcrypt::weakRandomBytes(8).toHex().constData()).
	   arg(info.suffix().trimmed()));

      while(fileInfo.exists())
	if(info.suffix().trimmed().isEmpty())
	  fileInfo.setFile(path, QString("%1(%2)").arg(fileName).
			   arg(dcrypt::weakRandomBytes(8).toHex().
			       constData()));
	else
	  fileInfo.setFile
	    (path, QString("%1(%2).%3").arg(info.completeBaseName()).
	     arg(dcrypt::weakRandomBytes(8).toHex().constData()).
	     arg(info.suffix().trimmed()));
    }

  return fileInfo.absoluteFilePath();
}

void dmisc::reencodeFavicons(QProgressBar *progress)
{
  if(dooble::s_settings.
     value("settingsWindow/"
	   "disableAllEncryptedDatabaseWrites", false).
     toBool())
    return;
  else if(!passphraseWasAuthenticated())
    return;

  if(progress)
    {
      progress->setMaximum(-1);
      progress->setVisible(true);
      progress->update();
    }

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "favicons");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "favicons.db");

    if(db.open())
      {
	if(progress)
	  {
	    progress->setMaximum(-1);
	    progress->update();
	  }

	int temporary = -1;
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT url, favicon FROM favicons WHERE "
		      "temporary = 0"))
	  while(query.next())
	    {
	      bool ok = true;
	      QUrl url
		(QUrl::fromEncoded
		 (daa
		  (s_reencodeCrypt, QByteArray::fromBase64
		   (query.value(0).toByteArray()), &ok),
		  QUrl::StrictMode));

	      if(!ok)
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.exec("PRAGMA secure_delete = ON");
		  deleteQuery.prepare("DELETE FROM favicons WHERE "
				      "temporary = ? AND url = ?");
		  deleteQuery.bindValue(0, temporary);
		  deleteQuery.bindValue(1, query.value(0));
		  deleteQuery.exec();
		  continue;
		}

	      if(ok && isSchemeAcceptedByDooble(url.scheme()))
		{
		  QIcon icon;
		  QBuffer buffer;
		  QByteArray bytes(query.value(1).toByteArray());

		  bytes = daa(s_reencodeCrypt, bytes, &ok);

		  if(ok)
		    buffer.setBuffer(&bytes);

		  if(ok && buffer.open(QIODevice::ReadOnly))
		    {
		      QDataStream in(&buffer);

		      in >> icon;

		      if(in.status() != QDataStream::Ok)
			icon = QIcon();

		      buffer.close();
		    }
		  else
		    icon = QIcon();

		  QSqlQuery insertQuery(db);
		  bool ok = true;

		  insertQuery.prepare
		    ("INSERT OR REPLACE INTO favicons "
		     "(url, url_hash, favicon, temporary) "
		     "VALUES (?, ?, ?, ?)");
		  insertQuery.bindValue
		    (0,
		     etm(url.toEncoded(QUrl::StripTrailingSlash).
			 toBase64(), true, &ok));

		  if(ok)
		    insertQuery.bindValue
		      (1,
		       hashedString(url.toEncoded(QUrl::StripTrailingSlash),
				    &ok).toBase64());

		  bytes.clear();

		  if(ok)
		    buffer.setBuffer(&bytes);

		  if(ok && buffer.open(QIODevice::WriteOnly))
		    {
		      QDataStream out(&buffer);

		      out << icon;

		      if(out.status() != QDataStream::Ok)
			bytes.clear();
		    }
		  else
		    bytes.clear();

		  if(ok)
		    insertQuery.bindValue(2, etm(bytes, true, &ok));

		  buffer.close();
		  insertQuery.bindValue(3, temporary);

		  if(ok)
		    insertQuery.exec();
		}
	    }

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM favicons WHERE temporary <> -1");
	query.exec("UPDATE favicons SET temporary = 0");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("favicons");

  if(progress)
    progress->setVisible(false);
}

void dmisc::clearFavicons(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "favicons");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");

	if(passphraseWasAuthenticated())
	  query.exec("DELETE FROM favicons");
	else
	  query.exec("DELETE FROM favicons WHERE temporary = 1");

	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("favicons");
}

void dmisc::purgeTemporaryData(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "favicons");

    db.setDatabaseName
      (dooble::s_homePath + QDir::separator() + "favicons.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA secure_delete = ON");
	query.exec("DELETE FROM favicons WHERE temporary = 1");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("favicons");
}

QRect dmisc::balancedGeometry(const QRect &geometry, QWidget *widget)
{
  /*
  ** Qt may place the window underneath a desktop's panel bar.
  ** The geometry variable contains the desired geometry.
  */

  QRect rect(geometry);
  QRect available(QApplication::desktop()->availableGeometry(widget));

  if(!available.contains(rect))
    {
      rect.setX(rect.x() + available.x());
      rect.setY(rect.y() + available.y());
#if (defined(Q_OS_LINUX) || defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)
      /*
      ** X11 is quite difficult to predict. Let's add this simple
      ** offset.
      */

      rect.setX(rect.x() + 50);
#endif

      if(rect.width() > available.width())
	rect.setWidth(0.85 * available.width());

      if(rect.height() > available.height())
	rect.setHeight(0.85 * available.height());
    }

  return rect;
}

bool dmisc::isLinkAcceptedByDooble(const QString &link)
{
  QString l(link.toLower().trimmed());

  if(l.startsWith("data:") ||
     l.startsWith("file://") ||
     l.startsWith("ftp://") ||
     l.startsWith("gopher://") ||
     l.startsWith("http://") ||
     l.startsWith("https://") ||
     l.startsWith("qrc:/"))
    return true;
  else
    return false;
}

bool dmisc::isSchemeAcceptedByDooble(const QString &scheme)
{
  QString l_scheme(scheme.toLower().trimmed());

  if(l_scheme == "data" ||
     l_scheme == "file" ||
     l_scheme == "ftp" ||
     l_scheme == "gopher" ||
     l_scheme == "http" ||
     l_scheme == "https" ||
     l_scheme == "qrc")
    return true;
  else
    return false;
}

void dmisc::removeRestorationFiles(const QUuid &id)
{
  if(passphraseWasAuthenticated())
    {
      /*
      ** Remove all restoration files or the files that are
      ** associated with the current process.
      */

      QDir dir(dooble::s_homePath + QDir::separator() + "Histories");
      QString idStr("");
      QFileInfoList files(dir.entryInfoList(QDir::Files, QDir::Name));

      if(!id.isNull())
	{
	  idStr = id.toString();
	  idStr.remove("{");
	  idStr.remove("}");
	  idStr.remove("-");
	}

      while(!files.isEmpty())
	{
	  QFileInfo fileInfo(files.takeFirst());

	  if(!idStr.isEmpty())
	    {
	      if(fileInfo.fileName().startsWith(idStr))
		QFile::remove(fileInfo.absoluteFilePath());
	    }
	  else
	    QFile::remove(fileInfo.absoluteFilePath());
	}
    }
}

void dmisc::removeRestorationFiles(const QUuid &pid,
				   const qint64 wid)
{
  if(passphraseWasAuthenticated())
    {
      /*
      ** Remove all restoration files that are associated
      ** with the current process and window.
      */

      QDir dir(dooble::s_homePath + QDir::separator() + "Histories");
      QString idStr(pid.toString().remove("{").remove("}").remove("-") +
		    QString::number(wid).rightJustified(20, '0'));
      QFileInfoList files(dir.entryInfoList(QDir::Files, QDir::Name));

      while(!files.isEmpty())
	{
	  QFileInfo fileInfo(files.takeFirst());

	  if(fileInfo.fileName().startsWith(idStr))
	    QFile::remove(fileInfo.absoluteFilePath());
	}
    }
}

void dmisc::setActionForFileSuffix(const QString &suffix,
				   const QString &action)
{
  if(!suffix.isEmpty())
    {
      QWriteLocker locker(&dooble::s_applicationsActionsLock);

      if(action.isEmpty())
	dooble::s_applicationsActions[suffix] = "prompt";
      else
	dooble::s_applicationsActions[suffix] = action;
    }
  else
    /*
    ** Damaged suffix.
    */

    return;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						"applications");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "applications.db");

    if(db.open())
      {
	QSqlQuery query(db);
	QByteArray bytes;

	query.setForwardOnly(true);
	query.exec("CREATE TABLE IF NOT EXISTS applications ("
		   "file_suffix TEXT PRIMARY KEY NOT NULL, "
		   "action TEXT DEFAULT NULL, "
		   "icon BLOB DEFAULT NULL)");
	query.prepare("SELECT icon FROM applications WHERE "
		      "file_suffix = ?");
	query.bindValue(0, suffix);

	if(query.exec())
	  if(query.next())
	    bytes = query.value(0).toByteArray();

	if(bytes.isEmpty())
	  {
	    QBuffer buffer;
	    QFileIconProvider iconProvider;

	    buffer.setBuffer(&bytes);

	    if(buffer.open(QIODevice::WriteOnly))
	      {
		QDataStream out(&buffer);

		out << iconProvider.icon(QFileIconProvider::File).pixmap
		  (16, QIcon::Normal, QIcon::On);

		if(out.status() != QDataStream::Ok)
		  bytes.clear();
	      }
	    else
	      bytes.clear();

	    buffer.close();
	  }

	query.prepare
	  ("INSERT OR REPLACE INTO applications ("
	   "file_suffix, action, icon) "
	   "VALUES (?, ?, ?)");
	query.bindValue(0, suffix);
	query.bindValue(1, action);
	query.bindValue(2, bytes);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("applications");
}

bool dmisc::canDoobleOpenLocalFile(const QUrl &url)
{
  static QStringList list;

  list << "gif"
       << "html"
       << "jpg"
       << "jpeg"
       << "png";

  QFileInfo fileInfo(url.toLocalFile());

  return list.contains(fileInfo.suffix().toLower().trimmed());
}

QString dmisc::fileNameFromUrl(const QUrl &url)
{
  QString path(url.path());
  QString fileName("");

  /*
  ** Attempt to gather the file name from the URL.
  */

  if(!path.isEmpty())
    fileName = QFileInfo(path).fileName();

  if(fileName.isEmpty())
    fileName = "dooble.download";

  return fileName;
}

void dmisc::launchApplication(const QString &program,
			      const QStringList &arguments)
{
#ifdef Q_OS_MAC
  QProcess::startDetached
    ("open", QStringList("-a") << program << "--args" << arguments);
#elif defined(Q_OS_WIN32)
  QString a("");

  for(int i = 0; i < arguments.size(); i++)
    a += QString(" \"%1\" ").arg(arguments.at(i));

  a = a.trimmed();

  HINSTANCE rc = ::ShellExecuteA(0,
				 "open",
				 program.toUtf8().constData(),
				 a.toUtf8().constData(),
				 0,
				 SW_SHOWNORMAL);

  if((int) rc == SE_ERR_ACCESSDENIED)
    /*
    ** Elevated?
    */

    ::ShellExecuteA(0,
		    "runas",
		    program.toUtf8().constData(),
		    a.toUtf8().constData(),
		    0,
		    SW_SHOWNORMAL);
#else
  QProcess::startDetached(program, arguments);
#endif
}

QIcon dmisc::iconForFileSuffix(const QString &suffix)
{
  QPixmap pixmap;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
						"applications");

    db.setDatabaseName(dooble::s_homePath +
		       QDir::separator() + "applications.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT icon FROM applications WHERE "
		      "file_suffix = ?");
	query.bindValue(0, suffix);

	if(query.exec())
	  if(query.next())
	    if(!query.isNull(0))
	      {
		QBuffer buffer;
		QByteArray bytes(query.value(0).toByteArray());

		buffer.setBuffer(&bytes);

		if(buffer.open(QIODevice::ReadOnly))
		  {
		    QDataStream in(&buffer);

		    in >> pixmap;

		    if(in.status() != QDataStream::Ok)
		      pixmap = QPixmap();

		    buffer.close();
		  }
		else
		  pixmap = QPixmap();
	      }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("applications");

  if(pixmap.isNull())
    {
      QFileIconProvider iconProvider;

      return iconProvider.icon(QFileIconProvider::File);
    }
  else
    return QIcon(pixmap);
}

bool dmisc::passphraseWasPrepared(void)
{
  QByteArray b1;
  QByteArray b2;
  QString str("");

  if(dooble::s_settings.contains("settingsWindow/passphraseHash"))
    b1 = dooble::s_settings.value("settingsWindow/passphraseHash", "").
      toByteArray();

  if(dooble::s_settings.contains("settingsWindow/passphraseHashType"))
    str = dooble::s_settings.value("settingsWindow/passphraseHashType", "").
      toString();

  if(dooble::s_settings.contains("settingsWindow/passphraseSalt"))
    b2 = dooble::s_settings.value("settingsWindow/passphraseSalt", "").
      toByteArray();

  return !b1.isEmpty() && !b2.isEmpty() && !str.isEmpty();
}

bool dmisc::isKDE(void)
{
  /*
  ** Trust the shell? Trust the user?
  */

  return QVariant(qgetenv("KDE_FULL_SESSION")).toBool();
}

bool dmisc::isGnome(void)
{
  /*
  ** Trust the shell? Trust the user?
  */

  QByteArray session(qgetenv("DESKTOP_SESSION").toLower().trimmed());

  if(session.contains("gnome") || session.contains("ubuntu"))
    return true;
  else
    return false;
}

void dmisc::logError(const QString &error)
{
  if(dooble::s_errorLog)
    dooble::s_errorLog->logError(error);
}

QString dmisc::elidedTitleText(const QString &text)
{
  QString l_text(text);

  if(l_text.length() > dooble::MAX_NUMBER_OF_MENU_TITLE_CHARACTERS)
    l_text = l_text.mid
      (0, dooble::MAX_NUMBER_OF_MENU_TITLE_CHARACTERS - 3).trimmed() + "...";

  l_text.replace("&", "&&");
  return l_text;
}

QString dmisc::formattedSize(const qint64 size)
{
  QString str("");

  if(size >= 0)
    {
      if(size == 0)
	str = QObject::tr("0 Bytes");
      else if(size == 1)
	str = QObject::tr("1 Byte");
      else if(size < 1024)
	str = QString(QObject::tr("%1 Bytes")).arg(size);
      else if(size < 1048576)
	str = QString(QObject::tr("%1 KiB")).arg
	  (QString::number(qRound(static_cast<double> (size) / 1024.0)));
      else
	str = QString(QObject::tr("%1 MiB")).arg
	  (QString::number(static_cast<double> (size) / 1048576.0, 'f', 1));
    }
  else
    str = "0 Bytes";

  return str;
}

QUrl dmisc::correctedUrlPath(const QUrl &url)
{
  QUrl l_url(url);

  if(!l_url.path().isEmpty())
    {
      QString path(QDir::cleanPath(l_url.path()));

      path = path.remove("../");
      path = path.remove("/..");
      path = path.remove("/../");
      l_url.setPath(path);
    }

  return l_url;
}

QByteArray dmisc::hashedString(const QByteArray &byteArray, bool *ok)
{
  if(s_crypt)
    return s_crypt->keyedHash(byteArray, ok);
  else
    {
      if(ok)
	*ok = false;

      return byteArray;
    }
}

qint64 dmisc::faviconsSize(void)
{
  return QFileInfo(dooble::s_homePath + QDir::separator() + "favicons.db").
    size();
}

void dmisc::prepareReencodeCrypt(void)
{
  if(s_reencodeCrypt)
    delete s_reencodeCrypt;

  s_reencodeCrypt = new dcrypt(s_crypt);
}

void dmisc::destroyReencodeCrypt(void)
{
  if(s_reencodeCrypt)
    {
      delete s_reencodeCrypt;
      s_reencodeCrypt = 0;
    }
}

bool dmisc::compareByteArrays(const QByteArray &a, const QByteArray &b)
{
  QByteArray bytes1;
  QByteArray bytes2;
  int length = qMax(a.length(), b.length());
  unsigned long rc = 0;

  bytes1 = a.leftJustified(length, 0);
  bytes2 = b.leftJustified(length, 0);

  /*
  ** x ^ y returns zero if x and y are identical.
  */

  for(int i = 0; i < length; i++)
    {
      std::bitset<CHAR_BIT * sizeof(unsigned long)>
	ba1(static_cast<unsigned long> (bytes1.at(i)));
      std::bitset<CHAR_BIT * sizeof(unsigned long)>
	ba2(static_cast<unsigned long> (bytes2.at(i)));

      for(size_t j = 0; j < ba1.size(); j++)
	rc |= ba1[j] ^ ba2[j];
    }

  return rc == 0; /*
		  ** Return true if a and b are identical. Should
		  ** this final comparison be embellished?
		  */
}

void dmisc::centerChildWithParent(QWidget *child, QWidget *parent)
{
  if(!child || !parent)
    return;

  if(child->height() == parent->height() &&
     child->width() == parent->width())
    child->setGeometry(parent->geometry());
  else
    {
      QPoint p(parent->pos());

      if(child->width() >= parent->width())
	p.setX(p.x() - (child->width() / 2 - parent->width() / 2));
      else
	p.setX(p.x() + (parent->width() / 2 - child->width() / 2));

      if(child->height() <= parent->height())
	p.setY(p.y() - (child->height() / 2 - parent->height() / 2));
      else
	p.setY(p.y() + (parent->height() / 2 - child->height() / 2));

      child->move(p);
    }
}

void dmisc::createPreferencesDatabase(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "preferences");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "preferences.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS http_status_codes ("
		   "status_code INTEGER PRIMARY KEY NOT NULL, "
		   "display_default_site_page INTEGER NOT NULL DEFAULT 1)");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("preferences");
}

void dmisc::updateHttpStatusCodes(const QHash<int, int> &statusCodes)
{
  s_httpStatusCodes = statusCodes;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "preferences");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "preferences.db");

    if(db.open())
      {
	QHashIterator<int, int> it(s_httpStatusCodes);
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	while(it.hasNext())
	  {
	    it.next();
	    query.prepare("INSERT OR REPLACE INTO http_status_codes "
			  "(status_code, display_default_site_page) "
			  "VALUES (?, ?)");
	    query.bindValue(0, it.key());
	    query.bindValue(1, it.value());
	    query.exec();
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("preferences");
}

void dmisc::populateHttpStatusCodesContainer(void)
{
  s_httpStatusCodes.clear();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "preferences");

    db.setDatabaseName(dooble::s_homePath + QDir::separator() +
		       "preferences.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT status_code, display_default_site_page "
		      "FROM http_status_codes ORDER BY status_code"))
	  while(query.next())
	    s_httpStatusCodes[query.value(0).toInt()] =
	      query.value(1).toInt();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("preferences");
}

void dmisc::prepareProxyIgnoreLists(void)
{
  s_browsingProxyIgnoreList.clear();
  s_downloadProxyIgnoreList.clear();

  QList<QString> list
    (dooble::s_settings.value("settingsWindow/browsingProxyIgnore",
			      "localhost, 127.0.0.1").
     toString().trimmed().split(",", QString::SkipEmptyParts));

  while(!list.isEmpty())
    {
      QString str(list.takeFirst().trimmed());

      if(str.isEmpty())
	continue;

      s_browsingProxyIgnoreList.append(str);
    }

  list = dooble::s_settings.value("settingsWindow/downloadProxyIgnore",
				  "localhost, 127.0.0.1").
    toString().trimmed().split(",", QString::SkipEmptyParts);

  while(!list.isEmpty())
    {
      QString str(list.takeFirst().trimmed());

      if(str.isEmpty())
	continue;

      s_downloadProxyIgnoreList.append(str);
    }
}

bool dmisc::shouldIgnoreProxyFor(const QString &host, const QString &type)
{
  QString h(host.trimmed());
  QString t(type.trimmed());

  if(h.isEmpty() || t.isEmpty())
    return false;

  if(type == "browsing")
    {
      if(s_browsingProxyIgnoreList.contains(host))
	return true;
      else if(!QHostAddress(host).isNull())
	{
	  for(int i = 0; i < s_browsingProxyIgnoreList.size(); i++)
	    {
	      QString str(s_browsingProxyIgnoreList.at(i));

	      if(!str.contains("/"))
		continue;

	      QHostAddress address(host);
	      QPair<QHostAddress, int> pair(QHostAddress::parseSubnet(str));

	      if(address.isInSubnet(pair))
		return true;
	    }
	}
      else
	{
	  for(int i = 0; i < s_browsingProxyIgnoreList.size(); i++)
	    {
	      QString str(s_browsingProxyIgnoreList.at(i));

	      if(!str.startsWith("."))
		continue;
	      else
		str.remove(0, 1);

	      if(str.isEmpty())
		continue;

	      if(host.contains(str))
		return true;
	    }
	}
    }
  else if(type == "download")
    {
      if(s_downloadProxyIgnoreList.contains(host))
	return true;
      else if(!QHostAddress(host).isNull())
	{
	  for(int i = 0; i < s_downloadProxyIgnoreList.size(); i++)
	    {
	      QString str(s_downloadProxyIgnoreList.at(i));

	      if(!str.contains("/"))
		continue;

	      QHostAddress address(host);
	      QPair<QHostAddress, int> pair(QHostAddress::parseSubnet(str));

	      if(address.isInSubnet(pair))
		return true;
	    }
	}
      else
	{
	  for(int i = 0; i < s_downloadProxyIgnoreList.size(); i++)
	    {
	      QString str(s_downloadProxyIgnoreList.at(i));

	      if(!str.startsWith("."))
		continue;
	      else
		str.remove(0, 1);

	      if(str.isEmpty())
		continue;

	      if(host.contains(str))
		return true;
	    }
	}
    }

  return false;
}

bool dmisc::hostblocked(const QString &host)
{
  if(s_blockedhosts.contains(host.toLower().trimmed()))
    return true;
  else
    {
      /*
      ** abc.def.ghi.jkl.org
      */

      QStringList items;
      QStringList list(host.toLower().trimmed().split('.'));

      /*
      ** abc.def.ghi.jkl.org
      ** def.ghi.jkl.org
      ** ghi.jkl.org
      ** jkl.org
      */

      for(int i = 0; i < list.size() - 1; i++)
	{
	  QString str("");

	  for(int j = i; j < list.size(); j++)
	    {
	      str.append(list.at(j));
	      str.append(".");
	    }

	  str.remove(str.length() - 1, 1);
	  items << str;
	}

      for(int i = items.size() - 1; i >= 0; i--)
	if(s_blockedhostswildcards.contains(items.at(i)))
	  return true;
    }

  return false;
}

void dmisc::initializeBlockedHosts(void)
{
  QFile file
    (dooble::s_homePath + QDir::separator() + "dooble-blocked-hosts.txt");

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      s_blockedhosts.clear();
      s_blockedhostswildcards.clear();

      QByteArray line(1024, 0);
      qint64 rc = 0;

      while((rc = file.readLine(line.data(), line.length())) > 0)
	{
	  QString str(line.mid(0, rc).constData());

	  str = str.trimmed();

	  if(str.length() > 3 && !str.startsWith("#"))
	    {
	      if(str.startsWith("*."))
		{
		  str.remove(0, 2);

		  if(str.length() > 3)
		    s_blockedhostswildcards.append(str);
		}
	      else
		s_blockedhosts[str] = 0;
	    }
	}

      qSort(s_blockedhostswildcards.begin(), s_blockedhostswildcards.end());
    }

  file.close();
}

void dmisc::showCryptInitializationError(QWidget *parent)
{
  if(!s_crypt || !s_crypt->initialized())
    {
      QSettings settings(dooble::s_settings.value("iconSet").toString(),
			 QSettings::IniFormat);
      QMessageBox mb(QMessageBox::Critical,
		     QObject::tr("Dooble Web Browser: Error"),
		     QObject::tr("A critical error occurred while preparing "
				 "the authentication and encryption "
				 "keys. Please report this problem."),
		     QMessageBox::Cancel,
		     parent);

#ifdef Q_OS_MAC
      mb.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
      mb.setWindowIcon
	(QIcon(settings.value("mainWindow/windowIcon").toString()));

      for(int i = 0; i < mb.buttons().size(); i++)
	{
	  mb.buttons().at(i)->setIcon
	    (QIcon(settings.value("cancelButtonIcon").toString()));
	  mb.buttons().at(i)->setIconSize(QSize(16, 16));
	}

      mb.exec();
    }
}
