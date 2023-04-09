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

#include <QCryptographicHash>
#include <QKeyEvent>
#include <QShortcut>
#include <QtConcurrent>

#include "dooble.h"
#include "dooble_about.h"
#include "dooble_swifty.h"
#include "dooble_version.h"

dooble_about::dooble_about(void):QMainWindow()
{
  m_swifty = new swifty
    (DOOBLE_VERSION_STRING,
     "#define DOOBLE_VERSION_STRING",
     QUrl::fromUserInput("https://raw.githubusercontent.com/"
			 "textbrowser/dooble/master/Source/dooble_version.h"),
     this);
  m_swifty->download();
  m_ui.setupUi(this);
  m_ui.digest->clear();
  connect(m_swifty,
	  SIGNAL(different(const QString &)),
	  this,
	  SLOT(slot_swifty(void)));
  connect(m_swifty,
	  SIGNAL(same(void)),
	  this,
	  SLOT(slot_swifty(void)));
  connect(m_ui.license,
	  SIGNAL(linkActivated(const QString &)),
	  this,
	  SLOT(slot_link_activated(const QString &)));
  connect(m_ui.release_notes,
	  SIGNAL(linkActivated(const QString &)),
	  this,
	  SLOT(slot_link_activated(const QString &)));
  connect(m_ui.site,
	  SIGNAL(linkActivated(const QString &)),
	  this,
	  SLOT(slot_link_activated(const QString &)));
  connect(this,
	  SIGNAL(file_digest_computed(const QByteArray &)),
	  this,
	  SLOT(slot_file_digest_computed(const QByteArray &)));
  new QShortcut(QKeySequence(tr("Ctrl+W")), this, SLOT(close(void)));

  QString qversion("");
  const auto tmp = qVersion();

  if(tmp)
    qversion = tmp;

  qversion = qversion.trimmed();

  if(qversion.isEmpty())
    qversion = "unknown";

  m_ui.license->setText
    (tr("<a href=\"qrc://Documentation/DoobleLicense.html\">"
	"Dooble 3-Clause BSD License</a>"));

  auto text
    (tr("Architecture %1.<br>"
	"Product: %2.<br>"
	"Qt version %3 (runtime %4).").
     arg(DOOBLE_ARCHITECTURE_STR).
     arg(QSysInfo::prettyProductName()).
     arg(QT_VERSION_STR).
     arg(qversion));

  m_ui.local_information->setText(text);
  m_ui.release_notes->setText
    (tr("<a href=\"qrc://Documentation/ReleaseNotes.html\">"
	"Release Notes</a>"));
  m_ui.user_agent->setText(dooble::s_default_http_user_agent);
  m_ui.version->setText
    (tr("Dooble version %1, X. The official version is <b>%1</b>.").
     arg(DOOBLE_VERSION_STRING));
  compute_self_digest();
}

dooble_about::~dooble_about()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void dooble_about::compute_self_digest(void)
{
  m_future.cancel();
  m_future.waitForFinished();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_future = QtConcurrent::run
    (this,
     &dooble_about::compute_self_digest_task,
     QApplication::applicationFilePath());
#else
  m_future = QtConcurrent::run
    (&dooble_about::compute_self_digest_task,
     this,
     QApplication::applicationFilePath());
#endif
}

void dooble_about::compute_self_digest_task(const QString &file_path)
{
  QByteArray buffer(4096, 0);
  QCryptographicHash hash(QCryptographicHash::Sha3_512);
  QFile file(file_path);

  if(file.open(QIODevice::ReadOnly))
    {
      qint64 rc = 0;

      while((rc = file.read(buffer.data(), buffer.length())) > 0)
	if(m_future.isCanceled())
	  break;
	else
	  hash.addData(buffer.mid(0, static_cast<int> (rc)));
    }

  file.close();

  if(!m_future.isCanceled())
    emit file_digest_computed(hash.result());
}

void dooble_about::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_about::slot_file_digest_computed(const QByteArray &digest)
{
  m_ui.digest->setText
    (tr("The SHA3-512 digest of %1 is %2.").
     arg(QApplication::applicationFilePath()).
     arg(digest.toHex().insert(64, '\n').constData()));
  resize(sizeHint());
}

void dooble_about::slot_link_activated(const QString &url)
{
  emit link_activated(QUrl::fromUserInput(url));
}

void dooble_about::slot_swifty(void)
{
  m_ui.version->setText
    (tr("Dooble version %1, X. The official version is <b>%2</b>.").
     arg(DOOBLE_VERSION_STRING).
     arg(m_swifty->newest_version()));
}
