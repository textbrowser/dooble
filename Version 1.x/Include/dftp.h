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

#ifndef _dftp_h_
#define _dftp_h_

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>
#include <QUrl>

class dftpfileinfo
{
 public:
  dftpfileinfo(void)
  {
    m_isDir = false;
    m_isFile = false;
    m_size = 0;
  }

  bool isDir(void) const
  {
    return m_isDir;
  }

  bool isFile(void) const
  {
    return m_isFile;
  }

  void setDir(const bool isDir)
  {
    m_isDir = isDir;
  }

  void setFile(const bool isFile)
  {
    m_isFile = isFile;
  }

  void setName(const QString &name)
  {
    m_name = name;
  }

  void setPath(const QString &path)
  {
    m_path = path;
  }

  void setSize(const qint64 size)
  {
    m_size = size;
  }

  void setLastModified(const QDateTime &lastModified)
  {
    m_lastModified = lastModified;
  }

  qint64 size(void) const
  {
    return m_size;
  }

  QString name(void) const
  {
    return m_name;
  }

  QString path(void) const
  {
    return m_path;
  }

  QDateTime lastModified(void) const
  {
    return m_lastModified;
  }

 private:
  bool m_isDir;
  bool m_isFile;
  qint64 m_size;
  QString m_name;
  QString m_path;
  QDateTime m_lastModified;
};

class dftp: public QObject
{
  Q_OBJECT

 private:
  enum CommandType
  {
    UnknownCommand = -1,
    GetCommand = 0,
    ListCommand = 1
  };

 public:
  dftp(QObject *parent);
  ~dftp();
  void get(const QUrl &url, const QString &command = QString(""));
  void abort(void);
  void close(void);
  void fetchList(const QUrl &url);
  QUrl url(void) const;
  QByteArray readAll(void);

 private:
  QUrl m_url;
  qint64 m_downloadSize;
  qint64 m_totalBytesDownloaded;
  QString m_command;
  QByteArray m_commandBytes;
  CommandType m_commandType;
  QPointer<QTcpSocket> m_commandSocket;
  QPointer<QTcpSocket> m_downloadSocket;

 private slots:
  void slotSocketError(QAbstractSocket::SocketError error);
  void slotCommandReadyRead(void);
  void slotDownloadReadyRead(void);
  void slotDownloadSocketConnected(void);

 signals:
  void finished(void);
  void finished(const bool ok);
  void listInfos(const QList<dftpfileinfo> &infos);
  void readyRead(void);
  void loadStarted(void);
  void directoryChanged(const QUrl &url);
  void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void unsupportedContent(const QUrl &url);
  void statusMessageReceived(const QString &message);
  void aboutToReceiveDirectoryContents(void);
};

#endif
