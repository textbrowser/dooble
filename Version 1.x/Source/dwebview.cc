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

#include <QMouseEvent>
#include <QNetworkProxy>
#include <QWebHitTestResult>
#include <QWebElementCollection>

#include "dmisc.h"
#include "dooble.h"
#include "dwebview.h"

dwebview::dwebview(QWidget *parent):QWebView(parent)
{
  m_allowPopup = false;
  m_lastButtonPressed = Qt::NoButton;
  connect(this,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slotLoadStarted(void)));
}

bool dwebview::checkAndClearPopup(void)
{
  bool allowPopup = m_allowPopup;

  m_allowPopup = false;
  return allowPopup;
}

void dwebview::mousePressEvent(QMouseEvent *event)
{
  /*
  ** We use this method to determine valid popups.
  */

  m_allowPopup = false;

  if(event)
    {
      m_lastButtonPressed = event->button();

      QWebFrame *frame = page()->currentFrame();

      if(frame)
	{
	  QWebHitTestResult hit = frame->hitTestContent(event->pos());

	  if(hit.linkUrl().isValid())
	    m_allowPopup = true;
	  else if(!hit.isNull())
	    {
	      bool found = false;
	      QWebElementCollection collection = frame->
		documentElement().findAll("input[type=button]");

	      foreach(QWebElement element, collection)
		{
		  if(element == hit.element())
		    {
		      found = true;
		      break;
		    }
		}

	      if(!found)
		{
		  collection = frame->
		    documentElement().findAll("input[type=image]");

		  foreach(QWebElement element, collection)
		    {
		      if(element == hit.element())
			{
			  found = true;
			  break;
			}
		    }
		}

	      if(!found)
		{
		  collection = frame->
		    documentElement().findAll("input[type=submit]");

		  foreach(QWebElement element, collection)
		    {
		      if(element == hit.element())
			{
			  found = true;
			  break;
			}
		    }
		}

	      if(found)
		m_allowPopup = true;
	      else
		{
		  foreach(QWebElement element, frame->documentElement().
			  findAll("*"))
		    if(element.geometry().contains(hit.pos()))
		      {
			if(element.attribute("role").toLower() == "link" ||
			   element.attribute("method").toLower() == "post" ||
			   element.hasAttribute("href") ||
			   element.hasAttribute("onclick") ||
			   (element.hasAttribute("id") &&
			    QUrl(element.attribute("id")).isValid()) ||
			   (element.hasAttribute("src") &&
			    QUrl(element.attribute("src")).isValid()))
			  {
			    found = true;
			    break;
			  }
		      }

		  if(found)
		    m_allowPopup = true;
		}
	    }
	}
    }

  QWebView::mousePressEvent(event);
}

Qt::MouseButton dwebview::mouseButtonPressed(void) const
{
  return m_lastButtonPressed;
}

void dwebview::slotLoadStarted(void)
{
  m_lastButtonPressed = Qt::NoButton;
}
