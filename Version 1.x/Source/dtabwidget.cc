/*
** Copyright (c) 2008 - present, Alexis Megas, Bernd Stramm.
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

#include <QCheckBox>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QMovie>
#include <QProgressBar>
#include <QSettings>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QWidgetAction>

#include "dmisc.h"
#include "dooble.h"
#include "dtabwidget.h"

dtabbar::dtabbar(QWidget *parent):QTabBar(parent)
{
  QString str
    (dooble::s_settings.value("settingsWindow/tabBarPosition", "north").
     toString().toLower().trimmed());

  if(str == "east")
    m_tabPosition = QTabWidget::East;
  else if(str == "north")
    m_tabPosition = QTabWidget::North;
  else if(str == "south")
    m_tabPosition = QTabWidget::South;
  else if(str == "west")
    m_tabPosition = QTabWidget::West;
  else
    m_tabPosition = QTabWidget::North;

  setAcceptDrops(true);
  setDocumentMode(true);
  setElideMode(Qt::ElideRight);
  setExpanding(true);
  setUsesScrollButtons(true);
}

void dtabbar::dropEvent(QDropEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      int index = tabAt(event->pos());

      event->accept();

      if(index != -1)
	emit urlsReceivedViaDrop(event->mimeData()->urls());

      return;
    }

  QTabBar::dropEvent(event);
}

void dtabbar::dragMoveEvent(QDragMoveEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      int index = tabAt(event->pos());

      if(index != -1)
	setCurrentIndex(index);

      event->accept();
      return;
    }

  QTabBar::dragMoveEvent(event);
}

void dtabbar::dragEnterEvent(QDragEnterEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      int index = tabAt(event->pos());

      if(index != -1)
	setCurrentIndex(index);

      event->accept();
      return;
    }

  QTabBar::dragEnterEvent(event);
}

void dtabbar::mouseDoubleClickEvent(QMouseEvent *event)
{
  if(event && event->button() == Qt::LeftButton)
    if(tabAt(event->pos()) == -1)
      if(dooble::s_settings.value("settingsWindow/addTabWithDoubleClick",
				  true).toBool())
	emit createTab();

  QTabBar::mouseDoubleClickEvent(event);
}

QSize dtabbar::tabSizeHint(int index) const
{
  QSize size(QTabBar::tabSizeHint(index));

  if(m_tabPosition == QTabWidget::East || m_tabPosition == QTabWidget::West)
    {
      int preferredTabHeight = 175;

      if(parentWidget() &&
	 count() * rect().height() < parentWidget()->size().height())
	preferredTabHeight = 175;
      else
	preferredTabHeight = qBound
	  (125,
	   qMax(size.height(), rect().height() / qMax(1, count())),
	   175);

      size.setHeight(preferredTabHeight);
    }
  else
    {
#ifdef Q_OS_MAC
      int preferred = 275;
#else
      int preferred = 225;
#endif
      int preferredTabWidth = preferred;

      if(parentWidget() &&
	 count() * rect().width() < parentWidget()->size().width())
	preferredTabWidth = preferred;
      else
	preferredTabWidth = qBound
	  (125,
	   qMax(size.width(), rect().width() / qMax(1, count())),
	   preferred);

      size.setWidth(preferredTabWidth);
    }

  return size;
}

void dtabbar::setTabPosition(const QTabWidget::TabPosition position)
{
  m_tabPosition = position;
}

dtabwidget::dtabwidget(QWidget *parent):QTabWidget(parent)
{
  if(tabBar())
    tabBar()->deleteLater();

  m_tabBar = new dtabbar(this);
  setTabBar(m_tabBar);
  setAcceptDrops(true);
  m_selectedTabIndex = -1;
  setStyleSheet(
#ifdef Q_OS_MAC
		"QTabWidget::pane {"
		"border: 1px solid #c4c4c3; "
		"position: absolute; "
		"top: 0px;"
		"}"
#endif
		"QTabWidget::tab-bar {"
		"alignment: left;"
		"}");
  m_tabBar->setStyleSheet
    ("QTabBar::tear {"
     "image: none;"
     "}"
     );
  m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
  m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
  connect(m_tabBar, SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotShowContextMenu(const QPoint &)));
  connect(m_tabBar, SIGNAL(tabMoved(int, int)),
	  this,
	  SIGNAL(tabMoved(int, int)));
  connect(m_tabBar, SIGNAL(urlsReceivedViaDrop(const QList<QUrl> &)),
	  this, SIGNAL(urlsReceivedViaDrop(const QList<QUrl> &)));
  connect(m_tabBar, SIGNAL(createTab(void)),
	  this, SIGNAL(createTab(void)));
  setMovable(true);
  connect(this,
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slotCloseTab(int)));

  foreach(QToolButton *toolButton, findChildren <QToolButton *> ())
    toolButton->setStyleSheet("QToolButton {background-color: white; "
			      "border: 1px solid #c4c4c3; "
#ifdef Q_OS_MAC
			      "margin-bottom: 1px;"
#else
			      "margin-bottom: 0px;"
#endif
			      "}"
			      "QToolButton::menu-button {border: none;}");

  slotSetIcons();
  slotSetPosition();
}

dtabwidget::~dtabwidget()
{
}

void dtabwidget::slotShowContextMenu(const QPoint &point)
{
  m_selectedTabIndex = m_tabBar->tabAt(point);

  if(m_selectedTabIndex > -1)
    {
      dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));
      QMenu menu(this);
      QAction *action = 0;

      if(p)
	{
	  menu.addAction(tr("&Bookmark"),
			 this, SLOT(slotBookmark(void)));
	  menu.addSeparator();
	  action = menu.addAction(tr("Close Ta&b"),
				  this, SLOT(slotCloseTab(void)));
	  action->setEnabled(count() != 1);
	  action = menu.addAction(tr("Close &Other Tabs"),
				  this, SLOT(slotCloseOtherTabs(void)));
	  action->setEnabled(count() > 1);
	  menu.addSeparator();
	  menu.addAction(tr("New Pr&ivate Tab"),
			 this, SIGNAL(createPrivateTab(void)));
	  menu.addAction(tr("New &Tab"),
			 this, SLOT(slotCreateTab(void)));
	  action = menu.addAction(tr("Open in &New Window"),
				  this, SLOT(slotOpenInNewWindow(void)));
	  action->setEnabled(count() > 1);
	  menu.addSeparator();
	  menu.addAction(tr("&Reload"),
			 this, SLOT(slotReloadTab(void)));
	  menu.addAction(tr("&Stop"),
			 this, SLOT(slotStopTab(void)));
	}
      else
	{
	  action = menu.addAction(tr("Close Ta&b"),
				  this, SLOT(slotCloseTab(void)));
	  action->setEnabled(count() != 1);
	  action = menu.addAction(tr("Close &Other Tabs"),
				  this, SLOT(slotCloseOtherTabs(void)));
	  action->setEnabled(count() > 1);
	  menu.addSeparator();
	  menu.addAction(tr("New Pr&ivate Tab"),
			 this, SIGNAL(createPrivateTab(void)));
	  menu.addAction(tr("New &Tab"),
			 this, SLOT(slotCreateTab(void)));
	}

      menu.addSeparator();
      action = menu.addAction(tr("&JavaScript"),
			      this, SLOT(slotJavaScript(void)));

      if(p)
	action->setCheckable(true);
      else
	action->setEnabled(false);

      if(p)
	action->setChecked(p->isJavaScriptEnabled());

      action = menu.addAction(tr("&Private Browsing"),
			      this, SLOT(slotPrivateBrowsing(void)));

      if(p)
	action->setCheckable(true);
      else
	action->setEnabled(false);

      if(p)
	action->setChecked(p->isPrivateBrowsingEnabled());

      action = menu.addAction(tr("&Web Plugins"),
			      this, SLOT(slotWebPlugins(void)));

      if(p)
	action->setCheckable(true);
      else
	action->setEnabled(false);

      if(p)
	action->setChecked(p->areWebPluginsEnabled());

      menu.addSeparator();
      action = menu.addAction(tr("&Private Cookies"),
			      this, SLOT(slotPrivateCookies(void)));

      if(p)
	action->setCheckable(true);
      else
	action->setEnabled(false);

      if(p)
	action->setChecked(p->arePrivateCookiesEnabled());

      menu.addAction(action);
      menu.addAction
	(tr("&View Private Cookies"),
	 this, SLOT(slotViewPrivateCookies(void)))->
	setEnabled(action->isChecked());
      menu.exec(m_tabBar->mapToGlobal(point));
    }
}

void dtabwidget::slotCloseTab(int index)
{
  if(index >= 0 && index < count())
    emit closeTab(index);
}

void dtabwidget::slotCloseTab(void)
{
  emit closeTab(m_selectedTabIndex);
  m_selectedTabIndex = -1;
}

void dtabwidget::slotCreateTab(void)
{
  emit createTab();
}

void dtabwidget::slotOpenInNewWindow(void)
{
  emit openInNewWindow(m_selectedTabIndex);
  m_selectedTabIndex = -1;
}

void dtabwidget::setBarVisible(const bool state)
{
  m_tabBar->setVisible(state);
}

void dtabwidget::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  m_spinningIconPath = settings.value("spinningIcon").toString();
  settings.beginGroup("mainWindow");
}

void dtabwidget::mousePressEvent(QMouseEvent *event)
{
  if(event && event->button() == Qt::MidButton)
    {
      if(!dooble::s_settings.value("settingsWindow/closeViaMiddle",
				   true).toBool())
	return;

      int index = m_tabBar->tabAt(event->pos());

      if(index != -1)
	emit closeTab(index);
    }

  QTabWidget::mousePressEvent(event);
}

void dtabwidget::slotBookmark(void)
{
  int index = m_selectedTabIndex;

  if(index == -1)
    return;

  emit bookmark(index);
}

void dtabwidget::slotCloseOtherTabs(void)
{
  int index = m_selectedTabIndex;

  if(index == -1)
    return;

  /*
  ** The slot that's connected to the closeTab() signal issues
  ** deleteLater() calls. Timing should not be a concern.
  */

  for(int i = count() - 1; i > index; i--)
    emit closeTab(i);

  for(int i = index - 1; i >= 0; i--)
    emit closeTab(i);

  m_selectedTabIndex = -1;
}

void dtabwidget::setTabButton(int index)
{
  if(index < 0 || index >= count())
    return;

  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tabBar);

  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;

  QStackedWidget *widget = qobject_cast<QStackedWidget *>
    (m_tabBar->tabButton(index, side));

  if(!widget)
    {
      widget = new QStackedWidget(m_tabBar);

      QLabel *label = new QLabel(widget);
      QPixmap pixmap(16, 16);
      QProgressBar *progressBar = new QProgressBar(widget);

      pixmap.fill(m_tabBar->backgroundRole());
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      label->setPixmap(pixmap);
      progressBar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      progressBar->setFormat("");
      progressBar->setMaximumHeight(18); // 16 + something.
      progressBar->setMaximumWidth(18); // 16 + something.
      progressBar->setOrientation(Qt::Vertical);
      progressBar->setStyleSheet
	("QProgressBar {"
	 "background-color: qlineargradient(x1: 0, y1: 0.5, x2: 0.5, "
	 "y2: 1.0, stop: 0 lightgrey, stop: 1 white); "
	 "border: 1px solid grey; "
	 "border-radius: 0px;"
	 "}"
	 "QProgressBar::chunk {"
	 "background-color: #cd96cd; "
	 "margin: 1px;"
	 "}");
      widget->addWidget(label);
      widget->addWidget(progressBar);
      widget->setContentsMargins(0, 0, 0, 0);
      widget->setCurrentIndex(0);
      widget->setMaximumWidth(progressBar->width());
      widget->setMaximumHeight
	(static_cast<int> (0.60 * qMax(label->height(),
				       progressBar->height())));
    }

  m_tabBar->setTabButton(index, side, 0);
  m_tabBar->setTabButton(index, side, widget);
}

void dtabwidget::animateIndex(const int index, const bool state,
			      const QIcon &icon, const int progress,
			      const bool statusBarIsVisible)
{
  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tabBar);

  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;

  QLabel *label = 0;
  QStackedWidget *widget = qobject_cast<QStackedWidget *>
    (m_tabBar->tabButton(index, side));

  if(widget)
    {
      label = qobject_cast<QLabel *> (widget->widget(0));

      if(progress >= 100 || !state)
	widget->setCurrentIndex(0);
      else if(currentIndex() != index || !statusBarIsVisible)
	{
	  QProgressBar *progressBar = qobject_cast<QProgressBar *>
	    (widget->widget(1));

	  if(progressBar)
	    {
	      progressBar->setToolTip(QString("%1%").arg(progress));
	      progressBar->setValue(progress);
	      progressBar->repaint();
	    }

	  widget->setCurrentIndex(1);
	}
      else
	widget->setCurrentIndex(0);
    }

  if(label)
    {
      if(state)
	{
	  QMovie *movie = label->movie();

	  if(!movie)
	    {
	      movie = new QMovie
		(m_spinningIconPath, QByteArray(), label);
	      movie->setScaledSize(QSize(16, 16));
	      label->setMovie(movie);
	      movie->start();
	    }
	  else
	    {
	      if(movie->state() != QMovie::Running)
		movie->start();
	    }
	}
      else
	{
	  if(label->movie())
	    {
	      label->movie()->stop();
	      label->movie()->deleteLater();
	      label->setMovie(0);
	    }

	  label->setPixmap(icon.pixmap(16, 16));
	}
    }
}

void dtabwidget::dropEvent(QDropEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      QList<QUrl> list(event->mimeData()->urls());

      while(!list.isEmpty())
	emit openLinkInNewTab(list.takeFirst());

      return;
    }

  QTabWidget::dropEvent(event);
}

void dtabwidget::dragMoveEvent(QDragMoveEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      event->accept();
      return;
    }

  QTabWidget::dragMoveEvent(event);
}

void dtabwidget::dragEnterEvent(QDragEnterEvent *event)
{
  if(event && event->source() &&
     event->source()->objectName() == "dooble_history_sidebar" &&
     event->mimeData() && event->mimeData()->hasUrls())
    {
      event->accept();
      return;
    }

  QTabWidget::dragEnterEvent(event);
}

void dtabwidget::slotIconChange(QWidget *tab, const QIcon &icon)
{
  int tabno = indexOf(tab);

  if(tabno >= 0)
    setTabIcon(tabno, icon);
}

void dtabwidget::slotTitleChange(QWidget *tab, const QString &title)
{
  int tabno = indexOf(tab);

  if(tabno >= 0)
    {
      setTabText(tabno, title);
      setTabToolTip(tabno, title);
    }
}

void dtabwidget::tabRemoved(int index)
{
  Q_UNUSED(index);
  quint64 id = 0;
  dooble *dbl = 0;
  QObject *prnt(this);

  do
    {
      prnt = prnt->parent();
      dbl = qobject_cast<dooble *> (prnt);
    }
  while(prnt != 0 && dbl == 0);

  if(dbl)
    id = dbl->id();

  if(dbl)
    {
      dmisc::removeRestorationFiles(dooble::s_id, id);

      for(int i = 0; i < count(); i++)
	{
	  dview *p = qobject_cast<dview *> (widget(i));

	  if(p)
	    p->recordRestorationHistory();
	}
    }
}

void dtabwidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  QTabWidget::mouseDoubleClickEvent(event);
}

void dtabwidget::slotReloadTab(void)
{
  emit reloadTab(m_selectedTabIndex);
}

void dtabwidget::slotStopTab(void)
{
  emit stopTab(m_selectedTabIndex);
}

void dtabwidget::tabInserted(int index)
{
  QTabWidget::tabInserted(index);
}

void dtabwidget::resizeEvent(QResizeEvent *event)
{
  QTabWidget::resizeEvent(event);
}

void dtabwidget::slotJavaScript(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));

      if(p)
	p->setJavaScriptEnabled(action->isChecked());
    }
}

void dtabwidget::slotPrivateCookies(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));

      if(p)
	p->setPrivateCookies(action->isChecked());
    }
}

void dtabwidget::slotViewPrivateCookies(void)
{
  dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));

  if(p)
    p->viewPrivateCookies();
}

void dtabwidget::slotWebPlugins(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));

      if(p)
	p->setWebPluginsEnabled(action->isChecked());
    }
}

void dtabwidget::slotPrivateBrowsing(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action)
    {
      dview *p = qobject_cast<dview *> (widget(m_selectedTabIndex));

      if(p)
	p->setPrivateBrowsingEnabled(action->isChecked());
    }
}

void dtabwidget::slotSetPosition(void)
{
  QString str
    (dooble::s_settings.value("settingsWindow/tabBarPosition", "north").
     toString().toLower().trimmed());
  QTabWidget::TabPosition position = QTabWidget::East;

  if(str == "east")
    position = QTabWidget::East;
  else if(str == "north")
    position = QTabWidget::North;
  else if(str == "south")
    position = QTabWidget::South;
  else if(str == "west")
    position = QTabWidget::West;
  else
    position = QTabWidget::North;

  if(position == tabPosition())
    return;

  setTabPosition(position);
  m_tabBar->setTabPosition(tabPosition());
  m_tabBar->resize(m_tabBar->sizeHint());
}
