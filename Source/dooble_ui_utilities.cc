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

#include <QApplication>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QDesktopWidget>
#endif
#include <QScreen>
#include <QWidget>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_ui_utilities.h"

QString dooble_ui_utilities::alignment_to_string(const Qt::Alignment alignment)
{
  switch(alignment)
    {
    case Qt::AlignAbsolute:
      {
	return QObject::tr("Absolute");
      }
    case Qt::AlignBaseline:
      {
	return QObject::tr("Baseline");
      }
    case Qt::AlignBottom:
      {
	return QObject::tr("Bottom");
      }
    case Qt::AlignCenter:
      {
	return QObject::tr("Center");
      }
    case Qt::AlignHCenter:
      {
	return QObject::tr("Horizontal Center");
      }
    case Qt::AlignJustify:
      {
	return QObject::tr("Justify");
      }
    case Qt::AlignLeft:
      {
	return QObject::tr("Left");
      }
    case Qt::AlignRight:
      {
	return QObject::tr("Right");
      }
    case Qt::AlignTop:
      {
	return QObject::tr("Top");
      }
    case Qt::AlignVCenter:
      {
	return QObject::tr("Vertical Center");
      }
    default:
      {
	return QObject::tr("None");
      }
    }
}

QString dooble_ui_utilities::orientation_to_string
(const Qt::Orientation orientation)
{
  switch(orientation)
    {
    case Qt::Horizontal:
      {
	return QObject::tr("Horizontal");
      }
    default:
      {
	return QObject::tr("Vertical");
      }
    }
}

QString dooble_ui_utilities::pretty_size(const qint64 size)
{
  if(size < 0)
    return QObject::tr("0 Bytes");

  if(size == 0)
    return QObject::tr("0 Bytes");
  else if(size == 1)
    return QObject::tr("1 Byte");
  else if(size < 1024)
    return QString(QObject::tr("%1 Bytes")).arg(size);
  else if(size < 1048576)
    return QString(QObject::tr("%1 KiB")).arg
      (QString::number(qRound(static_cast<double> (size) / 1024.0)));
  else if(size < 1073741824)
    return QString(QObject::tr("%1 MiB")).arg
      (QString::number(static_cast<double> (size) / 1048576.0, 'f', 1));
  return QString(QObject::tr("%1 GiB")).arg
    (QString::number(static_cast<double> (size) / 1073741824.0, 'f', 1));
}

QString dooble_ui_utilities::pretty_tool_tip(const QString &text)
{
  return "<html>" + text + "</html>";
}

QUrl dooble_ui_utilities::simplified_url(const QUrl &u)
{
  QUrl url;

  url.setHost(u.host());
  url.setScheme(u.scheme());
  return url;
}

Qt::Alignment dooble_ui_utilities::string_to_alignment(const QString &t)
{
  auto const text(t.trimmed());

  if(text == QObject::tr("Bottom"))
    return Qt::AlignBottom;
  else if(text == QObject::tr("Left"))
    return Qt::AlignLeft;
  else if(text == QObject::tr("Right"))
    return Qt::AlignRight;
  else if(text == QObject::tr("Top"))
    return Qt::AlignTop;
  else
    return Qt::AlignCenter;
}

Qt::Orientation dooble_ui_utilities::string_to_orientation(const QString &t)
{
  auto const text(t.trimmed());

  if(text == QObject::tr("Horizontal"))
    return Qt::Horizontal;
  else
    return Qt::Vertical;
}

bool dooble_ui_utilities::allowed_url_scheme(const QUrl &url)
{
  auto const scheme(url.scheme());

  return scheme == "file" ||
    scheme == "ftp" ||
    scheme == "gopher" ||
    scheme == "http" ||
    scheme == "https" ||
    scheme == "jar" ||
    scheme == "qrc";
}

bool dooble_ui_utilities::sort_by_row
(const QModelIndex &a, const QModelIndex &b)
{
  return a.row() < b.row();
}

dooble *dooble_ui_utilities::find_parent_dooble(QWidget *widget)
{
  if(!widget)
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto parent = widget->parentWidget();

  do
    {
      if(qobject_cast<dooble *> (parent))
	{
	  QApplication::restoreOverrideCursor();
	  return qobject_cast<dooble *> (parent);
	}
      else if(parent)
	parent = parent->parentWidget();
    }
  while(parent);

  QApplication::restoreOverrideCursor();
  return nullptr;
}

int dooble_ui_utilities::context_menu_width(QWidget *widget)
{
  if(!widget)
    return 250;

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  auto desktopWidget = QApplication::desktop();

  if(!desktopWidget)
    return 250;
  else
    return qMax(250, desktopWidget->screenGeometry(widget).width() / 4);
#else
  auto screen = QGuiApplication::screenAt(widget->pos());

  if(!screen)
    return 250;
  else
    return qMax(250, screen->geometry().width() / 4);
#endif
}

void dooble_ui_utilities::center_window_widget(QWidget *parent, QWidget *widget)
{
  if(!parent || !widget)
    return;

  auto geometry(widget->geometry());

  if(geometry.isNull())
    {
      widget->show();
      geometry = widget->geometry();
    }

  geometry.moveCenter(parent->geometry().center());
  widget->setGeometry(geometry);
}

void dooble_ui_utilities::enable_mac_brushed_metal(QWidget *widget)
{
  if(!widget)
    return;

#ifdef Q_OS_MACOS
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  widget->setAttribute
    (Qt::WA_MacBrushedMetal,
     dooble::s_application->style_name() == "macintosh");
#endif
#endif
}
