/***************************************************************************
                          oqprojecthandler.cpp  -  description
                             -------------------
    begin                : Thu Mar 02 18:00:00 CEST 2023
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

#include "oqprojecthandler.h"
#include "../libXrdIO/bgmnfileio.h"
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QDir>

OqProjectHandler::OqProjectHandler()
{

}

bool OqProjectHandler::createProject(const QString &name, const QString &targetDir, QString &scanFile, QString &filter, QString &errors)
{
    if (name.toLower() == "lab6") {
        // the content of this iq project is hard-coded, since it is not supposed to change at any time
        scanFile = QString("%1%2%3").arg(targetDir).arg(QDir::separator()).arg("LaB6_Profex-OQ-20230303.xrdml");
        filter = "PANalytical XRDML scan (*.xrdml *.XRDML)";

        QStringList xrdmlFileContent, savFileContent, strFileContent, gerFileContent, errorMsgs;
        QString xrdmlOfile(targetDir + QDir::separator() + "LaB6_Profex-OQ-20230303.xrdml");
        QString savOfile(targetDir   + QDir::separator() + "LaB6_Profex-OQ-20230303.sav");
        QString strOfile(targetDir   + QDir::separator() + "LaB6.str");
        QString gerOfile(targetDir   + QDir::separator() + "pw1800-ads-10mm.ger");
        QString geqOfile(targetDir   + QDir::separator() + "pw1800-ads-10mm.geq");

        xrdmlFileContent = BgmnFileIO::readTextFileLines(QString(":/oqproject/LaB6_PW1800_5-120.xrdml"));
        savFileContent   = BgmnFileIO::readTextFileLines(QString(":/oqproject/LaB6_PW1800_5-120.sav"));
        strFileContent   = BgmnFileIO::readTextFileLines(QString(":/oqproject/LaB6.str"));
        gerFileContent   = BgmnFileIO::readTextFileLines(QString(":/oqproject/pw1800-ads-10mm.ger"));
        QFile fGeq(":/oqproject/pw1800-ads-10mm.geq");

        if (xrdmlFileContent.isEmpty()) qDebug() << QString("OqProjectHandler::createProject(): Could not read resource file LaB6_PW1800_5-120.xrdml");
        if (savFileContent.isEmpty())   qDebug() << QString("OqProjectHandler::createProject(): Could not read resource file LaB6_PW1800_5-120.sav");
        if (strFileContent.isEmpty())   qDebug() << QString("OqProjectHandler::createProject(): Could not read resource file LaB6.str");
        if (gerFileContent.isEmpty())   qDebug() << QString("OqProjectHandler::createProject(): Could not read resource file pw1800-ads-10mm.ger");

        if (!BgmnFileIO::writeTextFile(xrdmlOfile, xrdmlFileContent.join("\n"))) errorMsgs.append(xrdmlOfile);
        if (!BgmnFileIO::writeTextFile(savOfile, savFileContent.join("\n")))     errorMsgs.append(savOfile);
        if (!BgmnFileIO::writeTextFile(strOfile, strFileContent.join("\n")))     errorMsgs.append(strOfile);
        if (!BgmnFileIO::writeTextFile(gerOfile, gerFileContent.join("\n")))     errorMsgs.append(gerOfile);
        if (!fGeq.copy(geqOfile))                                                errorMsgs.append(geqOfile);

        if (errorMsgs.size()) {
            errors  = QString("Errors occurred during project creation. ");
            errors += QString("Could not create files in the destination directory:\n\n");
            errors += targetDir;
            errors += QString("\n\nWrite access required. Check permissions in the destination directory or select a different destination.");

            qDebug() << QString("OqProjectHandler::createProject(): Errors occurred during project creation");
            qDebug() << QString("    Could not create files:");
            for (int i = 0; i < errorMsgs.size(); ++i) qDebug() << QString("    %1").arg(errorMsgs.at(i));

            return false;
        }

        errors = QString("OQ project was successfully created in:\n\n%1\n\n").arg(targetDir);
        errors += QString("Now run the refinement to test the backend configuration.");
        qDebug() << QString("OqProjectHandler::createProject(): Project successfully created in %1").arg(targetDir);
        return true;
     }

    qDebug() << QString("OqProjectHandler::createProject(): No project \"%1\" available.").arg(name);
    return false;
}


bool OqProjectHandler::createEmptyProject(const QString &name, const QString &targetDir, QString &scanFile, QString &filter, QString &errors)
{
    if (name.toLower() == "lab6") {
        // the content of this iq project is hard-coded, since it is not supposed to change at any time
        scanFile = QString("%1%2%3").arg(targetDir).arg(QDir::separator()).arg("LaB6_Profex-OQ-20230303.xrdml");
        filter = "PANalytical XRDML scan (*.xrdml *.XRDML)";

        QStringList xrdmlFileContent, errorMsgs;
        QString xrdmlOfile(targetDir + QDir::separator() + "LaB6_Profex-OQ-20230303.xrdml");

        xrdmlFileContent = BgmnFileIO::readTextFileLines(QString(":/oqproject/LaB6_PW1800_5-120.xrdml"));

        if (xrdmlFileContent.isEmpty()) qDebug() << QString("OqProjectHandler::createEmptyProject(): Could not read resource file LaB6_PW1800_5-120.xrdml");

        if (!BgmnFileIO::writeTextFile(xrdmlOfile, xrdmlFileContent.join("\n"))) errorMsgs.append(xrdmlOfile);

        if (errorMsgs.size()) {
            errors  = QString("Errors occurred during project creation. ");
            errors += QString("Could not create files in the destination directory:\n\n");
            errors += targetDir;
            errors += QString("\n\nWrite access required. Check permissions in the destination directory or select a different destination.");

            qDebug() << QString("OqProjectHandler::createEmptyProject(): Errors occurred during project creation");
            qDebug() << QString("    Could not create files:");
            for (int i = 0; i < errorMsgs.size(); ++i) qDebug() << QString("    %1").arg(errorMsgs.at(i));

            return false;
        }

        errors = QString("OQ project was successfully created in:\n\n%1\n\n").arg(targetDir);
        errors += QString("Now run the refinement to test the backend configuration.");
        qDebug() << QString("OqProjectHandler::createEmptyProject(): Project successfully created in %1").arg(targetDir);
        return true;
     }

    qDebug() << QString("OqProjectHandler::createEmptyProject(): No project \"%1\" available.").arg(name);
    return false;
}


