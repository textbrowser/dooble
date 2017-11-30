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

#include <QKeyEvent>
#include <QMenu>
#include <QToolButton>

#include "dooble.h"
#include "dooble_address_widget.h"
#include "dooble_address_widget_completer.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_ui_utilities.h"

dooble_address_widget::dooble_address_widget(QWidget *parent):QLineEdit(parent)
{
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  m_favorite = new QToolButton(this);
  m_favorite->setCursor(Qt::ArrowCursor);
  m_favorite->setEnabled(false);
  m_favorite->setIconSize(QSize(16, 16));
  m_favorite->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-top: 0px;"
     "padding-bottom: 0px;"
     "}");
  m_favorite->setToolTip(tr("Favorite"));
  m_completer = new dooble_address_widget_completer(this);
  m_information = new QToolButton(this);
  m_information->setCursor(Qt::ArrowCursor);
  m_information->setEnabled(false);
  m_information->setIconSize(QSize(16, 16));
  m_information->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-top: 0px;"
     "padding-bottom: 0px;"
     "}");
  m_information->setToolTip(tr("Site Information"));
  m_menu = new QMenu(this);
  m_pull_down = new QToolButton(this);
  m_pull_down->setCursor(Qt::ArrowCursor);
  m_pull_down->setIconSize(QSize(16, 16));
  m_pull_down->setStyleSheet
    ("QToolButton {"
     "border: none;"
     "padding-top: 0px;"
     "padding-bottom: 0px;"
     "}");
  m_pull_down->setToolTip(tr("Show History"));
  connect(dooble::s_application,
	  SIGNAL(favorites_cleared(void)),
	  this,
	  SLOT(slot_favorites_cleared(void)));
  connect(dooble::s_history,
	  SIGNAL(populated(const QListPairIconString &)),
	  this,
	  SLOT(slot_populate(const QListPairIconString &)));
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
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_text_edited(const QString &)));
  prepare_icons();
  setCompleter(m_completer);
  setMinimumHeight(sizeHint().height());
  setStyleSheet
    (QString("QLineEdit {padding-left: %1px; padding-right: %2px;}").
     arg(m_favorite->sizeHint().width() +
	 m_information->sizeHint().width() +
	 frame_width + 10).
     arg(m_pull_down->sizeHint().width() + frame_width + 10));
}

QRect dooble_address_widget::information_rectangle(void) const
{
  return m_information->rect();
}

bool dooble_address_widget::event(QEvent *event)
{
  if(event && event->type() == QEvent::KeyPress)
    {
      if(static_cast<QKeyEvent *> (event)->key() == Qt::Key_Escape)
	{
	  emit reset_url();
	  prepare_containers_for_url(m_url);
	}
      else if(static_cast<QKeyEvent *> (event)->key() == Qt::Key_Tab)
	{
	  QTableView *table_view = qobject_cast<QTableView *>
	    (m_completer->popup());

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
	  QKeySequence key_sequence
	    (static_cast<QKeyEvent *> (event)->modifiers() +
	     Qt::Key(static_cast<QKeyEvent *> (event)->key()));

	  if(QKeySequence(Qt::ControlModifier + Qt::Key_L) == key_sequence)
	    {
	      selectAll();
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

void dooble_address_widget::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    {
      emit reset_url();
      prepare_containers_for_url(m_url);
    }
  else if(event)
    {
      QKeySequence key_sequence(event->modifiers() + event->key());

      if(QKeySequence(Qt::ControlModifier + Qt::Key_L) == key_sequence)
	{
	  selectAll();
	  setFocus();
	}
    }

  QLineEdit::keyPressEvent(event);
}

void dooble_address_widget::prepare_containers_for_url(const QUrl &url)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  if(url.isEmpty() || !url.isValid())
    {
      m_favorite->setEnabled(false);
      m_favorite->setIcon(QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));
      m_information->setEnabled(false);
      m_information->setIcon
	(QIcon(QString(":/%1/18/information.png").arg(icon_set)));
    }
  else
    {
      m_favorite->setEnabled(true);

      if(dooble::s_history->is_favorite(url))
	m_favorite->setIcon
	  (QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
      else
	m_favorite->setIcon
	  (QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));

      m_information->setEnabled(true);
      m_information->setIcon
	(QIcon(QString(":/%1/18/information.png").arg(icon_set)));
    }
}

void dooble_address_widget::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_favorite->setIcon(QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));
  m_information->setIcon
    (QIcon(QString(":/%1/18/information.png").arg(icon_set)));
  m_pull_down->setIcon(QIcon(QString(":/%1/18/pulldown.png").arg(icon_set)));
}

void dooble_address_widget::resizeEvent(QResizeEvent *event)
{
  QSize size1 = m_favorite->sizeHint();
  QSize size2 = m_information->sizeHint();
  QSize size3 = m_pull_down->sizeHint();
  int d = 0;
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  d = (rect().height() - size1.height()) / 2;
  m_favorite->move(frame_width - rect().left() + size2.width() + 5,
		   rect().top() + d);
  d = (rect().height() - size2.height()) / 2;
  m_information->move(frame_width - rect().left() + 5, rect().top() + d);
  d = (rect().height() - size3.height()) / 2;
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

  QUrl url(QUrl::fromUserInput(text));

  if(!url.isEmpty() && !url.isLocalFile() && url.isValid())
    {
      QList<QTextLayout::FormatRange> formats;
      QString host(url.host());
      QString path
	(url.toString().mid(host.length() + url.toString().indexOf(host)));
      QTextCharFormat format;
      QTextLayout::FormatRange host_format_range;
      QTextLayout::FormatRange path_format_range;
      QTextLayout::FormatRange scheme_format_range;

      format.setFontStyleStrategy(QFont::PreferAntialias);
      format.setFontWeight(QFont::Normal);
      host_format_range.format = format;
      host_format_range.length = host.length();
      host_format_range.start = url.toString().indexOf(host);
      format.setForeground(QColor("#2962FF"));
      path_format_range.format = format;
      path_format_range.length = path.length();
      path_format_range.start =
	url.toString().indexOf(path, url.toString().indexOf(host));
      format.setForeground(QColor("#2962FF"));
      scheme_format_range.format = format;
      scheme_format_range.length = url.toString().indexOf(host);
      scheme_format_range.start = 0;
      formats << host_format_range;
      formats << path_format_range;
      formats << scheme_format_range;
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

  for(int i = 0; i < formats.size(); i++)
    {
      QInputMethodEvent::AttributeType
	attribute_type = QInputMethodEvent::TextFormat;
      QTextLayout::FormatRange format_range = formats.at(i);
      QVariant value = format_range.format;
      int start = format_range.start;
      int length = format_range.length;

      attributes << QInputMethodEvent::Attribute(attribute_type,
						 start,
						 length,
						 value);
    }

  QInputMethodEvent event(QInputMethodEvent(QString(), attributes));

  QApplication::sendEvent(this, &event);
}

void dooble_address_widget::slot_favorite(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  dooble::s_history->save_favorite
    (m_url, !dooble::s_history->is_favorite(m_url));
  emit favorite_changed(m_url, dooble::s_history->is_favorite(m_url));
  QApplication::restoreOverrideCursor();
}

void dooble_address_widget::slot_favorite_changed(const QUrl &url, bool state)
{
  if(m_url.isEmpty() || !m_url.isValid())
    return;

  if(m_url == url)
    {
      QString icon_set(dooble_settings::setting("icon_set").toString());

      if(state)
	m_favorite->setIcon
	  (QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
      else
	m_favorite->setIcon
	  (QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));
    }
}

void dooble_address_widget::slot_favorites_cleared(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_favorite->setIcon(QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));
}

void dooble_address_widget::slot_load_started(void)
{
  m_url = QUrl();
  prepare_containers_for_url(m_url);
}

void dooble_address_widget::slot_populate
(const QListPairIconString &list)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  for(int i = 0; i < list.size(); i++)
    m_completer->add_item(list.at(i).first, list.at(i).second);

  QApplication::restoreOverrideCursor();
}

void dooble_address_widget::slot_settings_applied(void)
{
  prepare_icons();
}

void dooble_address_widget::slot_show_site_information_menu(void)
{
  if(m_url.isEmpty() || !m_url.isValid())
    return;

  QMenu menu(this);
  QUrl url(dooble_ui_utilities::simplified_url(m_url));

  if(dooble_certificate_exceptions_menu_widget::has_exception(url))
    menu.addAction
      (QIcon(":/Miscellaneous/certificate_warning.png"),
       tr("Certificate exception accepted for this site..."),
       this,
       SIGNAL(show_certificate_exception(void)));

  menu.addAction(tr("Show Site Coo&kies..."), this, SIGNAL(show_cookies(void)));
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
  if(url.toString().length() > dooble::MAXIMUM_URL_LENGTH)
    return;

  m_url = url;

  QString icon_set(dooble_settings::setting("icon_set").toString());

  if(dooble::s_history->is_favorite(m_url))
    m_favorite->setIcon(QIcon(QString(":/%1/18/bookmarked.png").arg(icon_set)));
  else
    m_favorite->setIcon(QIcon(QString(":/%1/18/bookmark.png").arg(icon_set)));

  prepare_containers_for_url(m_url);
}
