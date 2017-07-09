/*
** Copyright (c) 2008 - present, Alexis Megas, Bernd Stramm.
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

#include "dplugintab.h"

#include <QDebug>
#include <QAction>
#include <QHBoxLayout>
#include <QMessageBox>

using namespace simpleplugin;

dplugintab::dplugintab(Extension *ext, QWidget *parent):QWidget(parent),
							m_extension(ext),
							extWidget(0),
							sagent(0)
{
  setLayout(new QHBoxLayout());
  layout()->setContentsMargins(0, 0, 0, 0);
  m_action = 0;

  if(m_extension)
    {
      extWidget = m_extension->widget();

      if(extWidget)
	{
	  extWidget->resize(size());
	  layout()->addWidget(extWidget);
	}

      sagent = m_extension->agent();

      if(sagent)
	connect(sagent, SIGNAL(exiting(Extension *, int)),
		this, SLOT(emitExiting(Extension *, int)));
    }
}

dplugintab::~dplugintab()
{
  if(m_action)
    m_action->deleteLater();

  if(sagent)
    disconnect(sagent, 0,0,0);

  /*
  ** We need to set the plugin's widget's parent to 0. Otherwise,
  ** Dooble will abort.
  */

  if(extWidget && extWidget->parent() == this)
    extWidget->setParent(0);
}

QWidget *dplugintab::pluginWidget(void) const
{
  return extWidget;
}

QIcon dplugintab::icon(void) const
{
  if(m_extension)
    return m_extension->icon();

  return QIcon();
}

QString dplugintab::title(void) const
{
  if(m_extension)
    return m_extension->title();

  return QString("");
}

bool dplugintab::canClose(void) const
{
  if(m_extension)
    return m_extension->canRestart();
  else
    return true;
}

void dplugintab::close(void)
{
  if(m_extension)
    m_extension->quit();
}

void dplugintab::emitExiting(Extension *ex, int status)
{
  Q_UNUSED(ex);
  emit exiting(this, status);
}

void dplugintab::resizeEvent(QResizeEvent *event)
{
  if(extWidget)
    extWidget->resize(size());

  QWidget::resizeEvent(event);
}

void dplugintab::slotIconChange(Extension *ext)
{
  if(ext == m_extension && ext)
    emit iconChange(this, m_extension->icon());
}

void dplugintab::slotTitleChange(Extension *ext)
{
  if(ext == m_extension && ext)
    emit titleChange(this, m_extension->title());
}

Extension *dplugintab::extension(void) const
{
  return m_extension;
}

void dplugintab::setTabAction(QAction *action)
{
  if(m_action)
    {
      removeAction(m_action);
      m_action->deleteLater();
    }

  m_action = action;

  if(m_action)
    addAction(m_action);
}

QAction *dplugintab::tabAction(void) const
{
  return m_action;
}
