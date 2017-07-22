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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QWebEngineProfile>

#include "dooble_settings.h"

QMap<QString, QVariant> dooble_settings::s_settings;
QReadWriteLock dooble_settings::s_settings_mutex;

dooble_settings::dooble_settings(void):QMainWindow(0)
{
  m_ui.setupUi(this);
  connect(m_ui.buttonBox->button(QDialogButtonBox::Apply),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_apply(void)));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Close),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(close(void)));
  s_settings["icon_set"] = "Snipicons";
}

QVariant dooble_settings::setting(const QString &key)
{
  QReadLocker lock(&s_settings_mutex);

  return s_settings.value(key);
}

void dooble_settings::set_setting(const QString &key, const QVariant &value)
{
  if(key.trimmed().isEmpty())
    return;
  else if(value.isNull())
    {
      QWriteLocker lock(&s_settings_mutex);

      s_settings.remove(key);
      return;
    }

  QWriteLocker lock(&s_settings_mutex);

  s_settings[key.trimmed()] = value;
  lock.unlock();
}

void dooble_settings::slot_apply(void)
{
  /*
  ** Cache
  */

  QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize
    (1024 * 1024 * m_ui.cache_size->value());

  if(m_ui.cache_type->currentIndex() == 0)
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::MemoryHttpCache);
  else
    QWebEngineProfile::defaultProfile()->setHttpCacheType
      (QWebEngineProfile::NoCache);
}
