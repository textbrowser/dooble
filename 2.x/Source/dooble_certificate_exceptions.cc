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

#include <QDir>
#include <QKeyEvent>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "dooble_certificate_exceptions.h"
#include "dooble_settings.h"

QAtomicInteger<qintptr> dooble_certificate_exceptions::s_db_id;

dooble_certificate_exceptions::dooble_certificate_exceptions(void):QMainWindow()
{
  m_ui.setupUi(this);
}

void dooble_certificate_exceptions::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_certificate_exceptions::keyPressEvent(QKeyEvent *event)
{
  if(!parent())
    {
      if(event && event->key() == Qt::Key_Escape)
	close();

      QMainWindow::keyPressEvent(event);
    }
  else if(event)
    event->ignore();
}

void dooble_certificate_exceptions::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_certificate_exceptions::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("certificate_exceptions_geometry", saveGeometry().toBase64());
}

void dooble_certificate_exceptions::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("certificate_exceptions_geometry").
			      toByteArray()));

  QMainWindow::show();
}

void dooble_certificate_exceptions::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("certificate_exceptions_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
}
