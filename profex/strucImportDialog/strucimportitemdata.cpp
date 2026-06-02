/***************************************************************************
                          strucimportlisitwidgetitem.cpp  -  description
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


#include "strucimportitemdata.h"
#include "strucimportbgmncheck.h"
#include "strucimportbgmnfixes.h"
#include "../libXrdIO/parser/cifparser2.h"
#include "../libXrdIO/parser/icddxmlparser.h"
#include "../libXrdIO/parser/rruffdifparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/bgmnfileio.h"

#include <QString>

StrucImportItemData::StrucImportItemData(QObject *parent)
    : QObject(parent)
{
    _uid = QUuid::createUuid();
    settings = SettingsManager::getInstance();
    strIndexer = new BgmnStrVerifier;
    connect(strIndexer, SIGNAL(signalVerifyComplete()), this, SLOT(verifyComplete()));
    _sgParser = nullptr;

    _sourceFileName = QString();
    _sourceString = QString();
    _strString = QString();
    _msgString = QString();
    _verifiedOk = false;
    _errorLines = QSet<int>();
    _crystStruct.setAuxInfo("limits", settings->value("cifimport/limits", 0.01).toDouble());
}

void StrucImportItemData::saveData(const QString &str, const QString &msg)
{
    _strString = str;
    if (!msg.isEmpty()) _msgString = msg;
}

void StrucImportItemData::loadData()
{
    QFileInfo fi(_sourceFileName);
    _sourceString = BgmnFileIO::readTextFile(fi.absoluteFilePath());
    parseSourceString();
}

void StrucImportItemData::parseSourceString()
{
    QFileInfo fi(_sourceFileName);
    QString suffix = fi.suffix().toLower();

    if (suffix == "str") {
        _strString    = _sourceString;
        return;
    }

    if (suffix == "cif") {
        CifParser2 cparser;
        cparser.parseSourceString(_sourceString, _sourceFileName);
        _crystStruct = cparser.getCrystalStructure(0);
    } else if (suffix == "xml") {
        IcddXmlParser xparser;
        xparser.parseSourceString(_sourceString, _sourceFileName);
        _crystStruct = xparser.getCrystalStructure();
    } else if ((suffix == "dif") || (suffix == "txt")) {
        RruffDifParser rparser;
        rparser.parseSourceString(_sourceString, _sourceFileName);
        _crystStruct = rparser.getCrystalStructure();
    }

    createStrFile();
}

void StrucImportItemData::createStrFile()
{
    _msgString.clear();

    QFileInfo fi(_sourceFileName);
    StrucImportBgmnFixes bgmnFix(_sgParser);

    _crystStruct = bgmnFix.fixStructure(_crystStruct, false);
    _msgString += QString("Importing %1<br>").arg(fi.absoluteFilePath());
    _msgString += bgmnFix.report();

    BgmnStrParser sparser(_crystStruct.toBgmnStr(), QString());
    _strString = sparser.getContent();
}

/*
 * returns true on success
 */
void StrucImportItemData::verify()
{
    qDebug() << QString("StrucImportItemData::verify(): Starting STR code verification of %1").arg(_sourceFileName);
    _msgString += QString("Running BGMN to verify the structure and calculate hkl line positions:<br>");

    StrucImportBgmnCheck bgmnCheck(_sgParser);
    _errorLines = bgmnCheck.verifyAtoms(_strString);

    if (_errorLines.size() > 0) {
        _msgString += bgmnCheck.report();
        qDebug() << QString("StrucImportItemData::verify(): bgmn code check failed, not running verification.");
        _verifiedOk = false;
        emit sigVerifyComplete(_uid);
    } else {
        strIndexer->runStrVerification(_strString);
    }
}

void StrucImportItemData::verifyComplete()
{
    _hklData = strIndexer->getLastIndexedHkl();

    if (_hklData.hklData().isEmpty()) {
        _verifiedOk = false;
        _msgString += QString("<font color=\"%1\">%2</font>").arg("#FF0000").arg(strIndexer->getLastIndexMessage());
    } else {
        _verifiedOk = true;
        _msgString += QString("Ok, %1 hkl lines calculated.<br>").arg(_hklData.hklData().size());
        _msgString += QString("XrayDensity=%1<br>").arg(_hklData.density(), 0, 'f', 4);
        _msgString += QString("<font color=\"%1\">%2</font>").arg("#009900").arg(strIndexer->getLastIndexMessage());
    }

    qDebug() << QString("StrucImportItemData::verifyComplete(): STR code verification of %1 complete").arg(_sourceFileName);
    emit sigVerifyComplete(_uid);
}
