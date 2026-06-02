/***************************************************************************
                          strfilefilter.cpp  -  description
                             -------------------
    begin                : Fri May 31 08:55:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "strfilefilter.h"
#include "../libXrdIO/bgmnfileio.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

StrFileFilter::StrFileFilter()
{
}

QList<HklPhaseData> StrFileFilter::eliminateUnindexed(const QList<HklPhaseData> &strFiles)
{
    QList<HklPhaseData> indexed;

    for (int i = 0; i < strFiles.size(); ++i) {
        if (!strFiles.at(i).hklBinary().isNull()) indexed.append(strFiles.at(i));
    }

    return indexed;
}

QList<HklPhaseData> StrFileFilter::eliminatePinnedFiles(const QList<HklPhaseData> &strFiles, const QStringList &acceptedPhases)
{
    QList<HklPhaseData> nonPinned;

    for (int i = 0; i < strFiles.size(); ++i) {
        bool skip = false;

        for (int j = 0; j < acceptedPhases.size(); ++j) {
            QFileInfo fiStr(strFiles.at(i).file());
            QFileInfo fiAcc(acceptedPhases.at(j));

            if (fiStr.fileName() == fiAcc.fileName()) {
                skip = true;
                break;
            }
        }

        if (!skip) nonPinned.append(strFiles.at(i));
    }

    return nonPinned;
}

QList<HklPhaseData> StrFileFilter::eliminateDuplicates(const QList<HklPhaseData> &strFiles, const QStringList &acceptedPhases, double thAxis, double thAngle)
{
    QList<HklPhaseData> unique;

    for (int i = 0; i < strFiles.size(); ++i) {
        bool skip = false;

        for (int j = 0; j < acceptedPhases.size(); ++j) {
            QFileInfo fiStr(strFiles.at(i).file());
            QFileInfo fiAcc(acceptedPhases.at(j));

            if (StrFileFilter::compareStrFiles(fiStr.absoluteFilePath(), fiAcc.absoluteFilePath(), thAxis, thAngle)) {
                skip = true;
                break;
            }
        }

        if (!skip) unique.append(strFiles.at(i));
    }

    return unique;
}

bool StrFileFilter::compareStrFiles(const QString &a, const QString &b, double thAxis, double thAngle)
{
    QMap<QString, QVariant> dataA = parseStr(a);
    QMap<QString, QVariant> dataB = parseStr(b);

    if (dataA.value("SGNR").toInt() != dataB.value("SGNR").toInt()) return false;
    if (dataA.value("HM") != dataB.value("HM")) return false;

    if (qAbs(dataA.value("A").toDouble() - dataB.value("A").toDouble()) > thAxis) return false;
    if (qAbs(dataA.value("B").toDouble() - dataB.value("B").toDouble()) > thAxis) return false;
    if (qAbs(dataA.value("C").toDouble() - dataB.value("C").toDouble()) > thAxis) return false;

    if (qAbs(dataA.value("ALPHA").toDouble() - dataB.value("ALPHA").toDouble()) > thAngle) return false;
    if (qAbs(dataA.value("BETA").toDouble()  - dataB.value("BETA").toDouble())  > thAngle) return false;
    if (qAbs(dataA.value("GAMMA").toDouble() - dataB.value("GAMMA").toDouble()) > thAngle) return false;

    return true;
}

QMap<QString, QVariant> StrFileFilter::parseStr(const QString &f)
{
    QString content = BgmnFileIO::readTextFile(f);

    QRegularExpression rxA("(?:PARAM=)?A=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    QRegularExpression rxB("(?:PARAM=)?B=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    QRegularExpression rxC("(?:PARAM=)?C=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");

    QRegularExpression rxAlpha("(?:PARAM=)?ALPHA=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    QRegularExpression rxBeta("(?:PARAM=)?BETA=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    QRegularExpression rxGamma("(?:PARAM=)?GAMMA=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");

    QRegularExpression rxSgNr("SpacegroupNo=(\\d+)");
    QRegularExpression rxHm("HermannMauguin=(\\S+)");

    QRegularExpressionMatch rmA = rxA.match(content);
    QRegularExpressionMatch rmB = rxB.match(content);
    QRegularExpressionMatch rmC = rxC.match(content);

    QRegularExpressionMatch rmAlpha = rxAlpha.match(content);
    QRegularExpressionMatch rmBeta  = rxBeta.match(content);
    QRegularExpressionMatch rmGamma = rxGamma.match(content);

    QRegularExpressionMatch rmSgNr = rxSgNr.match(content);
    QRegularExpressionMatch rmHm   = rxHm.match(content);

    QMap<QString, QVariant> data;

    data["A"]     = rmA.hasMatch()     ? rmA.captured(1).toDouble()     : -1.0;
    data["B"]     = rmB.hasMatch()     ? rmB.captured(1).toDouble()     : -1.0;
    data["C"]     = rmC.hasMatch()     ? rmC.captured(1).toDouble()     : -1.0;
    data["ALPHA"] = rmAlpha.hasMatch() ? rmAlpha.captured(1).toDouble() : -1.0;
    data["BETA"]  = rmBeta.hasMatch()  ? rmBeta.captured(1).toDouble()  : -1.0;
    data["GAMMA"] = rmGamma.hasMatch() ? rmGamma.captured(1).toDouble() : -1.0;
    data["SGNR"]  = rmSgNr.hasMatch()  ? rmSgNr.captured(1).toInt()     : -1;
    data["HM"]    = rmHm.hasMatch()    ? rmHm.captured(1)               : QString();

    return data;
}
