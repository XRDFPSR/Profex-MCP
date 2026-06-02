/***************************************************************************
                          contexthelpdisplay.cpp  -  description
                             -------------------
    begin                : Thu Oct 20 08:00:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#include <QFileInfo>
#include "contexthelpdisplay.h"
#include "../libXrdIO/bgmnfileio.h"

ContextHelpDisplay::ContextHelpDisplay(QWidget *parent) :
    QTextBrowser(parent)
{
    settings = SettingsManager::getInstance();
    document()->setDefaultStyleSheet(BgmnFileIO::readTextFile("://resources/contextHelp.css"));
}

void ContextHelpDisplay::setText(const QString &s)
{
    clear();
    setHtml(s);
}

void ContextHelpDisplay::setKeyword(const QString &file, const QString &keyword)
{
    clear();
    QFileInfo fi(file);
    QString key = keyword.trimmed();

    // let's determine which type of file the keyword comes from
    if (fi.suffix().toLower() == "sav") {
        setHtml(getHelpText(savHelp, key));
    } else if (fi.suffix().toLower() == "str") {
        setHtml(getHelpText(strHelp, key));
    } else { // if it's not a str or sav file, search both helpMaps
        setHtml(getHelpText(QMap<QString, QString>(), key));
    }
}

QString ContextHelpDisplay::getHelpText(const QMap<QString, QString> &map, const QString &kword)
{
    QMap<QString, QString> m = map.isEmpty() ? savHelp : map;

    QRegularExpression rx(QString("^%1(?:[\\(\\[].+[\\)\\]])?$").arg(kword));
    QMapIterator<QString, QString> itSav(m);

    while (itSav.hasNext()) {
        itSav.next();
        QRegularExpressionMatch rm = rx.match(itSav.key());
        if (rm.hasMatch()) return itSav.value();
    }

    // if map is empty, we continue searching the second map
    // if it is not empty and we arrived here, we bail out
    if (!map.isEmpty()) return QString("No help for <code>%1</code>").arg(kword);

    m = strHelp;
    QMapIterator<QString, QString> itStr(m);

    while (itStr.hasNext()) {
        itStr.next();
        QRegularExpressionMatch rm = rx.match(itStr.key());
        if (rm.hasMatch()) return itStr.value();
    }

    return QString("No help for <code>%1</code>").arg(kword);
}

void ContextHelpDisplay::parseHelpFiles()
{
    QFileInfo fi(settings->value("bgmnProject/bgmnExec", QString()).toString());
    sourceDir = fi.absolutePath();

    strHelp.clear();
    savHelp.clear();

    QString fStr(":/resources/STRUC.XML");
    QString fSav(":/resources/SAV.XML");

    qDebug() << QString("ContextHelpDisplay::parseHelpFiles(): Parsing help file %1").arg(fStr);
    qDebug() << QString("ContextHelpDisplay::parseHelpFiles(): Parsing help file %1").arg(fSav);

    QString contentStr;
    QString contentSav;

    if (readFile(fStr, contentStr)) {
        parseXml(contentStr, strHelp);
    }

    if (readFile(fSav, contentSav)) {
        parseXml(contentSav, savHelp);
    }
}

bool ContextHelpDisplay::readFile(const QString &str, QString &content)
{
    QFile file(str);

    // file can't be opened?
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("ContextHelpDisplay::readFile(): File cannot be opened for reading: %1").arg(str);
        return false;
    }

    content = file.readAll();
    file.close();
    return true;
}

// parses the xml content of doc and fills in the map with xml tags <helpID> and <content>

// note: - QDomDocument and related classes don't work, because they report tag errors
//         (the xml content of the help files is buggy).
//       - Using a QRegExp to grep for <item> is extremely slow, therefore we use
//         QString::split() first
bool ContextHelpDisplay::parseXml(const QString &doc, QMap<QString, QString> &map)
{
    QString document = doc;
    document.replace(QRegularExpression("\\n"), " ");
    QStringList lst = document.split("<item>", Qt::SkipEmptyParts);

    QRegularExpression rxId("(?:<helpID>)(.*)(?=</helpID>)");
    QRegularExpression rxCt("(?:<content>)(.*)(?=</content>)");
    QRegularExpressionMatch rmId;
    QRegularExpressionMatch rmCt;

    for (int i = 0; i < lst.size(); ++i) {
        rmId = rxId.match(lst.at(i));
        rmCt = rxCt.match(lst.at(i));

        if ((rmId.hasMatch()) && (rmCt.hasMatch())) {
            QString key = rmId.captured(1).trimmed();
            QString val = rmCt.captured(1).trimmed();

            if (key.right(1) == "=") key = key.left(key.length() - 1);

            map.insert(key, val);
        }
    }

    return true;
}
