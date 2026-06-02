/***************************************************************************
                          strucimportlisitwidgetitem.h  -  description
                             -------------------
    begin                : Wed Oct 07 14:02:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
    email                : ndoebelin@gmx.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef STRUCIMPORTITEMDATA_H
#define STRUCIMPORTITEMDATA_H

#include <QObject>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include "../libXrdIO/hkl.h"
#include "../libXrdIO/parser/bgmnsgdatparser.h"
#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/settingsmanager.h"
#include "projectWidget/bgmnstrverifier.h"

class StrucImportItemData : public QObject
{
    Q_OBJECT

public:
    StrucImportItemData(QObject *parent = nullptr);

    void setUiPointers(BgmnSgDatParser *sg)         {_sgParser = sg;}

    inline void setSourceFileName(const QString &s) {_sourceFileName = s;}
    inline void setSourceString(const QString &s)   {_sourceString = s;}
    inline void setStrString(const QString &s)      {_strString = s;}
    inline void setMsgString(const QString &s)      {_msgString = s;}
    inline void setCrystalStructure(const CrystalStructure &c) {_crystStruct = c;}

    inline QUuid uid()              const {return _uid;}
    inline QString sourceFileName() const {return _sourceFileName;}
    inline QString sourceString()   const {return _sourceString;}
    inline QString strString()      const {return _strString;}
    inline QString msgString()      const {return _msgString;}
    inline HklPhaseData hklData()   const {return _hklData;}
    inline bool verifiedOk()        const {return _verifiedOk;}
    inline QSet<int> errorLines()   const {return _errorLines;}
    inline CrystalStructure crystalStructure() const {return _crystStruct;}
    inline CrystalStructure & crystalStructure() {return _crystStruct;}

    void saveData(const QString &, const QString &);
    void loadData();
    void verify();

    void parseSourceString();
    void createStrFile();

private:
    SettingsManager *settings;
    BgmnSgDatParser *_sgParser;
    BgmnStrVerifier *strIndexer;

    QUuid _uid;
    QString _sourceFileName;
    QString _sourceString;
    QString _strString;
    QString _msgString;
    HklPhaseData _hklData;
    bool _verifiedOk;
    QSet<int> _errorLines;
    CrystalStructure _crystStruct;

private slots:
    void verifyComplete();

signals:
    void sigVerifyComplete(QUuid);
};

#endif // STRUCIMPORTITEMDATA_H
