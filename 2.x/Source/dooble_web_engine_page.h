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

#ifndef dooble_web_engine_page_h
#define dooble_web_engine_page_h

#include <QWebEnginePage>

#include "ui_dooble_certificate_exceptions_widget.h"

class dooble_web_engine_page: public QWebEnginePage
{
  Q_OBJECT

 public:
  dooble_web_engine_page(QWebEngineProfile *web_engine_profile,
			 bool is_private,
			 QWidget *parent);
  dooble_web_engine_page(QWidget *parent);
  ~dooble_web_engine_page();

 protected:
  bool acceptNavigationRequest(const QUrl &url,
			       NavigationType type,
			       bool isMainFrame);
  bool certificateError(const QWebEngineCertificateError &certificateError);

 private:
  QUrl m_certificate_error_url;
  QWidget *m_certificate_error_widget;
  Ui_dooble_certificate_exceptions_widget m_ui;
  bool m_is_private;

 private slots:
  void slot_certificate_exception_accepted(void);
  void slot_load_started(void);

 signals:
  void certificate_exception_accepted(const QUrl &url);
};

#endif
