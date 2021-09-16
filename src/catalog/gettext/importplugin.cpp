/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky <visnovsky@kde.org>
                2007-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>


  SPDX-License-Identifier: GPL-2.0-or-later

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

*/

#include "lokalize_debug.h"

#include "catalogfileplugin.h"
#include "importplugin_private.h"

#include "gettextstorage.h"

#include <QStringList>
#include <QLinkedList>

#include <kmessagebox.h>

namespace GettextCatalog
{

CatalogImportPlugin::CatalogImportPlugin()
    : _maxLineLength(0)
    , _trailingNewLines(0)
    , _errorLine(0)
    , d(new CatalogImportPluginPrivate)
{
}

CatalogImportPlugin::~CatalogImportPlugin()
{
    delete d;
}

void CatalogImportPlugin::appendCatalogItem(const CatalogItem& item, const bool obsolete)
{
    if (item.msgid().isEmpty())
        return;
    if (obsolete)
        d->_obsoleteEntries.append(item);
    else
        d->_entries.append(item);
}

void CatalogImportPlugin::setCatalogExtraData(const QStringList& data)
{
    d->_catalogExtraData = data;
    d->_updateCatalogExtraData = true;
}

void CatalogImportPlugin::setGeneratedFromDocbook(const bool generated)
{
    d->_generatedFromDocbook = generated;
    d->_updateGeneratedFromDocbook = true;
}

void CatalogImportPlugin::setErrorIndex(const QList<int>& errors)
{
    d->_errorList = errors;
    d->_updateErrorList = true;
}

void CatalogImportPlugin::setHeader(const CatalogItem& item)
{
    d->_header = item;
    d->_updateHeader = true;
}

void CatalogImportPlugin::setCodec(QTextCodec* codec)
{
    d->_codec = codec;
}

ConversionStatus CatalogImportPlugin::open(QIODevice* device, GettextStorage* catalog, int* line)
{
    d->_catalog = catalog;
    startTransaction();

    ConversionStatus result = load(device);

    if (result == OK || result == RECOVERED_PARSE_ERROR || result == RECOVERED_HEADER_ERROR)
        commitTransaction();
    if (line)
        (*line) = _errorLine;
    return result;
}

void CatalogImportPlugin::startTransaction()
{
    d->_updateCodec = false;
    d->_updateCatalogExtraData = false;
    d->_updateGeneratedFromDocbook = false;
    d->_updateErrorList = false;
    d->_updateHeader = false;
    d->_entries.clear();
}

void CatalogImportPlugin::commitTransaction()
{
    GettextStorage* catalog = d->_catalog;

    //catalog->clear();

    // fill in the entries
    QVector<CatalogItem>& entries = catalog->m_entries;
    entries.reserve(d->_entries.count());   //d->_catalog->setEntries( e );
    for (QLinkedList<CatalogItem>::const_iterator it = d->_entries.begin(); it != d->_entries.end(); ++it/*,++i*/)
        entries.append(*it);

    // The codec is specified in the header, so it must be updated before the header is.
    catalog->setCodec(d->_codec);

    catalog->m_catalogExtraData = d->_catalogExtraData;
    catalog->m_generatedFromDocbook = d->_generatedFromDocbook;
    catalog->setHeader(d->_header);
    //if( d->_updateErrorList ) d->_catalog->setErrorIndex(d->_errorList);

    catalog->m_maxLineLength = _maxLineLength;
    catalog->m_trailingNewLines = _trailingNewLines;
}

}
