/*
** Copyright (c) Alexis Megas.
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
**    derived from Woody without specific prior written permission.
**
** WOODY IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** WOODY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _woody_collapse_expand_tool_button_h_
#define _woody_collapse_expand_tool_button_h_

#include <QHeaderView>
#include <QPointer>
#include <QScrollBar>
#include <QToolButton>
#include <QTreeWidget>

class woody_collapse_expand_tool_button: public QToolButton
{
  Q_OBJECT

 public:
  woody_collapse_expand_tool_button(QTreeWidget *tree):QToolButton(tree)
  {
    m_tree = tree;

    if(m_tree && m_tree->header())
      {
	m_tree->header()->setDefaultAlignment(Qt::AlignCenter);
	m_tree->header()->setMinimumHeight(30);
	move(5, (m_tree->header()->size().height() - 25) / 2 + 2);
	resize(25, 25);
	setCheckable(true);

	auto font(this->font());

	font.setStyleHint(QFont::Courier);
	setFont(font);
	setStyleSheet("QToolButton {border: none;}"
		      "QToolButton::menu-button {border: none;}");
	setText(tr("+"));
	setToolTip(tr("Collapse / Expand"));
	connect(m_tree,
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		this,
		SLOT(slot_item_collapsed_expanded(void)));
	connect(m_tree,
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		this,
		SLOT(slot_item_collapsed_expanded(void)));
	connect(m_tree->horizontalScrollBar(),
		SIGNAL(valueChanged(int)),
		this,
		SLOT(slot_scroll(int)));
	connect(this,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_collapse(void)));
      }
    else
      setVisible(false);
  }

 private:
  QPointer<QTreeWidget> m_tree;

 private slots:
  void slot_collapse(void)
  {
    if(m_tree)
      {
	disconnect(m_tree,
		   SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		   this,
		   SLOT(slot_item_collapsed_expanded(void)));
	disconnect(m_tree,
		   SIGNAL(itemExpanded(QTreeWidgetItem *)),
		   this,
		   SLOT(slot_item_collapsed_expanded(void)));
      }

    if(isChecked())
      {
	if(m_tree)
	  m_tree->expandAll();

	setText(tr("-"));
      }
    else
      {
	if(m_tree)
	  m_tree->collapseAll();

	setText(tr("+"));
      }

    if(m_tree)
      {
	connect(m_tree,
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		this,
		SLOT(slot_item_collapsed_expanded(void)));
	connect(m_tree,
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		this,
		SLOT(slot_item_collapsed_expanded(void)));
      }
  }

  void slot_item_collapsed_expanded(void)
  {
    if(!m_tree)
      return;

    int expanded = 0;

    for(int i = 0; i < m_tree->topLevelItemCount(); i++)
      if(m_tree->topLevelItem(i) && m_tree->topLevelItem(i)->childCount() > 0)
	expanded += m_tree->topLevelItem(i)->isExpanded() ? 1 : 0;

    if(expanded == 0)
      {
	setChecked(false);
	setText(tr("+"));
      }
    else
      {
	setChecked(true);
	setText(tr("-"));
      }
  }

  void slot_scroll(int value)
  {
    setVisible(geometry().right() - geometry().width() / 2 > value);
  }
};

#endif
