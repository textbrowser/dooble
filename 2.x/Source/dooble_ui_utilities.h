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

#ifndef dooble_ui_utilities_h
#define dooble_ui_utilities_h

class QWidget;
class dooble;

class dooble_ui_utilities
{
 public:
  static QString alignment_to_string(const Qt::Alignment alignment);
  static QString orientation_to_string(const Qt::Orientation orientation);
  static QString pretty_size(const qint64 size);
  static QString pretty_tool_tip(const QString &text);
  static QUrl simplified_url(const QUrl &u);
  static Qt::Alignment string_to_alignment(const QString &t);
  static Qt::Orientation string_to_orientation(const QString &t);
  static bool allowed_scheme(const QUrl &url);
  static dooble *find_parent_dooble(QWidget *widget);
  static int context_menu_width(QWidget *widget);
  static void center_window_widget(QWidget *parent, QWidget *widget);
  static void enable_mac_brushed_metal(QWidget *widget);

  static void memset(void *s, int c, size_t n)
  {
    if(!s)
      return;

    volatile unsigned char *v = (unsigned char *) s;

    while(n--)
      *v++ = (unsigned char) c;
  }

private:
  dooble_ui_utilities(void);
};

#endif
