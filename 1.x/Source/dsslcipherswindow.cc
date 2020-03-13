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

#include <QKeyEvent>
#include <QSettings>
#include <QSslCipher>
#if QT_VERSION >= 0x050000
#include <QSslConfiguration>
#else
#include <QSslSocket>
#endif

#include "dmisc.h"
#include "dooble.h"
#include "dsslcipherswindow.h"

dsslcipherswindow::dsslcipherswindow(void):QMainWindow()
{
  setObjectName("sslcipherswindow");
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  connect(ui.allSSL, SIGNAL(toggled(bool)), this,
	  SLOT(slotToggleChoices(bool)));
  connect(ui.allTLS, SIGNAL(toggled(bool)), this,
	  SLOT(slotToggleChoices(bool)));
  connect(ui.close, SIGNAL(clicked(void)), this,
	  SLOT(slotClose(void)));
  slotSetIcons();
  createTable();
#if QT_VERSION < 0x040800
  ui.protocol->addItem("Any Protocol");
  ui.protocol->addItem("SSLv2");
  ui.protocol->addItem("SSLv3");
  ui.protocol->addItem("TLSv1.0");
  ui.protocol->addItem("Unknown Protocol");
#elif QT_VERSION < 0x050000
  ui.protocol->addItem("Any Protocol");
  ui.protocol->addItem("SSLv2");
  ui.protocol->addItem("SSLv3 & TLSv1.0");
  ui.protocol->addItem("SSLv3");
  ui.protocol->addItem("Secure Protocols");
  ui.protocol->addItem("TLSv1.0");
  ui.protocol->addItem("Unknown Protocol");
#else
  ui.protocol->addItem("Any Protocol");
  ui.protocol->addItem("SSLv2");
  ui.protocol->addItem("SSLv3 & TLSv1.0");
  ui.protocol->addItem("SSLv3");
  ui.protocol->addItem("Secure Protocols");
  ui.protocol->addItem("TLSv1.0");
  ui.protocol->addItem("TLSv1.0+");
  ui.protocol->addItem("TLSv1.1");
  ui.protocol->addItem("TLSv1.1+");
  ui.protocol->addItem("TLSv1.2");
  ui.protocol->addItem("TLSv1.2+");
  ui.protocol->addItem("Unknown Protocol");
#endif

  QString defaultProtocol("");

#if QT_VERSION < 0x040800
  defaultProtocol = "SSLv3";
#else
  defaultProtocol = "Secure Protocols";
#endif

  int index = ui.protocol->findText
    (dooble::s_settings.value(QString("%1/protocol").arg(objectName()),
			      defaultProtocol).toString());

  if(index >= 0)
    ui.protocol->setCurrentIndex(index);
  else
    ui.protocol->setCurrentIndex(ui.protocol->findText(defaultProtocol));

  connect(ui.protocol, SIGNAL(currentIndexChanged(const QString &)), this,
	  SLOT(slotProtocolChanged(const QString &)));
}

dsslcipherswindow::~dsslcipherswindow()
{
  saveState();
}

void dsslcipherswindow::populate(void)
{
  createTable();

  QHash<QString, bool> allChecked; // SSL, TLS
#if QT_VERSION >= 0x050000
  QList<QSslCipher> list(QSslConfiguration::supportedCiphers());
#else
  QList<QSslCipher> list(QSslSocket::supportedCiphers());
#endif

  allChecked["ssl"] = allChecked["tls"] = true;

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "allowedsslciphers");

    db.setDatabaseName
      (dooble::s_homePath + QDir::separator() + "allowedsslciphers.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	for(int i = 0; i < list.size(); i++)
	  {
	    query.prepare("INSERT INTO allowedsslciphers "
			  "(name, protocol, allowed) "
			  "VALUES (?, ?, ?)");
	    query.bindValue(0, list.at(i).name());
	    query.bindValue(1, list.at(i).protocolString());
	    query.bindValue(2, 1);
	    query.exec();
	  }

	disconnect(ui.listWidget,
		   SIGNAL(itemChanged(QListWidgetItem *)),
		   this,
		   SLOT(slotItemChanged(QListWidgetItem *)));
	query.prepare("SELECT name, protocol, allowed, OID "
		      "FROM allowedsslciphers");

	if(query.exec())
	  while(query.next())
	    {
	      QListWidgetItem *item = new QListWidgetItem
		(query.value(0).toString() + "-" +
		 query.value(1).toString());
	      QString protocol(query.value(1).toString().toLower());

	      item->setData
		(Qt::UserRole, query.value(0).toString());
	      item->setData
		(Qt::ItemDataRole(Qt::UserRole + 1),
		 query.value(1).toString().toLower());
	      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

	      if(query.value(2).toBool())
		item->setCheckState(Qt::Checked);
	      else
		{
		  for(int i = list.size() - 1; i >= 0; i--)
		    {
		      if(list.at(i).name().toLower() ==
			 query.value(0).toString().toLower() &&
			 list.at(i).protocolString().toLower() ==
			 query.value(1).toString().toLower())
			{
			  list.removeAt(i);
			  break;
			}
		    }

		  if(protocol.contains("ssl"))
		    allChecked["ssl"] = false;
		  else
		    allChecked["tls"] = false;

		  item->setCheckState(Qt::Unchecked);
		}

	      ui.listWidget->addItem(item);
	    }

	connect(ui.listWidget,
		SIGNAL(itemChanged(QListWidgetItem *)),
		this,
		SLOT(slotItemChanged(QListWidgetItem *)));
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("allowedsslciphers");

#if QT_VERSION >= 0x050000
  QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

  configuration.setCiphers(list);
  QSslConfiguration::setDefaultConfiguration(configuration);
#else
  QSslSocket::setDefaultCiphers(list);
#endif

  if(allChecked.value("ssl"))
    {
      ui.allSSL->blockSignals(true);
      ui.allSSL->setChecked(true);
      ui.allSSL->blockSignals(false);
    }

  if(allChecked.value("tls"))
    {
      ui.allTLS->blockSignals(true);
      ui.allTLS->setChecked(true);
      ui.allTLS->blockSignals(false);
    }
}

void dsslcipherswindow::slotClose(void)
{
  close();
}

void dsslcipherswindow::closeEvent(QCloseEvent *event)
{
  saveState();
  QMainWindow::closeEvent(event);
}

void dsslcipherswindow::slotProtocolChanged(const QString &text)
{
  dooble::s_settings[QString("%1/protocol").arg(objectName())] = text;

  QSettings settings;

  settings.setValue(QString("%1/protocol").arg(objectName()), text);
}

void dsslcipherswindow::slotShow(void)
{
  QAction *action = qobject_cast<QAction *> (sender());
  QRect rect(100, 100, 800, 600);

  if(action && action->parentWidget())
    {
      rect = action->parentWidget()->geometry();
      rect.setX(rect.x() + 50);
      rect.setY(rect.y() + 50);
      rect.setHeight(600);
      rect.setWidth(800);
    }

  if(!isVisible())
    {
      /*
      ** Don't annoy the user.
      */

      if(dooble::s_settings.contains(QString("%1/geometry").
				     arg(objectName())))
	{
	  if(dmisc::isGnome())
	    setGeometry(dooble::s_settings.value(QString("%1/geometry").
						 arg(objectName()),
						 rect).toRect());
	  else
	    {
	      QByteArray g(dooble::s_settings.
			   value(QString("%1/geometry").arg(objectName())).
			   toByteArray());

	      if(!restoreGeometry(g))
		setGeometry(rect);
	    }
	}
      else
	setGeometry(rect);
    }

  if(action)
    if(dooble::s_settings.value("settingsWindow/centerChildWindows",
				true).toBool())
      dmisc::centerChildWithParent(this, action->parentWidget());

  showNormal();
  activateWindow();
  raise();
}

void dsslcipherswindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  setWindowIcon
    (QIcon(settings.value("mainWindow/windowIcon").toString()));
  ui.close->setIcon
    (QIcon(settings.value("cancelButtonIcon").toString()));
}

void dsslcipherswindow::saveState(void)
{
  /*
  ** geometry() may return (0, 0) coordinates if the window is
  ** not visible.
  */

  if(isVisible())
    {
      if(dmisc::isGnome())
	dooble::s_settings[QString("%1/geometry").arg(objectName())] =
	  geometry();
      else
	dooble::s_settings[QString("%1/geometry").arg(objectName())] =
	  saveGeometry();
    }

  QSettings settings;

  if(isVisible())
    {
      if(dmisc::isGnome())
	settings.setValue(QString("%1/geometry").arg(objectName()),
			  geometry());
      else
	settings.setValue(QString("%1/geometry").arg(objectName()),
			  saveGeometry());
    }
}

void dsslcipherswindow::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Escape)
	close();
    }

  QMainWindow::keyPressEvent(event);
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool dsslcipherswindow::event(QEvent *event)
{
  if(event)
    if(event->type() == QEvent::WindowStateChange)
      if(windowState() == Qt::WindowNoState)
	{
	  /*
	  ** Minimizing the window on OS 10.6.8 and Qt 5.x will cause
	  ** the window to become stale once it has resurfaced.
	  */

	  hide();
	  show();
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool dsslcipherswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool dsslcipherswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif

void dsslcipherswindow::createTable(void)
{
  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "allowedsslciphers");

    db.setDatabaseName
      (dooble::s_homePath + QDir::separator() + "allowedsslciphers.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS allowedsslciphers ("
		   "allowed INTEGER NOT NULL DEFAULT 1, "
		   "name TEXT NOT NULL, "
		   "protocol TEXT NOT NULL, "
		   "PRIMARY KEY(name, protocol))");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("allowedsslciphers");
}

void dsslcipherswindow::slotItemChanged(QListWidgetItem *item)
{
  if(!item)
    return;

  createTable();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", "allowedsslciphers");

    db.setDatabaseName
      (dooble::s_homePath + QDir::separator() + "allowedsslciphers.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("UPDATE allowedsslciphers "
		      "SET allowed = ? WHERE LOWER(name) = ? "
		      "AND LOWER(protocol) = ?");
	query.bindValue(0, item->checkState() == Qt::Checked ? 1 : 0);
	query.bindValue(1, item->data(Qt::UserRole).toString().toLower());
	query.bindValue(2, item->data(Qt::ItemDataRole(Qt::UserRole + 1)).
			toString().toLower());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("allowedsslciphers");

  QList<QSslCipher> list;

  if(item->checkState() == Qt::Checked)
    {
#if QT_VERSION >= 0x050000
      list = QSslConfiguration::supportedCiphers();
#else
      list = QSslSocket::supportedCiphers();
#endif

      QSslCipher cipher;

      for(int i = 0; i < list.size(); i++)
	{
	  if(list.at(i).name().toLower() ==
	     item->data(Qt::UserRole).toString().toLower() &&
	     list.at(i).protocolString().toLower() ==
	     item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toString().
	     toLower())
	    {
	      cipher = list.at(i);
	      break;
	    }
	}

      if(!cipher.isNull())
	{
#if QT_VERSION >= 0x050000
	  list = QSslConfiguration::defaultConfiguration().ciphers();
#else
	  list = QSslSocket::defaultCiphers();
#endif
	  list.append(cipher);
	}
    }
  else
    {
#if QT_VERSION >= 0x050000
      list = QSslConfiguration::defaultConfiguration().ciphers();
#else
      list = QSslSocket::defaultCiphers();
#endif

      for(int i = list.size() - 1; i >= 0; i--)
	{
	  if(list.at(i).name().toLower() ==
	     item->data(Qt::UserRole).toString().toLower() &&
	     list.at(i).protocolString().toLower() ==
	     item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toString().
	     toLower())
	    {
	      list.removeAt(i);
	      break;
	    }
	}
    }

#if QT_VERSION >= 0x050000
  QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

  configuration.setCiphers(list);
  QSslConfiguration::setDefaultConfiguration(configuration);
#else
  QSslSocket::setDefaultCiphers(list);
#endif
}

void dsslcipherswindow::slotToggleChoices(bool state)
{
  Q_UNUSED(state);
  QCheckBox *checkBox = qobject_cast<QCheckBox *> (sender());

  if(!checkBox)
    return;

  QString type("");

  if(checkBox == ui.allSSL)
    type = "ssl";
  else
    type = "tls";

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < ui.listWidget->count(); i++)
    {
      QListWidgetItem *item = ui.listWidget->item(i);

      if(!item)
	continue;

      if(item->text().toLower().contains(type))
	{
	  if(item->checkState() == Qt::Checked)
	    item->setCheckState(Qt::Unchecked);
	  else
	    item->setCheckState(Qt::Checked);
	}
    }

  QApplication::restoreOverrideCursor();
}

QSsl::SslProtocol dsslcipherswindow::protocol(void) const
{
#if QT_VERSION < 0x040800
  QSsl::SslProtocol protocol = QSsl::SslV3;
#else
  QSsl::SslProtocol protocol = QSsl::SecureProtocols;
#endif
  QString text(ui.protocol->currentText());

#if QT_VERSION < 0x040800
  if(text == "Any Protocol")
    protocol = QSsl::AnyProtocol;
  else if(text == "SSLv2")
    protocol = QSsl::SslV2;
  else if(text == "SSLv3")
    protocol = QSsl::SslV3;
  else if(text == "TLSv1.0")
    protocol = QSsl::TlsV1;
  else if(text == "Unknown Protocol")
    protocol = QSsl::UnknownProtocol;
#elif QT_VERSION < 0x050000
  if(text == "Any Protocol")
    protocol = QSsl::AnyProtocol;
  else if(text == "SSLv2")
    protocol = QSsl::SslV2;
  else if(text == "SSLv3 & TLSv1.0")
    protocol = QSsl::TlsV1SslV3;
  else if(text == "SSLv3")
    protocol = QSsl::SslV3;
  else if(text == "Secure Protocols")
    protocol = QSsl::SecureProtocols;
  else if(text == "TLSv1.0")
    protocol = QSsl::TlsV1;
  else if(text == "Unknown Protocol")
    protocol = QSsl::UnknownProtocol;
#else
  if(text == "Any Protocol")
    protocol = QSsl::AnyProtocol;
  else if(text == "SSLv2")
    protocol = QSsl::SslV2;
  else if(text == "SSLv3 & TLSv1.0")
    protocol = QSsl::TlsV1SslV3;
  else if(text == "SSLv3")
    protocol = QSsl::SslV3;
  else if(text == "Secure Protocols")
    protocol = QSsl::SecureProtocols;
  else if(text == "TLSv1.0")
    protocol = QSsl::TlsV1_0;
  else if(text == "TLSv1.0+")
    protocol = QSsl::TlsV1_0OrLater;
  else if(text == "TLSv1.1")
    protocol = QSsl::TlsV1_1;
  else if(text == "TLSv1.1+")
    protocol = QSsl::TlsV1_1OrLater;
  else if(text == "TLSv1.2")
    protocol = QSsl::TlsV1_2;
  else if(text == "TlsV1_2+")
    protocol = QSsl::TlsV1_2OrLater;
  else if(text == "Unknown Protocol")
    protocol = QSsl::UnknownProtocol;
#endif

  return protocol;
}
