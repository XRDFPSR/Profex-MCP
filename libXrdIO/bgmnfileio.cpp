/***************************************************************************
                          bgmnfileio.cpp  -  description
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

#include "bgmnfileio.h"
#include "parser/bgmninstrumentsavparser.h"
#include <QDebug>
#include <QDir>
#include <QTextStream>

BgmnFileIO::BgmnFileIO()
{

}

bool BgmnFileIO::copyFile(const QString &src, const QString &dst)
{
    QFile ifile(src);
    QFile ofile(dst);

    if (!ifile.exists()) {
        qDebug() << QString("BgmnFileIO::copyFile(): Source file doesn't exist: %1").arg(src);
        return false;
    }

    QFileInfo fisrc(src);
    QFileInfo fidst(dst);

    if (fisrc == fidst) {
        qDebug() << QString("BgmnFileIO::copyFile(): Source and destination are identical: %1").arg(fisrc.absoluteFilePath());
        return true;
    }

    if (!fidst.absoluteDir().exists()) {
        if (!fidst.absoluteDir().mkpath(".")) {
            qDebug() << QString("BgmnFileIO::copyFile(): could not create destination dir at: %1").arg(fidst.absolutePath());
            return false;
        }
    }

    if (!ifile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("BgmnFileIO::copyFile(): could not open file for reading: %1").arg(src);
        return false;
    }

    if (!ofile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << QString("BgmnFileIO::copyFile(): could not open file for writing: %1").arg(dst);
        ifile.close(); // this one was already opened
        return false;
    }

    QTextStream istr(&ifile);
    QTextStream ostr(&ofile);

    while (!istr.atEnd()) {
        ostr << istr.readLine() << Qt::endl;
    }

    ifile.close();
    ofile.close();

    return true;
}

QString BgmnFileIO::gatherDevFiles(const QString &geq, const QString &dest, bool overwrite, bool *ok)
{
    if (ok) *ok = true;
    QFileInfo df(geq);
    QString baseName(df.completeBaseName());
    QString sourcePath(df.absolutePath());

    QString geqSource = geq;
    QString geqDest = QDir::fromNativeSeparators(dest + "/" + df.fileName());

    QString gerSource1 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".ger");
    QString gerDest1 = QDir::fromNativeSeparators(dest + "/" + baseName + ".ger");

    QString gerSource2 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".GER");
    QString gerDest2 = QDir::fromNativeSeparators(dest + "/" + baseName + ".GER");

    QString savSource1 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".sav");
    QString savDest1 = QDir::fromNativeSeparators(dest + "/" + baseName + ".sav");

    QString savSource2 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".SAV");
    QString savDest2 = QDir::fromNativeSeparators(dest + "/" + baseName + ".SAV");

    QString tplSource1 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".tpl");
    QString tplDest1 = QDir::fromNativeSeparators(dest + "/" + baseName + ".tpl");

    QString tplSource2 = QDir::fromNativeSeparators(sourcePath + "/" + baseName + ".TPL");
    QString tplDest2 = QDir::fromNativeSeparators(dest + "/" + baseName + ".TPL");

    QString ttailSource;
    QString ttailDest;

    if (QFile::exists(savSource1)) {
        bool sok;
        BgmnInstrumentSavParser isParser(savSource1, &sok);
        if (sok && isParser.hasTubeTails()) {
            ttailSource = QDir::fromNativeSeparators(sourcePath + "/" + isParser.getTubeTailsFile());
            ttailDest   = QDir::fromNativeSeparators(dest + "/" + isParser.getTubeTailsFile());
        }
    } else if (QFile::exists(savSource2)) {
        bool sok;
        BgmnInstrumentSavParser isParser(savSource2, &sok);
        if (sok && isParser.hasTubeTails()) {
            ttailSource = QDir::fromNativeSeparators(sourcePath + "/" + isParser.getTubeTailsFile());
            ttailDest   = QDir::fromNativeSeparators(dest + "/" + isParser.getTubeTailsFile());
        }
    }

    bool copyGeq = true;
    bool copyGer1 = true;
    bool copyGer2 = true;
    bool copySav1 = true;
    bool copySav2 = true;
    bool copyTpl1 = true;
    bool copyTpl2 = true;
    bool copyTtail = true;

    // If overwriting is checked and the destination files exist, remove them.
    // Else skip copying because we don't want to overwrite a (potentially customized)
    // existing file.

    if (QFile::exists(geqDest)) {
        if (overwrite) QFile::remove(geqDest);
        else copyGeq = false;
    }

    if (QFile::exists(gerDest1)) {
        if (overwrite) QFile::remove(gerDest1);
        else copyGer1 = false;
    }

    if (QFile::exists(gerDest2)) {
        if (overwrite) QFile::remove(gerDest2);
        else copyGer2 = false;
    }

    if (QFile::exists(savDest1)) {
        if (overwrite) QFile::remove(savDest1);
        else copySav1 = false;
    }

    if (QFile::exists(savDest2)) {
        if (overwrite) QFile::remove(savDest2);
        else copySav2 = false;
    }

    if (QFile::exists(tplDest1)) {
        if (overwrite) QFile::remove(tplDest1);
        else copyTpl1 = false;
    }

    if (QFile::exists(tplDest2)) {
        if (overwrite) QFile::remove(tplDest2);
        else copyTpl2 = false;
    }

    if (QFile::exists(ttailSource)) {
        if (overwrite) QFile::remove(ttailSource);
        else copyTtail = false;
    }

    int copyErrors = 0;

    QFile fGeqSource(geqSource);
    if (fGeqSource.exists() && copyGeq) {        // binary file
        if (!fGeqSource.copy(geqDest)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(geqSource, geqDest);
        }
    }

    if (QFile::exists(gerSource1) && copyGer1)  { // text file
        if (!BgmnFileIO::copyFile(gerSource1, gerDest1)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(gerSource1, gerDest1);
        }
    }

    if (QFile::exists(gerSource2) && copyGer2)  { // text file
        if (!BgmnFileIO::copyFile(gerSource2, gerDest2)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(gerSource2, gerDest2);
        }
    }

    if (QFile::exists(savSource1) && copySav1)  { // text file
        if (!BgmnFileIO::copyFile(savSource1, savDest1)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(savSource1, savDest1);
        }
    }

    if (QFile::exists(savSource2) && copySav2)  { // text file
        if (!BgmnFileIO::copyFile(savSource2, savDest2)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(savSource2, savDest2);
        }
    }

    if (QFile::exists(tplSource1) && copyTpl1)  { // text file
        if (!BgmnFileIO::copyFile(tplSource1, tplDest1)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(tplSource1, tplDest1);
        }
    }

    if (QFile::exists(tplSource2) && copyTpl2)  { // text file
        if (!BgmnFileIO::copyFile(tplSource2, tplDest2)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(tplSource2, tplDest2);
        }
    }

    if (QFile::exists(ttailSource) && copyTtail) { // text file
        if (!BgmnFileIO::copyFile(ttailSource, ttailDest)) {
            ++copyErrors;
            qDebug() << QString("BgmnFileIO::gatherDevFiles(): Error copying file %1 to %2").arg(ttailSource, ttailDest);
        }
    }

    if (copyErrors > 0) {
        if (ok) *ok = false;
    }
    return geqDest;
}


/*
 * returns a checksum for file "file"
 */
QByteArray BgmnFileIO::checksum(const QString &file, QCryptographicHash::Algorithm algo)
{
    QFile f(file);

    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(algo);
        if (hash.addData(&f)) {
            return hash.result();
        }
    }

    return QByteArray();
}


QStringList BgmnFileIO::gatherStrFiles(const QStringList &files, const QString &dest, bool overwrite, int verbose, bool *ok)
{
    QStringList destFiles;
    if (ok) *ok = true;

    // loop through the files and copy them to the destination location
    for (int i = 0; i < files.size(); ++i) {
        QFileInfo fi(files.at(i));
        QFile strSource(fi.absoluteFilePath());
        QString strDest = QDir::fromNativeSeparators(dest + "/" + fi.fileName());
        destFiles.append(strDest);

        // if the file exists and overwriting is checked, remove the existing file
        if (QFile::exists(strDest)) {
            if (!overwrite) {
                // file exists and must not be overwritten. Skipping it
                if (verbose > 1) qDebug() << QString("BgmnFileIO::gatherStrFiles: File %1 exists and overwriting not requested. Skipping.").arg(strDest);
                continue;
            } else {
                if (verbose > 1) qDebug() << QString("BgmnFileIO::gatherStrFiles: File %1 exists and overwriting requested. Overwriting.").arg(strDest);
            }
        } else {
            if (verbose > 2) qDebug() << QString("BgmnFileIO::gatherStrFiles: File %1 does not exist yet. Copying.").arg(strDest);
        }

        QFile::remove(strDest); // just returns false if the file does not exist
        bool copyOk = BgmnFileIO::copyFile(strSource.fileName(), strDest);
        if (ok && !copyOk) *ok = copyOk;
    }

    return destFiles;
}

QString BgmnFileIO::readTextFile(const QString &fileName)
{
    QFile f(QDir::fromNativeSeparators(fileName));

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("BgmnFileIO::readTextFile(): Opening file for reading failed: %1").arg(fileName);
        return QString();
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::System);
    QString r(in.readAll());
    f.close();
    return r;
}

QStringList BgmnFileIO::readTextFileLines(const QString &fileName)
{
    QFile f(fileName);

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("BgmnFileIO::readTextFileLines(): Opening file for reading failed: %1").arg(fileName);
        return QStringList();
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::System);
    QStringList l;
    QString str;
    bool read = true;

    while (read) {
        str = in.readLine();

        if (str.isNull()) {
            read = false;
        } else {
            l.append(str);
        }
    }

    f.close();
    return l;
}

bool BgmnFileIO::writeTextFile(const QString &fileName, const QString &content)
{
    QFile f(fileName);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << QString("BgmnFileIO::writeTextFile(): Opening file for writing failed: %1").arg(fileName);
        return false;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::System);
    out << content;
    f.close();

    return true;
}

bool BgmnFileIO::readBinaryFile(const QString &fileName, QByteArray &content, int debug)
{
    QFile f(fileName);

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << QString("BgmnFileIO::readBinaryFile(): Opening file for reading failed: %1").arg(fileName);
        return false;
    }

    content = f.readAll();
    f.close();

    // for reverse engineering of binary file formats, set the following
    // section to >= 0

    if (debug >= 0) {
        int bytes = debug > 0 ? debug : content.size();

        for (int i = 0; i < qMin(content.size(), bytes); ++i) {
            qDebug() << decode(content, i);
        }
    }

    return true;
}

QList<QByteArray> BgmnFileIO::readBinaryFileLines(const QString &fileName)
{
    QFile f(fileName);
    QList<QByteArray> out;

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << QString("BgmnFileIO::readBinaryFileLines(): Opening file for reading failed: %1").arg(fileName);
        return out;
    }

    while (!f.atEnd()) {
        out.append(f.readLine());
    }

    return out;
}

bool BgmnFileIO::writeBinaryFile(const QString &fileName, const QByteArray &content)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << QString("BgmnFileIO::writeBinaryFile(): Opening file for writing failed: %1").arg(fileName);
        return false;
    }

    if (file.write(content) != content.size()) {
        qDebug() << QString("BgmnFileIO::writeBinaryFile(): Failed to write all data to file: %1").arg(fileName);
        file.close();
        return false;
    }

    file.close();
    return true;
}

/*
 * returns a recursive list of files in "root" and subdirs, with extension "filters".
 * first string: filename relative to root, second string: absolute file name.
 */
QMap<QString,QString> BgmnFileIO::getFileList(QDir root, QStringList filters)
{
    QMap<QString, QString> fileList;

    QDirIterator it(root.absolutePath(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo fi(it.fileInfo());
        if (!fi.isDir()) {
            if (filters.contains(fi.suffix())) {
                fileList.insert(root.relativeFilePath(fi.absoluteFilePath()), fi.absoluteFilePath());
            }
        }
    }

    return fileList;
}

QString BgmnFileIO::getTempLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

/*
 * this function is only used for reverse engineering of binary file formats
 * It returns a byte array decoded into several types of variables
 */
QString BgmnFileIO::decode(const QByteArray &ba, int pos, bool bStr, bool bChr, bool bSht, bool bInt, bool bLng, bool bFlt, bool bDbl)
{
    int l = ba.size();
    QString r = QString("%1 ").arg(pos);

    if (bStr && (l >= 8))                            r += QString("string: %1 ").arg(QString(ba.mid(pos, 8)), 8);
    if (bChr && (l >= 1))                            r += QString("char: %1 ").arg(char2int(ba.mid(pos, 1)), 4);
    if (bSht && ((uint)l >= sizeof(unsigned short))) r += QString("ushort: %1 ").arg(hex2ushort(ba.mid(pos, sizeof(unsigned short))), 8);
    if (bInt && ((uint)l >= sizeof(int)))            r += QString("int: %1 ").arg(hex2int(ba.mid(pos, sizeof(int))), 12);
    if (bLng && ((uint)l >= sizeof(long)))           r += QString("long: %1 ").arg(hex2long(ba.mid(pos, sizeof(long))), 12);
    if (bFlt && ((uint)l >= sizeof(float)))          r += QString("float: %1 ").arg(hex2float(ba.mid(pos, sizeof(float))), 8, 'g', 8);
    if (bDbl && ((uint)l >= sizeof(double)))         r += QString("double: %1").arg(hex2double(ba.mid(pos, sizeof(double))), 8, 'g', 8);

    return r;
}

QByteArray BgmnFileIO::mapToByteArray(const QMap<double, double> &m)
{
    QByteArray ba;

    QDataStream stream(&ba, QIODevice::WriteOnly);
    QMapIterator<double, double> it(m);

    while (it.hasNext()) {
        it.next();
        stream << it.key() << it.value();
    }

    return ba;
}

QMap<double, double> BgmnFileIO::byteArrayToMap(QByteArray &ba)
{
    QMap<double, double> m;

    QDataStream stream(&ba, QIODevice::ReadOnly);
    double k, v;

    while (!stream.atEnd()) {
        stream >> k >> v;
        m[k] = v;
    }

    return m;
}
