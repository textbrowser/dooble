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

#include <QTranslator>

#include "dooble_application.h"
#include "dooble_settings.h"

dooble_application::dooble_application(int &argc, char **argv):
  QApplication(argc, argv)
{
  m_translator = 0;

  QFont font(this->font());

  font.setStyleStrategy
    (QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
  setAttribute(Qt::AA_DontUseNativeDialogs);
  setAttribute(Qt::AA_DontUseNativeMenuBar);
  setFont(font);
}

QString dooble_application::style_name(void) const
{
  static QString style_name
    (style() ? style()->objectName().toLower().trimmed() : "");

  return style_name;
}

void dooble_application::install_translator(void)
{
  if(m_translator)
    return;

  if(dooble_settings::setting("language_index").toInt() == 1)
    {
      m_translator = new QTranslator(this);
      m_translator->load("dooble_" + QLocale::system().name(), "Translations");
      installTranslator(m_translator);
    }
}
