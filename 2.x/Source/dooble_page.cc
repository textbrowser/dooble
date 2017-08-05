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

#include <QAuthenticator>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QStackedWidget>
#include <QWebEngineHistoryItem>
#include <QWidgetAction>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_label_widget.h"
#include "dooble_page.h"
#include "dooble_web_engine_view.h"
#include "ui_dooble_authentication_dialog.h"

dooble_page::dooble_page(dooble_web_engine_view *view, QWidget *parent):
  QWidget(parent)
{
  m_shortcuts_prepared = false;
  m_ui.setupUi(this);
  m_ui.backward->setEnabled(false);
  m_ui.backward->setMenu(new QMenu(this));
  m_ui.find_frame->setVisible(false);
  m_ui.forward->setEnabled(false);
  m_ui.forward->setMenu(new QMenu(this));
  m_ui.menus->setMenu(new QMenu(this));
  m_ui.progress->setVisible(false);

  if(view)
    {
      m_view = view;
      m_view->setParent(this);
    }
  else
    m_view = new dooble_web_engine_view(this);

  m_ui.frame->layout()->addWidget(m_view);
  connect(dooble::s_settings,
	  SIGNAL(credentials_created(void)),
	  this,
	  SLOT(slot_credentials_created(void)));
  connect(m_ui.address,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_load_page(void)));
  connect(m_ui.address,
	  SIGNAL(pull_down_clicked(void)),
	  this,
	  SLOT(slot_show_pull_down_menu(void)));
  connect(m_ui.authenticate,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_authenticate(void)));
  connect(m_ui.backward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_backward(void)));
  connect(m_ui.backward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_backward_menu(void)));
  connect(m_ui.find,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_find_next(void)));
  connect(m_ui.find,
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_find_text_edited(const QString &)));
  connect(m_ui.find_next,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_find_next(void)));
  connect(m_ui.find_previous,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_find_previous(void)));
  connect(m_ui.find_stop,
	  SIGNAL(clicked(void)),
	  m_ui.find_frame,
	  SLOT(hide(void)));
  connect(m_ui.forward,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_go_forward(void)));
  connect(m_ui.forward->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_prepare_forward_menu(void)));
  connect(m_ui.menus->menu(),
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_standard_menus(void)));
  connect(m_ui.menus,
	  SIGNAL(clicked(void)),
	  m_ui.menus,
	  SLOT(showMenu(void)));
  connect(m_ui.reload,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_reload_or_stop(void)));
  connect(m_view,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_tab(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SIGNAL(create_window(dooble_web_engine_view *)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SIGNAL(iconChanged(const QIcon &)));
  connect(m_view,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SIGNAL(loadFinished(bool)));
  connect(m_view,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slot_load_finished(bool)));
  connect(m_view,
	  SIGNAL(loadProgress(int)),
	  this,
	  SLOT(slot_load_progress(int)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SIGNAL(loadStarted(void)));
  connect(m_view,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)));
  connect(m_view,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SIGNAL(titleChanged(const QString &)));
  connect(m_view,
	  SIGNAL(urlChanged(const QUrl &)),
	  this,
	  SLOT(slot_url_changed(const QUrl &)));
  connect(m_view->page(),
	  SIGNAL(authenticationRequired(const QUrl &, QAuthenticator *)),
	  this,
	  SLOT(slot_authentication_required(const QUrl &, QAuthenticator *)));
  connect(m_view->page(),
	  SIGNAL(linkHovered(const QString &)),
	  this,
	  SLOT(slot_link_hovered(const QString &)));
  connect(m_view->page(),
	  SIGNAL(proxyAuthenticationRequired(const QUrl &,
					     QAuthenticator *,
					     const QString &)),
	  this,
	  SLOT(slot_proxy_authentication_required(const QUrl &,
						  QAuthenticator *,
						  const QString &)));
  prepare_icons();
  prepare_shortcuts();
  prepare_standard_menus();
  prepare_tool_buttons_for_mac();
  slot_credentials_created();
}

QIcon dooble_page::icon(void) const
{
  return m_view->icon();
}

QString dooble_page::title(void) const
{
  return m_view->title();
}

QUrl dooble_page::url(void) const
{
  return m_view->url();
}

QWebEngineSettings *dooble_page::web_engine_settings(void) const
{
  return m_view->settings();
}

dooble_web_engine_view *dooble_page::view(void) const
{
  return m_view;
}

void dooble_page::enable_web_setting(QWebEngineSettings::WebAttribute setting,
				     bool state)
{
  QWebEngineSettings *settings = m_view->settings();

  if(settings)
    settings->setAttribute(setting, state);
}

void dooble_page::find_text(QWebEnginePage::FindFlags find_flags,
			    const QString &text)
{
  m_view->findText
    (text,
     find_flags,
     [=] (bool found)
     {
       static QPalette palette(m_ui.find->palette());

       if(!found)
	 {
	   if(!text.isEmpty())
	     {
	       QColor color(240, 128, 128); // Light Coral
	       QPalette palette(m_ui.find->palette());

	       palette.setColor(m_ui.find->backgroundRole(), color);
	       m_ui.find->setPalette(palette);
	     }
	   else
	     m_ui.find->setPalette(palette);
	 }
       else
	 m_ui.find->setPalette(palette);
     });
}

void dooble_page::go_to_backward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::go_to_forward_item(int index)
{
  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::load_page(const QUrl &url)
{
  m_view->load(url);
}

void dooble_page::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.authenticate->setIcon
    (QIcon(QString(":/%1/32/authenticate.png").arg(icon_set)));
  m_ui.backward->setIcon(QIcon(QString(":/%1/32/backward.png").arg(icon_set)));
  m_ui.find_next->setIcon(QIcon(QString(":/%1/20/next.png").arg(icon_set)));
  m_ui.find_previous->setIcon
    (QIcon(QString(":/%1/20/previous.png").arg(icon_set)));
  m_ui.find_stop->setIcon(QIcon(QString(":/%1/20/stop.png").arg(icon_set)));
  m_ui.forward->setIcon(QIcon(QString(":/%1/32/forward.png").arg(icon_set)));
  m_ui.menus->setIcon(QIcon(QString(":/%1/32/menu.png").arg(icon_set)));
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/reload.png").arg(icon_set)));
}

void dooble_page::prepare_shortcuts(void)
{
#ifdef Q_OS_MACOS
  if(!m_shortcuts_prepared)
    {
      m_shortcuts_prepared = true;
      new QShortcut
	(QKeySequence(tr("Ctrl+F")), this, SLOT(slot_show_find(void)));
      new QShortcut
	(QKeySequence(tr("Ctrl+G")), this, SIGNAL(show_settings(void)));
      new QShortcut
	(QKeySequence(tr("Ctrl+L")), this, SLOT(slot_open_url(void)));
      new QShortcut(QKeySequence(tr("Ctrl+N")), this, SIGNAL(new_window(void)));
      new QShortcut(QKeySequence(tr("Ctrl+R")), m_view, SLOT(reload(void)));
      new QShortcut(QKeySequence(tr("Ctrl+T")), this, SIGNAL(new_tab(void)));
      new QShortcut(QKeySequence(tr("Ctrl+W")), this, SIGNAL(close_tab(void)));
      new QShortcut(QKeySequence(tr("Esc")), this, SLOT(slot_escape(void)));
    }
#else
  if(!m_shortcuts_prepared)
    {
      m_shortcuts_prepared = true;
      new QShortcut(QKeySequence(tr("Ctrl+R")), m_view, SLOT(reload(void)));
      new QShortcut(QKeySequence(tr("Esc")), this, SLOT(slot_escape(void)));
    }
#endif
}

void dooble_page::prepare_standard_menus(void)
{
  m_ui.menus->menu()->clear();

  QAction *action = 0;
  QMenu *menu = 0;
  QString icon_set(dooble_settings::setting("icon_set").toString());

  /*
  ** File Menu
  */

  menu = m_ui.menus->menu()->addMenu(tr("&File"));
  menu->addAction(tr("New &Tab"),
		  this,
		  SIGNAL(new_tab(void)),
		  QKeySequence(tr("Ctrl+T")));
  menu->addAction(tr("&New Window..."),
		  this,
		  SIGNAL(new_window(void)),
		  QKeySequence(tr("Ctrl+N")));
  menu->addAction(tr("&Open URL"),
		  this,
		  SLOT(slot_open_url(void)),
		  QKeySequence(tr("Ctrl+L")));
  menu->addSeparator();
  action = m_action_close_tab = menu->addAction(tr("&Close Tab"),
						this,
						SIGNAL(close_tab(void)),
						QKeySequence(tr("Ctrl+W")));

  if(qobject_cast<QStackedWidget *> (parentWidget()))
    action->setEnabled
      (qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);

  menu->addSeparator();
  menu->addAction(tr("E&xit Dooble"),
		  this,
		  SIGNAL(quit_dooble(void)),
		  QKeySequence(tr("Ctrl+Q")));

  /*
  ** Edit Menu
  */

  menu = m_ui.menus->menu()->addMenu(tr("&Edit"));
  menu->addAction(QIcon(QString(":/%1/16/find.png").arg(icon_set)),
		  tr("&Find"),
		  this,
		  SLOT(slot_show_find(void)),
		  QKeySequence(tr("Ctrl+F")));
  menu->addAction(QIcon(QString(":/%1/16/settings.png").arg(icon_set)),
		  tr("Settin&gs..."),
		  this,
		  SIGNAL(show_settings(void)),
		  QKeySequence(tr("Ctrl+G")));

  /*
  ** Tools Menu
  */

  menu = m_ui.menus->menu()->addMenu(tr("&Tools"));
  menu->addAction(tr("&Blocked Domains..."),
		  this,
		  SIGNAL(show_blocked_domains(void)));
}

void dooble_page::prepare_tool_buttons_for_mac(void)
{
#ifdef Q_OS_MACOS
  foreach(QToolButton *tool_button, findChildren<QToolButton *> ())
    if(m_ui.authenticate == tool_button ||
       m_ui.find_match_case == tool_button)
      {
      }
    else if(m_ui.backward == tool_button ||
       m_ui.forward == tool_button ||
       m_ui.menus == tool_button)
      tool_button->setStyleSheet
	("QToolButton {border: none; padding-right: 10px}"
	 "QToolButton::menu-button {border: none;}");
    else
      tool_button->setStyleSheet("QToolButton {border: none;}"
				 "QToolButton::menu-button {border: none;}");
#endif
}

void dooble_page::slot_about_to_show_standard_menus(void)
{
  if(m_action_close_tab)
    if(qobject_cast<QStackedWidget *> (parentWidget()))
      m_action_close_tab->setEnabled
	(qobject_cast<QStackedWidget *> (parentWidget())->count() > 1);
}

void dooble_page::slot_authenticate(void)
{
  if(!dooble_settings::has_dooble_credentials())
    emit show_settings_panel(dooble_settings::PRIVACY_PANEL);
  else
    {
      QInputDialog dialog(this);

      dialog.setLabelText(tr("Dooble Password"));
      dialog.setTextEchoMode(QLineEdit::Password);
      dialog.setWindowIcon(windowIcon());
      dialog.setWindowTitle(tr("Dooble: Password"));

      if(dialog.exec() != QDialog::Accepted)
	return;

      QString text = dialog.textValue();

      if(text.isEmpty())
	return;

      QByteArray salt
	(QByteArray::fromHex(dooble_settings::setting("authentication_salt").
			     toByteArray()));
      QByteArray salted_password
	(QByteArray::fromHex(dooble_settings::
			     setting("authentication_salted_password").
			     toByteArray()));
      int iteration_count = dooble_settings::setting
	("authentication_iteration_count").toInt();

      dooble::s_cryptography->authenticate(salt, salted_password, text);

      if(dooble::s_cryptography->authenticated())
	{
	  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	  dooble::s_cryptography->prepare_keys
	    (text.toUtf8(), salt, iteration_count);
	  QApplication::restoreOverrideCursor();
	  m_ui.authenticate->setEnabled(false);
	  m_ui.authenticate->setToolTip
	    (tr("Dooble credentials have been authenticated."));
	}
      else
	QMessageBox::critical
	  (this,
	   tr("Dooble: Error"),
	   tr("Unable to authenticate the provided password."));
    }
}

void dooble_page::slot_authentication_required(const QUrl &url,
					       QAuthenticator *authenticator)
{
  if(!authenticator || authenticator->isNull() || !url.isValid())
    return;

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);
  ui.label->setText(tr("The site %1 is requesting credentials.").
		    arg(url.toString().trimmed()));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    m_view->stop();
}

void dooble_page::slot_credentials_created(void)
{
  if(dooble::s_cryptography->authenticated())
    {
      m_ui.authenticate->setEnabled(false);
      m_ui.authenticate->setToolTip
	(tr("Dooble credentials have been authenticated."));
      return;
    }

  if(dooble_settings::has_dooble_credentials())
    m_ui.authenticate->setToolTip(tr("Dooble Credentials Authentication"));
  else
    m_ui.authenticate->setToolTip
      (tr("Please prepare Dooble's credentials via Settings -> Privacy. "
	  "Click this button to display the Settings window's "
	  "Privacy panel."));
}

void dooble_page::slot_escape(void)
{
  if(m_ui.find->hasFocus())
    m_ui.find_frame->setVisible(false);
  else
    {
      m_ui.address->setText(m_view->url().toString());
      m_view->stop();
    }
}

void dooble_page::slot_find_next(void)
{
  slot_find_text_edited(m_ui.find->text());
}

void dooble_page::slot_find_previous(void)
{
  QString text(m_ui.find->text());

  if(m_ui.find_match_case->isChecked())
    find_text
      (QWebEnginePage::FindFlag(QWebEnginePage::FindBackward |
				QWebEnginePage::FindCaseSensitively),
       text);
  else
    find_text(QWebEnginePage::FindBackward, text);
}

void dooble_page::slot_find_text_edited(const QString &text)
{
  if(m_ui.find_match_case->isChecked())
    find_text(QWebEnginePage::FindCaseSensitively, text);
  else
    find_text(QWebEnginePage::FindFlags(), text);
}

void dooble_page::slot_go_backward(void)
{
  m_view->history()->back();
}

void dooble_page::slot_go_forward(void)
{
  m_view->history()->forward();
}

void dooble_page::slot_go_to_backward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_backward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_forward_item(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    go_to_forward_item(action->property("index").toInt());
}

void dooble_page::slot_go_to_item(void)
{
  m_ui.address->menu()->hide();

  QLabel *label = qobject_cast<QLabel *> (sender());

  if(!label)
    return;

  QList<QWebEngineHistoryItem> items(m_view->history()->items());
  int index = label->property("index").toInt();

  if(index >= 0 && index < items.size())
    m_view->history()->goToItem(items.at(index));
}

void dooble_page::slot_icon_changed(const QIcon &icon)
{
  if(!m_ui.address->menu()->isVisible())
    return;

  dooble_web_engine_view *view = qobject_cast<dooble_web_engine_view *>
    (sender());

  if(!view)
    return;

  QList<QAction *> list(m_ui.address->menu()->actions());

  for(int i = 0; i < list.size(); i++)
    {
      QWidgetAction *widget_action = qobject_cast<QWidgetAction *> (list.at(i));

      if(!widget_action)
	continue;
      else if(view->url() != widget_action->property("url").toUrl())
	continue;

      QWidget *widget = widget_action->defaultWidget();

      if(!widget)
	break;

      QLabel *pixmap_label = widget->findChild<QLabel *> ("pixmap_label");

      if(pixmap_label)
	pixmap_label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

      break;
    }

  Q_UNUSED(icon);
}

void dooble_page::slot_link_hovered(const QString &url)
{
  QFontMetrics fm(m_ui.link_hovered->fontMetrics());

  m_ui.link_hovered->setText
    (fm.elidedText(url.trimmed(),
		   Qt::ElideMiddle,

		   /*
		   ** Always consider the progress bar's width.
		   */

		   qAbs(width() - m_ui.progress->width() - 20)));
}

void dooble_page::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.progress->setVisible(false);
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/reload.png").arg(icon_set)));
  m_ui.reload->setToolTip(tr("Reload"));
}

void dooble_page::slot_load_page(void)
{
  load_page(QUrl::fromUserInput(m_ui.address->text().trimmed()));
}

void dooble_page::slot_load_progress(int progress)
{
  m_ui.backward->setEnabled(m_view->history()->canGoBack());
  m_ui.forward->setEnabled(m_view->history()->canGoForward());
  m_ui.progress->setValue(progress);
}

void dooble_page::slot_load_started(void)
{
  emit iconChanged(QIcon());
  emit titleChanged("");

  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_ui.progress->setVisible(true);
  m_ui.reload->setIcon(QIcon(QString(":/%1/32/stop.png").arg(icon_set)));
  m_ui.reload->setToolTip(tr("Stop Page Load"));
}

void dooble_page::slot_open_url(void)
{
  m_ui.address->selectAll();
  m_ui.address->setFocus();
}

void dooble_page::slot_prepare_backward_menu(void)
{
  m_ui.backward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->backItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.backward->setEnabled(items.size() > 0);

  for(int i = items.size() - 1; i >= 0; i--)
    {
      QAction *action = 0;
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      action = m_ui.backward->menu()->addAction
	(title, this, SLOT(slot_go_to_backward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_prepare_forward_menu(void)
{
  m_ui.forward->menu()->clear();

  QList<QWebEngineHistoryItem> items
    (m_view->history()->forwardItems(MAXIMUM_HISTORY_ITEMS));

  m_ui.forward->setEnabled(items.size() > 0);

  for(int i = 0; i < items.size(); i++)
    {
      QAction *action = 0;
      QString title(items.at(i).title().trimmed());

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      action = m_ui.forward->menu()->addAction
	(title, this, SLOT(slot_go_to_forward_item(void)));
      action->setProperty("index", i);
    }
}

void dooble_page::slot_proxy_authentication_required
(const QUrl &url, QAuthenticator *authenticator, const QString &proxy_host)
{
  if(!authenticator ||
     authenticator->isNull() ||
     proxy_host.isEmpty() ||
     !url.isValid())
    return;

  QDialog dialog(this);
  Ui_dooble_authentication_dialog ui;

  ui.setupUi(&dialog);
  dialog.setWindowTitle(tr("Dooble: Proxy Authentication"));
  ui.label->setText(tr("The proxy %1 is requesting credentials.").
		    arg(proxy_host));
  dialog.resize(dialog.sizeHint());

  if(dialog.exec() == QDialog::Accepted)
    {
      authenticator->setPassword(ui.password->text());
      authenticator->setUser(ui.username->text());
    }
  else
    m_view->stop();
}

void dooble_page::slot_reload_or_stop(void)
{
  if(m_ui.progress->isVisible())
    m_view->stop();
  else
    m_view->reload();
}

void dooble_page::slot_show_find(void)
{
  m_ui.find->selectAll();
  m_ui.find->setFocus();
  m_ui.find_frame->setVisible(true);
}

void dooble_page::slot_show_pull_down_menu(void)
{
  m_ui.address->menu()->clear();

  QList<QWebEngineHistoryItem> items(m_view->history()->items());

  if(items.isEmpty())
    return;

  m_ui.address->menu()->setMaximumWidth(width());
  m_ui.address->menu()->setMinimumWidth(width());

  QFontMetrics fm(m_ui.address->menu()->fontMetrics());

  for(int i = 0; i < MAXIMUM_HISTORY_ITEMS && i < items.size(); i++)
    {
      QLabel *pixmap_label = 0;
      QString title(items.at(i).title().trimmed());
      QString url(items.at(i).url().toString().trimmed());
      QWidgetAction *widget_action = new QWidgetAction(m_ui.address->menu());
      dooble_label_widget *label = 0;

      if(title.isEmpty())
	title = items.at(i).url().toString().trimmed();

      widget_action->setProperty("url", items.at(i).url());

      QWidget *widget = new QWidget(m_ui.address->menu());
      QIcon icon(dooble_favicons::icon(url));
      QString text("<html>" +
		   title +
		   " - " +
		   QString("<font color='blue'><u>%1</u></font>").arg(url) +
		   "</html>");

      label = new dooble_label_widget(fm.elidedText(text,
						    Qt::ElideRight,
						    width() - 10),
				      m_ui.address->menu());
      label->setMargin(5);
      label->setMinimumHeight(fm.height() + 10);
      label->setProperty("index", i);
      pixmap_label = new QLabel(m_ui.address->menu());
      pixmap_label->setMaximumSize(QSize(16, 16));
      pixmap_label->setObjectName("pixmap_label");
      pixmap_label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
      widget->setLayout(new QHBoxLayout(widget));
      widget->layout()->addWidget(pixmap_label);
      widget->layout()->addWidget(label);
      widget->layout()->setContentsMargins(5, 0, 0, 0);
      widget->layout()->setSpacing(5);
      connect(label,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slot_go_to_item(void)));
      widget_action->setDefaultWidget(widget);
      m_ui.address->menu()->addAction(widget_action);
    }

  m_ui.address->menu()->popup(mapToGlobal(QPoint(0, m_ui.frame->pos().y())));
}

void dooble_page::slot_url_changed(const QUrl &url)
{
  m_ui.address->setText(url.toString());
}
