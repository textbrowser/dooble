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

#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_address_widget_completer.h"
#include "dooble_address_widget_completer_popup.h"
#include "dooble_application.h"
#include "dooble_favicons.h"
#include "dooble_page.h"

QHash<QUrl, char> dooble_address_widget_completer::s_urls;
QList<QStandardItem *> dooble_address_widget_completer::s_purged_items;
QStandardItemModel *dooble_address_widget_completer::s_model = 0;

dooble_address_widget_completer::dooble_address_widget_completer
(QWidget *parent):QCompleter(parent)
{
  if(!s_model)
    {
      s_model = new QStandardItemModel();
      s_model->setSortRole(Qt::UserRole);
    }

  m_popup = new dooble_address_widget_completer_popup(parent);
  m_popup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_popup->horizontalHeader()->setVisible(false);
  m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
  m_popup->setShowGrid(false);
  m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_popup->verticalHeader()->setVisible(false);
  connect(dooble::s_application,
	  SIGNAL(history_cleared(void)),
	  this,
	  SLOT(slot_history_cleared(void)));
  connect(m_popup,
	  SIGNAL(clicked(const QModelIndex &)),
	  this,
	  SLOT(slot_clicked(const QModelIndex &)));
  connect(qobject_cast<QLineEdit *> (parent),
	  SIGNAL(textEdited(const QString &)),
	  this,
	  SLOT(slot_text_edited(const QString &)));
  setCaseSensitivity(Qt::CaseInsensitive);
  setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  setModel(s_model);
  setModelSorting(QCompleter::UnsortedModel);
  setPopup(m_popup);
  setWrapAround(false);
}

dooble_address_widget_completer::~dooble_address_widget_completer()
{
}

int dooble_address_widget_completer::levenshtein_distance
(const QString &str1, const QString &str2) const
{
  if(str1.isEmpty())
    return str2.length();
  else if(str2.isEmpty())
    return str1.length();

  QChar str1_c = 0;
  QChar str2_c = 0;
  QVector<QVector<int> > matrix(str1.length() + 1,
				QVector<int> (str2.length() + 1));
  int cost = 0;

  for(int i = 0; i <= str1.length(); i++)
    matrix[i][0] = i;

  for(int i = 0; i <= str2.length(); i++)
    matrix[0][i] = i;

  for(int i = 1; i <= str1.length(); i++)
    {
      str1_c = str1.at(i - 1);

      for(int j = 1; j <= str2.length(); j++)
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
  ** Prevent duplicates. We can't use the model's findItems()
  ** as the model may contain a subset of the completer's
  ** items because of filtering activity.
  */

  if(s_urls.contains(url))
    return;
  else
    s_urls[url] = 0;

  QStandardItem *item = new QStandardItem(icon, url.toString());

  item->setToolTip(url.toString());
  s_model->insertRow(0, item);
}

void dooble_address_widget_completer::complete(void)
{
  complete("");
}

void dooble_address_widget_completer::complete(const QString &text)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  s_model->blockSignals(true);

  while(!s_purged_items.isEmpty())
    {
      s_model->setRowCount(s_model->rowCount() + 1);
      s_model->setItem(s_model->rowCount() - 1, s_purged_items.takeFirst());
    }

  QList<QStandardItem *> list;

  if(text.trimmed().isEmpty())
    {
      for(int i = 0; i < s_model->rowCount(); i++)
	if(s_model->item(i, 0))
	  list << s_model->item(i, 0)->clone();
    }
  else
    {
      QMultiMap<int, QStandardItem *> map;
      QString c(text.toLower().trimmed());

      for(int i = 0; i < s_model->rowCount(); i++)
	if(s_model->item(i, 0))
	  {
	    if(s_model->item(i, 0)->text().toLower().contains(c))
	      map.insert
		(levenshtein_distance(c, s_model->item(i, 0)->text().toLower()),
		 s_model->item(i, 0)->clone());
	    else
	      s_purged_items << s_model->item(i, 0)->clone();
	  }

      list << map.values();
    }

  s_model->clear();

  while(list.size() > 1)
    {
      s_model->setRowCount(s_model->rowCount() + 1);
      s_model->setItem(s_model->rowCount() - 1, list.takeFirst());
    }

  /*
  ** Unblock signals on the model and add the last list entry. This little
  ** trick will allow for a smoother update of the table's contents.
  */

  s_model->blockSignals(false);

  while(!list.isEmpty())
    {
      s_model->setRowCount(s_model->rowCount() + 1);
      s_model->setItem(s_model->rowCount() - 1, list.takeFirst());
    }

  if(s_model->rowCount() > 0)
    {
      if(s_model->rowCount() > dooble_page::MAXIMUM_HISTORY_ITEMS)
	m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      else
	m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      m_popup->setMaximumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      s_model->rowCount()) * m_popup->rowHeight(0));
      m_popup->setMinimumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      s_model->rowCount()) * m_popup->rowHeight(0));

      /*
      ** The model should only be sorted when the pulldown
      ** is activated. Otherwise, the Levenshtein algorithm
      ** loses its potential.
      */

      if(s_purged_items.isEmpty())
	s_model->sort(0, Qt::DescendingOrder);

      QCompleter::complete();
    }
  else
    m_popup->setVisible(false);

  QApplication::restoreOverrideCursor();
}

void dooble_address_widget_completer::remove_item(const QUrl &url)
{
  QList<QStandardItem *> list(s_model->findItems(url.toString()));

  if(!list.isEmpty())
    if(list.at(0))
      s_model->removeRow(list.at(0)->row());

  s_urls.remove(url);
}

void dooble_address_widget_completer::set_item_icon(const QIcon &icon,
						    const QUrl &url)
{
  QList<QStandardItem *> list(s_model->findItems(url.toString()));

  if(!list.isEmpty())
    if(list.at(0))
      {
	if(icon.isNull())
	  list.at(0)->setIcon(dooble_favicons::icon(QUrl()));
	else
	  list.at(0)->setIcon(icon);
      }
}

void dooble_address_widget_completer::slot_clicked(const QModelIndex &index)
{
  qobject_cast<QLineEdit *> (widget())->setText(index.data().toString());
  QApplication::postEvent
    (widget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier));
}

void dooble_address_widget_completer::slot_history_cleared(void)
{
  s_model->clear();
  s_purged_items.clear();
  s_urls.clear();
}

void dooble_address_widget_completer::slot_text_edited(const QString &text)
{
  if(text.trimmed().isEmpty())
    m_popup->setVisible(false);
  else
    complete(text);
}
