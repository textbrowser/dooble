#ifndef SIMPLEPLUGIN_EXTENSION_H
#define SIMPLEPLUGIN_EXTENSION_H

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

#include <QIcon>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QWidget>
#include <QtPlugin>

namespace simpleplugin
{
  class SignalAgent;

  class Extension
  {
  public:
    virtual ~Extension()
    {
    }

    virtual QString title(void) const = 0;         //  Identifies the plugin.
    virtual QPointer<SignalAgent> agent(void) = 0; //  Signals from the
                                                   //  extension.
    virtual QPointer<QWidget> widget(void) = 0;    //  The top display widget
                                                   //  for the extension.
    virtual QIcon icon(void) const = 0;
    virtual void run(void) = 0;
    virtual void quit(void) = 0;
    virtual bool running(void) = 0;
    virtual bool canRestart(void) = 0;
  };
} // Namespace.

Q_DECLARE_INTERFACE (simpleplugin::Extension, "net.sf.dooble/1.0")

#endif
