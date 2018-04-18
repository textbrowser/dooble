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

#include <QXmlStreamReader>

#include "dooble_text_utilities.h"

QString dooble_text_utilities::web_engine_page_feature_to_pretty_string
(QWebEnginePage::Feature feature)
{
  switch(feature)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    case QWebEnginePage::DesktopAudioVideoCapture:
      {
	return QObject::tr("Desktop Audio Video Capture");
      }
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    case QWebEnginePage::DesktopVideoCapture:
      {
	return QObject::tr("Desktop Video Capture");
      }
#endif
    case QWebEnginePage::Geolocation:
      {
	return QObject::tr("Geo Location");
      }
    case QWebEnginePage::MediaAudioCapture:
      {
	return QObject::tr("Media Audio Capture");
      }
    case QWebEnginePage::MediaAudioVideoCapture:
      {
	return QObject::tr("Media Audio Video Capture");
      }
    case QWebEnginePage::MediaVideoCapture:
      {
	return QObject::tr("Media Video Capture");
      }
    case QWebEnginePage::MouseLock:
      {
	return QObject::tr("Mouse Lock");
      }
    case QWebEnginePage::Notifications:
      {
	return QObject::tr("Notifications");
      }
    default:
      {
	return QObject::tr("Unknown Feature");
      }
    }
}

int dooble_text_utilities::visual_length_of_string(const QString &text)
{
  QXmlStreamReader xml_stream_reader(text);
  int length = 0;

  while(!xml_stream_reader.atEnd())
    if(xml_stream_reader.readNext() == QXmlStreamReader::Characters)
      length += xml_stream_reader.text().length();

  return length;
}
