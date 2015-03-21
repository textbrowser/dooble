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

#ifndef _dnetworkaccessmanager_h_
#define _dnetworkaccessmanager_h_

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QPointer>

class QNetworkRequest;
class dexceptionswindow;
class dftp;

class dnetworkblockreply: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dnetworkblockreply(void):QNetworkReply(0)
  {
  }

  dnetworkblockreply(const dnetworkblockreply &reply):QNetworkReply(0)
  {
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  ~dnetworkblockreply()
  {
  }

  dnetworkblockreply(QObject *parent, const QNetworkRequest &request);
  QByteArray html(void) const;
  bool isSequential(void) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void load(void);

 private:
  QByteArray m_content;

 protected:
  qint64 readData(char *data, qint64 maxSize);

 signals:
  void finished(dnetworkblockreply *reply);
};

class dnetworkdirreply: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dnetworkdirreply(void):QNetworkReply(0)
  {
  }

  dnetworkdirreply(const dnetworkdirreply &reply):QNetworkReply(0)
  {
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  ~dnetworkdirreply()
  {
  }

  dnetworkdirreply(QObject *parent, const QUrl &url);
  bool isSequential(void) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void load(void);

 protected:
  qint64 readData(char *data, qint64 maxSize);

 signals:
  void finished(dnetworkdirreply *reply);
};

class dnetworkerrorreply: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dnetworkerrorreply(void):QNetworkReply(0)
  {
  }

  dnetworkerrorreply(const dnetworkerrorreply &reply):QNetworkReply(0)
  {
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  ~dnetworkerrorreply()
  {
  }

  dnetworkerrorreply(QObject *parent, const QNetworkRequest &request);
  QByteArray html(void) const;
  bool isSequential(void) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void load(void);

 private:
  QByteArray m_content;

 protected:
  qint64 readData(char *data, qint64 maxSize);

 signals:
  void finished(dnetworkerrorreply *reply);
};

class dnetworkftpreply: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dnetworkftpreply(void):QNetworkReply(0)
  {
  }

  dnetworkftpreply(const dnetworkftpreply &reply):QNetworkReply(0)
  {
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  ~dnetworkftpreply()
  {
  }

  dnetworkftpreply(QObject *parent, const QUrl &url);
  QPointer<dftp> ftp(void) const;
  bool isSequential(void) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void load(void);

 private:
  QPointer<dftp> m_ftp;

 protected:
  qint64 readData(char *data, qint64 maxSize);

 private slots:
  void slotFtpFinished(bool ok);
  void slotUnsupportedContent(const QUrl &url);

 signals:
  void finished(dnetworkftpreply *reply);
};

class dnetworksslerrorreply: public QNetworkReply
{
  Q_OBJECT

 public:
  /*
  ** qRegisterMetaType() requires a public default contructor,
  ** a public copy constructor, and a public destructor.
  */

  dnetworksslerrorreply(void):QNetworkReply(0)
  {
  }

  dnetworksslerrorreply(const dnetworksslerrorreply &reply):QNetworkReply(0)
  {
    setOperation(reply.operation());
    setRequest(reply.request());
    setUrl(reply.url());
  }

  ~dnetworksslerrorreply()
  {
  }

  dnetworksslerrorreply(QObject *parent, const QNetworkRequest &request);
  QByteArray html(void) const;
  bool isSequential(void) const;
  qint64 bytesAvailable(void) const;
  void abort(void);
  void load(void);

 private:
  QByteArray m_content;

 protected:
  qint64 readData(char *data, qint64 maxSize);

 signals:
  void finished(dnetworksslerrorreply *reply);
};

class dnetworkaccessmanager: public QNetworkAccessManager
{
  Q_OBJECT

 public:
  dnetworkaccessmanager(QObject *parent);
  QNetworkReply *createRequest(Operation op,
			       const QNetworkRequest &req,
			       QIODevice *outgoingData = 0);
  void setLinkClicked(const QUrl &url);

 private:
  QUrl m_linkClicked;

 private slots:
  void slotFinished(QNetworkReply *reply);

 signals:
  void blockThirdPartyHost(const QString &host, const QUrl &url,
			   const QDateTime &dateTime);
  void doNotTrack(const QString &host, const QUrl &url,
		  const QDateTime &dateTime);
  void exceptionRaised(dexceptionswindow *window, const QUrl &url);
  void finished(dnetworkblockreply *reply);
  void finished(dnetworkdirreply *reply);
  void finished(dnetworkerrorreply *reply);
  void finished(dnetworkftpreply *reply);
  void finished(dnetworksslerrorreply *reply);
  void loadErrorPage(const QUrl &url);
  void loadImageRequest(const QString &host, const QUrl &url,
			const QDateTime &dateTime);
  void loadStarted(void);
  void suppressHttpReferrer(const QString &host, const QUrl &url,
			    const QDateTime &dateTime);
  void urlRedirectionRequest(const QString &host, const QUrl &url,
			     const QDateTime &dateTime);
};

#endif
