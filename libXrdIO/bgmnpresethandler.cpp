/***************************************************************************
                          bgmnpresethandler.cpp  -  description
                             -------------------
    begin                : Sat Jan 07 10:30:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include "bgmnpresethandler.h"
#include "bgmnfileio.h"
#include "parser/bgmnsavparser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStringList>
#include <QDebug>
#include <QDomDocument>
#include <QMap>

/*
 * Apply a refinement preset file (*.pfp)
 *
 * create a control file and copy all structure and device files
 * specified in the preset file of structure:
 *
 *  <preset>
 *      <device MD5hash="..." RelativePath="...">../Devices/cubix-ads-15mm.sav</device>
 *      <device MD5hash="..." RelativePath="...">../Devices/cubix-ads-15mm.geq</device>
 *      <template MD5hash="..." RelativePath="...">../Devices/cubix-ads-15mm.tpl</template>
 *      <structures>
 *   		<structure MD5hash="..." RelativePath="...">../Structures/betaTCP.str</structure>
 *          <structure MD5hash="..." RelativePath="...">../Structures/hydroxylapatite.str</structure>
 *      </structures>
 *      <other>
 *          <textfile MD5hash="..." RelativePath="...">anyOtherFile.any</textfile>
 *          <textfile MD5hash="..." RelativePath="...">egBackground.xy</textfile>
 *          <binaryfile MD5hash="..." RelativePath="...">anyOtherFile.any</binaryfile>
 *      </other>
 *  </preset>
 *
 * relative or absolute paths can be used. Files specified in "other" tags
 * will just be copied to the destination. It could be a background file
 * for use with UNT=, or a description, or anything else.
 *
 * For <textfiles> BgmnProjectWidget::copyFile() will be used to convert line endings
 * correctly for platform independence.
 *
 * For <binaryfiles> QFile::copy() will be used
 *
 * Presets can also be used for baselines, peak integral ranges, and curve fits only, without
 * a refinement project. Such a preset would look as follows:
 *
 * <preset>
 *     <baseline Mode="SNIP" m="60" w="1"/>
 * </preset>
 *
 * or
 *
 *  <preset>
 *      <peakIntegrals>
 *          <peakIntegral Name="..." Start="..." End="..." Background="..."/>
 *          <peakIntegral Name="..." Start="..." End="..." Background="..."/>
 *      </peakIntegrals>
 *  </preset>
 *
 * or
 *
 *  <preset>
 *       <curveFit>
 *           <curveRange RangeEnd="..." RangeStart="..."/>
 *               <curveVariable LowerLimit="..." Name="..." UpperLimit="..." CheckState="..." Value="..."/>
 *               <curveVariable LowerLimit="..." Name="..." UpperLimit="..." CheckState="..." Value="..."/>
 *               <curveFunction Type="...">
 *                  <ParameterNames>...;...;...</ParameterNames>
 *                  <Variables>...;...;...</Variables>
 *               </curveFunction>
 *           <curveRange>
 *           <curveStepSize StepSizeCustom="..." StepSizeValue="..."/>
 *           <curveControls EpsX="..." MaxInt="..." DiffStep="..."/>
 *       </curveFit>
 *  </preset>
 *
 * or
 *
 * <preset>
 *      <excelWorkbook file="...">
 *          <excelCell WorkSheet=int Row=int Col=int Phase="..." Parameter="..."/>
 *      </excelWorkbook>
 * </preset>
 */

BgmnPresetHandler::BgmnPresetHandler()
{
    fileTags << "device" << "template" << "structure" << "textfile" << "binaryfile";
    allTags = fileTags;
    allTags << "peakIntegral" << "baseline" << "baseLine" << "curveVariable"
            << "curveFunction" << "curveControls" << "curveRange"
            << "curveStepSize" << "excelWorkbook" << "excelCell";
}

QMap<QString, QString> BgmnPresetHandler::getPresets(const QStringList &dirs, const QString &type)
{
    QMap<QString, QString> pfpFiles;

    if (dirs.isEmpty()) {
        qDebug() << QString("BgmnPresetHandler::getPresets(): No Preset repositories found");
        return pfpFiles;
    }

    for (int i = 0; i < dirs.size(); ++i) {
        QDirIterator it(dirs.at(i), QDirIterator::Subdirectories);

        while (it.hasNext()) {
            it.next();

            QFileInfo pfpfi(it.fileInfo());

            if (!pfpfi.isDir()) {
                if ((pfpfi.suffix() == "pfp") || (pfpfi.suffix() == "PFP")) {
                    QString pfiPath = pfpfi.absoluteFilePath();

                    PresetFileInfo pfi = getPresetFileInfo(pfiPath);
                    if (pfi.name.isEmpty()) continue;

                    QString key = QString("%1 (version %2)").arg(pfi.name, pfi.version);

                    if (type == "a")                              pfpFiles.insert(key, pfiPath);
                    else if (type == "b" && pfi.hasBaseLine)      pfpFiles.insert(key, pfiPath);
                    else if (type == "r" && pfi.hasRefinement)    pfpFiles.insert(key, pfiPath);
                    else if (type == "c" && pfi.hasCurveFit)      pfpFiles.insert(key, pfiPath);
                    else if (type == "i" && pfi.hasPeakIntegrals) pfpFiles.insert(key, pfiPath);
                    else if (type == "e" && pfi.hasExcelExport)   pfpFiles.insert(key, pfiPath);
                }
            }
        }
    }

    return pfpFiles;
}

BgmnPresetHandler::PresetFileInfo BgmnPresetHandler::getPresetFileInfo(const QString &f)
{
    PresetFileInfo pfi;

    QString pfiContent = BgmnFileIO::readTextFile(f);
    if (pfiContent.isEmpty()) return pfi;

    QDomDocument doc("presetDoc");
    doc.setContent(pfiContent);

    QDomElement docElem = doc.documentElement();
    pfi.name = docElem.attribute("name", QString());
    pfi.version = docElem.attribute("version", QString());

    pfi.hasBaseLine = !doc.elementsByTagName("baseLine").isEmpty();
    pfi.hasRefinement = !doc.elementsByTagName("device").isEmpty();
    pfi.hasCurveFit = !doc.elementsByTagName("curveFit").isEmpty();
    pfi.hasPeakIntegrals = !doc.elementsByTagName("peakIntegrals").isEmpty();
    pfi.hasExcelExport = !doc.elementsByTagName("excelWorkbook").isEmpty();

    return pfi;
}

/********************************************************************************************
 *                                     Creating a preset                                    *
 ********************************************************************************************/

QDomDocument BgmnPresetHandler::createPresetXmlDocument(const QString &presetFileName)
{
    QDomDocument doc("ProfexPreset");
    QFileInfo fiPresetFile(presetFileName);

    QDomElement rootEl = doc.createElement("preset");
    rootEl.setAttribute("name", fiPresetFile.completeBaseName());
    rootEl.setAttribute("version", QDateTime::currentDateTime().toString("yyyyMMdd"));
    doc.appendChild(rootEl);

    return doc;
}

QDomDocument BgmnPresetHandler::createPreset(const QString &cfile, const QString &pfile, const QString &cfileContent, QDomDocument &doc, bool writeFiles)
{
    QFileInfo fiControlFile(cfile);
    QFileInfo fiPresetFile(pfile);

    if (writeFiles) {
        if (!createPresetDir(fiPresetFile.absolutePath())) {
            qDebug() << QString("BgmnPresetHandler::createPreset(): Could not create directory %1").arg(fiPresetFile.absolutePath());
            return QDomDocument();
        }
    }

    QList<BgmnPresetFile> srcFiles;
    QList<BgmnPresetFile> destFiles;

    if (!cfileContent.isEmpty()) {
        BgmnSavParser sparser(cfileContent, cfile);
        QStringList projectFiles;
        projectFiles << sparser.getStruc();
        projectFiles << allDevFiles(sparser.deviceFile(), fiControlFile, true);

        QString uf = sparser.untFile();
        if (!uf.isEmpty()) projectFiles << uf;

        srcFiles = stringsToFileList(projectFiles, fiControlFile);

        if (writeFiles) {
            destFiles = copyFilesToTemplate(srcFiles, fiPresetFile.absolutePath());
            destFiles << writeTemplateFile(fiPresetFile, sparser.createTemplate());
        } else {
            destFiles = srcFiles;
        }
    }

    createPresetXml(destFiles, doc);

    if (writeFiles) {
        QString presOutFile(fiPresetFile.absoluteFilePath() + (fiPresetFile.suffix().isEmpty() ? ".pfp" : ""));

        if (!BgmnFileIO::writeTextFile(presOutFile, doc.toString())) {
            qDebug() << QString("BgmnPresetHandler::createPreset(): Could not write to file %1").arg(fiPresetFile.absoluteFilePath());
            return QDomDocument();
        }
    }

    return doc;
}

bool BgmnPresetHandler::createPresetDir(const QString &s)
{
    QDir dir(s);
    if (dir.exists()) return true;

    return dir.mkpath(s);
}

QList<BgmnPresetFile> BgmnPresetHandler::stringsToFileList(const QStringList &strings, const QFileInfo &fiControlFile)
{
    QList<BgmnPresetFile> files;

    for (int i = 0; i < strings.size(); ++i) {
        BgmnPresetFile file;

        file.fileInfo = QFileInfo(strings.at(i));

        if (file.fileInfo.isRelative()) {
            file.relativePath = file.fileInfo.path();
            file.fileInfo.setFile(fiControlFile.absolutePath() + "/" + strings.at(i));
        }

        file.type = getFileType(file.fileInfo, fiControlFile);
        // warning: The MD5 hash may be calculated at the source location. If the file is copied with line
        // end conversion, the hash must be re-calculated at the destination!
        file.md5Hash = QString(BgmnFileIO::checksum(file.fileInfo.absoluteFilePath(), QCryptographicHash::Md5).toHex());

        qDebug() << QString("BgmnPresetHandler::stringsToFileList: Parsing file %1").arg(file.fileInfo.absoluteFilePath());
        qDebug() << QString("                                      Relative path %1").arg(file.relativePath);
        qDebug() << QString("                                      Type %1").arg(file.type);
        qDebug() << QString("                                      MD5 hash %1").arg(file.md5Hash);

        files.append(file);
    }

    return files;
}

FileType BgmnPresetHandler::getFileType(const QFileInfo &fi, const QFileInfo &fiControlFile)
{
    QStringList asciiFiles;
    QStringList binFiles;
    asciiFiles << "xy" << "csv" << "xyp" << "dat" << "asc" << "txt" << "xrdml"
               << "sav" << "lst" << "par" << "out" << "dia" << "ger"
               << "fcf" << "res" << "pdb";
    binFiles   << "raw" << "brml" << "rd" << "geq" << "jpg" << "png";

    if (fi.suffix().toLower() == "ger") return DEVICE_ASCII;
    if (fi.suffix().toLower() == "geq") return DEVICE_BINARY;
    if (fi.suffix().toLower() == "str") return STRUCTURE;

    if (fi.suffix().toLower() == "sav") {
        if (fi.completeBaseName() == fiControlFile.completeBaseName()) {
            return TEMPLATE;
        } else {
            return DEVICE_ASCII;
        }
    }

    if (asciiFiles.contains(fi.suffix().toLower())) return TEXT;
    if (binFiles.contains(fi.suffix().toLower())) return BINARY;

    // fallback
    qDebug() << QString("BgmnPresetHandler::getFileType: File %1 is of unknown file type. Falling back to %2").arg(fi.fileName()).arg(BINARY);
    return BINARY;
}

/*
 * returns a file list of destination files
 * Here we use QFile::copy for all file types, because we don't need any conversion of line endings
 * when creating presets.
 */
QList<BgmnPresetFile> BgmnPresetHandler::copyFilesToTemplate(const QList<BgmnPresetFile> &files, const QString &dest)
{
    QList<BgmnPresetFile> destFiles;

    for (int i = 0; i < files.size(); ++i) {
        BgmnPresetFile file = files.at(i);

        QString fsource(file.fileInfo.absoluteFilePath());
        QString fdest(dest + "/" + file.fileInfo.fileName());
        QFile::copy(fsource, fdest);

        file.fileInfo = QFileInfo(fdest);
        destFiles.append(file);

        qDebug() << QString("BgmnPresetHandler::copyFilesToTemplate: Copy %1 to %2").arg(fsource, fdest);
    }

    return destFiles;
}

QStringList BgmnPresetHandler::allDevFiles(const QString &devFile, const QFileInfo &fiControlFile, bool relative)
{
    QStringList allFiles;

    QDir cdir(fiControlFile.absoluteDir());
    QFileInfo fidev(QFileInfo(cdir, devFile));

    QList<QFileInfo> searchFiles;
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".geq");
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".ger");
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".sav");

#ifndef Q_OS_WIN
    // On Unix we must check for different cases.
    // On Windows this would lead to double entries in the preset file
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".GEQ");
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".GER");
    searchFiles << QFileInfo(cdir, fidev.completeBaseName() + ".SAV");
#endif

    for (int i = 0; i < searchFiles.size(); ++i) {
        if (searchFiles.at(i).exists()) {
            if (relative) allFiles << cdir.relativeFilePath(searchFiles.at(i).absoluteFilePath());
            else          allFiles << searchFiles.at(i).absoluteFilePath();
        }
    }

    return allFiles;
}

BgmnPresetFile BgmnPresetHandler::writeTemplateFile(const QFileInfo &fiPresetFile, const QString &content)
{
    BgmnPresetFile file;
    QString tplFileName(fiPresetFile.absolutePath() + "/" + fiPresetFile.completeBaseName() + ".tpl");
    BgmnFileIO::writeTextFile(tplFileName, content);

    file.fileInfo = QFileInfo(tplFileName);
    file.md5Hash = QString(BgmnFileIO::checksum(tplFileName, QCryptographicHash::Md5).toHex());
    file.relativePath = ".";
    file.type = TEMPLATE;

    return file;
}

void BgmnPresetHandler::createPresetXml(const QList<BgmnPresetFile> &files, QDomDocument &doc)
{
    QDomElement rootEl = doc.documentElement();

    bool hasStructs = false;
    bool hasOther   = false;

    for (int i = 0; i < files.size(); ++i) {
        BgmnPresetFile file = files.at(i);
        if (file.type == STRUCTURE) hasStructs = true;
        if (file.type == TEXT)      hasOther   = true;
        if (file.type == BINARY)    hasOther   = true;
    }

    for (int i = 0; i < files.size(); ++i) {
        BgmnPresetFile file = files.at(i);

        if ((file.type == DEVICE_ASCII) || (file.type == DEVICE_BINARY)) {
            QDomElement el = doc.createElement("device");
            el.setAttribute("MD5hash", file.md5Hash);
            el.setAttribute("RelativePath", file.relativePath);
            QDomText txt = doc.createTextNode(file.fileInfo.fileName());
            el.appendChild(txt);
            rootEl.appendChild(el);
        }

        if (file.type == TEMPLATE) {
            QDomElement el = doc.createElement("template");
            el.setAttribute("MD5hash", file.md5Hash);
            el.setAttribute("RelativePath", file.relativePath);
            QDomText txt = doc.createTextNode(file.fileInfo.fileName());
            el.appendChild(txt);
            rootEl.appendChild(el);
        }
    }

    if (hasStructs) {
        QDomElement strucsEl = doc.createElement("structures");

        for (int i = 0; i < files.size(); ++i) {
            BgmnPresetFile file = files.at(i);

            if (file.type == STRUCTURE) {
                QDomElement el = doc.createElement("structure");
                el.setAttribute("MD5hash", file.md5Hash);
                el.setAttribute("RelativePath", file.relativePath);
                QDomText txt = doc.createTextNode(file.fileInfo.fileName());
                el.appendChild(txt);
                strucsEl.appendChild(el);
            }
        }

        rootEl.appendChild(strucsEl);
    }

    if (hasOther) {
        QDomElement otherEl  = doc.createElement("other");

        for (int i = 0; i < files.size(); ++i) {
            BgmnPresetFile file = files.at(i);

            if (file.type == TEXT) {
                QDomElement el = doc.createElement("textfile");
                el.setAttribute("MD5hash", file.md5Hash);
                el.setAttribute("RelativePath", file.relativePath);
                QDomText txt = doc.createTextNode(file.fileInfo.fileName());
                el.appendChild(txt);
                otherEl.appendChild(el);
            }

            if (file.type == BINARY) {
                QDomElement el = doc.createElement("binaryfile");
                el.setAttribute("MD5hash", file.md5Hash);
                el.setAttribute("RelativePath", file.relativePath);
                QDomText txt = doc.createTextNode(file.fileInfo.fileName());
                el.appendChild(txt);
                otherEl.appendChild(el);
            }
        }

        rootEl.appendChild(otherEl);
    }
}

bool BgmnPresetHandler::writePresetFile(const QDomDocument &doc, const QFileInfo &fiPresetFile)
{
    QString presOutFile(fiPresetFile.absoluteFilePath() + (fiPresetFile.suffix().isEmpty() ? ".pfp" : ""));
    return BgmnFileIO::writeTextFile(presOutFile, doc.toString());
}

/********************************************************************************************
 *                                    Applying a preset                                     *
 ********************************************************************************************/

QDomDocument BgmnPresetHandler::parsePresetFile(const QString &s)
{
    QDomDocument doc("preset");
    doc.setContent(BgmnFileIO::readTextFile(s));
    return doc;
}

bool BgmnPresetHandler::runFileConsistencyChecks(const QString &pfile, const QDomDocument &doc, QStringList &missing, QStringList &checksum)
{
    QFileInfo fiPresetFile(pfile);

    QList<BgmnPresetFile> files(domElementsToFiles(doc));
    files = toAbsolutePath(files, fiPresetFile.absolutePath());

    return fileChecks(files, missing, checksum);
}

QString BgmnPresetHandler::createProject(const QString &pfile, const QString &cfile, const QDomDocument &doc, const QString &id, int nth)
{
    QFileInfo fiPresetFile(pfile);
    QFileInfo fiControlFile(cfile);

    QList<BgmnPresetFile> files(domElementsToFiles(doc));
    files = toAbsolutePath(files, fiPresetFile.absolutePath());

    QList<FileType> exclude;
    exclude << TEMPLATE;
    copyFilesToDestination(files, fiControlFile.absolutePath(), exclude);

    QList<BgmnPresetFile> templateFiles(getFilesOfType(files, TEMPLATE));
    QList<BgmnPresetFile> deviceBinFiles(getFilesOfType(files, DEVICE_BINARY));

    if (templateFiles.isEmpty() || deviceBinFiles.isEmpty()) {
        qDebug() << QString("BgmnPresetHandler::createProject: No template or device file found. Exiting.");
        return QString();
    }

    PresetFileInfo pfi = getPresetFileInfo(pfile);

    BgmnSavParser sparser(BgmnFileIO::readTextFile(templateFiles.first().fileInfo.absoluteFilePath()), cfile);
    sparser.setDeviceFile(toDestinationFilePath(deviceBinFiles.first(), QString()));
    sparser.setNumberOfThreads(nth);
    sparser.setPresetName(QString("%1 (v%2)").arg(pfi.name, pfi.version));
    sparser.setSampleId(id);

    return sparser.getContent();
}

QList<BgmnPresetFile> BgmnPresetHandler::getFilesOfType(const QList<BgmnPresetFile> &files, FileType t)
{
    QList<BgmnPresetFile> tFiles;

    for (int i = 0; i < files.size(); ++i) {
        if (files.at(i).type == t) {
            tFiles.append(files.at(i));
        }
    }

    return tFiles;
}

QList<BgmnPresetFile> BgmnPresetHandler::domElementsToFiles(const QDomDocument &doc)
{
    QList<BgmnPresetFile> files;

    for (int i = 0; i < fileTags.size(); ++i) {
        QDomNodeList lst = doc.elementsByTagName(fileTags.at(i));

        for (int j = 0; j < lst.size(); ++j) {
            QDomElement el = lst.at(j).toElement();

            BgmnPresetFile file;
            file.fileInfo = QFileInfo(el.text());
            file.md5Hash = el.attribute("MD5hash", QString());
            file.relativePath = el.attribute("RelativePath", QString());

            if ((el.tagName().toLower() == "device") && (file.fileInfo.suffix().toLower() == "ger")) file.type = DEVICE_ASCII;
            if ((el.tagName().toLower() == "device") && (file.fileInfo.suffix().toLower() == "sav")) file.type = DEVICE_ASCII;
            if ((el.tagName().toLower() == "device") && (file.fileInfo.suffix().toLower() == "geq")) file.type = DEVICE_BINARY;
            if (el.tagName().toLower() == "template")   file.type = TEMPLATE;
            if (el.tagName().toLower() == "structure")  file.type = STRUCTURE;
            if (el.tagName().toLower() == "textfile")   file.type = TEXT;
            if (el.tagName().toLower() == "binaryfile") file.type = BINARY;

            files.append(file);
        }
    }

    return files;
}

QList<BgmnPresetFile> BgmnPresetHandler::toAbsolutePath(const QList<BgmnPresetFile> &files, const QString &src)
{
    QList<BgmnPresetFile> absFiles;

    for (int i = 0; i < files.size(); ++i) {
        qDebug() << QString("BgmnPresetHandler::absoluteSourcePath: Current file path is %1").arg(files.at(i).fileInfo.filePath());

        if (files.at(i).fileInfo.isAbsolute()) {
            qDebug() << QString("    Path is absolute, nothing to change");
            absFiles.append(files.at(i));
        } else {
            BgmnPresetFile file = files.at(i);

            if ((file.fileInfo.path() != ".") && (file.relativePath.isEmpty())) {
                // if the file is stored in a sub dir and relative path is not specified,
                // we must use the subdir as relative path for compatibility with previous
                // versions
                file.relativePath = file.fileInfo.path();
                qDebug() << QString("    Setting relative path to %1").arg(file.fileInfo.path());
            }

            file.fileInfo = QFileInfo(QDir(src), file.fileInfo.filePath());
            qDebug() << QString("    File path changed to %1").arg(file.fileInfo.filePath());
            absFiles.append(file);
        }
    }

    return absFiles;
}

/*
 * true: test passed, continue
 * false: test failed or aborted, abort
 */
bool BgmnPresetHandler::fileChecks(const QList<BgmnPresetFile> &files, QStringList &missingFiles, QStringList &wrongChecksums)
{
    missingFiles = checkExisting(files);
    wrongChecksums = checkMD5sum(files);

    if (!missingFiles.isEmpty()) return false;
    if (!wrongChecksums.isEmpty()) return false;

    return true;
}

QStringList BgmnPresetHandler::checkExisting(const QList<BgmnPresetFile> &files)
{
    QStringList missing;

    for (int i = 0; i < files.size(); ++i) {
        BgmnPresetFile file = files.at(i);

        if (file.fileInfo.exists()) {
            qDebug() << QString("BgmnPresetHandler::checkExisting: File exists:  %1").arg(file.fileInfo.absoluteFilePath());
        } else {
            missing.append(file.fileInfo.absoluteFilePath());
            qDebug() << QString("BgmnPresetHandler::checkExisting: File missing: %1").arg(file.fileInfo.absoluteFilePath());
        }
    }

    return missing;
}

QStringList BgmnPresetHandler::checkMD5sum(const QList<BgmnPresetFile> &l)
{
    QStringList md5failed;

    for (int i = 0; i < l.size(); ++i) {
        BgmnPresetFile file = l.at(i);

        QByteArray savedHash;
        savedHash.append(file.md5Hash.toUtf8());

        QByteArray currentHash = BgmnFileIO::checksum(file.fileInfo.absoluteFilePath(), QCryptographicHash::Md5);

        if (currentHash != QByteArray::fromHex(savedHash)) {
            md5failed.append(file.fileInfo.absoluteFilePath());
            qDebug() << QString("BgmnPresetHandler::checkMD5sum: Checksum mismatch in file %1").arg(file.fileInfo.absoluteFilePath());
        } else {
            qDebug() << QString("BgmnPresetHandler::checkMD5sum: Checksum matches of file %1").arg(file.fileInfo.absoluteFilePath());
        }
    }

    return md5failed;
}

QStringList BgmnPresetHandler::checkOverwrite(const QList<BgmnPresetFile> &l, const QString &dest)
{
    QStringList owWarning;

    for (int i = 0; i < l.size(); ++i) {
        QString destFile;
        BgmnPresetFile file = l.at(i);

        destFile = toDestinationFilePath(file, dest);

        QFileInfo fiDestFile(destFile);
        if (fiDestFile.exists()) {
            owWarning.append(fiDestFile.absoluteFilePath());
            qDebug() << QString("BgmnPresetHandler::checkOverwrite: Destination file already exists %1").arg(fiDestFile.absoluteFilePath());
        } else {
            qDebug() << QString("BgmnPresetHandler::checkOverwrite: Destination file does not exist %1").arg(fiDestFile.absoluteFilePath());
        }
    }

    return owWarning;
}

void BgmnPresetHandler::copyFilesToDestination(const QList<BgmnPresetFile> &files, const QString &dest, const QList<FileType> &exclude)
{
    for (int i = 0; i < files.size(); ++i) {
        if (exclude.contains(files.at(i).type)) {
            continue;
        }

        QString src(files.at(i).fileInfo.absoluteFilePath());
        QString dst(toDestinationFilePath(files.at(i), dest));

        bool b = false;

        if ((files.at(i).type == DEVICE_BINARY) || (files.at(i).type == BINARY)) {
            b = QFile::copy(src, dst);
        } else {
            b = BgmnFileIO::copyFile(src, dst);
        }

        if (b) qDebug() << QString("BgmnPresetHandler::copyFilesToDestination: File copied from %1 to %2").arg(src, dst);
        else   qDebug() << QString("BgmnPresetHandler::copyFilesToDestination: Error copying file from %1 to %2").arg(src, dst);
    }
}

QString BgmnPresetHandler::toDestinationFilePath(const BgmnPresetFile &f, const QString &destDir)
{
    if (f.relativePath.isEmpty()) {
        if (destDir.isEmpty()) return f.fileInfo.fileName();
        return QDir::toNativeSeparators(destDir + "/" + f.fileInfo.fileName());
    }

    if (destDir.isEmpty()) return QDir::toNativeSeparators(f.relativePath + "/" + f.fileInfo.fileName());
    return QDir::toNativeSeparators(destDir + "/" + f.relativePath + "/" + f.fileInfo.fileName());
}
