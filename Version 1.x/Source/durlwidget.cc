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

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QStyle>
#include <QUrl>

#include "dmisc.h"
#include "dooble.h"
#include "durlwidget.h"

durlcompleterview::durlcompleterview(QLineEdit *lineEdit):QTableView()
{
  m_lineEdit = lineEdit;
  setAlternatingRowColors(true);
  setMouseTracking(true);
}

void durlcompleterview::wheelEvent(QWheelEvent *event)
{
  if(event && event->type() == QEvent::Wheel)
    {
      QModelIndex index(indexAt(event->pos()));

      if(index.isValid())
	{
	  QString str("");

	  if(m_lineEdit)
	    str = m_lineEdit->text();

	  selectRow(index.row());

	  if(m_lineEdit)
	    m_lineEdit->setText(str);
	}
    }

  QTableView::wheelEvent(event);
}

void durlcompleterview::mouseMoveEvent(QMouseEvent *event)
{
  if(event && event->type() == QEvent::MouseMove)
    {
      QModelIndex index(indexAt(event->pos()));

      if(index.isValid())
	{
	  QString str("");

	  if(m_lineEdit)
	    str = m_lineEdit->text();

	  selectRow(index.row());

	  if(m_lineEdit)
	    m_lineEdit->setText(str);
	}
    }

  QTableView::mouseMoveEvent(event);
}

durlcompleter::durlcompleter(QWidget *parent):QCompleter(parent)
{
  m_model = 0;
  m_tableView = new durlcompleterview(qobject_cast<QLineEdit *> (parent));
  m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_tableView->verticalHeader()->setVisible(false);
  m_tableView->horizontalHeader()->setVisible(false);
#if QT_VERSION >= 0x050000
  m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
  m_tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
  setPopup(m_tableView);
}

durlcompleter::~durlcompleter()
{
  clear();

  /*
  ** We don't need to delete m_tableView as setPopup()
  ** forces the completer to take ownership of the view.
  */
}

bool durlcompleter::exists(const QString &text) const
{
  return m_allUrls.contains(text);
}

void durlcompleter::clear(void)
{
  if(m_model)
    m_model->clear();

  m_allUrls.clear();

  while(!m_purgedItems.isEmpty())
    delete m_purgedItems.takeFirst();
}

void durlcompleter::setModel(QStandardItemModel *model)
{
  m_model = model;

  if(m_model)
    m_model->setSortRole(Qt::UserRole);

  QCompleter::setModel(m_model);
}

void durlcompleter::saveItemUrl(const QString &url)
{
  m_allUrls.append(url);
}

void durlcompleter::setCompletion
(const QString &completion)
{
  if(!m_model)
    return;

  m_model->blockSignals(true);

  while(!m_purgedItems.isEmpty())
    {
      m_model->setRowCount(m_model->rowCount() + 1);
      m_model->setItem(m_model->rowCount() - 1,
		       m_purgedItems.takeFirst());
    }

  QList<QStandardItem *> list;

  if(completion.trimmed().isEmpty())
    {
      for(int i = 0; i < m_model->rowCount(); i++)
	if(m_model->item(i, 0))
	  list.append(m_model->item(i, 0)->clone());
    }
  else
    {
      QString c(completion.toLower().trimmed());
      QMultiMap<int, QStandardItem *> map;

      for(int i = 0; i < m_model->rowCount(); i++)
	if(m_model->item(i, 0))
	  {
	    if(m_model->item(i, 0)->text().toLower().contains(c))
	      map.insert
		(dmisc::levenshteinDistance(m_model->item(i, 0)->text().
					    toLower(), c),
		 m_model->item(i, 0)->clone());
	    else
	      m_purgedItems.append(m_model->item(i, 0)->clone());
	  }

      list << map.values();
    }

  m_model->clear();

  while(list.size() > 1)
    {
      m_model->setRowCount(m_model->rowCount() + 1);
      m_model->setItem(m_model->rowCount() - 1,
		       list.takeFirst());
    }

  /*
  ** Unblock signals on the model and add the last list entry. This little
  ** trick will allow for a smoother update of the table's contents.
  */

  m_model->blockSignals(false);

  while(!list.isEmpty())
    {
      m_model->setRowCount(m_model->rowCount() + 1);
      m_model->setItem(m_model->rowCount() - 1,
		       list.takeFirst());
    }

  if(m_model->rowCount() > 0)
    {
      if(m_model->rowCount() > 10)
	m_tableView->setVerticalScrollBarPolicy
	  (Qt::ScrollBarAlwaysOn);
      else
	m_tableView->setVerticalScrollBarPolicy
	  (Qt::ScrollBarAlwaysOff);

      m_tableView->setMaximumHeight
	(qMin(10, m_model->rowCount()) * m_tableView->rowHeight(0));
      m_tableView->setMinimumHeight
	(qMin(10, m_model->rowCount()) * m_tableView->rowHeight(0));

      /*
      ** The model should only be sorted when the pulldown
      ** is activated. Otherwise, the Levenshtein algorithm
      ** loses its potential.
      */

      if(m_purgedItems.isEmpty())
	m_model->sort(0, Qt::DescendingOrder);

      complete();
    }
  else
    popup()->setVisible(false);
}

void durlcompleter::copyContentsOf(durlcompleter *c)
{
  if(!c)
    return;

  /*
  ** Aha, c's model may be truncated because of filtering.
  ** We'll also need to copy data from m_purgedItems.
  */

  if(m_model && c->m_model)
    {
      for(int i = 0; i < c->m_purgedItems.size(); i++)
	{
	  saveItemUrl
	    (c->m_purgedItems.at(i)->text());
	  m_model->appendRow(c->m_purgedItems.at(i)->clone());
	}

      for(int i = 0; i < c->m_model->rowCount(); i++)
	{
	  saveItemUrl(c->m_model->item(i, 0)->text());
	  m_model->appendRow(c->m_model->item(i, 0)->clone());
	}
    }
}

durlwidgettoolbutton::durlwidgettoolbutton(QWidget *parent):
  QToolButton(parent)
{
}

void durlwidgettoolbutton::mousePressEvent(QMouseEvent *event)
{
  if(event && (event->type() == QEvent::MouseButtonPress ||
	       event->type() == QEvent::MouseButtonDblClick))
    emit clicked();
  else
    QToolButton::mousePressEvent(event);
}

durlwidget::durlwidget(QWidget *parent):QLineEdit(parent)
{
  setMaxLength(35000);
  m_counter = 0;
  goToolButton = new QToolButton(this);
  goToolButton->setToolTip(tr("Load"));
  goToolButton->setIconSize(QSize(16, 16));
  goToolButton->setCursor(Qt::ArrowCursor);
  goToolButton->setStyleSheet("QToolButton {"
			      "border: none; "
			      "padding-top: 0px; "
			      "padding-bottom: 0px; "
			      "}");
  bookmarkToolButton = new QToolButton(this);
  bookmarkToolButton->setIconSize(QSize(16, 16));
  bookmarkToolButton->setCursor(Qt::ArrowCursor);
  m_iconToolButton = new QToolButton(this);
  m_iconToolButton->setIconSize(QSize(16, 16));
  m_iconToolButton->setCursor(Qt::ArrowCursor);
  bookmarkToolButton->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
  m_iconToolButton->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
  m_fakePulldownMenu = new durlwidgettoolbutton(this);
  m_fakePulldownMenu->setCursor(Qt::ArrowCursor);
  m_fakePulldownMenu->setIconSize(QSize(16, 16));
  m_fakePulldownMenu->setArrowType(Qt::DownArrow);
  m_fakePulldownMenu->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}"
     "QToolButton::menu-button {border: none;}");
  m_fakePulldownMenu->setPopupMode(QToolButton::InstantPopup);
  m_spotonButton = new QToolButton(this);
  m_spotonButton->setCursor(Qt::ArrowCursor);
  m_spotonButton->setIconSize(QSize(16, 16));
  m_spotonButton->setStyleSheet
    ("QToolButton {"
     "border: none; "
     "padding-top: 0px; "
     "padding-bottom: 0px; "
     "}");
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  m_spotonButton->setToolTip
    (tr("Submit URL to Spot-On."));
#else
  m_spotonButton->setToolTip(tr("Spot-On support is not available."));
#endif
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  if(dooble::s_spoton)
    dooble::s_spoton->registerWidget(m_spotonButton);
  else
    dmisc::logError("durlwidget::durlwidget(): dooble::s_spoton is 0.");
#endif
  slotSetIcons();
#ifndef DOOBLE_LINKED_WITH_LIBSPOTON
  setSpotOnColor(false);
#else
  setSpotOnColor(true);
#endif
  connect(this, SIGNAL(returnPressed(void)), this,
	  SLOT(slotReturnPressed(void)));
  connect(m_fakePulldownMenu, SIGNAL(clicked(void)), this,
	  SLOT(slotPulldownClicked(void)));
  connect(goToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotLoadPage(void)));
  connect(bookmarkToolButton, SIGNAL(clicked(void)), this,
          SLOT(slotBookmark(void)));
  connect(m_iconToolButton, SIGNAL(clicked(void)), this,
	  SIGNAL(iconToolButtonClicked(void)));
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  connect(m_spotonButton, SIGNAL(clicked(void)), this,
	  SIGNAL(submitUrlToSpotOn(void)));
#endif

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet
    (QString("QLineEdit {padding-right: %1px; padding-left: %2px; "
	     "selection-background-color: darkgray;}").
     arg(goToolButton->sizeHint().width() +
	 m_fakePulldownMenu->sizeHint().width() + frameWidth + 5).
     arg(m_iconToolButton->sizeHint().width() +
	 m_spotonButton->sizeHint().width() +
	 bookmarkToolButton->sizeHint().width() + frameWidth + 5));
  setMinimumHeight(sizeHint().height() + 10);
  m_completer = new durlcompleter(this);
  m_completer->setModel(new QStandardItemModel(m_completer));
  m_completer->setModelSorting(QCompleter::UnsortedModel);
  m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  m_completer->setCaseSensitivity(Qt::CaseInsensitive);
  connect(m_completer,
	  SIGNAL(activated(const QString &)),
	  this,
	  SLOT(slotLoadPage(const QString &)));
  setCompleter(m_completer);
  updateToolTips();
}

void durlwidget::addItem(const QString &text)
{
  addItem(text, QIcon());
}

void durlwidget::addItem(const QString &text, const QIcon &icon)
{
  if(text.isEmpty())
    return;

  /*
  ** Some items are not acceptable.
  */

  QString l_text(text.toLower().trimmed());

  if(!(l_text.startsWith("data:") |
       l_text.startsWith("file://") ||
       l_text.startsWith("ftp://") ||
       l_text.startsWith("gopher://") ||
       l_text.startsWith("http://") ||
       l_text.startsWith("https://") ||
       l_text.startsWith("qrc:/")))
    return;

  if(!m_completer)
    return;

  /*
  ** Prevent duplicates. We can't use the model's findItems()
  ** as the model may contain a subset of the completer's
  ** items because of filtering activity.
  */

  if(m_completer->exists(text))
    return;

  if(!dooble::s_settings.value("settingsWindow/rememberHistory",
			       true).toBool())
    return;

  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_completer->model());

  if(model)
    {
      m_completer->saveItemUrl(text);

      QStandardItem *item = 0;

      if(icon.isNull())
	{
	  QUrl url(QUrl::fromUserInput(text));

	  if(url.host().toLower().trimmed().startsWith("gopher"))
	    url.setScheme("gopher");

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  item = new QStandardItem(dmisc::iconForUrl(url), text);
	}
      else
	item = new QStandardItem(icon, text);

      item->setToolTip(text);
      m_counter += 1;
      item->setData(m_counter, Qt::UserRole);
      model->insertRow(0, item);
    }
}

void durlwidget::appendItem(const QString &text, const QIcon &icon)
{
  if(text.isEmpty())
    return;

  if(!m_completer)
    return;

  /*
  ** Prevent duplicates. We can't use the model's findItems()
  ** as the model may contain a subset of the completer's
  ** items because of filtering.
  */

  if(m_completer->exists(text))
    return;

  if(!dooble::s_settings.value("settingsWindow/rememberHistory",
			       true).toBool())
    return;

  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_completer->model());

  if(model)
    {
      m_completer->saveItemUrl(text);

      QStandardItem *item = 0;

      if(icon.isNull())
	{
	  QUrl url(QUrl::fromUserInput(text));

	  if(url.host().toLower().trimmed().startsWith("gopher"))
	    url.setScheme("gopher");

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  item = new QStandardItem(dmisc::iconForUrl(url), text);
	}
      else
	item = new QStandardItem(icon, text);

      item->setToolTip(text);
      m_counter += 1;
      item->setData(m_counter, Qt::UserRole);
      model->appendRow(item);
    }
}

void durlwidget::setIcon(const QIcon &icon)
{
  if(icon.isNull())
    {
      QUrl url(QUrl::fromUserInput(text()));

      if(url.host().toLower().trimmed().startsWith("gopher"))
	url.setScheme("gopher");

      url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
      m_iconToolButton->setIcon(dmisc::iconForUrl(url));
    }
  else
    m_iconToolButton->setIcon(icon);

  m_secureButtonIcon = m_iconToolButton->icon();
}

void durlwidget::setText(const QString &text)
{
  QLineEdit::setText(text);
  setCursorPosition(0);
}

void durlwidget::selectAll(void)
{
  setCursorPosition(text().length());
  cursorBackward(true, text().length());
}

void durlwidget::resizeEvent(QResizeEvent *event)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size1 = goToolButton->sizeHint();
  QSize size2 = m_iconToolButton->sizeHint();
  QSize size3 = bookmarkToolButton->sizeHint();
  QSize size4 = m_fakePulldownMenu->sizeHint();
  QSize size5 = m_spotonButton->sizeHint();

  goToolButton->move
    (rect().right() - size1.width() - size4.width() - 5,
     (rect().bottom() + 2 - size1.height()) / 2);
  m_spotonButton->move
    (frameWidth - rect().left() + 6,
     (rect().bottom() + 2 - size5.height()) / 2);
  m_iconToolButton->move
    (frameWidth - rect().left() + size3.width() + size5.width() + 5,
     (rect().bottom() + 2 - size2.height()) / 2);
  bookmarkToolButton->move
    (frameWidth - rect().left() + size5.width() + 5,
     (rect().bottom() + 2 - size3.height()) / 2);
  m_fakePulldownMenu->move
    (rect().right() - frameWidth - size4.width() - 5,
     (rect().bottom() + 2 - size4.height()) / 2);

  if(selectedText().isEmpty())
    setCursorPosition(0);

  QLineEdit::resizeEvent(event);
}

void durlwidget::setItemIcon(const int index , const QIcon &icon)
{
  if(!m_completer)
    return;

  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_completer->model());

  if(!model)
    return;

  QStandardItem *item = model->item(index, 0);

  if(item)
    {
      if(icon.isNull())
	{
	  QUrl url(QUrl::fromUserInput(item->text()));

	  if(url.host().toLower().trimmed().startsWith("gopher"))
	    url.setScheme("gopher");

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  item->setIcon(dmisc::iconForUrl(url));
	}
      else
	item->setIcon(icon);
    }
}

void durlwidget::slotPulldownClicked(void)
{
  if(!m_completer)
    return;

  m_completer->setCompletion("");

  /*
  ** I'm not sure why I need to do this. It appears
  ** that the activated() signal is lost whenever
  ** m_completer's model has changed.
  */

  disconnect(m_completer,
	     SIGNAL(activated(const QString &)),
	     this,
	     SLOT(slotLoadPage(const QString &)));
  connect(m_completer,
	  SIGNAL(activated(const QString &)),
	  this,
	  SLOT(slotLoadPage(const QString &)));
}

int durlwidget::findText(const QString &text) const
{
  int index = -1;

  if(text.isEmpty())
    return index;

  if(!m_completer)
    return index;

  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_completer->model());

  if(!model)
    return index;

  QList<QStandardItem *> list(model->findItems(text));

  if(!list.isEmpty())
    index = list.at(0)->row();

  return index;
}

void durlwidget::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("urlWidget");
  goToolButton->setIcon(QIcon(settings.value("goToolButton").toString()));
  bookmarkToolButton->setIcon
    (QIcon(settings.value("bookmarkToolButton").toString()));
  m_bookmarkButtonIcon = bookmarkToolButton->icon();

  if(m_iconToolButton->icon().isNull())
    m_iconToolButton->setIcon(QIcon(settings.value("emptyIcon").toString()));

  m_secureButtonIcon = m_iconToolButton->icon();
  settings.endGroup();
  m_spotonButton->setIcon(QIcon(settings.value("spotonIcon").toString()));

  if(m_spotonButton->icon().isNull())
    {
      settings.beginGroup("urlWidget");
      m_spotonButton->setIcon(QIcon(settings.value("emptyIcon").toString()));
    }

  m_spotonIcon = m_spotonButton->icon();
  setBookmarkColor(bookmarkToolButton->property("is_bookmarked").toBool());
  setSpotOnColor(m_spotonButton->property("is_loaded").toBool());
}

void durlwidget::slotBookmark(void)
{
  emit bookmark();
}

bool durlwidget::event(QEvent *e)
{
  if(e && e->type() == QEvent::KeyPress &&
     static_cast<QKeyEvent *> (e)->key() == Qt::Key_Tab)
    {
      QTableView *table = qobject_cast<QTableView *> (m_completer->popup());

      if(table && table->isVisible())
	{
	  int row = 0;

	  if(table->selectionModel()->
	     isRowSelected(table->currentIndex().row(),
			   table->rootIndex()))
	    {
	      row = table->currentIndex().row() + 1;

	      if(row >= m_completer->model()->rowCount())
		row = 0;
	    }

	  table->selectRow(row);
	  return true;
	}
    }

  return QLineEdit::event(e);
}

void durlwidget::keyPressEvent(QKeyEvent *event)
{
  bool state = true;

  if(event)
    {
      QKeySequence userKeys(event->modifiers() + event->key());

      if(event->modifiers() == Qt::AltModifier)
	{
	  /*
	  ** Delegate the event to the parent.
	  */

	  event->ignore();
	  return;
	}
      else if(event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
	slotPulldownClicked();
      else if(userKeys == QKeySequence(Qt::AltModifier + Qt::Key_Enter) ||
	      userKeys == QKeySequence(Qt::AltModifier + Qt::Key_Return) ||
	      userKeys == QKeySequence(Qt::AltModifier + Qt::MetaModifier +
				       Qt::Key_Enter) ||
	      userKeys == QKeySequence(Qt::AltModifier + Qt::MetaModifier +
				       Qt::Key_Return))
	{
	  state = false;

	  QUrl url(QUrl::fromUserInput(text().trimmed()));

	  if(url.host().toLower().trimmed().startsWith("gopher"))
	    url.setScheme("gopher");

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit openLinkInNewTab(url);
	}
      else if(userKeys == QKeySequence(Qt::Key_Escape))
	emit resetUrl();
      else if(userKeys == QKeySequence(Qt::ControlModifier + Qt::Key_L))
	{
	  setFocus();
	  selectAll();
	  update();
	  event->ignore();
	  return;
	}
      else if(event->key() == Qt::Key_Enter ||
	      event->key() == Qt::Key_Return)
	{
	  state = false;

	  QUrl url(QUrl::fromUserInput(text().trimmed()));

	  if(url.host().toLower().trimmed().startsWith("gopher"))
	    url.setScheme("gopher");

	  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
	  emit loadPage(url);
	}
      else if(userKeys == QKeySequence(Qt::ControlModifier + Qt::Key_B))
	{
	  /*
	  ** Delegate the event to the parent. If the Bookmarks menu has
	  ** not been made visible, the shortcut will not be available.
	  */

	  event->ignore();
	  return;
	}
    }

  if(state)
    QLineEdit::keyPressEvent(event);
  else if(m_completer)
    m_completer->popup()->setVisible(false);

  if(state && !text().trimmed().isEmpty() &&
     event &&
     event->key() != Qt::Key_Left &&
     event->key() != Qt::Key_Right &&
     event->key() != Qt::Key_Escape &&
     event->modifiers() == Qt::NoModifier &&
     QApplication::keyboardModifiers() == Qt::NoModifier && m_completer)
    {
      m_completer->setCompletion(text().trimmed());

      /*
      ** I'm not sure why I need to do this. It appears
      ** that the activated() signal is lost whenever
      ** m_completer's model has changed.
      */

      disconnect(m_completer,
		 SIGNAL(activated(const QString &)),
		 this,
		 SLOT(slotLoadPage(const QString &)));
      connect(m_completer,
	      SIGNAL(activated(const QString &)),
	      this,
	      SLOT(slotLoadPage(const QString &)));
    }
}

void durlwidget::slotLoadPage(void)
{
  QUrl url(QUrl::fromUserInput(text().trimmed()));

  if(url.host().toLower().trimmed().startsWith("gopher"))
    url.setScheme("gopher");

  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
  emit loadPage(url);
}

void durlwidget::slotLoadPage(const QString &urlText)
{
#ifdef Q_OS_MAC
  if(m_completer)
    m_completer->popup()->setVisible(false);
#endif

  QUrl url(QUrl::fromUserInput(urlText));

  if(url.host().toLower().trimmed().startsWith("gopher"))
    url.setScheme("gopher");

  url = QUrl::fromEncoded(url.toEncoded(QUrl::StripTrailingSlash));
  emit loadPage(url);
}

void durlwidget::clearHistory(void)
{
  m_counter = 0;

  if(m_completer)
    m_completer->clear();
}

void durlwidget::slotReturnPressed(void)
{
  if(m_completer)
    m_completer->popup()->setVisible(false);
}

void durlwidget::setSecureColor(const bool isHttps)
{
  QIcon icon(m_secureButtonIcon);
  QPixmap pixmap;

  if(isHttps)
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
  else
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Disabled, QIcon::Off);

  m_iconToolButton->setIcon(pixmap);
}

void durlwidget::setBookmarkColor(const bool isBookmarked)
{
  QIcon icon(m_bookmarkButtonIcon);
  QPixmap pixmap;

  if(isBookmarked)
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
  else
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Disabled, QIcon::On);

  bookmarkToolButton->setIcon(pixmap);
  bookmarkToolButton->setProperty("is_bookmarked", isBookmarked);
}

void durlwidget::setSpotOnColor(const bool isLoaded)
{
  QIcon icon(m_spotonIcon);
  QPixmap pixmap;
  bool state = isLoaded;

#ifndef DOOBLE_LINKED_WITH_LIBSPOTON
  state = false;
#endif

  if(state)
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On);
  else
    pixmap = icon.pixmap(QSize(16, 16), QIcon::Disabled, QIcon::On);

  m_spotonButton->setIcon(pixmap);
  m_spotonButton->setProperty("is_loaded", state);
}

void durlwidget::copyContentsOf(durlwidget *c)
{
  if(m_completer && c->m_completer)
    m_completer->copyContentsOf(c->m_completer);
}

void durlwidget::appendItems
(const QMultiMap<QDateTime, QVariantList> &items)
{
  QStandardItemModel *model = qobject_cast<QStandardItemModel *>
    (m_completer->model());

  if(model)
    {
      QMapIterator<QDateTime, QVariantList> i(items);

      while(i.hasNext())
	{
	  i.next();

	  if(i.value().size() < 2)
	    continue;

	  QString text(i.value().at(0).toUrl().
		       toString(QUrl::StripTrailingSlash));

	  if(m_completer->exists(text))
	    continue;

	  m_completer->saveItemUrl(text);

	  QIcon icon(i.value().at(1).value<QIcon> ());

	  if(icon.isNull())
	    icon = dmisc::iconForUrl(i.value().at(0).toUrl());

	  QStandardItem *item = new QStandardItem(icon, text);

	  item->setToolTip(text);
	  m_counter += 1;
	  item->setData(m_counter, Qt::UserRole);
	  model->appendRow(item);
	}
    }
}

void durlwidget::setBookmarkButtonEnabled(const bool state)
{
  bookmarkToolButton->setEnabled(state);
}

void durlwidget::setIconButtonEnabled(const bool state)
{
  m_iconToolButton->setEnabled(state);
}

void durlwidget::setSpotOnButtonEnabled(const bool state)
{
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  m_spotonButton->setEnabled(state);
#else
  Q_UNUSED(state);
  m_spotonButton->setEnabled(false);
#endif
}

QPoint durlwidget::bookmarkButtonPopupPosition(void) const
{
  QPoint point(bookmarkToolButton->pos());

  point.setY(point.y() + bookmarkToolButton->height());
  return point;
}

QPoint durlwidget::iconButtonPopupPosition(void) const
{
  QPoint point(m_iconToolButton->pos());

  point.setY(point.y() + m_iconToolButton->height());
  return point;
}

bool durlwidget::isBookmarkButtonEnabled(void) const
{
  return bookmarkToolButton->isEnabled();
}

bool durlwidget::isIconButtonEnabled(void) const
{
  return m_iconToolButton->isEnabled();
}

void durlwidget::popdown(void) const
{
  if(m_completer)
    m_completer->popup()->setVisible(false);
}

void durlwidget::updateToolTips(void)
{
  if(dooble::s_settings.value("settingsWindow/"
			      "disableAllEncryptedDatabaseWrites",
			      false).toBool())
    bookmarkToolButton->setToolTip(tr("Encrypted database writes are "
				      "disabled. Therefore, bookmarks "
				      "are also disabled."));
  else
    bookmarkToolButton->setToolTip(tr("Bookmark"));
}
