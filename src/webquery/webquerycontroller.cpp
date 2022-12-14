/*****************************************************************************
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "webquerycontroller.h"

#include "lokalize_debug.h"

#include <QTextCodec>
#include "catalog.h"
#include "webqueryview.h"

#include <QUrl>
// #include <kio/netaccess.h>
#include <kio/jobclasses.h>
#include <kio/job.h>

WebQueryController::WebQueryController(const QString& name, QObject* parent)
//     : QThread(parent)
    : QObject(parent)
    , m_running(false)
    , m_name(name)
{
}

void WebQueryController::query(const CatalogData& data)
{
    m_queue.enqueue(data);
    if (!m_running) {
        m_running = true;
        Q_EMIT doQuery();
    }
}



QString WebQueryController::msg()
{
    return m_queue.head().msg;
}

QString WebQueryController::filePath()
{
    return QString();
}
void WebQueryController::setTwinLangFilePath(QString)
{

}

QString WebQueryController::twinLangMsg()
{
    return QString();
}


void WebQueryController::doDownloadAndFilter(QString urlStr, QString _codec, QString rx/*, int rep*/)
{
    QString result;
    QUrl url;
    url.setUrl(urlStr);

    qCWarning(LOKALIZE_LOG) << "_real url: " << url.toString();
    KIO::StoredTransferJob* readJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(readJob, &KIO::StoredTransferJob::result, this, &WebQueryController::slotDownloadResult);

    codec = QTextCodec::codecForName(_codec.toUtf8());
    filter = QRegExp(rx);
}

void WebQueryController::slotDownloadResult(KJob* job)
{
    m_running = false;
    if (job->error()) {
        m_queue.dequeue();
        return;
    }

    QTextStream stream(static_cast<KIO::StoredTransferJob*>(job)->data());
    stream.setCodec(codec);
    if (filter.indexIn(stream.readAll()) != -1) {
        Q_EMIT postProcess(filter.cap(1));
        //qCWarning(LOKALIZE_LOG)<<result;
    } else
        m_queue.dequeue();
}


void WebQueryController::setResult(QString result)
{
    //webQueryView may be deleted before we get result...
    WebQueryView* a = m_queue.dequeue().webQueryView;
    connect(this, &WebQueryController::addWebQueryResult, a, &WebQueryView::addWebQueryResult);
    Q_EMIT addWebQueryResult(m_name, result);
    disconnect(this, &WebQueryController::addWebQueryResult, a, &WebQueryView::addWebQueryResult);

    if (!m_queue.isEmpty()) {
        m_running = true;
        Q_EMIT doQuery();
    }

}



