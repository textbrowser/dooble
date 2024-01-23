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

#include <QMimeData>
#include <QToolButton>
#include <QtMath>

#include "dooble.h"
#include "dooble_address_widget.h"
#include "dooble_address_widget_completer.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_history_window.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_view.h"

dooble_address_widget::dooble_address_widget(QWidget *parent):QLineEdit(parent)
{
  m_favorite = new QToolButton(this);
  m_favorite->setAutoRaise(true);
  m_favorite->setCursor(Qt::ArrowCursor);
  m_favorite->setEnabled(false);
  m_favorite->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_favorite->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-bottom: 0px;"
     "padding-top: 0px;"
     "}");
#endif
  m_favorite->setToolTip(tr("Empty or invalid URL."));
  m_completer = new dooble_address_widget_completer(this);
  m_information = new QToolButton(this);
  m_information->setAutoRaise(true);
  m_information->setCursor(Qt::ArrowCursor);
  m_information->setEnabled(false);
  m_information->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_information->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-bottom: 0px;"
     "padding-top: 0px;"
     "}");
#endif
  m_information->setToolTip(tr("Site Information (Cookies, etc.)"));
  m_menu = new QMenu(this);
  m_pull_down = new QToolButton(this);
  m_pull_down->setAutoRaise(true);
  m_pull_down->setCursor(Qt::ArrowCursor);
  m_pull_down->setIconSize(QSize(18, 18));
#ifdef Q_OS_MACOS
  m_pull_down->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-bottom: 0px;"
     "padding-top: 0px;"
     "}");
#endif
  m_pull_down->setToolTip(tr("Show History"));
  m_pull_down->setVisible
    (dooble_settings::setting("show_address_widget_completer").toBool() &&
     m_completer->completionMode() == QCompleter::UnfilteredPopupCompletion);
  m_view = nullptr;
  connect(dooble::s_application,
	  SIGNAL(favorites_cleared(void)),
	  this,
	  SLOT(slot_favorites_cleared(void)));
  connect(dooble::s_history,
	  SIGNAL(populated(const QListPairIconString &)),
	  this,
	  SLOT(slot_populate(const QListPairIconString &)));
  connect(dooble::s_history,
	  SIGNAL(populated_favorites(const QListVectorByteArray &)),
	  this,
	  SLOT(slot_favorites_populated(void)));
  connect(dooble::s_history_window,
	  SIGNAL(favorite_changed(const QUrl &, bool)),
	  this,
	  SLOT(slot_favorite_changed(const QUrl &, bool)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(m_favorite,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_favorite(void)));
  connect(m_information,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_show_site_information_menu(void)));
  connect(m_pull_down,
	  SIGNAL(clicked(void)),
	  this,
	  SIGNAL(pull_down_clicked(void)));
  connect(this,
	  SIGNAL(favorite_changed(const QUrl &, bool)),
	  dooble::s_history_window,
	  SLOT(slot_favorite_changed(const QUrl &, bool)));
  connect(this,
	  SIGNAL(favorite_changed(const QUrl &, bool)),
	  this,
	  SLOT(slot_favorite_changed(const QUrl &, bool)));
  connect(this,
	  SIGNAL(populated(void)),
	  dooble::s_application,
	  SIGNAL(address_widget_populated(void)));
  connect(this,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_return_pressed(void)));
  connect(this,
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_text_edited(const QString &)));
  prepare_icons();
  setCompleter(m_completer);

  auto font(this->font());

  font.setPointSize(font.pointSize() + 1.25);
  setFont(font);
  setMinimumHeight
    (qBound(0,
	    dooble_settings::getenv("DOOBLE_"
				    "ADDRESS_WIDGET_HEIGHT_OFFSET").toInt(),
	    50) +
     sizeHint().height());

  auto frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet
    (QString("QLineEdit {padding-left: %1px; padding-right: %2px;}").
     arg(m_favorite->sizeHint().width() +
	 m_information->sizeHint().width() +
	 frame_width +
	 5).
     arg(m_pull_down->sizeHint().width() + frame_width + 5));
}

QRect dooble_address_widget::information_rectangle(void) const
{
  return m_information->rect();
}

QSize dooble_address_widget::sizeHint(void) const
{
  auto size(QLineEdit::sizeHint());

  size.setHeight(size.height() + 5);
  return size;
}

bool dooble_address_widget::event(QEvent *event)
{
  if(event && event->type() == QEvent::KeyPress)
    {
      if(dynamic_cast<QKeyEvent *> (event)->key() == Qt::Key_Tab &&
	 m_completer->completionMode() == QCompleter::UnfilteredPopupCompletion)
	{
	  auto table_view = qobject_cast<QTableView *> (m_completer->popup());

	  if(table_view && table_view->isVisible())
	    {
	      event->accept();

	      int row = 0;

	      if(table_view->selectionModel()->
		 isRowSelected(table_view->currentIndex().row(),
			       table_view->rootIndex()))
		{
		  row = table_view->currentIndex().row() + 1;

		  if(row >= m_completer->model()->rowCount())
		    row = 0;
		}

	      table_view->selectRow(row);
	      return true;
	    }
	}
      else
	{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	  QKeySequence key_sequence
	    (dynamic_cast<QKeyEvent *> (event)->modifiers() +
	     Qt::Key(dynamic_cast<QKeyEvent *> (event)->key()));
#else
	  QKeySequence key_sequence
	    (dynamic_cast<QKeyEvent *> (event)->modifiers() |
	     Qt::Key(dynamic_cast<QKeyEvent *> (event)->key()));
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	  if(QKeySequence(Qt::ControlModifier + Qt::Key_L) == key_sequence)
#else
	  if(QKeySequence(Qt::ControlModifier | Qt::Key_L) == key_sequence)
#endif
	    {
	      selectAll();

	      if(isVisible())
		setFocus();
	    }
	}
    }

  return QLineEdit::event(event);
}

void dooble_address_widget::add_item(const QIcon &icon, const QUrl &url)
{
  m_completer->add_item(icon, url);
}

void dooble_address_widget::complete(void)
{
  m_completer->complete();
}

void dooble_address_widget::dropEvent(QDropEvent *event)
{
  if(event && event->mimeData())
    {
      QUrl url(event->mimeData()->text());

      if(!url.isEmpty() && url.isValid())
	{
	  setText(event->mimeData()->text());
	  emit load_page(url);
	}
    }
}

void dooble_address_widget::hide_popup(void)
{
  if(m_completer->completionMode() == QCompleter::UnfilteredPopupCompletion)
    m_completer->popup()->hide();
}

void dooble_address_widget::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      QKeySequence key_sequence(event->modifiers() + event->key());
#else
      QKeySequence key_sequence(event->modifiers() | event->key());
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      if(QKeySequence(Qt::ControlModifier + Qt::Key_L) == key_sequence)
#else
      if(QKeySequence(Qt::ControlModifier | Qt::Key_L) == key_sequence)
#endif
	{
	  selectAll();

	  if(isVisible())
	    setFocus();
	}
    }

  QLineEdit::keyPressEvent(event);
}

void dooble_address_widget::prepare_containers_for_url(const QUrl &url)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(url.isEmpty() || !url.isValid())
    {
      m_favorite->setEnabled(false);
      m_favorite->setIcon
	(QIcon::fromTheme(use_material_icons + "emblem-default",
			  QIcon(QString(":/%1/18/bookmark.png").
				arg(icon_set))));
      m_favorite->setToolTip(tr("Empty or invalid URL."));
      m_information->setEnabled(false);
      m_information->setIcon
	(QIcon::fromTheme(use_material_icons + "help-about",
			  QIcon(QString(":/%1/18/information.png").
				arg(icon_set))));
    }
  else
    {
      m_favorite->setEnabled(true);

      if(dooble::s_history->is_favorite(url))
	{
	  m_favorite->setIcon
	    (QIcon::fromTheme(use_material_icons + "emblem-favorite",
			      QIcon(QString(":/%1/18/bookmarked.png").
				    arg(icon_set))));
	  m_favorite->setToolTip(tr("Remove this page from Favorites."));
	}
      else
	{
	  m_favorite->setIcon
	    (QIcon::fromTheme(use_material_icons + "emblem-default",
			      QIcon(QString(":/%1/18/bookmark.png").
				    arg(icon_set))));
	  m_favorite->setToolTip(tr("Add this page to Favorites."));
	}

      m_information->setEnabled(true);
      m_information->setIcon
	(QIcon::fromTheme(use_material_icons + "help-about",
			  QIcon(QString(":/%1/18/information.png").
				arg(icon_set))));
    }
}

void dooble_address_widget::prepare_icons(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  m_favorite->setIcon
    (QIcon::fromTheme(use_material_icons + "emblem-default",
		      QIcon(QString(":/%1/18/bookmark.png").arg(icon_set))));
  m_information->setIcon
    (QIcon::fromTheme(use_material_icons + "help-about",
		      QIcon(QString(":/%1/18/information.png").arg(icon_set))));
  m_pull_down->setIcon
     (QIcon::fromTheme(use_material_icons + "go-down",
		       QIcon(QString(":/%1/18/pulldown.png").arg(icon_set))));
}

void dooble_address_widget::resizeEvent(QResizeEvent *event)
{
  auto frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  auto size1(m_favorite->sizeHint());
  auto size2(m_information->sizeHint());
  auto size3(m_pull_down->sizeHint());
  int d = 0;

  d = (rect().height() - (size1.height() - size1.height() % 2)) / 2;
  m_favorite->move(frame_width - rect().left() + size2.width() + 5,
		   rect().top() + d);
  d = (rect().height() - (size2.height() - size2.height() % 2)) / 2;
  m_information->move(frame_width - rect().left() + 5, rect().top() + d);
  d = (rect().height() - (size3.height() - size3.height() % 2)) / 2;
  m_pull_down->move
    (rect().right() - frame_width - size3.width() - 5, rect().top() + d);

  if(selectedText().isEmpty())
    setCursorPosition(0);

  QLineEdit::resizeEvent(event);
}

void dooble_address_widget::setText(const QString &text)
{
  QLineEdit::setText(text.trimmed());
  setCursorPosition(0);

  auto url(QUrl::fromUserInput(text));

  if(!url.isEmpty() && url.isValid())
    {
      QList<QTextLayout::FormatRange> formats;
      QTextCharFormat format;
      QTextLayout::FormatRange all_format_range;
      QTextLayout::FormatRange host_format_range;
      auto host(url.host());

      format.setFontHintingPreference(QFont::PreferFullHinting);
      format.setFontStyleStrategy(QFont::PreferAntialias);
      format.setFontWeight(QFont::Normal);
      format.setForeground(QColor("#2962ff"));
      all_format_range.format = format;
      all_format_range.length = url.toString().length();
      all_format_range.start = 0;
      format.setForeground(palette().color(QPalette::WindowText));
      host_format_range.format = format;
      host_format_range.length = host.length();
      host_format_range.start = url.toString().indexOf
	(host, url.scheme().length());
      formats << all_format_range;
      formats << host_format_range;
      set_text_format(formats);
    }

  prepare_containers_for_url(url);
  setToolTip(QLineEdit::text());
}

void dooble_address_widget::set_item_icon(const QIcon &icon, const QUrl &url)
{
  m_completer->set_item_icon(icon, url);
}

void dooble_address_widget::set_text_format
(const QList<QTextLayout::FormatRange> &formats)
{
  QList<QInputMethodEvent::Attribute> attributes;

  foreach(const auto format, formats)
    {
      auto attribute_type = QInputMethodEvent::TextFormat;
      auto length = format.length;
      auto start = format.start;
      const QVariant &value(format.format);

      attributes << QInputMethodEvent::Attribute(attribute_type,
						 start,
						 length,
						 value);
    }

  QInputMethodEvent event(QString(), attributes);

  QApplication::sendEvent(this, &event);
}

void dooble_address_widget::set_view(dooble_web_engine_view *view)
{
  m_view = view;
}

void dooble_address_widget::slot_favorite(void)
{
  if(!m_view)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dooble::s_history->save_favorite
    (m_view->url(), !dooble::s_history->is_favorite(m_view->url()));
  emit favorite_changed
    (m_view->url(), dooble::s_history->is_favorite(m_view->url()));
  QApplication::restoreOverrideCursor();
}

void dooble_address_widget::slot_favorite_changed(const QUrl &url, bool state)
{
  if(!m_view)
    return;

  if(m_view->url().isEmpty() || !m_view->url().isValid())
    return;

  if(m_view->url() == url)
    {
      auto icon_set(dooble_settings::setting("icon_set").toString());
      auto use_material_icons(dooble_settings::use_material_icons());

      if(state)
	{
	  m_favorite->setIcon
	    (QIcon::fromTheme(use_material_icons + "emblem-favorite",
			      QIcon(QString(":/%1/18/bookmarked.png").
				    arg(icon_set))));
	  m_favorite->setToolTip(tr("Remove this page from Favorites."));
	}
      else
	{
	  m_favorite->setIcon
	    (QIcon::fromTheme(use_material_icons + "emblem-default",
			      QIcon(QString(":/%1/18/bookmark.png").
				    arg(icon_set))));
	  m_favorite->setToolTip(tr("Add this page to Favorites."));
	}
    }
}

void dooble_address_widget::slot_favorites_cleared(void)
{
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  m_favorite->setIcon
    (QIcon::fromTheme(use_material_icons + "emblem-default",
		      QIcon(QString(":/%1/18/bookmark.png").arg(icon_set))));
}

void dooble_address_widget::slot_favorites_populated(void)
{
  if(m_view)
    prepare_containers_for_url(m_view->url());
}

void dooble_address_widget::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  if(m_view)
    prepare_containers_for_url(m_view->url());
}

void dooble_address_widget::slot_load_started(void)
{
  if(m_view)
    {
      auto url1(QUrl::fromUserInput(text()));
      auto url2(m_view->url());

      /*
      ** Prepare the containers for a future URL.
      */

      if(url1.host() == url2.host())
	prepare_containers_for_url(m_view->url());
      else
	prepare_containers_for_url(url1);
    }
}

void dooble_address_widget::slot_populate
(const QListPairIconString &list)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(const auto &i, list)
    m_completer->add_item(i.first, i.second);

  QApplication::restoreOverrideCursor();
  emit populated();
}

void dooble_address_widget::slot_return_pressed(void)
{
  if(m_completer->completionMode() == QCompleter::UnfilteredPopupCompletion)
    m_completer->popup()->setVisible(false);

  if(dooble_ui_utilities::allowed_url_scheme(QUrl(text().trimmed())))
    setText(text());
}

void dooble_address_widget::slot_settings_applied(void)
{
  m_pull_down->setVisible
    (dooble_settings::setting("show_address_widget_completer").toBool() &&
     m_completer->completionMode() == QCompleter::UnfilteredPopupCompletion);
  prepare_icons();

  if(m_view)
    prepare_containers_for_url(m_view->url());
}

void dooble_address_widget::slot_show_site_information_menu(void)
{
  if(!m_view)
    return;

  if(m_view->url().isEmpty() || !m_view->url().isValid())
    return;

  QMenu menu(this);
  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto url(dooble_ui_utilities::simplified_url(m_view->url()));
  auto use_material_icons(dooble_settings::use_material_icons());

  if(dooble_certificate_exceptions_menu_widget::has_exception(url))
    menu.addAction
      (QIcon(":/Miscellaneous/certificate_warning.png"),
       tr("Certificate exception accepted for this site..."),
       this,
       SIGNAL(show_certificate_exception(void)));

  menu.addAction
    (tr("Inject Custom Style Sheet..."),
     this,
     SIGNAL(inject_custom_css(void)));
  menu.addAction
    (tr("JavaScript Console..."),
     this,
     SIGNAL(javascript_console(void)));
  menu.addAction
    (QIcon::fromTheme(use_material_icons + "preferences-web-browser-cookies",
		      QIcon(QString(":/%1/48/cookies.png").arg(icon_set))),
     tr("Show Site Coo&kies..."),
     this,
     SIGNAL(show_site_cookies(void)));
  menu.exec(QCursor::pos());
}

void dooble_address_widget::slot_text_edited(const QString &text)
{
  Q_UNUSED(text);
  prepare_containers_for_url(QUrl());
  set_text_format(QList<QTextLayout::FormatRange> ());
}

void dooble_address_widget::slot_url_changed(const QUrl &url)
{
  if(!m_view)
    return;

  auto length = url.toString().length();

  if(length >
     static_cast<decltype(length)> (dooble::Limits::MAXIMUM_URL_LENGTH))
    return;

  auto icon_set(dooble_settings::setting("icon_set").toString());
  auto use_material_icons(dooble_settings::use_material_icons());

  if(dooble::s_history->is_favorite(m_view->url()))
    {
      m_favorite->setIcon
	(QIcon::fromTheme(use_material_icons + "emblem-favorite",
			  QIcon(QString(":/%1/18/bookmarked.png").
				arg(icon_set))));
      m_favorite->setToolTip(tr("Remove this page from Favorites."));
    }
  else
    {
      m_favorite->setIcon
	(QIcon::fromTheme(use_material_icons + "emblem-default",
			  QIcon(QString(":/%1/18/bookmark.png").
				arg(icon_set))));
      m_favorite->setToolTip(tr("Add this page to Favorites."));
    }

  prepare_containers_for_url(m_view->url());
}

void dooble_address_widget::slot_zoom_reset(void)
{
  slot_zoomed(1.0);
  emit zoom_reset();
}

void dooble_address_widget::slot_zoomed(qreal percent)
{
  Q_UNUSED(percent);
}
