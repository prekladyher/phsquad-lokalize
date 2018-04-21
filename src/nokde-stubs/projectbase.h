// This file is generated by kconfig_compiler_kf5 from projectbase.kcfg.
#ifndef PROJECTBASE_H
#define PROJECTBASE_H

#include "lokalize_debug.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <QPointer>

#include "pos.h"

class FileSearchTab;
class EditorTab;
namespace TM{class TMTab;};

class ProjectBase: public QObject
{
    Q_OBJECT
public:

    ProjectBase();
    ~ProjectBase(){save();}

    bool eventFilter(QObject *obj, QEvent *event);

public slots:
    EditorTab* fileOpen(QString url=QString(),int entry=0, bool setAsActive=true, const QString& mergeFile=QString(), bool silent=false);
    EditorTab* fileOpen(const QString& filePath, const QString& source, const QString& ctxt);
    EditorTab* fileOpen(const QString& filePath, DocPosition docPos, int selection);

    void lookupInTranslationMemory(const QString& source, const QString& target);
    TM::TMTab* showTM();
    void showFileSearch();
    void fileSearchNext();

    void editorClosed(QObject* obj);
private:
    //using QPointer switches it.value() to 0 before we get to destroyed() handler
    //typedef QMap<QUrl, QPointer<QMdiSubWindow> > FileToEditor;
    typedef QMap<QString, EditorTab*> FileToEditor;
    FileToEditor m_fileToEditor;
    QByteArray m_lastEditorState;
    QPointer<TM::TMTab> m_tmTab;
    QPointer<FileSearchTab> m_fileSearchTab;


public:
    void setProjectID( const QString & v )
    {
        mProjectID = v;
    }

    QString projectID() const
    {
      return mProjectID;
    }

    void setKind( const QString & v )
    {
        mKind = v;
    }

    QString kind() const
    {
      return mKind;
    }

    void setLangCode( const QString & )
    {
        //this is called from setDefaults()
        //mTargetLangCode = v;
    }

    QString langCode() const
    {
      return mTargetLangCode;
    }
public slots:
    void setTargetLangCode( const QString & v ){mTargetLangCode = v;}
    void setSourceLangCode( const QString & v ){mSourceLangCode = v;}
public:
    QString targetLangCode() const {return mTargetLangCode;}
    QString sourceLangCode() const {return mSourceLangCode;}

    void setMailingList( const QString & v )
    {
        mMailingList = v;
    }

    QString mailingList() const
    {
      return mMailingList;
    }

    void setPoBaseDir( const QString & v )
    {
        mPoBaseDir = v;
    }

    QString poBaseDir() const
    {
      return mPoBaseDir;
    }

    /**
      Set PotBaseDir
    */
    void setPotBaseDir( const QString & v )
    {
        mPotBaseDir = v;
    }

    /**
      Get PotBaseDir
    */
    QString potBaseDir() const
    {
      return mPotBaseDir;
    }

    /**
      Set BranchDir
    */
    void setBranchDir( const QString & v )
    {
        mBranchDir = v;
    }

    /**
      Get BranchDir
    */
    QString branchDir() const
    {
      return mBranchDir;
    }

    /**
      Set AltDir
    */
    void setAltDir( const QString & v )
    {
        mAltDir = v;
    }

    /**
      Get AltDir
    */
    QString altDir() const
    {
      return mAltDir;
    }

    /**
      Set GlossaryTbx
    */
    void setGlossaryTbx( const QString & v )
    {
        mGlossaryTbx = v;
    }

    /**
      Get GlossaryTbx
    */
    QString glossaryTbx() const
    {
      return mGlossaryTbx;
    }

    /**
      Set MainQA
    */
    void setMainQA( const QString & v )
    {
        mMainQA = v;
    }

    /**
      Get MainQA
    */
    QString mainQA() const
    {
      return mMainQA;
    }

    /**
      Set Accel
    */
    void setAccel( const QString & v )
    {
        mAccel = v;
    }

    /**
      Get Accel
    */
    QString accel() const
    {
      return mAccel;
    }

    /**
      Set Markup
    */
    void setMarkup( const QString & v )
    {
        mMarkup = v;
    }

    /**
      Get Markup
    */
    QString markup() const
    {
      return mMarkup;
    }

    /**
      Set WordWrap
    */
    void setWordWrap( int v )
    {
        mWordWrap = v;
    }

    /**
      Get WordWrap
    */
    int wordWrap() const
    {
      return mWordWrap;
    }

    void save();
    void setDefaults(){}
  protected:

    // General
    QString mProjectID;
    QString mKind;
    QString mLangCode;
    QString mTargetLangCode;
    QString mSourceLangCode;
    QString mMailingList;
    QString mPoBaseDir;
    QString mPotBaseDir;
    QString mBranchDir;
    QString mAltDir;
    QString mGlossaryTbx;
    QString mMainQA;

    // RegExps
    QString mAccel;
    QString mMarkup;
    int mWordWrap;

  private:
};

#endif

