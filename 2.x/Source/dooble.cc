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

#include <QUrl>

#include "dooble.h"
#include "dooble_favicons.h"
#include "dooble_page.h"
#include "dooble_web_engine_view.h"

QMap<QString, QVariant> dooble::s_settings;

dooble::dooble(dooble_web_engine_view *view):QMainWindow()
{
  m_ui.setupUi(this);
  connect(m_ui.tab,
	  SIGNAL(currentChanged(int)),
	  this,
	  SLOT(slot_tab_index_changed(int)));
  connect(m_ui.tab,
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slot_tab_close_requested(int)));
  s_settings["icon_set"] = "Snipicons";
  new_page(view);
}

dooble::dooble(void):dooble(0)
{
}

QVariant dooble::setting(const QString &key)
{
  return s_settings.value(key);
}

void dooble::new_page(dooble_web_engine_view *view)
{
  dooble_page *page = new dooble_page(view, m_ui.tab);

  connect(page,
	  SIGNAL(close_tab(void)),
	  this,
	  SLOT(slot_close_tab(void)));
  connect(page,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_tab(dooble_web_engine_view *)));
  connect(page,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_window(dooble_web_engine_view *)));
  connect(page,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)));
  connect(page,
	  SIGNAL(loadFinished(bool)),
	  m_ui.tab,
	  SLOT(slot_load_finished(void)));
  connect(page,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slot_load_finished(bool)));
  connect(page,
	  SIGNAL(loadStarted(void)),
	  m_ui.tab,
	  SLOT(slot_load_started(void)));
  connect(page,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
  connect(page,
	  SIGNAL(new_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)));
  connect(page,
	  SIGNAL(new_window(void)),
	  this,
	  SLOT(slot_new_window(void)));
  connect(page,
	  SIGNAL(quit_dooble(void)),
	  this,
	  SLOT(slot_quit_dooble(void)));
  connect(page,
	  SIGNAL(show_blocked_domains(void)),
	  this,
	  SLOT(slot_show_blocked_domains(void)));
  connect(page,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SLOT(slot_title_changed(const QString &)));
  m_ui.tab->addTab(page, tr("Dooble"));
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
}

void dooble::set_setting(const QString &key, const QVariant &value)
{
  if(key.trimmed().isEmpty() || value.isNull())
    return;

  s_settings[key.trimmed()] = value;
}

void dooble::show(void)
{
  QMainWindow::show();
}

void dooble::slot_close_tab(void)
{
  if(m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  m_ui.tab->removeTab(m_ui.tab->indexOf(page));
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  page->deleteLater();
}

void dooble::slot_create_tab(dooble_web_engine_view *view)
{
  new_page(view);
}

void dooble::slot_create_window(dooble_web_engine_view *view)
{
  dooble *d = new dooble(view);

  d->show();
}

void dooble::slot_icon_changed(const QIcon &icon)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  dooble_favicons::save_icon(icon, page->url());
  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), icon);
}

void dooble::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);
}

void dooble::slot_load_started(void)
{
}

void dooble::slot_new_tab(void)
{
  new_page(0);
}

void dooble::slot_new_window(void)
{
  dooble *d = new dooble();

  d->show();
}

void dooble::slot_quit_dooble(void)
{
  /*
  ** May require some confirmation from the user.
  */

  QApplication::exit(0);
}

void dooble::slot_show_blocked_domains(void)
{
}

void dooble::slot_tab_close_requested(int index)
{
  if(index < 0 || m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;

  page->deleteLater();
  m_ui.tab->removeTab(index);
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
}

void dooble::slot_tab_index_changed(int index)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;

  if(page->title().trimmed().isEmpty())
    setWindowTitle(tr("Dooble"));
  else
    setWindowTitle(tr("%1 - Dooble").arg(page->title().trimmed()));
}

void dooble::slot_title_changed(const QString &title)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  QString text(title.trimmed());

  if(text.isEmpty())
    text = page->url().toString().trimmed();

  if(text.isEmpty())
    {
      text = tr("Dooble");
      setWindowTitle(text);
    }
  else
    setWindowTitle(tr("%1 - Dooble").arg(text));

  m_ui.tab->setTabText(m_ui.tab->indexOf(page), text);
  m_ui.tab->setTabToolTip(m_ui.tab->indexOf(page), text);
}
