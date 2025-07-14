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

#include <QPointer>
#include <QShortcut>
#include <QTimer>
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
#include <QWebEnginePage>
#else
#include <QWebEnginePermission>
#endif
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
  bool is_pinned(void) const;
  bool is_private(void) const;
  void download(const QString &file_name, const QUrl &url);
  void save(const QString &file_name);
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
  void set_feature_permission(const QUrl &security_origin,
			      const QWebEnginePage::Feature feature,
			      const QWebEnginePage::PermissionPolicy policy);
#else
  void set_feature_permission
    (const QUrl &security_origin,
     const QWebEnginePermission::PermissionType feature,
     const QWebEnginePermission::State policy);
#endif
  void set_pinned(bool state);

 protected:
  QSize sizeHint(void) const;
  QWebEngineView *createWindow(QWebEnginePage::WebWindowType type);
  void contextMenuEvent(QContextMenuEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QList<dooble_web_engine_view *> m_dialog_requests;
  QPointer<QShortcut> m_scroll_down;
  QPointer<QShortcut> m_scroll_up;
  QTimer m_dialog_requests_timer;
  bool m_is_pinned;
  bool m_is_private;
  dooble_web_engine_page *m_page;
  void prepare_shortcuts(void);
  void scroll(const qreal value);

 private slots:
  void slot_accept_or_block_domain(void);
  void slot_certificate_exception_accepted(const QUrl &url);
  void slot_create_dialog_requests(void);
  void slot_load_progress(int progress);
  void slot_load_started(void);
  void slot_open_link_in_current_page(void);
  void slot_open_link_in_new_private_window(void);
  void slot_open_link_in_new_tab(void);
  void slot_open_link_in_new_window(void);
  void slot_peekaboo(void);
  void slot_scroll_down(void);
  void slot_scroll_up(void);
  void slot_search(void);
  void slot_settings_applied(void);

 signals:
  void create_dialog(dooble_web_engine_view *view);
  void create_dialog_request(dooble_web_engine_view *view);
  void create_tab(dooble_web_engine_view *view);
  void create_window(dooble_web_engine_view *view);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  void downloadRequested(QWebEngineDownloadItem *download);
#else
  void downloadRequested(QWebEngineDownloadRequest *download);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
  void featurePermissionRequestCanceled(const QUrl &security_origin,
					QWebEnginePage::Feature feature);
  void featurePermissionRequested(const QUrl &security_origin,
				  QWebEnginePage::Feature feature);
#else
  void permissionRequested(QWebEnginePermission permission);
#endif
  void open_link_in_current_page(const QUrl &url);
  void open_link_in_new_private_window(const QUrl &url);
  void open_link_in_new_tab(const QUrl &url);
  void open_link_in_new_window(const QUrl &url);
  void peekaboo_text(const QString &text);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  void printRequested(void);
#endif
  void show_full_screen(bool state);
  void windowCloseRequested(void);
};

#endif
