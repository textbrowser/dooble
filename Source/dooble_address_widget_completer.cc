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

#include <QPainter>
#include <QStandardItem>
#include <QStyledItemDelegate>

#include "dooble.h"
#include "dooble_address_widget_completer.h"
#include "dooble_address_widget_completer_popup.h"
#include "dooble_application.h"
#include "dooble_favicons.h"
#include "dooble_page.h"

class dooble_address_widget_completer_popup_item_delegate:
  public QStyledItemDelegate
{
public:
  dooble_address_widget_completer_popup_item_delegate(QObject *parent):
    QStyledItemDelegate(parent)
  {
  }

protected:
  void paint(QPainter *painter,
	     const QStyleOptionViewItem &option,
	     const QModelIndex &index) const
  {
    QStyledItemDelegate::paint(painter, option, index);
  }
};

QHash<QUrl, QStandardItem *> dooble_address_widget_completer::s_urls;
QStandardItemModel *dooble_address_widget_completer::s_model = nullptr;

dooble_address_widget_completer::dooble_address_widget_completer
(QWidget *parent):QCompleter(parent)
{
  if(!s_model)
    s_model = new QStandardItemModel();

  m_model = new QStandardItemModel(nullptr);
  m_popup = new dooble_address_widget_completer_popup(parent);
  m_popup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_popup->horizontalHeader()->setVisible(false);
  m_popup->setItemDelegateForColumn
    (0, new dooble_address_widget_completer_popup_item_delegate(this));
  m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
  m_popup->verticalHeader()->setVisible(false);
  m_text_edited_timer.setSingleShot(true);
  m_text_edited_timer.setInterval(150);
  connect(&m_text_edited_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_text_edited_timeout(void)));
  connect(dooble::s_application,
	  SIGNAL(history_cleared(void)),
	  this,
	  SLOT(slot_history_cleared(void)));
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  connect(qobject_cast<dooble_address_widget *> (parent),
	  SIGNAL(returnPressed(void)),
	  &m_text_edited_timer,
	  SLOT(stop(void)));
  connect(qobject_cast<dooble_address_widget *> (parent),
	  SIGNAL(textEdited(const QString &)),
	  &m_text_edited_timer,
	  SLOT(start(void)));
  connect(m_popup,
	  SIGNAL(clicked(const QModelIndex &)),
	  this,
	  SLOT(slot_clicked(const QModelIndex &)));
  setCaseSensitivity(Qt::CaseInsensitive);
  setCompletionColumn(0);
  setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  setFilterMode(Qt::MatchContains);
  setModelSorting(QCompleter::UnsortedModel);
  setPopup(m_popup);
  setWrapAround(false);
  slot_settings_applied();
}

dooble_address_widget_completer::~dooble_address_widget_completer()
{
  m_model->deleteLater();
}

int dooble_address_widget_completer::levenshtein_distance
(const QString &str1, const QString &str2) const
{
  if(str1.isEmpty())
    return str2.length();
  else if(str2.isEmpty())
    return str1.length();

  QChar str1_c = QChar(0);
  QChar str2_c = QChar(0);
  QVector<QVector<int> > matrix(str1.length() + 1,
				QVector<int> (str2.length() + 1));
  int cost = 0;

  for(int i = 0; i <= str1.length(); ++i)
    matrix[i][0] = i;

  for(int i = 0; i <= str2.length(); ++i)
    matrix[0][i] = i;

  for(int i = 1; i <= str1.length(); ++i)
    {
      str1_c = str1.at(i - 1);

      for(int j = 1; j <= str2.length(); ++j)
	{
	  str2_c = str2.at(j - 1);

	  if(str1_c == str2_c)
	    cost = 0;
	  else
	    cost = 1;

	  matrix[i][j] = qMin(qMin(matrix[i - 1][j] + 1,
				   matrix[i][j - 1] + 1),
			      matrix[i - 1][j - 1] + cost);
	}
    }

  return matrix[str1.length()][str2.length()];
}

void dooble_address_widget_completer::add_item(const QIcon &icon,
					       const QUrl &url)
{
  if(url.isEmpty() || !url.isValid())
    return;

  /*
  ** Prevent duplicates.
  */

  if(s_urls.contains(url))
    if(s_urls.value(url))
      {
	s_model->takeRow(s_urls.value(url)->row());
	s_model->insertRow(0, s_urls.value(url));
	return;
      }

  auto item = new QStandardItem(icon, url.toString());

  item->setToolTip(url.toString());
  s_model->insertRow(0, item);
  s_urls[url] = item;
}

void dooble_address_widget_completer::complete(const QRect &rect)
{
  Q_UNUSED(rect);

  if(dooble_settings::setting("show_address_widget_completer").toBool())
    complete("");
}

void dooble_address_widget_completer::complete(const QString &text)
{
  if(!dooble_settings::setting("show_address_widget_completer").toBool())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QStandardItem *> list;

  if(text.trimmed().isEmpty())
    {
      auto j = qMin(s_model->rowCount(),
		    2 * static_cast<int> (dooble_page::ConstantsEnum::
					  MAXIMUM_HISTORY_ITEMS));

      for(int i = 0; i < j; ++i)
	if(s_model->item(i, 0))
	  {
	    if(s_model->item(i, 0)->icon().isNull())
	      s_model->item(i, 0)->setIcon
		(dooble_favicons::icon_from_host(s_model->item(i, 0)->text()));

	    list << s_model->item(i, 0);
	  }
    }
  else
    {
      QMultiMap<int, QStandardItem *> map;
      auto c(text.toLower().trimmed());

      for(int i = 0; i < s_model->rowCount(); ++i)
	if(s_model->item(i, 0))
	  if(s_model->item(i, 0)->text().toLower().contains(c))
	    map.insert
	      (levenshtein_distance(c, s_model->item(i, 0)->text().toLower()),
	       s_model->item(i, 0));

      list << map.values().mid
	(0, 2 * static_cast<int> (dooble_page::ConstantsEnum::
				  MAXIMUM_HISTORY_ITEMS));
    }

  m_model->clear();

  foreach(auto item, list)
    if(item)
      {
	if(item->icon().isNull())
	  item->setIcon(dooble_favicons::icon_from_host(item->text()));

	m_model->setRowCount(m_model->rowCount() + 1);
	m_model->setItem(m_model->rowCount() - 1, item->clone());

	if(completionMode() == QCompleter::UnfilteredPopupCompletion)
	  m_popup->setRowHeight(m_model->rowCount() - 1, 36);
      }

  if(m_model->rowCount() > 0)
    {
      if(completionMode() == QCompleter::UnfilteredPopupCompletion)
	{
	  auto height = 2 * m_popup->frameWidth() +
	    m_popup->horizontalHeader()->height() +
	    m_popup->rowHeight(0) *
	    qMin(m_model->rowCount(),
		 static_cast<int> (dooble_page::
				   ConstantsEnum::
				   MAXIMUM_HISTORY_ITEMS));

	  m_popup->setMaximumHeight(height);
	  m_popup->setMinimumHeight(height);
	  setMaxVisibleItems(m_model->rowCount());
	  QCompleter::complete();
	  m_popup->setCurrentIndex(QModelIndex());
	}
      else
	{
	  setMaxVisibleItems(m_model->rowCount());
	  QCompleter::complete();
	}
    }
  else if(completionMode() == QCompleter::UnfilteredPopupCompletion)
    m_popup->setVisible(false);

  QApplication::restoreOverrideCursor();
}

void dooble_address_widget_completer::remove_item(const QUrl &url)
{
  auto list(s_model->findItems(url.toString()));

  if(!list.isEmpty())
    if(list.at(0))
      s_model->removeRow(list.at(0)->row());

  s_urls.remove(url);
}

void dooble_address_widget_completer::set_item_icon(const QIcon &icon,
						    const QUrl &url)
{
  auto list(s_model->findItems(url.toString()));

  if(!list.isEmpty())
    if(list.at(0))
      {
	if(icon.isNull())
	  list.at(0)->setIcon(dooble_favicons::icon_from_host(url));
	else
	  list.at(0)->setIcon(icon);
      }
}

void dooble_address_widget_completer::slot_clicked(const QModelIndex &index)
{
  qobject_cast<dooble_address_widget *> (widget())->
    setText(index.data().toString());
  QApplication::postEvent
    (widget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier));
}

void dooble_address_widget_completer::slot_history_cleared(void)
{
  m_model->clear();
  s_model->clear();
  s_urls.clear();
}

void dooble_address_widget_completer::slot_settings_applied(void)
{
  if(dooble_settings::setting("show_address_widget_completer").toBool())
    {
      if(m_model != model())
	{
	  auto mode = dooble_settings::setting
	    ("address_widget_completer_mode_index").toInt() == 0 ?
	    QCompleter::InlineCompletion :
	    QCompleter::UnfilteredPopupCompletion;

	  setCompletionMode(mode);
	  setModel(m_model);
	}
    }
  else
    {
      setCompletionMode(QCompleter::UnfilteredPopupCompletion);
      setModel(nullptr);
    }
}

void dooble_address_widget_completer::slot_text_edited_timeout(void)
{
  if(!dooble_settings::setting("show_address_widget_completer").toBool() ||
     !parent())
    return;

  auto text(qobject_cast<dooble_address_widget *> (parent())->text());

  if(text.trimmed().isEmpty())
    {
      if(completionMode() == QCompleter::UnfilteredPopupCompletion)
	m_popup->setVisible(false);
      else
	complete("");
    }
  else
    complete(text);
}
