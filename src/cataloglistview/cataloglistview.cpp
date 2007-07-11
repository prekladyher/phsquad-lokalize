/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */


#include "cataloglistview.h"
#include "catalogmodel.h"
#include "catalog.h"

#include <klocale.h>
#include <kdebug.h>

#include <QTreeView>
#include <QModelIndex>
#include <QSortFilterProxyModel>

CatalogTreeView::CatalogTreeView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18n("Message Tree"), parent)
    , m_browser(new QTreeView(this))
    , m_model(new CatalogTreeModel(this,catalog))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    setObjectName("catalogTreeView");
    setWidget(m_browser);

    //connect(catalog,SIGNAL(signalFileLoaded()),m_browser,SLOT(reset()));
    connect(catalog,SIGNAL(signalFileLoaded()),m_model,SIGNAL(modelReset()));

    //connect(m_browser,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    connect(m_browser,SIGNAL(clicked(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    m_browser->setRootIsDecorated(false);
    m_browser->setAllColumnsShowFocus(true);

    m_proxyModel->setSourceModel(m_model);
    m_browser->setModel(m_proxyModel);
    m_browser->setColumnWidth(0,m_browser->columnWidth(0)/3);
    m_browser->setSortingEnabled(true);
    m_browser->sortByColumn(0, Qt::AscendingOrder);
}

CatalogTreeView::~CatalogTreeView()
{
    delete m_browser;
    delete m_model;
}


void CatalogTreeView::slotNewEntryDisplayed(uint entry)
{
    m_browser->setCurrentIndex(m_proxyModel->mapFromSource(m_model->index(entry,0)));
}


void CatalogTreeView::slotItemActivated(const QModelIndex& idx)
{
    DocPosition pos;
    pos.entry=m_proxyModel->mapToSource(idx).row();
//kWarning() << pos.entry << endl;
    emit gotoEntry(pos,0);
}





#include "cataloglistview.moc"
