/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "glossary.h"

#include "lokalize_debug.h"

#include "stemming.h"
// #include "tbxparser.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "domroutines.h"

#include <QStringBuilder>
#include <QElapsedTimer>
#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QApplication>

#include <klocalizedstring.h>

using namespace GlossaryNS;

static const QString defaultLang = QStringLiteral("en_US");
static const QString xmlLang = QStringLiteral("xml:lang");
static const QString ntig = QStringLiteral("ntig");
static const QString tig = QStringLiteral("tig");
static const QString termGrp = QStringLiteral("termGrp");
static const QString langSet = QStringLiteral("langSet");
static const QString term = QStringLiteral("term");
static const QString id = QStringLiteral("id");



QList<QByteArray> Glossary::idsForLangWord(const QString& lang, const QString& word) const
{
    return idsByLangWord[lang].values(word);
}


Glossary::Glossary(QObject* parent)
    : QObject(parent)
    , m_clean(true)
{
}


//BEGIN DISK
bool Glossary::load(const QString& newPath)
{
    QElapsedTimer a; a.start();
//BEGIN NEW
    QIODevice* device = new QFile(newPath);
    if (!device->open(QFile::ReadOnly | QFile::Text)) {
        delete device;
        //return;
        device = new QBuffer();
        static_cast<QBuffer*>(device)->setData(QByteArray(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<!DOCTYPE martif PUBLIC \"ISO 12200:1999A//DTD MARTIF core (DXFcdV04)//EN\" \"TBXcdv04.dtd\">\n"
                "<martif type=\"TBX\" xml:lang=\"en_US\">\n"
                "    <text>\n"
                "        <body>\n"
                "        </body>\n"
                "    </text>\n"
                "</martif>\n"
                                               ));
    }

    QXmlSimpleReader reader;
    reader.setFeature("http://qt-project.org/xml/features/report-whitespace-only-CharData", true);
    reader.setFeature("http://xml.org/sax/features/namespaces", false);
    QXmlInputSource source(device);

    QDomDocument newDoc;
    QString errorMsg;
    int errorLine;//+errorColumn;
    bool success = newDoc.setContent(&source, &reader, &errorMsg, &errorLine/*,errorColumn*/);

    delete device;

    if (!success) {
        qCWarning(LOKALIZE_LOG) << errorMsg;
        return false; //errorLine+1;
    }
    clear();//does setClean(true);
    m_path = newPath;
    m_doc = newDoc;

    //QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    m_entries = m_doc.elementsByTagName(QStringLiteral("termEntry"));
    for (int i = 0; i < m_entries.size(); i++)
        hashTermEntry(m_entries.at(i).toElement());
    m_idsForEntriesById = m_entriesById.keys();

//END NEW
#if 0
    TbxParser parser(this);
    QXmlSimpleReader reader1;
    reader1.setContentHandler(&parser);

    QFile file(p);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader1.parse(xmlInputSource))
        qCWarning(LOKALIZE_LOG) << "failed to load " << path;
#endif
    Q_EMIT loaded();

    if (a.elapsed() > 50) qCDebug(LOKALIZE_LOG) << "glossary loaded in" << a.elapsed();

    return true;
}

bool Glossary::save()
{
    if (m_path.isEmpty())
        return false;

    QFile* device = new QFile(m_path);
    if (!device->open(QFile::WriteOnly | QFile::Truncate)) {
        device->deleteLater();
        return false;
    }
    QTextStream stream(device);
    m_doc.save(stream, 2);

    device->deleteLater();

    setClean(true);
    return true;
}

void Glossary::setClean(bool clean)
{
    m_clean = clean;
    Q_EMIT changed();//may be emitted multiple times in a row. so what? :)
}

//END DISK

//BEGIN MODEL
#define FETCH_SIZE 128

void GlossarySortFilterProxyModel::setFilterRegExp(const QString& s)
{
    if (!sourceModel())
        return;

    //static const QRegExp lettersOnly("^[a-z]");
    QSortFilterProxyModel::setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    QSortFilterProxyModel::setFilterRegExp(s);

    fetchMore(QModelIndex());
}

void GlossarySortFilterProxyModel::fetchMore(const QModelIndex&)
{
    int expectedCount = rowCount() + FETCH_SIZE / 2;
    while (rowCount(QModelIndex()) < expectedCount && sourceModel()->canFetchMore(QModelIndex())) {
        sourceModel()->fetchMore(QModelIndex());
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    }
}

GlossaryModel::GlossaryModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_visibleCount(0)
    , m_glossary(Project::instance()->glossary())
{
    connect(m_glossary, &Glossary::loaded, this, &GlossaryModel::forceReset);
}

void GlossaryModel::forceReset()
{
    beginResetModel();
    m_visibleCount = 0;
    endResetModel();
}

bool GlossaryModel::canFetchMore(const QModelIndex&) const
{
    return false;//!parent.isValid() && m_glossary->size()!=m_visibleCount;
}

void GlossaryModel::fetchMore(const QModelIndex& parent)
{
    int newVisibleCount = qMin(m_visibleCount + FETCH_SIZE, m_glossary->size());
    beginInsertRows(parent, m_visibleCount, newVisibleCount - 1);
    m_visibleCount = newVisibleCount;
    endInsertRows();
}

int GlossaryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_glossary->size();//m_visibleCount;
}

QVariant GlossaryModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    //case ID: return i18nc("@title:column","ID");
    case English: return i18nc("@title:column Original text", "Source");;
    case Target: return i18nc("@title:column Text in target language", "Target");
    case SubjectField: return i18nc("@title:column", "Subject Field");
    }
    return QVariant();
}

QVariant GlossaryModel::data(const QModelIndex& index, int role) const
{
    //if (role==Qt::SizeHintRole)
    //    return QVariant(QSize(50, 30));

    if (role != Qt::DisplayRole)
        return QVariant();

    static const QString nl = QStringLiteral(" ") + QChar(0x00B7) + ' ';
    static Project* project = Project::instance();
    Glossary* glossary = m_glossary;

    QByteArray id = glossary->id(index.row());
    switch (index.column()) {
    case ID:      return id;
    case English: return glossary->terms(id, project->sourceLangCode()).join(nl);
    case Target:  return glossary->terms(id, project->targetLangCode()).join(nl);
    case SubjectField: return glossary->subjectField(id);
    }
    return QVariant();
}

/*
QModelIndex GlossaryModel::index (int row,int column,const QModelIndex& parent) const
{
    return createIndex (row, column);
}
*/

int GlossaryModel::columnCount(const QModelIndex&) const
{
    return GlossaryModelColumnCount;
}

/*
Qt::ItemFlags GlossaryModel::flags ( const QModelIndex & index ) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    //if (index.column()==FuzzyFlag)
    //    return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
    //return QAbstractItemModel::flags(index);
}
*/


//END MODEL general (GlossaryModel continues below)

//BEGIN EDITING

QByteArray Glossary::generateNewId()
{
    // generate unique ID
    int idNumber = 0;
    QList<int> busyIdNumbers;

    QString authorId(Settings::authorName().toLower());
    authorId.replace(' ', '_');
    QRegExp rx('^' + authorId + QStringLiteral("\\-([0-9]*)$"));


    for (const QByteArray& id : qAsConst(m_idsForEntriesById)) {
        if (rx.exactMatch(QString::fromLatin1(id)))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    int i = removedIds.size();
    while (--i >= 0) {
        if (rx.exactMatch(QString::fromLatin1(removedIds.at(i))))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    if (!busyIdNumbers.isEmpty()) {
        std::sort(busyIdNumbers.begin(), busyIdNumbers.end());
        while (busyIdNumbers.contains(idNumber))
            ++idNumber;
    }

    return authorId.toLatin1() + '-' + QByteArray::number(idNumber);
}

QStringList Glossary::subjectFields() const
{
    QSet<QString> result;
    for (const QByteArray& id : m_idsForEntriesById)
        result.insert(subjectField(id));
    return result.values();
}

QByteArray Glossary::id(int index) const
{
    if (index < m_idsForEntriesById.size())
        return m_idsForEntriesById.at(index);
    return QByteArray();
}

QStringList Glossary::terms(const QByteArray& id, const QString& language) const
{
    QString minusLang = language; minusLang.replace('_', '-');
    QStringRef soleLang = language.leftRef(2);
    QStringList result;
    QDomElement n = m_entriesById.value(id).firstChildElement(langSet);
    while (!n.isNull()) {
        QString lang = n.attribute(xmlLang, defaultLang);

        if (language == lang || minusLang == lang || soleLang == lang) {
            QDomElement ntigElem = n.firstChildElement(ntig);
            while (!ntigElem.isNull()) {
                result << ntigElem.firstChildElement(termGrp).firstChildElement(term).text();
                ntigElem = ntigElem.nextSiblingElement(ntig);
            }
            QDomElement tigElem = n.firstChildElement(tig);
            while (!tigElem.isNull()) {
                result << tigElem.firstChildElement(term).text();
                tigElem = tigElem.nextSiblingElement(tig);
            }
        }

        n = n.nextSiblingElement(langSet);
    }

    return result;
}

// QDomElement ourLangSetElement will reference the lang tag we want (if it exists)
static void getElementsForTermLangIndex(QDomElement termEntry, QString& lang, int index,
                                        QDomElement& ourLangSetElement,
                                        QDomElement& tigElement, //<-- can point to <ntig> as well
                                        QDomElement& termElement)
{
    QString minusLang = lang; minusLang.replace('_', '-');
    QStringRef soleLang = lang.leftRef(2);

    QDomElement n = termEntry.firstChildElement(langSet);
    QDomDocument document = n.ownerDocument();
    int i = 0;
    while (!n.isNull()) {
        QString nLang = n.attribute(xmlLang, defaultLang);

        if (lang == nLang || minusLang == nLang || soleLang == nLang) {
            ourLangSetElement = n;
            QDomElement ntigElem = n.firstChildElement(ntig);
            while (!ntigElem.isNull()) {
                if (i == index) {
                    tigElement = ntigElem;
                    termElement = ntigElem.firstChildElement(termGrp).firstChildElement(term);
                    return;
                }
                ntigElem = ntigElem.nextSiblingElement(ntig);
                i++;
            }
            QDomElement tigElem = n.firstChildElement(tig);
            while (!tigElem.isNull()) {
                //qCDebug(LOKALIZE_LOG)<<i<<tigElem.firstChildElement(term).text();
                if (i == index) {
                    tigElement = tigElem;
                    termElement = tigElem.firstChildElement(term);
                    return;
                }
                tigElem = tigElem.nextSiblingElement(tig);
                i++;
            }
        }
        n = n.nextSiblingElement(langSet);
    }
}


void Glossary::setTerm(const QByteArray& id, QString lang, int index, const QString& termText)
{
    setClean(false);

    QDomElement ourLangSetElement; //will reference the lang we want if it exists
    QDomElement tigElement;
    QDomElement termElement;
    getElementsForTermLangIndex(m_entriesById.value(id), lang, index,
                                ourLangSetElement, tigElement, termElement);

    if (!termElement.isNull()) {
        setText(termElement, termText);
        return;
    }
    QDomElement entry = m_entriesById.value(id);
    QDomDocument document = entry.ownerDocument();
    if (ourLangSetElement.isNull()) {
        ourLangSetElement = entry.appendChild(document.createElement(langSet)).toElement();
        lang.replace('_', '-');
        ourLangSetElement.setAttribute(xmlLang, lang);
    }
    /*
        QDomElement ntigElement=ourLangSetElement.appendChild( document.createElement(ntig)).toElement();
        QDomElement termGrpElement=ntigElement.appendChild( document.createElement(termGrp)).toElement();
        termElement=termGrpElement.appendChild( document.createElement(term)).toElement();
        termElement.appendChild( document.createTextNode(termText));
    */
    tigElement = ourLangSetElement.appendChild(document.createElement(tig)).toElement();
    termElement = tigElement.appendChild(document.createElement(term)).toElement();
    termElement.appendChild(document.createTextNode(termText));
}

void Glossary::rmTerm(const QByteArray& id, QString lang, int index)
{
    setClean(false);

    QDomElement ourLangSetElement; //will reference the lang we want if it exists
    QDomElement tigElement;
    QDomElement termElement;
    getElementsForTermLangIndex(m_entriesById.value(id), lang, index,
                                ourLangSetElement, tigElement, termElement);

    if (!tigElement.isNull())
        ourLangSetElement.removeChild(tigElement);
}


static QDomElement firstDescripElemForLang(QDomElement termEntry, const QString& lang)
{
    if (lang.isEmpty())
        return termEntry.firstChildElement(QStringLiteral("descrip"));

    QString minusLang = lang;
    minusLang.replace('_', '-');

    //disable this for now
    //bool enUSLangGiven=defaultLang==lang; //treat en_US and en as equal
    //bool enLangGiven="en"==lang; //treat en_US and en as equal

    QDomElement n = termEntry.firstChildElement(langSet);
    while (!n.isNull()) {
        const QString& curLang = n.attribute(xmlLang);
        if (curLang == lang || curLang == minusLang
            //|| (enUSLangGiven && curLang=="en")
            //|| (enLangGiven && curLang==defaultLang)
           )
            return n.firstChildElement(QStringLiteral("descrip"));

        n = n.nextSiblingElement(langSet);
    }
    return QDomElement();
}


QString Glossary::descrip(const QByteArray& id, const QString& lang, const QString& type) const
{
    QDomElement n = firstDescripElemForLang(m_entriesById.value(id), lang);
    if (n.isNull())
        return QString();

    while (!n.isNull()) {
        if (n.attribute(QStringLiteral("type")) == type)
            return n.text();

        n = n.nextSiblingElement(QStringLiteral("descrip"));
    }
    return QString();
}

void Glossary::setDescrip(const QByteArray& id, QString lang, const QString& type, const QString& value)
{
    setClean(false);

    QDomElement descripElem = firstDescripElemForLang(m_entriesById.value(id), lang);

    QDomDocument document = descripElem.ownerDocument();
    while (!descripElem.isNull()) {
        if (descripElem.attribute(QStringLiteral("type")) == type)
            return setText(descripElem, value);;

        descripElem = descripElem.nextSiblingElement(QStringLiteral("descrip"));
    }

    //create new descrip element
    QDomElement parentForDescrip = m_entriesById.value(id);
    if (!lang.isEmpty()) {
        //find/create a langSet elem that should be parent of the new descrip
        QDomElement langSetElem = m_entriesById.value(id).firstChildElement(langSet);
        while (!langSetElem.isNull()) {
            QString nLang = langSetElem.attribute(xmlLang, defaultLang);
            nLang.replace('-', '_');
            if (lang == QLatin1String("en")) { //NOTE COMPAT
                lang = defaultLang;
                nLang.replace('_', '-');
                langSetElem.setAttribute(xmlLang, defaultLang);
            }

            if (lang == nLang)
                break;

            langSetElem = langSetElem.nextSiblingElement(langSet);
        }
        if (!langSetElem.isNull())
            parentForDescrip = langSetElem;
        else {
            parentForDescrip = parentForDescrip.appendChild(document.createElement(langSet)).toElement();
            lang.replace('_', '-');
            parentForDescrip.setAttribute(xmlLang, lang);
        }
    }
    QDomElement descrip = parentForDescrip.insertBefore(document.createElement(QStringLiteral("descrip")), parentForDescrip.firstChild()).toElement();
    descrip.setAttribute(QStringLiteral("type"), type);
    descrip.appendChild(document.createTextNode(value));
}

QString Glossary::subjectField(const QByteArray& id, const QString& lang) const
{
    return descrip(id, lang, QStringLiteral("subjectField"));
}

QString Glossary::definition(const QByteArray& id, const QString& lang) const
{
    return descrip(id, lang, QStringLiteral("definition"));
}

void Glossary::setSubjectField(const QByteArray& id, const QString& lang, const QString& value)
{
    setDescrip(id, lang, QStringLiteral("subjectField"), value);
}

void Glossary::setDefinition(const QByteArray& id, const QString& lang, const QString& value)
{
    setDescrip(id, lang, QStringLiteral("definition"), value);
}



//add words to the hash
// static void addToHash(const QMultiHash<QString,int>& wordHash,
//                       const QString& what,
//                       int index)
void Glossary::hashTermEntry(const QDomElement& termEntry)
{
    QByteArray entryId = termEntry.attribute(::id).toLatin1();
    if (entryId.isEmpty())
        return;

    m_entriesById.insert(entryId, termEntry);

    QString sourceLangCode = Project::instance()->sourceLangCode();
    const auto termTexts = terms(entryId, sourceLangCode);
    for (const QString& termText : termTexts) {
        const auto words = termText.split(' ', Qt::SkipEmptyParts);
        for (const QString& word : words)
            idsByLangWord[sourceLangCode].insert(stem(sourceLangCode, word.toLower()), entryId);
    }
}

void Glossary::unhashTermEntry(const QDomElement& termEntry)
{
    QByteArray entryId = termEntry.attribute(::id).toLatin1();
    m_entriesById.remove(entryId);

    QString sourceLangCode = Project::instance()->sourceLangCode();
    const auto termTexts = terms(entryId, sourceLangCode);
    for (const QString& termText : termTexts) {
        const auto words = termText.split(' ', Qt::SkipEmptyParts);
        for (const QString& word : words)
            idsByLangWord[sourceLangCode].remove(stem(sourceLangCode, word.toLower()), entryId);
    }
}

#if 0
void Glossary::hashTermEntry(int index)
{
    Q_ASSERT(index < termList.size());
    foreach (const QString& term, termList_.at(index).english) {
        foreach (const QString& word, term.split(' ', Qt::SkipEmptyParts))
            wordHash_.insert(stem(Project::instance()->sourceLangCode(), word), index);
    }
}

void Glossary::unhashTermEntry(int index)
{
    Q_ASSERT(index < termList.size());
    foreach (const QString& term, termList_.at(index).english) {
        foreach (const QString& word, term.split(' ', Qt::SkipEmptyParts))
            wordHash_.remove(stem(Project::instance()->sourceLangCode(), word), index);
    }
}
#endif

void Glossary::removeEntry(const QByteArray& id)
{
    if (!m_entriesById.contains(id))
        return;

    QDomElement entry = m_entriesById.value(id);
    if (entry.nextSibling().isCharacterData())
        entry.parentNode().removeChild(entry.nextSibling()); //nice formatting
    entry.parentNode().removeChild(entry);
    m_entriesById.remove(id);

    unhashTermEntry(entry);
    m_idsForEntriesById = m_entriesById.keys();
    removedIds.append(id); //for new id generation goodness

    setClean(false);
}

static void appendTerm(QDomElement langSetElem, const QString& termText)
{
    QDomDocument doc = langSetElem.ownerDocument();
    /*
        QDomElement ntigElement=doc.createElement(ntig); langSetElem.appendChild(ntigElement);
        QDomElement termGrpElement=doc.createElement(termGrp); ntigElement.appendChild(termGrpElement);
        QDomElement termElement=doc.createElement(term); termGrpElement.appendChild(termElement);
        termElement.appendChild(doc.createTextNode(termText));
    */
    QDomElement tigElement = doc.createElement(tig); langSetElem.appendChild(tigElement);
    QDomElement termElement = doc.createElement(term); tigElement.appendChild(termElement);
    termElement.appendChild(doc.createTextNode(termText));
}

QByteArray Glossary::append(const QStringList& sourceTerms, const QStringList& targetTerms)
{
    if (!m_doc.elementsByTagName(QStringLiteral("body")).count())
        return QByteArray();

    setClean(false);
    QDomElement termEntry = m_doc.createElement(QStringLiteral("termEntry"));
    m_doc.elementsByTagName(QStringLiteral("body")).at(0).appendChild(termEntry);

    //m_entries=m_doc.elementsByTagName("termEntry");

    QByteArray newId = generateNewId();
    termEntry.setAttribute(::id, QString::fromLatin1(newId));

    QDomElement sourceElem = m_doc.createElement(langSet); termEntry.appendChild(sourceElem);
    sourceElem.setAttribute(xmlLang, Project::instance()->sourceLangCode().replace('_', '-'));
    for (const QString &sourceTerm : sourceTerms)
        appendTerm(sourceElem, sourceTerm);

    QDomElement targetElem = m_doc.createElement(langSet); termEntry.appendChild(targetElem);
    targetElem.setAttribute(xmlLang, Project::instance()->targetLangCode().replace('_', '-'));
    for (const QString &targetTerm : targetTerms)
        appendTerm(targetElem, targetTerm);

    hashTermEntry(termEntry);
    m_idsForEntriesById = m_entriesById.keys();

    return newId;
}

void Glossary::append(const QString& _english, const QString& _target)
{
    append(QStringList(_english), QStringList(_target));
}

void Glossary::clear()
{
    setClean(true);
    //path.clear();
    idsByLangWord.clear();
    m_entriesById.clear();
    m_idsForEntriesById.clear();

    removedIds.clear();
    changedIds_.clear();
    addedIds_.clear();
    wordHash_.clear();
    termList_.clear();
    langWordEntry_.clear();
    subjectFields_ = QStringList(QString());

    m_doc.clear();
}


bool GlossaryModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    Glossary* glossary = Project::instance()->glossary();
    int i = row + count;
    while (--i >= row)
        glossary->removeEntry(glossary->id(i));

    endRemoveRows();
    return true;
}

// bool GlossaryModel::insertRows(int row,int count,const QModelIndex& parent)
// {
//     if (row!=rowCount())
//         return false;
QByteArray GlossaryModel::appendRow(const QString& _english, const QString& _target)
{
    bool notify = !canFetchMore(QModelIndex());
    if (notify)
        beginInsertRows(QModelIndex(), rowCount(), rowCount());

    QByteArray id = m_glossary->append(QStringList(_english), QStringList(_target));

    if (notify) {
        m_visibleCount++;
        endInsertRows();
    }
    return id;
}

//END EDITING

