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
#include <QSslConfiguration>
#include <QSslSocket>

#include "dmisc.h"
#include "dooble.h"
#include "dsslcipherswindow.h"

dsslcipherswindow::dsslcipherswindow(void):QMainWindow()
{
  setObjectName("sslcipherswindow");
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
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
}

dsslcipherswindow::~dsslcipherswindow()
{
  saveState();
}

void dsslcipherswindow::populate(void)
{
  createTable();

  /*
  ** Remove archaic ciphers.
  */

  QHash<QString, bool> allChecked; // SSL, TLS
  QList<QSslCipher> allowed;
  QList<QSslCipher> list(QSslSocket::supportedCiphers());
  QList<bool> states;

  allChecked["ssl"] = allChecked["tls"] = true;

  for(int i = 0; i < list.size(); i++)
    if(list.at(i).protocolString().toLower().contains("sslv1") ||
       list.at(i).protocolString().toLower().contains("sslv2") ||
       list.at(i).protocolString().toLower().contains("sslv3") ||
       list.at(i).supportedBits() < 128 ||
       list.at(i).usedBits() < 128)
      states.append(false);
    else
      {
	allowed.append(list.at(i));
	states.append(true);
      }

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
	    query.bindValue(2, states.at(i) ? 1 : 0);
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
	      QSslCipher cipher;
	      QString protocol(query.value(1).toString().toLower());

	      if(protocol.contains("sslv3"))
		cipher = QSslCipher
		  (query.value(0).toString(), QSsl::SslV3);
	      else if(protocol.contains("tlsv1.0"))
		cipher = QSslCipher
		  (query.value(0).toString(), QSsl::TlsV1_0);
	      else if(protocol.contains("tlsv1.1"))
		cipher = QSslCipher
		  (query.value(0).toString(), QSsl::TlsV1_1);
	      else if(protocol.contains("tlsv1.2"))
		cipher = QSslCipher
		  (query.value(0).toString(), QSsl::TlsV1_2);
	      else
		cipher = QSslCipher
		  (query.value(0).toString(), QSsl::UnknownProtocol);

	      if(cipher.isNull())
		{
		  QSqlQuery deleteQuery(db);

		  deleteQuery.prepare("DELETE FROM allowedsslciphers "
				      "WHERE OID = ?");
		  deleteQuery.bindValue(0, query.value(3));
		  deleteQuery.exec();
		  allowed.removeOne(cipher);
		  continue;
		}

	      QListWidgetItem *item = new QListWidgetItem
		(query.value(0).toString() + "-" +
		 query.value(1).toString());

	      item->setData
		(Qt::UserRole,
		 query.value(0).toString());
	      item->setData
		(Qt::ItemDataRole(Qt::UserRole + 1),
		 query.value(1).toString().toLower());
	      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

	      if(query.value(2).toBool())
		{
		  allowed.append(cipher);
		  item->setCheckState(Qt::Checked);
		}
	      else
		{
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

  bool sslv3 = false;
  bool tlsv10 = false;
  bool tlsv11 = false;
  bool tlsv12 = false;

  for(int i = 0; i < ui.listWidget->count(); i++)
    {
      QListWidgetItem *item = ui.listWidget->item(i);

      if(!item)
	continue;

      if(item->text().toLower().contains("sslv3"))
	if(item->checkState() == Qt::Checked)
	  sslv3 = true;

      if(item->text().toLower().contains("tlsv1.0"))
	if(item->checkState() == Qt::Checked)
	  tlsv10 = true;

      if(item->text().toLower().contains("tlsv1.1"))
	if(item->checkState() == Qt::Checked)
	  tlsv11 = true;

      if(item->text().toLower().contains("tlsv1.2"))
	if(item->checkState() == Qt::Checked)
	  tlsv12 = true;
    }

  QSsl::SslProtocol protocol = QSsl::UnknownProtocol;

  if(sslv3 && tlsv10 && tlsv11 && tlsv12)
    protocol = QSsl::UnknownProtocol;
  else if(sslv3 && tlsv10)
    protocol = QSsl::TlsV1SslV3;
  else if(sslv3 && (tlsv11 || tlsv12))
    protocol = QSsl::UnknownProtocol;
  else if(tlsv12 && !tlsv10 && !tlsv11)
    protocol = QSsl::TlsV1_2;
  else if(tlsv11 && !tlsv10)
    protocol = QSsl::TlsV1_1;
  else if(tlsv10)
    protocol = QSsl::TlsV1_0;
  else if(sslv3)
    protocol = QSsl::SslV3;
  else
    protocol = QSsl::UnknownProtocol;

  QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

  configuration.setProtocol(protocol);
  QSslConfiguration::setDefaultConfiguration(configuration);
  QSslSocket::setDefaultCiphers(allowed);

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
			   value(QString("%1/geometry").
				 arg(objectName())).
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
				false).toBool())
      dmisc::centerChildWithParent(this, action->parentWidget());

  showNormal();
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

bool dsslcipherswindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}

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
		   "name TEXT PRIMARY KEY NOT NULL, "
		   "protocol TEXT KEY NOT NULL, "
		   "allowed INTEGER NOT NULL DEFAULT 1)");
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
		      "SET allowed = ? WHERE name = ?");
	query.bindValue(0, item->checkState() == Qt::Checked ? 1 : 0);
	query.bindValue(1, item->data(Qt::UserRole).toString());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("allowedsslciphers");

  QList<QSslCipher> list(QSslSocket::defaultCiphers());
  QSslCipher cipher;

  if(item->data(Qt::ItemDataRole(Qt::UserRole + 1)) == "sslv3")
    cipher = QSslCipher(item->data(Qt::UserRole).toString(), QSsl::SslV3);
  else if(item->data(Qt::ItemDataRole(Qt::UserRole + 1)) == "tlsv1.0")
    cipher = QSslCipher(item->data(Qt::UserRole).toString(), QSsl::TlsV1_0);
  else if(item->data(Qt::ItemDataRole(Qt::UserRole + 1)) == "tlsv1.1")
    cipher = QSslCipher(item->data(Qt::UserRole).toString(), QSsl::TlsV1_1);
  else if(item->data(Qt::ItemDataRole(Qt::UserRole + 1)) == "tlsv1.2")
    cipher = QSslCipher(item->data(Qt::UserRole).toString(), QSsl::TlsV1_2);
  else
    cipher = QSslCipher
      (item->data(Qt::UserRole).toString(), QSsl::UnknownProtocol);

  if(!cipher.isNull())
    {
      list.append(cipher);
      QSslSocket::setDefaultCiphers(list);
    }
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
