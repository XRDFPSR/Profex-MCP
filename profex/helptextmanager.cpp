/***************************************************************************
                          helptextmanager.cpp  -  description
                             -------------------
    begin                : Mon Jul 31 18:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#include "helptextmanager.h"
#include "../libXrdIO/bgmnfileio.h"

HelpTextManager::HelpTextManager()
{

}

QString HelpTextManager::getHelpText(const QString &module)
{
    if (!_helpTextBuffer.contains(module)) {
        parseHelpFile(module);
    }

    return _helpTextBuffer.value(module);
}

void HelpTextManager::parseHelpFile(const QString &module)
{
    if (module == "graphWindow") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpGraphWindow.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "searchMatchWidget") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpSearchMatchWidget.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "peakFitWidget") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpPeakFitWidget.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "peakintegrationWidget") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpPeakIntegrationWidget.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "scanList") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpScanList.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "peakListWidget") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpPeakListWidget.html"));
        _helpTextBuffer.insert(module, str);
    } else if (module == "controlFileEditor") {
        QString str = translateToMac(BgmnFileIO::readTextFile(":/helpText/resources/moduleHelpControlFileEditor.html"));
        _helpTextBuffer.insert(module, str);
    }
}

QString HelpTextManager::translateToMac(const QString &s)
{
#ifdef Q_OS_MAC
    QString r = s;
    return r.replace(QString("Ctrl"), QString("Cmd"));
#else
    return s;
#endif
}
