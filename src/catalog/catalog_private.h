/* ****************************************************************************
  This file is part of KAider
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>
		2007	  by Nick Shaforostoff <shafff@ukr.net>


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

#ifndef CatalogPrivate_H
#define CatalogPrivate_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <kurl.h>

// #include "msgfmt.h"
// #include "catalogsettings.h"
#include "catalogitem.h"
// #include "regexpextractor.h"
// #include "kbabel_export.h"

class QTextStream;
class QTextCodec;

class CatalogPrivate
{

public:

    /** url of the po-file, that belongs to this catalog */
    KUrl _url;
    QString _packageName;
    QString _packageDir;

    /** holds the entries in the catalog */
    QVector<CatalogItem> _entries;
    /** The header of the po-file. */
    CatalogItem _header;
    /** list of obsolete entries */
    QLinkedList<CatalogItem> _obsoleteEntries;

    /** identification string for used import filter*/
    QString _importID;
    QString _mimeTypes;

    QTextCodec *fileCodec;

    QStringList msgidDiffList;

    int _numberOfPluralForms;
    QString _language;
    QString _langCode;
    QString _emptyStr;

    //for wrapping
    short _maxLineLength;

    bool _readOnly;
    bool _generatedFromDocbook;
    bool _active;
    bool _stop;

    QList<uint> _fuzzyIndex;
    QList<uint> _untransIndex;
    QList<uint> _errorIndex;

    QList<uint> _bookmarkIndex;

    QStringList _catalogExtraData;

    //for undo/redo
    DocPosition _posBuffer;


   explicit CatalogPrivate(/*Project::Ptr project*/) : 
	//_packageName( QString::null ), _packageDir( QString::null ),
	//_header (project), 
	_mimeTypes( "text/plain" ),
	fileCodec(0),
	_numberOfPluralForms(-1),
	_readOnly(false), 
	_generatedFromDocbook(false), 
	_active(false), 
	_stop(false)
   {
   }
};


#endif //CatalogPrivate_H