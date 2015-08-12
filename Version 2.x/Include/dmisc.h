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

#ifndef _dmisc_h_
#define _dmisc_h_

extern "C"
{
#include <gcrypt.h>
}

#include <QDir>
#include <QUuid>

#include "dcrypt.h"
#include "dtypes.h"

class QIcon;
class QNetworkProxy;
class QProgressBar;
class QRect;
class QUrl;

class dmisc
{
 public:
  static QHash<int, int> s_httpStatusCodes;
  static dcrypt *s_crypt;
  static dcrypt *s_reencodeCrypt;
  static QByteArray daa(const QByteArray &byteArray, bool *ok);
  static QByteArray daa(dcrypt *crypt,const QByteArray &byteArray,bool *ok);
  static QByteArray etm(const QByteArray &byteArray,
			const bool shouldEncode,
			bool *ok);
  static QByteArray hashedString(const QByteArray &byteArray, bool *ok);
  static QByteArray passphraseHash(const QString &passphrase,
				   const QByteArray &salt,
				   const QString &hashType);
  static QIcon iconForFileSuffix(const QString &suffix);
  static QIcon iconForUrl(const QUrl &url);
  static QNetworkProxy proxyByFunctionAndUrl
    (const DoobleDownloadType::DoobleDownloadTypeEnum functionType,
     const QUrl &url);
  static QNetworkProxy proxyByUrl(const QUrl &url);
  static QRect balancedGeometry(const QRect &geometry, QWidget *widget);
  static QString elidedTitleText(const QString &text);
  static QString fileNameFromUrl(const QUrl &url);
  static QString findUniqueFileName(const QString &fileName,
				    const QDir &path = QDir());
  static QString formattedSize(const qint64 fileSize);
  static QStringList cipherTypes(void);
  static QStringList hashTypes(void);
  static QUrl correctedUrlPath(const QUrl &url);
  static bool canDoobleOpenLocalFile(const QUrl &url);
  static bool compareByteArrays(const QByteArray &a, const QByteArray &b);
  static bool hostblocked(const QString &host);
  static bool isGnome(void);
  static bool isHashTypeSupported(const QString &hashType);
  static bool isKDE(void);
  static bool isSchemeAcceptedByDooble(const QString &scheme);
  static bool passphraseWasAuthenticated(void);
  static bool passphraseWasPrepared(void);
  static bool shouldIgnoreProxyFor(const QString &host, const QString &type);
  static int levenshteinDistance(const QString &str1, const QString &str);
  static qint64 faviconsSize(void);
  static void centerChildWithParent(QWidget *child, QWidget *parent);
  static void clearFavicons(void);
  static void createPreferencesDatabase(void);
  static void destroyCrypt(void);
  static void destroyReencodeCrypt(void);
  static void initializeBlockedHosts(void);
  static void initializeCrypt(void);
  static void launchApplication(const QString &program,
				const QStringList &arguments);
  static void logError(const QString &error);
  static void populateHttpStatusCodesContainer(void);
  static void prepareProxyIgnoreLists(void);
  static void prepareReencodeCrypt(void);
  static void purgeTemporaryData(void);
  static void reencodeFavicons(QProgressBar *progress);
  static void removeRestorationFiles(const QUuid &id = QUuid());
  static void removeRestorationFiles(const QUuid &pid, const quint64 wid);
  static void saveIconForUrl(const QIcon &icon, const QUrl &url);
  static void setActionForFileSuffix(const QString &suffix,
				     const QString &action);
  static void setCipherPassphrase(const QString &passphrase,
				  const bool save,
				  const QString &hashType,
				  const QString &cipherType,
				  const int iterationCount,
				  const QByteArray &salt);
  static void updateHttpStatusCodes(const QHash<int, int> &statusCodes);

 private:
  dmisc(void);
  static QHash<QString, char> s_blockedhosts;
  static QList<QString> s_browsingProxyIgnoreList;
  static QList<QString> s_downloadProxyIgnoreList;
  static bool s_passphraseWasAuthenticated;
};

#endif
