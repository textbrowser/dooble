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

#include <QApplication>
#include <QPainter>
#include <QPrinter>
#include <QUrl>
#include <QWebEnginePage>

class dprintfromcommandprompt: public QWebEnginePage
{
  Q_OBJECT

 public:
  dprintfromcommandprompt(const QUrl &url, const int index):QWebEnginePage()
  {
    connect(this,
	    SIGNAL(loadFinished(bool)),
	    this,
	    SLOT(slotLoadFinished(bool)));
    m_index = index;
    load(url);
  }

  static int s_count;

 private:
  int m_index;

 private slots:
  void slotLoadFinished(bool ok)
  {
    if(ok)
      {
	QPainter painter;
	QPrinter printer;

	painter.setRenderHints(QPainter::Antialiasing |
			       QPainter::NonCosmeticDefaultPen |
			       QPainter::HighQualityAntialiasing |
			       QPainter::SmoothPixmapTransform |
			       QPainter::TextAntialiasing);
	printer.setOutputFileName
	  (QString("dooble_print_%1.pdf").arg(m_index));
	printer.setOutputFormat(QPrinter::PdfFormat);
      }

    s_count -= 1;
    deleteLater();

    if(s_count <= 0)
      QApplication::instance()->exit(0);
  }
};
