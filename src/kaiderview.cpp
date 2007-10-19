/* ****************************************************************************
  This file is part of KAider (some bits of KBabel code were reused)

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>

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

#include "kaiderview.h"
#include "project.h"
#include "catalog.h"
#include "cmd.h"
#include "prefs_kaider.h"
#include "syntaxhighlighter.h"

#include <QTextCodec>
#include <QTabBar>
#include <QTimer>
#include <QMenu>
#include <QDragEnterEvent>

#include <QLabel>
#include <QHBoxLayout>

#include <kled.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstandardshortcut.h>
//parent is set on qsplitter insertion
class LedsWidget:public QWidget
{
public:
    LedsWidget(QWidget* parent)
    : QWidget(parent)
//     , _ledFuzzy(0)
//     , _ledUntr(0)
//     , _ledErr(0)
    {
        QHBoxLayout* layout=new QHBoxLayout(this);
        layout->addStretch();
        layout->addWidget(new QLabel(i18nc("@label whether entry is fuzzy","Fuzzy:")));
        layout->addWidget(ledFuzzy=new KLed(Qt::green,KLed::Off,KLed::Sunken,KLed::Rectangular));
        layout->addWidget(new QLabel(i18nc("@label whether entry is fuzzy","Untranslated:")));
        layout->addWidget(ledUntr=new KLed(Qt::red,KLed::Off,KLed::Sunken,KLed::Rectangular));
        layout->addStretch();
        setMaximumHeight(minimumSizeHint().height());
    }

//NOTE the config shit doesn't work
// private:
//     void contextMenuEvent(QContextMenuEvent* event)
//     {
//         QMenu menu;
//         menu.addAction(i18nc("@action","Hide"));
//         if (menu.exec(event->globalPos()))
//         {
//             Settings::setLeds(false);
//             kWarning() << Settings::leds();
//             hide();
//         }
//     }

public:
    KLed* ledFuzzy;
    KLed* ledUntr;
    KLed* ledErr;

};

void ProperTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    // ALT+123 feature TODO this is general so should be on another level
    if( (keyEvent->modifiers()&Qt::AltModifier)
         &&!keyEvent->text().isEmpty()
         &&keyEvent->text().at(0).isDigit() )
    {
        QString text=keyEvent->text();
        while (!text.isEmpty()&& text.at(0).isDigit() )
        {
            m_currentUnicodeNumber = 10*m_currentUnicodeNumber+(text.at(0).digitValue());
            text.remove(0,1);
        }
    }
    else
        KTextEdit::keyPressEvent(keyEvent);
}

void ProperTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    if ( (e->key()==Qt::Key_Alt) && m_currentUnicodeNumber >= 32 )
    {
        insertPlainText(QChar( m_currentUnicodeNumber ));
        m_currentUnicodeNumber=0;
    }
    else
        KTextEdit::keyReleaseEvent(e);
}


KAiderView::KAiderView(QWidget *parent,Catalog* catalog/*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical,parent)
    , _catalog(catalog)
    , _msgidEdit(new ProperTextEdit(this))
    , _msgstrEdit(new ProperTextEdit(this))
    , m_msgidHighlighter(new SyntaxHighlighter(_msgidEdit->document()))
    , m_msgstrHighlighter(new SyntaxHighlighter(_msgstrEdit->document()))
/*    , m_msgidHighlighter(0)
    , m_msgstrHighlighter(0)*/
    , _tabbar(new QTabBar(this))
    , _leds(0)
    , _currentEntry(-1)

{
//    _catalog=Catalog::instance();
    //ui_kaiderview_base.setupUi(this);
//    settingsChanged();
    _tabbar->hide();
//     _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
//                                   "<p>This part of the window shows the original message\n"
//                                   "of the currently displayed entry.</p></qt>"));

    _msgidEdit->setReadOnly(true);

    //apply changes to catalog via undo/redo
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    _msgidEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setAcceptRichText(false);
    _msgstrEdit->installEventFilter(this);

    addWidget(_tabbar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);

//     QTimer::singleShot(3000,this,SLOT(setupWhatsThis()));
    settingsChanged();
}

KAiderView::~KAiderView()
{
    /*
    delete _msgidEdit;
    delete _msgstrEdit;
    delete _tabbar;
    */
}

// void KAiderView::setupWhatsThis()
// {
//     _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
//                                   "<p>This part of the window shows the original message\n"
//                                   "of the currently displayed entry.</p></qt>"));
// }

static bool isMasked(const QString& str, uint col)
{
    if(col == 0 || str.isEmpty())
        return false;

    uint counter=0;
    int pos=col;

    while(pos >= 0 && str.at(pos) == '\\')
    {
        counter++;
        pos--;
    }

    return !(bool)(counter%2);
}


bool KAiderView::eventFilter(QObject */*obj*/, QEvent *event)
{
//     if (obj!=_msgstrEdit)
//     {
//         kWarning() << "THIS IS VERY STRANGE";
//         return QSplitter::eventFilter(obj, event);
//     }

    if (event->type() != QEvent::KeyPress)
        return false;//QObject::eventFilter(obj, event);
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    static QString spclChars("abfnrtv'\"?\\");
    if(keyEvent->matches(QKeySequence::Undo))
    {
        if (_catalog->canUndo())
        {
            emit signalUndo();
            fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
            return true;
        }
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        if (_catalog->canRedo())
        {
            emit signalRedo();
            fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
            return true;
        }
    }
    else if (keyEvent->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
    {
        if(keyEvent->key()==Qt::Key_Home)
        {
            emit signalGotoFirst();
            return true;
        }
        else if(keyEvent->key()==Qt::Key_End)
        {
            emit signalGotoLast();
            return true;
        }
    }
    else if (!keyEvent->modifiers()&&(keyEvent->key()==Qt::Key_Backspace||keyEvent->key()==Qt::Key_Delete))
    {
        if (_catalog->isFuzzy(_currentEntry))
        {
            _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,false));
            _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
        }
        fuzzyEntryDisplayed(false);
        return false;
    }
    //clever editing
    else if(keyEvent->key()==Qt::Key_Return||keyEvent->key()==Qt::Key_Enter)
    {
        QString str=_msgstrEdit->toPlainText();
        QTextCursor t=_msgstrEdit->textCursor();
        int pos=t.position();
        QString ins;
        if( keyEvent->modifiers()&Qt::ShiftModifier )
        {
            if(pos>0
               &&!str.isEmpty()
               &&str.at(pos-1)=='\\'
               &&!isMasked(str,pos-1))
            {
                ins='n';
            }
            else
            {
                ins="\\n";
            }
        }
        else if(!(keyEvent->modifiers()&Qt::ControlModifier))
        {
            if(pos>0
               &&!str.isEmpty()
               &&!str.at(pos-1).isSpace())
            {
                if(str.at(pos-1)=='\\'
                   &&!isMasked(str,pos-1))
                    ins='\\';
                // if there is no new line at the end
                if(pos<2||str.mid(pos-2,2)!="\\n")
                    ins+=' ';
            }
            else if(str.isEmpty())
            {
                ins="\\n";
            }
        }
        if (!str.isEmpty())
            ins+='\n';
        t.insertText(ins);
        _msgstrEdit->setTextCursor(t);

        return true;
    }
    else if(keyEvent->key() == Qt::Key_Tab)
    {
        _msgstrEdit->insertPlainText("\\t");
        return true;
    }
    else if( (keyEvent->modifiers()&Qt::ControlModifier)?
                (keyEvent->key()==Qt::Key_D) :
                (keyEvent->key()==Qt::Key_Delete))
    {
        QTextCursor t=_msgstrEdit->textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=_msgstrEdit->toPlainText();
            if(!str.isEmpty()
//                 &&pos<str.length()
                &&str.at(pos) == '\\'
                &&!isMasked(str,pos))
            {
//                 QString spclChars="abfnrtv'\"?\\";
                if(pos<str.length()-1&&spclChars.contains(str.at(pos+1)))
                    t.deleteChar();
            }
        }

        t.deleteChar();
        _msgstrEdit->setTextCursor(t);

        return true;
    }
    else if( keyEvent->key()==Qt::Key_Backspace
            || ( ( keyEvent->modifiers() & Qt::ControlModifier ) && keyEvent->key() == Qt::Key_H ) )
    {
        QTextCursor t=_msgstrEdit->textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=_msgstrEdit->toPlainText();
//             QString spclChars="abfnrtv'\"?\\";
            if(!str.isEmpty() && pos>0 && spclChars.contains(str.at(pos-1)))
            {
                if(pos>1 && str.at(pos-2)=='\\' && !isMasked(str,pos-2))
                    t.deletePreviousChar();
            }

        }
        t.deletePreviousChar();
        _msgstrEdit->setTextCursor(t);

        return true;
    }
    else if(keyEvent->text()=="\"")
    {
        QTextCursor t=_msgstrEdit->textCursor();
        int pos=t.position();
        QString str=_msgstrEdit->toPlainText();
        QString ins;

        if(pos==0 || str.at(pos-1)!='\\' || isMasked(str,pos-1))
            ins="\\\"";
        else
            ins="\"";

        t.insertText(ins);
        _msgstrEdit->setTextCursor(t);
        return true;
    }
    else if( keyEvent->key() == Qt::Key_Space && ( keyEvent->modifiers() & Qt::AltModifier ) )
    {
        _msgstrEdit->insertPlainText(QChar(0x00a0U));
        return true;
    }

    return false;
}




void KAiderView::settingsChanged()
{
    kWarning() << " AAA " <<endl;
    //Settings::self()->config()->setGroup("Editor");
    _msgidEdit->document()->setDefaultFont(Settings::msgFont());
    _msgstrEdit->document()->setDefaultFont(Settings::msgFont());
    if (Settings::leds())
    {
        if (_leds)
            _leds->show();
        else
        {
            _leds=new LedsWidget(this);
            if (_catalog->isFuzzy(_currentEntry))
                _leds->ledFuzzy->on();
            if (_catalog->msgstr(_currentPos).isEmpty())
                _leds->ledUntr->on();
            insertWidget(2,_leds);
        }
    }
    else if (_leds)
        _leds->hide();

    kWarning() << "BBB " <<endl;
}

void KAiderView::contentsChanged(int offset, int charsRemoved, int charsAdded )
{
    if (_currentEntry==-1)
        return;

    DocPosition pos=_currentPos;
    pos.offset=offset;

    if (charsRemoved)
        _catalog->push(new DelTextCmd(_catalog,pos,_oldMsgstr.mid(offset,charsRemoved)));

    _oldMsgstr=_msgstrEdit->toPlainText();//newStr becomes OldStr
    if (charsAdded)
        _catalog->push(new InsTextCmd(_catalog,pos,_oldMsgstr.mid(offset,charsAdded)));

    if (_leds)
    {
        if (_catalog->msgstr(pos).isEmpty())
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }

    if (_catalog->isFuzzy(_currentEntry))
    {
        _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,false));
        fuzzyEntryDisplayed(false);
//         _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
//         if (_leds)
//             _leds->ledFuzzy->off();
    }

    // for mergecatalog (delete entry from index)
    // and for statusbar
    emit signalChanged(pos.entry);
}

void KAiderView::gotoEntry(const DocPosition& pos,int selection/*, bool updateHistory*/)
{
    _currentPos=pos;
    _currentEntry=pos.entry;

    if (_catalog->pluralFormType(_currentEntry)==Gettext)
    {
        if (_catalog->numberOfPluralForms()!=_tabbar->count())
        {
            int i=_tabbar->count();
            if (_catalog->numberOfPluralForms()>_tabbar->count())
                while (i<_catalog->numberOfPluralForms())
                    _tabbar->addTab(i18nc("@title:tab","Plural Form %1",++i));
            else
                while (i>_catalog->numberOfPluralForms())
                    _tabbar->removeTab(i--);
        }
        kDebug()<<"_tabbar->count()"<<_tabbar->count();
        _tabbar->show();
        _tabbar->blockSignals(true);
        _tabbar->setCurrentIndex(_currentPos.form);
        _tabbar->blockSignals(false);
    }
    else
        _tabbar->hide();

    _msgidEdit->setPlainText(_catalog->msgid(_currentPos)/*, _catalog->msgctxt(_currentIndex)*/);
    unwrap(_msgidEdit);
    _msgstrEdit->document()->blockSignals(true);
    _msgstrEdit->setPlainText(_catalog->msgstr(_currentPos));
    _msgstrEdit->document()->blockSignals(false);

    bool untrans=_catalog->msgstr(_currentPos).isEmpty();
    if (_leds)
    {
        if (untrans)
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }

    ProperTextEdit* msgEdit=_msgstrEdit;
    if (pos.offset || selection)
    {
        if (pos.part==Msgid)
        msgEdit=_msgidEdit;

        QTextCursor t=msgEdit->textCursor();
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,pos.offset);
        //kWarning()<<_catalog->msgid(pos).mid(t.position(),selection);
        //NOTE this was kinda bug due to on-the-fly msgid wordwrap
        if (selection)
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,selection);
        msgEdit->setTextCursor(t);
    }
    else
    //what if msg starts with a tag?
    if (!untrans && _catalog->msgstr(_currentPos).startsWith('<'))
    {
        int offset=_catalog->msgstr(_currentPos).indexOf(QRegExp(">[^<]"));
        if (offset!=-1)
        {
            QTextCursor t=msgEdit->textCursor();
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
            msgEdit->setTextCursor(t);
        }
    }

    //for undo/redo tracking
    _oldMsgstr=_catalog->msgstr(_currentPos);
    //_oldMsgstr=_msgstrEdit->toPlainText();

    disconnect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    m_msgstrHighlighter->rehighlight();
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    msgEdit->setFocus();

    //force this because there are conditions when the
    //signal isn't emitted (didn't identify them yet)
    fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
}

void KAiderView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
    {
        //kWarning() << " " <<;
        event->acceptProposedAction();
    };
}

void KAiderView::dropEvent(QDropEvent *event)
{
    emit fileOpenRequested(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}



// edit actions that are easier to do in this class
void KAiderView::clearMsgStr()
{
    if (_currentEntry==-1)
        return;

    _currentPos.offset=0;
    _catalog->push(new DelTextCmd(_catalog,_currentPos,_catalog->msgstr(_currentPos)));
    if (_catalog->isFuzzy(_currentEntry))
    {
        toggleFuzzy(false);
        fuzzyEntryDisplayed(false);
    }

    gotoEntry(_currentPos);
    emit signalChanged(_currentEntry);
}

void KAiderView::toggleBookmark(bool checked)
{
    if (_currentEntry==-1)
        return;

    _catalog->setBookmark(_currentEntry,checked);
}

void KAiderView::toggleFuzzy(bool checked)
{
    if (_currentEntry==-1)
        return;

    _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,checked));
}


void KAiderView::fuzzyEntryDisplayed(bool fuzzy)
{
    kDebug()<<"fuzzy"<<_currentEntry<<fuzzy;
    if (_currentEntry==-1)
        return;

    if (fuzzy)
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::AlternateBase);
        if (_leds)
            _leds->ledFuzzy->on();
    }
    else
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
        if (_leds)
            _leds->ledFuzzy->off();
    }
}

void KAiderView::msgid2msgstr()
{
    QString text(_catalog->msgid(_currentPos));
    QString out;
    QString ctxt(_catalog->msgctxt(_currentPos.entry));

   // this is KDE specific:
    if( ctxt.startsWith( "NAME OF TRANSLATORS" ) || text.startsWith( "_: NAME OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorLocalizedName();
    }
    else if( ctxt.startsWith( "EMAIL OF TRANSLATORS" ) || text.startsWith( "_: EMAIL OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorEmail();
    }
    else if( /*_catalog->isGeneratedFromDocbook() &&*/ text.startsWith( "ROLES_OF_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<othercredit role=\\\"translator\\\">\n"
        "<firstname></firstname><surname></surname>\n"
        "<affiliation><address><email>"+Settings::authorEmail()+"</email></address>\n"
        "</affiliation><contrib></contrib></othercredit>";
    }
    else if( text.startsWith( "CREDIT_FOR_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<para>"+Settings::authorLocalizedName()+'\n'+
            "<email>"+Settings::authorEmail()+"</email></para>";
    }
/*    else if(text.contains(_catalog->miscSettings().singularPlural))
    {
        text.replace(_catalog->miscSettings().singularPlural,"");
    }*/
    // end of KDE specific part


/*    QRegExp reg=_catalog->miscSettings().contextInfo;
    if(text.contains(reg))
    {
        text.replace(reg,"");
    }*/
    
    //modifyMsgstrText(0,text,true);
    
    if (out.isEmpty())
        _msgstrEdit->setPlainText(text);
    else
    {
        QTextCursor t=_msgstrEdit->textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        _msgstrEdit->setTextCursor(t);
    }
}



void KAiderView::unwrap(ProperTextEdit* editor)
{
    if (!editor)
        editor=_msgstrEdit;

    QTextCursor t=editor->document()->find(QRegExp("[^(\\\\n)]$"));
    if (t.isNull())
        return;

    if (editor==_msgstrEdit)
        _catalog->beginMacro(i18nc("@item Undo action item","Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    QRegExp rx("[^(\\\\n)>]$");
    //remove '\n's skipping "\\\\n"
    while (!(t=editor->document()->find(rx,t)).isNull())
    {
        t.movePosition(QTextCursor::EndOfLine);
        if (!t.atEnd())
            t.deleteChar();
    }
    if (editor==_msgstrEdit)
        _catalog->endMacro();
}

void KAiderView::insertTerm(const QString& term)
{
    _msgstrEdit->insertPlainText(term);
    _msgstrEdit->setFocus();
}

void KAiderView::replaceText(const QString& txt)
{
    if (txt==_msgstrEdit->toPlainText())
        return;

    int oldOffset=_msgstrEdit->textCursor().position();
    _msgstrEdit->insertPlainText(txt);

    _msgstrEdit->setFocus();

    if (_catalog->msgstr(_currentPos).size()==txt.size())
        return;

    _catalog->beginMacro(i18nc("@item Undo action item","Remove old translation"));
    DocPosition pos=_currentPos;
    pos.offset=0;
    _catalog->push(new DelTextCmd(_catalog,pos,_catalog->msgstr(pos).left(oldOffset)));
    pos.offset=txt.size();
    _catalog->push(new DelTextCmd(_catalog,pos,_catalog->msgstr(pos).mid(txt.size())));

    _catalog->endMacro();
    pos.offset=0;

    _msgstrEdit->document()->blockSignals(true);
    _msgstrEdit->setPlainText(_catalog->msgstr(_currentPos));
    _msgstrEdit->document()->blockSignals(false);
    disconnect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    m_msgstrHighlighter->rehighlight();
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

}


void KAiderView::tagMenu()
{
    if (Project::instance()->markup().isEmpty())
        return;

    QMenu menu;

    //QRegExp tag("(<[^>]*>)+|\\&\\w+\\;");
    QRegExp tag(Project::instance()->markup());
    tag.setMinimal(true);
    QString en(_msgidEdit->toPlainText());
    QString target(_msgstrEdit->toPlainText());
    en.remove('\n');
    target.remove('\n');
    int pos=0;
    //tag.indexIn(en);
    //kWarning() << tag.capturedTexts();
    //kWarning() << tag.cap(0);
    int posInMsgStr=0;
    QAction* txt(0);
    while ((pos=tag.indexIn(en,pos))!=-1)
    {
/*        QString str(tag.cap(0));
        str.replace("&","&&");*/
        txt=menu.addAction(tag.cap(0));
        pos+=tag.matchedLength();

        if (posInMsgStr!=-1 && (posInMsgStr=target.indexOf(tag.cap(0),posInMsgStr))==-1)
        {
            menu.setActiveAction(txt);
        }
        else if (posInMsgStr!=-1)
        {
            posInMsgStr+=tag.matchedLength();
        }
    }
    if (!txt)
        return;

    txt=menu.exec(_msgidEdit->mapToGlobal(QPoint(0,0)));
    if (txt)
        _msgstrEdit->insertPlainText(txt->text());

}










#include "kaiderview.moc"
