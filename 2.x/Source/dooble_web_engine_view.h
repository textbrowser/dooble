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

#ifndef dooble_web_engine_view_h
#define dooble_web_engine_view_h

#include <QWebEngineView>

class dooble_web_engine_page;

class dooble_web_engine_view: public QWebEngineView
{
  Q_OBJECT

 public:
  dooble_web_engine_view(QWebEngineProfile *web_engine_profile,
			 QWidget *parent);
  ~dooble_web_engine_view();
  QWebEngineProfile *web_engine_profile(void) const;
  bool is_private(void) const;
  void download(const QString &file_name, const QUrl &url);
  void save(const QString &file_name);
  void set_feature_permission(const QUrl &security_origin,
			      QWebEnginePage::Feature feature,
			      QWebEnginePage::PermissionPolicy policy);

 protected:
  QSize sizeHint(void) const;
  dooble_web_engine_view *createWindow(QWebEnginePage::WebWindowType type);
  void contextMenuEvent(QContextMenuEvent *event);

 private:
  QList<dooble_web_engine_view *> m_dialog_requests;
  bool m_is_private;
  dooble_web_engine_page *m_page;

 private slots:
  void slot_accept_or_block_domain(void);
  void slot_certificate_exception_accepted(const QUrl &url);
  void slot_create_dialog_requests(void);
  void slot_load_progress(int progress);
  void slot_open_link_in_current_page(void);
  void slot_open_link_in_new_private_window(void);
  void slot_open_link_in_new_tab(void);
  void slot_open_link_in_new_window(void);
  void slot_search(void);
  void slot_settings_applied(void);

 signals:
  void create_dialog(dooble_web_engine_view *view);
  void create_dialog_request(dooble_web_engine_view *view);
  void create_tab(dooble_web_engine_view *view);
  void create_window(dooble_web_engine_view *view);
  void downloadRequested(QWebEngineDownloadItem *download);
  void featurePermissionRequestCanceled(const QUrl &security_origin,
					QWebEnginePage::Feature feature);
  void featurePermissionRequested(const QUrl &security_origin,
				  QWebEnginePage::Feature feature);
  void open_link_in_current_page(const QUrl &url);
  void open_link_in_new_private_window(const QUrl &url);
  void open_link_in_new_tab(const QUrl &url);
  void open_link_in_new_window(const QUrl &url);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  void printRequested(void);
#endif
  void windowCloseRequested(void);
};

#endif
