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

#include "projectmodel.h"
#include "project.h"

#include <klocale.h>
#include <kapplication.h>

//#include <QEventLoop>
#include <QTime>



ProjectModel::ProjectModel()
    : KDirModel()
    , m_dirIcon(KIcon(QLatin1String("inode-directory")))
    , m_poIcon(KIcon(QLatin1String("flag-blue")))
    , m_poComplIcon(KIcon(QLatin1String("flag-green")))
    , m_potIcon(KIcon(QLatin1String("flag-black")))
{
    connect (this,SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&) ),
             this,SLOT(aa()));

    setDirLister(new ProjectLister);
}


/**
 * we use QRect to pass data through QVariant tunnel
 *
 * order is tran,  untr, fuzzy
 *          left() top() width()
 *
 * 32 is a secret code that we use to say that info isnot ready yet
 */
QVariant ProjectModel::data ( const QModelIndex& index, int role) const
{
    const ProjectModelColumns& column=(ProjectModelColumns)index.column();

    if (column<Graph)
    {
        if (role==Qt::DecorationRole)
        {
            KFileItem item(itemForIndex(index));
            if (item.isDir())
                return m_dirIcon;
            QString path(item.url().path());
            if (path.endsWith(".po"))
                return m_poIcon;
            if (path.endsWith(".pot"))
                return m_potIcon;
        }

        return KDirModel::data(index,role);
    }

    if (role!=Qt::DisplayRole)
        return QVariant();
//     kWarning()<<"+++++++++++++00";
//     kWarning()<<"+++++++++++++01";
    KFileItem item(itemForIndex(index));
    //we handle dirs in special way for all columns left
    if (item.isDir())
    {
        if (column>=Graph&&column<=Untranslated)
        {
            int untranslated=0;
            int translated=0;
            int fuzzy=0;

//takes 5 mseconds on kdebase
#if 0
            //first, check if we have already calcalated real stats
            if (!metaInfo.item("translation.translated").value().isNull())
            {
                translated=metaInfo.item("translation.translated").value().toInt();
                untranslated=metaInfo.item("translation.untranslated").value().toInt();
                fuzzy=metaInfo.item("translation.fuzzy").value().toInt();
                if (fuzzy+untranslated+translated>0)
                    return QRect(translated,untranslated,fuzzy,0);
            }
#endif
//still, we cache data because it might be needed for recursive stats
            KFileMetaInfo metaInfo(item.metaInfo(false));

            //now we try to iterate through dir's children and get the sums
            //if the children are already have been scanned

            int count=rowCount(index);
            //if (parent.isValid()
            int i=0;
            bool infoIsFull=true;
            //QTime a;a.start();
            for (;i<count;++i)
            {
//                 QModelIndex childIndex(index.child(i,0));
//                 KFileItem* childItem(itemForIndex(childIndex));
//                 //force population of metainfo. kfilemetainfo's internal is a shit
//             if (item->metaInfo(false).keys().empty()
//             && item->url().fileName().endsWith(".po"))
//             {
//                 item->setMetaInfo(KFileMetaInfo( item->url() ));
//             }

//                 kWarning()<<"-----------1----"<<i;
                const KFileMetaInfo& childMetaInfo(itemForIndex(index.child(i,0)).metaInfo(false));

//                 kWarning()<<"-----------1";
                if (!childMetaInfo.item("translation.translated").value().isNull())
                {
                    translated+=childMetaInfo.item("translation.translated").value().toInt();
                    untranslated+=childMetaInfo.item("translation.untranslated").value().toInt();
                    fuzzy+=childMetaInfo.item("translation.fuzzy").value().toInt();
                }
                else if (hasChildren(index.child(i,0)))
                {
                    //"inode/directory"
                    infoIsFull=false;
                }
//                 kWarning()<<"-----------2";
            }
//             kWarning()<<"/////////////////"<<a.elapsed();
            if (infoIsFull&&(untranslated+translated+fuzzy))
            {
//                 kWarning()<<"-----------3";
//                 KFileMetaInfo dirInfo(item->metaInfo(false));
#if 1
                metaInfo.item("translation.untranslated").setValue(untranslated);
                metaInfo.item("translation.translated").setValue(translated);
                metaInfo.item("translation.fuzzy").setValue(fuzzy);
                item.setMetaInfo(metaInfo);
#endif
//                 kWarning()<<"-----------4";
                switch(column)
                {
                    case Graph:
                        return QRect(translated,untranslated,fuzzy,0);
                    case Total:
                        return translated+untranslated+fuzzy;
                    case Translated:
                        return translated;
                    case Fuzzy:
                        return fuzzy;
                    case Untranslated:
                        return untranslated;
                    default://shut up stupid compiler
                        return 0;
                }
            }
            else if(column==Graph)
                    return QRect(0,0,0,32);

        }
        //else -->other columns handling
        //TODO make smth cool
        return QVariant();
    }
    //ok, so item is no dir
    const KFileMetaInfo& metaInfo(item.metaInfo(false));

    static const char* columnToMetaInfoItem[ProjectModelColumnCount]={
                                "",//KDirModel::Name
                                "",//Graph = 1/*KDirModel::ColumnCount*/,
                                "",//Total,
                                "translation.translated",//Translated,
                                "translation.fuzzy",//Fuzzy,
                                "translation.untranslated",//Untranslated,
                                "translation.source_date",//SourceDate,
                                "translation.translation_date",//TranslationDate,
                                "translation.last_translator"//LastTranslator,
                                };
/*    switch(index.column())
    {
        case Graph:
        {*/
    if (column>Total)
    {
        return metaInfo.item(
                columnToMetaInfoItem[column]
                            ).value();
    }
    else if (column==Graph)
    {
        if (metaInfo.item("translation.untranslated").value().isNull())
            return QRect(0,0,0,32);

        return QRect(metaInfo.item("translation.translated").value().toInt(),
                        metaInfo.item("translation.untranslated").value().toInt(),
                        metaInfo.item("translation.fuzzy").value().toInt(),
                        0
                    );
    }
    else if (column==Total)
    {
        if (metaInfo.item("translation.untranslated").value().isNull())
            return QVariant();

        return metaInfo.item("translation.untranslated").value().toInt()
                +metaInfo.item("translation.translated").value().toInt()
                +metaInfo.item("translation.fuzzy").value().toInt();
    }
    return KDirModel::data(index,role);
}

QVariant ProjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case Graph:
            return i18nc("@title:column","Graph");
        case Total:
            return i18nc("@title:column","Total");
        case Translated:
            return i18nc("@title:column","Translated");
        case Fuzzy:
            return i18nc("@title:column","Fuzzy");
        case Untranslated:
            return i18nc("@title:column","Untranslated");
        case TranslationDate:
            return i18nc("@title:column","Last Translation");
        case SourceDate:
            return i18nc("@title:column","Template Revision");
        case LastTranslator:
            return i18nc("@title:column","Last Translator");
    }

    return KDirModel::headerData(section, orientation, role);

}



ProjectLister::ProjectLister(QObject *parent)
    : KDirLister(parent)
    , m_templates(new KDirLister (this))
    , m_reactOnSignals(true)
{
    connect(m_templates,SIGNAL(newItems(QList<KFileItem>)),
            this, SLOT(slotNewTemplItems(QList<KFileItem>)));
//     connect(m_templates,SIGNAL(newItems(KFileItemList)),
//             this, SLOT(slotNewTemplItems(KFileItemList)));
    connect(m_templates, SIGNAL(deleteItem(const KFileItem&)),
            this, SLOT(slotDeleteTemplItem(const KFileItem&)));
    connect(m_templates, SIGNAL(refreshItems(QList<KFileItem>)),
            this, SLOT(slotRefreshTemplItems(QList<KFileItem>)));
//     connect(m_templates, SIGNAL(refreshItems(KFileItemList)),
//             this, SLOT(slotRefreshTemplItems(KFileItemList)));

    m_templates->setNameFilter("*.pot");
    setNameFilter("*.po *.pot");
//         connect( m_templates, SIGNAL(clear()),
//                 this, SLOT(slotClear()) );
    //NOTE ??
//     connect(m_templates, SIGNAL(clear()),
//             this, SLOT(clearTempl()));
// 
//     connect(m_templates, SIGNAL(clear(const KUrl&)),
//             this, SLOT(clearTempl(const KUrl&)));

    connect(this, SIGNAL(completed(const KUrl&)),
            this, SLOT(slotCompleted(const KUrl&)),
                        Qt::QueuedConnection);

    connect(this,SIGNAL(newItems(KFileItemList)),
            this, SLOT(slotNewItems(KFileItemList)));
    connect(this,SIGNAL(refreshItems(KFileItemList)),
            this, SLOT(slotNewItems(KFileItemList)));
    connect(this,SIGNAL(deleteItem(KFileItem)),
            this, SLOT(slotDeleteItem(KFileItem)));


}

bool ProjectLister::openUrl(const KUrl &_url, bool _keep, bool _reload)
{
    if (QFile::exists(_url.path(KUrl::RemoveTrailingSlash)))
        return KDirLister::openUrl(_url,_keep,_reload);

    slotCompleted(_url);
    return true;
}

bool ProjectLister::openUrlRecursive(const KUrl &_url, bool _keep, bool _reload)
{
    m_recursiveUrls.append(_url);

    return openUrl(_url,_keep,_reload);
}

/* doesnt handle .po to .po_t_ */
static bool poToPot(QString& path)
{
    if (path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty())
        return false;

    path.replace(Project::instance()->poDir(),Project::instance()->potDir());
    return true;
}

/* doesnt handle .po to .po_t_ */
static bool potToPo(QString& path)
{
    if (path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty())
        return false;

    path.replace(Project::instance()->potDir(),Project::instance()->poDir());
    return true;
}

void ProjectLister::slotCompleted(const KUrl& _url)
{
    kWarning()<<_url;
//     kWarning()<<"-";
//     kWarning()<<"-";
//     kWarning()<<_url;

    QString path(_url.path(KUrl::RemoveTrailingSlash));
    if (!poToPot(path))
        return;
    if (QFile::exists(path)&&!m_listedTemplDirs.contains(path))
    {
        if (m_templates->openUrl(KUrl::fromPath(path),true,true))
            m_listedTemplDirs.insert(path,true);
    }


    //recursive opening
    int pos=0;
    if ((pos=m_recursiveUrls.indexOf(_url))!=-1)
    {
        m_recursiveUrls.removeAt(pos);
        const KFileItemList& list(itemsForDir(_url));
        int i=list.size();
        while(--i>=0)
        {
            if (list.at(i)->isDir())
                openUrlRecursive(list.at(i)->url());
        }

    }
}

//there are limitations by levels
void ProjectLister::slotNewItems(const KFileItemList& list)
//we wanna add metainfo to original items
//void ProjectLister::slotNewItems(const QList<KFileItem>& list)
{
    if (!m_reactOnSignals)
        return;
    QTime a;a.start();
    kWarning()<<"start";
    m_reactOnSignals=false;
    //this code
    //1. sets metainfo
    //2. removes template items from the view if they have been translated after initial folder scanning

    //we don't wanna emit deletion of files if their folders are being removed too
    QList<KFileItem> templDirsToRemove;//stores real paths
    //QList<KFileItem> templFilesToRemove;//stores real paths
    //QList<KFileItem> removedTemplDirs;
    int i=list.size();
    while(--i>=0)
    {
        QString path(list.at(i)->url().path(KUrl::RemoveTrailingSlash));
        if (path.endsWith(".po"))
        {
            //force population of metainfo. kfilemetainfo's internal is a shit
            if (list.at(i)->metaInfo(false).keys().empty())
                list.at(i)->setMetaInfo(KFileMetaInfo( list.at(i)->url() ));
        }
//         else
//         {
//             KUrl u(list.at(i)->url().upUrl());
//             u.adjustPath(KUrl::RemoveTrailingSlash);
//             if (m_recursiveUrls.contains(u.path()))
//                 openUrlRecursive(u,true,false);
//             else
//                 kWarning()<<" shit shit shit";
//         }

        //maybe this is update and new translations have appeared
        //so remove corresponding template entries in favor of 'em
        if (poToPot(path))
        {
            QString potPath(path+'t');//.pot => .po
            kDebug()<<"m_templates->findByUrl()"<<potPath;
            KFileItem* pot(m_templates->findByUrl(KUrl::fromPath(potPath)));
            if (pot)
            {
                kDebug()<<"m_templates->findByUrl()"<<potPath<<"ok";
//                 if (po)
//                 {
//                     if (po->metaInfo(false).item("translation.source_date").value()
//                         ==list.at(i)->metaInfo(false).item("translation.source_date").value())
//                         po->metaInfo(false).item("translation.templ").addValue("ok");
//                     else
//                         po->metaInfo(false).item("translation.templ").addValue("outdated");
//                 }
                if (!m_hiddenTemplItems.contains(*pot))
                {
                    m_hiddenTemplItems.append(*pot);
                    //m_hiddenTemplItems.append(templFilesToRemove.at(i));
                    kDebug()<<"templFilesToRemove"<<pot->url();
    
                    //now create KFileItem that should be deleted
                    KFileItem po(*pot);
                    QString path(po.url().path(KUrl::RemoveTrailingSlash));
                    if (potToPo(path))
                    {
                        po.setUrl(KUrl::fromPath(path));
                        kWarning()<<"emit delete 2"<<path;
                        emit deleteItem(po);
                    }
                    //templFilesToRemove.append(*pot);
                    //kWarning()<<"fil"<<po->url();
                }
            }
            else if ((pot=m_templates->findByUrl(KUrl::fromPath(path))))
            {
                //dir, the name part is the same

                //removedTemplDirs.append(*pot);
                //m_hiddenTemplItems.append(*pot);
                templDirsToRemove.append(*pot);

/*                //now create KFileItem that should be deleted
                KFileItem po(*pot);
                po.setUrl(list.at(i)->url());
                kWarning()<<"emit delete 1"<<list.at(i)->url().path(KUrl::RemoveTrailingSlash);
                emit deleteItem(po);*/
                //delete m_items.value(po);
                //m_items.erase(m_items.find(po));
            }
            //kWarning()<<path;
        }
    }

    //find files of dirs being removed
    i=templDirsToRemove.size();
    while(--i>=0)
    {
        m_hiddenTemplItems.append(templDirsToRemove.at(i));
        //now create KFileItem that should be deleted
        KFileItem po(templDirsToRemove.at(i));
        QString path(po.url().path(KUrl::RemoveTrailingSlash));
        if (potToPo(path))
        {
            po.setUrl(KUrl::fromPath(path));
            kWarning()<<"emit delete 1"<<path;
            emit deleteItem(po);
            if (templDirsToRemove.at(i).isDir())//sanity
            {
                //TODO levels
                KFileItemList li(itemsForDir(templDirsToRemove.at(i).url()));
                int j=li.size();
                while(--j>=0)
                {
                    if (!m_hiddenTemplItems.contains(*li.at(j)))
                    m_hiddenTemplItems.append(*li.at(j));
                }
            }
        }

/*        QList<KFileItem>::const_iterator it = removedTemplDirs.constBegin();
        while (it != removedTemplDirs.constEnd())
        {
//             KUrl u(it->url());
//             u.adjustPath(KUrl::AddTrailingSlash);// will give negative result on files.. but that's ok
//             u=KUrl::fromPath(u.path());
//             KUrl u2
//             kDebug()<<"m_hiddenTemplItems.at.url("<<u<<").isParentOf"
//                     <<templFilesToRemove.at(i).url();
//             if (u.isParentOf(templFilesToRemove.at(i).url()));
            //KUrl's isParentOf sucks
            if (templFilesToRemove.at(i).url().path().contains(it->url().path()))
            {
                kDebug()<<"yes. path:"
                        <<templFilesToRemove.at(i).url().path()
                        <<"contains"
                        <<it->url().path();
                break;
            }
            ++it;
        }

        if (it == m_hiddenTemplItems.constEnd())*/
        {

//             emit deleteItem(m_items.value(filesToRemove.at(i)));
//             delete m_items.value(filesToRemove.at(i));
//             m_items.erase(m_items.find(filesToRemove.at(i)));
        }

    }

    m_reactOnSignals=true;
    kWarning()<<"end"<<a.elapsed();
}

void ProjectLister::slotDeleteItem(const KFileItem& item)
{
    kDebug()<<"|||||";
    if (!m_reactOnSignals)
        return;
    kDebug()<<"!!!!!!!!!!!";

    QString path(item.url().path(KUrl::RemoveTrailingSlash));
    if (!poToPot(path))
        return;
    QString potPath=path+'t';

    KFileItem* pot(m_templates->findByUrl(KUrl::fromPath(potPath)));
    if (pot||(pot=m_templates->findByUrl(KUrl::fromPath(path))))
    {
        kDebug()<<"found in m_templates";

        int pos;
        if ((pos=m_hiddenTemplItems.indexOf(*pot))!=-1)
        {
            m_hiddenTemplItems.removeAt(pos);
            kDebug()<<"found in hidden";

            QString poPath(pot->url().path(KUrl::RemoveTrailingSlash));
            if (!potToPo(poPath))
                return;
            KFileItem po(*pot);
            po.setUrl(KUrl::fromPath(poPath));

            m_reactOnSignals=false;
            QList<KFileItem> list;
            list.append(po);
            emit newItems(list);
            list.clear();

            //ok, but what if the whole dir was deleted?
            if (pot->isDir())
            {
                kDebug()<<"///////isdir///////";
                //TODO levels
                KFileItemList li(m_templates->itemsForDir(pot->url()));
                int aa=li.size();
                kDebug()<<"itemsForDir"
                        <<pot->url()
                        <<aa;
                while(--aa>=0)
                {
                    poPath=li.at(aa)->url().path(KUrl::RemoveTrailingSlash);
                    if (!potToPo(poPath))
                        continue;
                    KFileItem po(*li.at(aa));
                    po.setUrl(KUrl::fromPath(poPath));

                    kDebug()<<"add"<<poPath;
                    list.append(po);
                    if ((pos=m_hiddenTemplItems.indexOf(*li.at(aa)))!=-1)
                    {
                        m_hiddenTemplItems.removeAt(pos);
                    }
                }
            }
            if (!list.isEmpty())
                emit newItems(list);
            m_reactOnSignals=true;
        }
    }
}
/*
//nono
void ProjectLister::slotRefreshItems(KFileItemList list)
{
}*/




void ProjectLister::clearTempl(const KUrl& u)
{
    
}

void ProjectLister::clearTempl()
{
    //kWarning()<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
}

// void ProjectLister::slotNewTemplItems(const KFileItemList& list)
// {
//     int i=list.size();
//     while(--i>=0)
//         list.at(i)->setMetaInfo(KFileMetaInfo(list.at(i)->url()));
// }

void ProjectLister::slotNewTemplItems(QList<KFileItem> list)
{
    int i;
    QTime a;a.start();
    kDebug()<<"start";
    i=list.size();
    while(--i>=0)
        kDebug()<<"got:"<<list.at(i).url();

    i=list.size();
    while(--i>=0)
    {
        QString path(list.at(i).url().path(KUrl::RemoveTrailingSlash));
        if (potToPo(path))
        {
            QString poPath(path);
            bool isDir=list.at(i).isDir();
            if (!isDir)
                poPath.chop(1);//.pot => .po

            KFileItem* po=findByUrl(KUrl::fromPath(poPath));
            if (po/*||(po=findByUrl(KUrl::fromPath(path)))*/)
            {
                kDebug()<<"+++++++++aaaaaaaaaaaaa1"<<po->url();
//                 if (po)
//                 {
//                     if (po->metaInfo(false).item("translation.source_date").value()
//                         ==list.at(i)->metaInfo(false).item("translation.source_date").value())
//                         po->metaInfo(false).item("translation.templ").addValue("ok");
//                     else
//                         po->metaInfo(false).item("translation.templ").addValue("outdated");
//                 }

                m_hiddenTemplItems.append(list.at(i));
                list.removeAt(i);
            }
            else
            {
                kDebug()<<"2";
                if (!isDir)
                    list.at(i).setMetaInfo(KFileMetaInfo(list.at(i).url()));
                kDebug()<<"not foundByUrl:"<<list.at(i).url()
                        <<"path:"<<path;
                //causes deep copy
                list[i].setUrl(KUrl::fromPath(path));
                /*KFileItem* a=new KFileItem(*list.at(i));
                m_items.insert(list.at(i),a);
                a->setUrl(KUrl::fromPath(path));
                list[i]=a;*/

            }
            //kWarning()<<path;
        }
        else
        {
            kWarning()<<"strange";
            m_hiddenTemplItems.append(list.at(i));
            list.removeAt(i);
        }
    }
//     kWarning()<<"ddd";
    i=list.size();
    while(--i>=0)
        kDebug()<<"going to emit as new:"<<list.at(i).url();

    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit newItems(list);
        m_reactOnSignals=true;
    }
    kDebug()<<"end"<<a.elapsed();
}

void ProjectLister::slotDeleteTemplItem(const KFileItem& item)
{
    kDebug()<<item.url();

    if (!m_hiddenTemplItems.contains(item))
       //&&m_items.contains(item))
    {
        QString path(item.url().path(KUrl::RemoveTrailingSlash));
        if (!potToPo(path))
            return;
        //sanity? findByUrl(KUrl::fromPath(path));

        m_reactOnSignals=false;
        //now create KFileItem that should be deleted
        KFileItem po(item);
//         if (path.endsWith(".pot"))
//             path.chop(1); //==>NOPE...
        po.setUrl(KUrl::fromPath(path));
        kWarning()<<"emit delete 3"<<path;
        emit deleteItem(po);

//        emit deleteItem(m_items.value(item));
        m_reactOnSignals=true;

    }
}


// void ProjectLister::slotRefreshTemplItems(const KFileItemList& list)
// {
//     int i=list.size();
//     while(--i>=0)
//     {
//         if (!m_hiddenTemplItems.contains(list.at(i)))
//             list.at(i)->setMetaInfo(KFileMetaInfo(list.at(i)->url()));
//     }
// }

void ProjectLister::slotRefreshTemplItems(QList<KFileItem> list)
{
    int i=list.size();
    while(--i>=0)
        kDebug()<<"got:"<<list.at(i).url();

    i=list.size();
    while(--i>=0)
    {
        if (!m_hiddenTemplItems.contains(list.at(i)))
        {
            list.at(i).setMetaInfo(KFileMetaInfo(list.at(i).url()));

            QString path(list.at(i).url().path(KUrl::RemoveTrailingSlash));
            if (!potToPo(path))
                return;
            list[i].setUrl(KUrl::fromPath(path));
        }
    }


    i=list.size();
    while(--i>=0)
        kWarning()<<"going to refresh:"<<list.at(i).url();

//     kWarning()<<"ddd";
    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit refreshItems(list);
        m_reactOnSignals=true;
    }
//     kWarning()<<"end";
}

