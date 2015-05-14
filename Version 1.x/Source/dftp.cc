/*
** Copyright (c) 2008 - present, Alexis Megas.
**
** The method slotDownloadReadyRead() uses some logic from QFtp source.
** Thanks.
**
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

#include <QtDebug>
#include <QApplication>
#include <QNetworkProxy>

#include "dftp.h"
#include "dmisc.h"
#include "dooble.h"

dftp::dftp(QObject *parent):QObject(parent)
{
  m_downloadSize = 0;
  m_totalBytesDownloaded = 0;
  m_commandType = UnknownCommand;
  m_commandSocket = new QTcpSocket(this);
  m_commandSocket->setReadBufferSize(0);

  /*
  ** Connect to an imaginary host. Fixes an unexplainable
  ** QAbstractSocket::UnsupportedSocketOperationError error.
  ** Replicate:
  ** 1. Launch Dooble.
  ** 2. Prepare a Sockets5 FTP proxy.
  ** 3. Visit ftp://ftp.debian.org.
  ** 4. Error surfaces.
  */

  m_commandSocket->connectToHost("0.0.0.0", 0);
  connect(m_commandSocket, SIGNAL(connected(void)),
	  this, SIGNAL(loadStarted(void)));
  connect(m_commandSocket, SIGNAL(error(QAbstractSocket::SocketError)),
	  this, SLOT(slotSocketError(QAbstractSocket::SocketError)));
  connect(m_commandSocket, SIGNAL(readyRead(void)),
	  this, SLOT(slotCommandReadyRead(void)));
  m_downloadSocket = new QTcpSocket(this);
  m_downloadSocket->setReadBufferSize(0);
  connect(m_downloadSocket, SIGNAL(connected(void)),
	  this, SLOT(slotDownloadSocketConnected(void)));
  connect(m_downloadSocket, SIGNAL(error(QAbstractSocket::SocketError)),
	  this, SLOT(slotSocketError(QAbstractSocket::SocketError)));
  connect(m_downloadSocket, SIGNAL(readyRead(void)),
	  this, SLOT(slotDownloadReadyRead(void)));
}

dftp::~dftp()
{
}

void dftp::get(const QUrl &url, const QString &command)
{
  if(url.isEmpty() || !url.isValid() || url.path().isEmpty())
    return;

  abort();

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    {
      emit finished(false);
      return;
    }

  m_url = url;
  m_command = command.trimmed();
  m_commandType = GetCommand;

  if(m_command.isEmpty())
    m_command = QString("SIZE %1\r\nRETR %1\r\n").arg(m_url.path());
  else if(m_command.endsWith("\r\n"))
    m_command = QString("SIZE %1\r\n%2RETR %1\r\n").arg(m_url.path()).
      arg(command);
  else
    m_command = QString("SIZE %1\r\n%2\r\nRETR %1\r\n").
      arg(m_url.path()).arg(command);

  disconnect(m_downloadSocket, SIGNAL(readChannelFinished(void)),
	     this, SIGNAL(finished(void)));
  connect(m_downloadSocket, SIGNAL(readChannelFinished(void)),
	  this, SIGNAL(finished(void)));
  disconnect(m_downloadSocket, SIGNAL(readyRead(void)),
	     this, SIGNAL(readyRead(void)));
  connect(m_downloadSocket, SIGNAL(readyRead(void)),
	  this, SIGNAL(readyRead(void)));
  m_commandSocket->setProxy
    (dmisc::proxyByFunctionAndUrl(DoobleDownloadType::Ftp, m_url));
  m_commandSocket->connectToHost
    (m_url.host(), static_cast<quint16> (m_url.port(21)));
}

void dftp::abort(void)
{
  m_commandBytes.clear();
  m_commandSocket->abort();
  m_downloadSocket->abort();
}

void dftp::close(void)
{
  m_commandBytes.clear();
  m_commandSocket->close();
  m_downloadSocket->close();
}

void dftp::fetchList(const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  abort();

  if(dooble::s_settings.value("mainWindow/offlineMode", false).toBool())
    {
      emit finished(false);
      return;
    }

  m_url = url;

  if(m_url.path().isEmpty())
    m_command = "CWD .\r\n";
  else
    m_command = QString("CWD %1\r\n").arg(m_url.path());

  m_commandType = ListCommand;
  m_commandSocket->setProxy(dmisc::proxyByUrl(m_url));
  m_commandSocket->connectToHost
    (m_url.host(), static_cast<quint16> (m_url.port(21)));
}

void dftp::slotSocketError(QAbstractSocket::SocketError error)
{
  if(error == QAbstractSocket::RemoteHostClosedError)
    return;

  QTcpSocket *socket = qobject_cast<QTcpSocket *> (sender());

  if(!socket)
    return;

  QString errorString(socket->errorString().trimmed());

  if(!errorString.isEmpty())
    if(!errorString.at(errorString.length() - 1).isPunct())
      errorString += ".";

  if(!errorString.isEmpty())
    emit statusMessageReceived(errorString);

  socket->abort();
  emit finished(false);
}

void dftp::slotCommandReadyRead(void)
{
  m_commandBytes.append(m_commandSocket->readAll());

  while(!m_commandBytes.isEmpty())
    {
      if(m_commandBytes.indexOf("\r\n") < 0)
	break;

      QByteArray bytes
	(m_commandBytes.mid(0, m_commandBytes.indexOf("\r\n")));

      emit statusMessageReceived(bytes.constData());
      m_commandBytes.remove(0, bytes.size() + 2);

      if(bytes.startsWith("150 "))
	{
	  /*
	  ** About to receive directory contents.
	  */

	  emit aboutToReceiveDirectoryContents();
	}
      else if(bytes.startsWith("213 "))
	{
	  /*
	  ** We received a response from the SIZE request.
	  */

	  m_downloadSize = bytes.mid(4).toLongLong();
	  emit downloadProgress(m_totalBytesDownloaded, m_downloadSize);
	}
      else if(bytes.startsWith("220 "))
	{
	  /*
	  ** We're connected. Let's send the username.
	  */

	  if(!m_url.userName().isEmpty())
	    m_commandSocket->write
	      (QString("USER %1\r\n").arg(m_url.userName()).toLatin1());
	  else
	    m_commandSocket->write("USER anonymous\r\n");

	  m_commandSocket->flush();
	}
      else if(bytes.startsWith("221 "))
	{
	  /*
	  ** Command socket closing.
	  */

	  m_commandSocket->close();
	  emit finished(true);
	}
      else if(bytes.startsWith("226 "))
	{
	  /*
	  ** Download complete. Let's quit!
	  */

	  m_commandSocket->write("QUIT\r\n");
	  m_commandSocket->flush();
	}
      else if(bytes.startsWith("227 "))
	{
	  /*
	  ** We've entered passive mode. Let's parse the provided
	  ** IP address and port and commence our download.
	  */

	  /*
	  ** Connect to the specified host and port.
	  */

	  QRegExp pattern("(\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)");

	  if(pattern.indexIn(bytes) == -1)
	    {
	      dmisc::logError("dftp::slotReadyRead(): "
			      "Invalid IP address. "
			      "Aborting connection.");
	      abort();
	    }
	  else
	    {
	      QUrl url;
	      quint16 port = 0;
	      QString host("");
	      QStringList list(pattern.capturedTexts());

	      host = list.value(1) + "." + list.value(2) + "." +
		list.value(3) + "." + list.value(4);
	      port = static_cast<quint16>
		((list.value(5).toUShort() << 8) + list.value(6).toUShort());
	      url.setHost(host);
	      url.setScheme("ftp");
	      m_downloadSocket->abort();

	      if(m_commandType == ListCommand)
		m_downloadSocket->setProxy(dmisc::proxyByUrl(url));
	      else
		m_downloadSocket->setProxy
		  (dmisc::proxyByFunctionAndUrl(DoobleDownloadType::Ftp, url));

	      m_downloadSocket->connectToHost(host, port, QIODevice::ReadOnly);
	    }
	}
      else if(bytes.startsWith("230 "))
	{
	  /*
	  ** We're logged in!
	  */

	  if(m_commandType == ListCommand)
	    m_commandSocket->write("TYPE A\r\nPASV\r\n");
	  else
	    m_commandSocket->write("TYPE I\r\nPASV\r\n");

	  m_commandSocket->flush();
	}
      else if(bytes.startsWith("250 "))
	{
	  /*
	  ** Directory changed successfully.
	  */

	  emit directoryChanged(m_url);
	  m_commandSocket->write("LIST\r\n");
	  m_commandSocket->flush();
	}
      else if(bytes.startsWith("331 "))
	{
	  /*
	  ** Send the password.
	  */

	  if(!m_url.password().isEmpty())
	    m_commandSocket->write
	      (QString("PASS %1\r\n").arg(m_url.password()).toLatin1());
	  else
	    m_commandSocket->write("PASS anonymous@\r\n");

	  m_commandSocket->flush();
	}
      else if(bytes.startsWith("550 "))
	{
	  /*
	  ** The target is not a directory.
	  */

	  emit unsupportedContent(m_url);
	  emit finished(false);
	}
      else if(bytes.mid(0, 3).toInt() >= 400)
	{
	  /*
	  ** Errors!
	  */

	  m_commandSocket->close();
	  m_downloadSocket->close();
	  emit finished(false);
	}
    }
}

void dftp::slotDownloadReadyRead(void)
{
  if(m_commandType == GetCommand)
    {
      m_totalBytesDownloaded = qAbs
	(m_totalBytesDownloaded +
	 m_downloadSocket->bytesAvailable()); // +=
      emit downloadProgress(m_totalBytesDownloaded, m_downloadSize);
      return;
    }

  QList<dftpfileinfo> infos;

  while(m_downloadSocket->canReadLine())
    {
      QByteArray bytes(m_downloadSocket->readLine().trimmed());

      /*
      ** This should be directory data.
      */

      QStringList list(QString(bytes.constData()).trimmed().
		       split(" ", QString::SkipEmptyParts));

      if(list.size() < 9)
	continue;

      if(!list.isEmpty())
	{
	  dftpfileinfo info;
	  QDateTime lastModified;

	  if(list.value(0).trimmed().startsWith("d") ||
	     list.value(0).trimmed().startsWith("l"))
	    {
	      if(list.value(0).trimmed().startsWith("d"))
		{
		  info.setName(list.value(list.size() - 1).trimmed());
		  info.setPath(info.name());
		}
	      else
		{
		  info.setName(list.value(8).trimmed());
		  info.setPath(list.value(list.size() - 1).trimmed());
		}

	      info.setDir(true);
	      info.setFile(false);
	    }
	  else
	    {
	      info.setDir(false);
	      info.setFile(true);
	      info.setName(list.value(list.size() - 1).trimmed());
	    }

	  if(info.name() == "." || info.name() == "..")
	    continue;

	  info.setSize(list.value(4).toLongLong());

	  /*
	  ** Some of the following logic is based on QFtp source.
	  ** Thanks Qt!
	  */

	  QStringList dateFormats;

	  dateFormats << "MMM dd yyyy"
		      << "MMM dd hh:mm"
		      << "MMM d yyyy"
		      << "MMM d hh:mm"
		      << "MMM d yyyy"
		      << "MMM dd yyyy";

	  int n = 0;
	  QString dateString = list.value(5) + " " +
	    list.value(6) + " " + list.value(7);

	  if(!dateString.isEmpty())
	    dateString[0] = dateString.at(0).toUpper();

	  do
	    {
	      lastModified = QLocale::c().toDateTime
		(dateString, dateFormats.at(n++));
	    }
	  while(n < dateFormats.size() && (!lastModified.isValid()));

	  if(n == 2 || n == 4)
	    {
	      lastModified.setDate(QDate(QDate::currentDate().year(),
					 lastModified.date().month(),
					 lastModified.date().day()));

	      const int futureTolerance = 86400;

	      if(lastModified.secsTo(QDateTime::currentDateTime()) <
		 -futureTolerance)
		{
		  QDate d = lastModified.date();

#if QT_VERSION >= 0x050000
		  d.setDate(d.year() - 1, d.month(), d.day());
#else
		  d.setYMD(d.year() - 1, d.month(), d.day());
#endif
		  lastModified.setDate(d);
		}
	    }

	  if(lastModified.isValid())
	    info.setLastModified(lastModified);

	  infos.append(info);
	}
    }

  if(!infos.isEmpty())
    emit listInfos(infos);
}

void dftp::slotDownloadSocketConnected(void)
{
  m_commandSocket->write(m_command.toLatin1());
  m_commandSocket->flush();
}

QUrl dftp::url(void) const
{
  return m_url;
}

QByteArray dftp::readAll(void)
{
  return m_downloadSocket->readAll();
}
