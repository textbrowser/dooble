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
#include <QTranslator>

#include "dooble.h"
#include "dooble_application.h"

QHash<QString, QColor> dooble_application::s_theme_colors;

dooble_application::dooble_application(int &argc, char **argv):
  QApplication(argc, argv)
{
  m_application_locked = false;
  m_default_font = QApplication::font();

  auto const string
    (dooble_settings::setting("display_application_font").toString().trimmed());
  auto font(this->font());

  if(string.isEmpty() || !font.fromString(string))
    font = QApplication::font();

  font.setHintingPreference(QFont::PreferFullHinting);
  font.setStyleStrategy
    (QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
  prepare_theme_colors();
  setAttribute(Qt::AA_DontUseNativeMenuBar);
  setFont(font);
  setWindowIcon(QIcon(":/Logo/dooble.png"));
}

QFont dooble_application::default_font(void) const
{
  return m_default_font;
}

QString dooble_application::style_name(void) const
{
  static auto const style_name
    (style() ? style()->objectName().toLower().trimmed() : "");

  return style_name;
}

bool dooble_application::application_locked(void) const
{
  return m_application_locked;
}

void dooble_application::install_translator(void)
{
  if(m_translator)
    return;

  if(dooble_settings::setting("language_index").toInt() == 1) // System
    {
      QString path("");
      auto const variable
	(qEnvironmentVariable("DOOBLE_TRANSLATIONS_PATH").trimmed());

      if(!variable.isEmpty())
	path = variable;

      if(path.isEmpty())
	path = QDir::currentPath() + QDir::separator() + "Translations";

      auto const absolute_path(QFileInfo(path).absoluteFilePath());

      qDebug() << tr("Dooble will search the directory %1 for translation "
		     "files.").arg(absolute_path);
      m_translator = new QTranslator(this);

      if(m_translator->load(QLocale(), "dooble", "_", absolute_path, ".qm"))
	{
	  if(!installTranslator(m_translator))
	    qDebug() << tr("Translator m_translator was not installed.");
	}
      else
	qDebug() << tr("Could not load a Dooble translation file.");

      auto other = new QTranslator(this);

      if(other->load(QLocale(), "qtbase", "_", absolute_path, ".qm"))
	{
	  if(!installTranslator(other))
	    qDebug() << tr("Translator other was not installed.");
	}
      else
	qDebug() << tr("Could not load a Qt translation file.");
    }
}

void dooble_application::prepare_theme_colors(void)
{
  if(!s_theme_colors.isEmpty())
    return;

  s_theme_colors["blue-grey-corner-widget-background-color"] = "#90a4ae";
  s_theme_colors["blue-grey-hovered-tab-color"] = "#c5cae9";
  s_theme_colors["blue-grey-menubar-text-color"] = "white";
  s_theme_colors["blue-grey-not-selected-tab-text-color"] = "black";
  s_theme_colors["blue-grey-selected-tab-color"] = "#7986cb";
  s_theme_colors["blue-grey-status-bar-text-color"] = "white";
  s_theme_colors["blue-grey-tabbar-background-color"] = "#90a4ae";
  s_theme_colors["dark-corner-widget-background-color"] = "#424242";
  s_theme_colors["dark-hovered-tab-color"] = "#616161";
  s_theme_colors["dark-menubar-text-color"] = "white";
  s_theme_colors["dark-not-selected-tab-text-color"] = "white";
  s_theme_colors["dark-selected-tab-color"] = "#757575";
  s_theme_colors["dark-status-bar-text-color"] = "white";
  s_theme_colors["dark-tabbar-background-color"] = "#424242";
  s_theme_colors["indigo-corner-widget-background-color"] = "#5c6bc0";
  s_theme_colors["indigo-hovered-tab-color"] = "#b388ff";
  s_theme_colors["indigo-menubar-text-color"] = "white";
  s_theme_colors["indigo-not-selected-tab-text-color"] = "white";
  s_theme_colors["indigo-selected-tab-color"] = "#536dfe";
  s_theme_colors["indigo-status-bar-text-color"] = "white";
  s_theme_colors["indigo-tabbar-background-color"] = "#5c6bc0";
  s_theme_colors["orange-corner-widget-background-color"] = "#e65100";
  s_theme_colors["orange-hovered-tab-color"] = "white";
  s_theme_colors["orange-menubar-text-color"] = "white";
  s_theme_colors["orange-not-selected-tab-text-color"] = "#e65100";
  s_theme_colors["orange-selected-tab-color"] = "#616161";
  s_theme_colors["orange-status-bar-text-color"] = "white";
  s_theme_colors["orange-tabbar-background-color"] = "#e65100";
}

void dooble_application::set_application_locked(bool state)
{
  m_application_locked = state;
}

void dooble_application::slot_application_locked(bool state, dooble *d)
{
  m_application_locked = true;
  emit application_locked(state, d);
}
