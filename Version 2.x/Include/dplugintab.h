#ifndef DOOBLE_PLUGIN_TAB_H
#define DOOBLE_PLUGIN_TAB_H

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

#include <QWidget>

#include "plugin-spec/extension.h"
#include "plugin-spec/signal-agent.h"

using namespace simpleplugin;

class dplugintab: public QWidget
{
  Q_OBJECT

 public:
  dplugintab(Extension *ext, QWidget *parent = 0);
  ~dplugintab();
  Extension *extension(void) const;
  QAction *tabAction(void) const;
  QIcon icon(void) const;
  QString title(void) const;
  QWidget *pluginWidget(void) const;
  bool canClose(void) const;
  void setTabAction(QAction *action);

 public slots:
  void close(void);
  void slotIconChange(Extension *ext);
  void slotTitleChange(Extension *ext);

 signals:
  void exiting(dplugintab *dp, int status);
  void iconChange(QWidget *tab, const QIcon &icon);
  void titleChange(QWidget *tab, const QString &title);

 private slots:
  void emitExiting(Extension *ex, int status);

 protected:
  void resizeEvent(QResizeEvent *event);

 private:
  Extension *m_extension;
  QPointer<QAction> m_action;
  QWidget *extWidget;
  SignalAgent *sagent;
};

#endif
