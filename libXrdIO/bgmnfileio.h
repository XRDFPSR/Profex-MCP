/***************************************************************************
                          bgmnfileio.h  -  description
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

#ifndef BGMNFILEIO_H
#define BGMNFILEIO_H

#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QByteArray>
#include <QCryptographicHash>
#include <QtCore>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnFileIO
{
public:
    BgmnFileIO();

    /*
     * Finds all BGMN device files associated with the GEQ file "geq" and copies them to directory "dest".
     * geq must be given as absolute file path.
     *
     * The gathered device files include: *.geq, *.GEQ, *.ger, *.GER, *.sav, *.SAV
     *
     * Note: geq refers to the file at the SOURCE location, e.g. in the device file directory.
     * It will be copied to dest, too.
     *
     * overwrite = true: existing files in dest will be overwritten
     * overwrite = false: existing files in dest will be kept
     *
     * Line endings will be converted to the present platform.
     *
     * Returns the absolute file name of the geq file (with absolute path).
     */
    static QString gatherDevFiles(const QString &geq, const QString &dest, bool overwrite, bool *ok = nullptr);

    /*
     * Finds all BGMN structure files given in "files" and copies them to directory "dest"
     * files in "files" must be given as absolute file paths.
     *
     * overwrite = true: existing files in dest will be overwritten
     * overwrite = false: existing files in dest will be kept
     *
     * returns a list of all destination files with absolute path
     */
    static QStringList gatherStrFiles(const QStringList &files, const QString &dest, bool overwrite, int verbose = 3, bool *ok = nullptr);

    /*
     * Copies text file src to dst. At the same time end of line characters are converted
     * to the present platform.
     */
    static bool copyFile(const QString &src, const QString &dst);

    /*
     * Calculates the checksum of file "file" using algorithm "algo"
     *
     * returns the checksum if file was read successfully
     * returns QByteArray() if reading file failed
     */
    static QByteArray checksum(const QString &file, QCryptographicHash::Algorithm algo);

    /*
     * Reads a text file
     * provide the absolute file path as fileName
     *
     * returns the content as a string if reading was successful
     * returns QString() if reading was not successful
     *
     * Note: End of line characters will be converted to \n
     */
    static QString readTextFile(const QString &fileName);

    /*
     * Reads a text file
     * provide the absolute file path as fileName
     *
     * returns the content as a stringList if reading was successful
     * returns QStringList() if reading was not successful
     */
    static QStringList readTextFileLines(const QString &fileName);

    /*
     * writes the string content to file fileName
     *
     * returns true if successful
     * returns false if not successful
     *
     * Note: End of line characters will be converted to the current platform
     */
    static bool writeTextFile(const QString &fileName, const QString &content);


    /*
     * Reads the binary file fileName into QByteArray content.
     *
     * Returns true on success.
     * Returns false on failure.
     *
     * If debug >= 0: the binary file will be decoded and output will be written to
     * stdout. Warning: Major performance loss! Only use for reverse engineering of
     * binary file formats.
     */
    static bool readBinaryFile(const QString &fileName, QByteArray &content, int debug = -1);

    /*
     * Reads a binary file
     * provide the absolute file path as fileName
     *
     * returns the content as a QByteArrayList if reading was successful
     * returns QList<QByteArray>() if reading was not successful
     */
    static QList<QByteArray> readBinaryFileLines(const QString &fileName);

    /*
     * Writes a binary file
     * provide the absolute file path as fileName
     *
     * returns true if successful
     * returns false if writing failed
     */
    static bool writeBinaryFile(const QString &fileName, const QByteArray &content);

    /*
     * Serializes a QMap<double, double> to a QByteArray, e.g. for storage in a QVariant
     *
     * returns the serialized byte array
     */
    static QByteArray mapToByteArray(const QMap<double, double> &m);


    /*
     * Reads from a serialized QMap<double, double> and returns the map.
     */
    static QMap<double, double> byteArrayToMap(QByteArray &ba);


    /*
     * Functions to decode QByteArrays into variables. Used to read binary data.
     *
     * To read the first 8 bytes of a QByteArray into a double variable, use
     * these functions as follows:
     *
     * QByteArray a; // holds the binary data read from the raw file
     * bool b;
     * double d = hex2double(a.mid(0, 8), &b);
     *
     * Note that the size of variables is defined by the binary file format, not
     * by the host platform. Thus using a.mid(0, sizeof(long)) etc. will not work,
     * because sizeof(long) is 4 on 32bit, and 8 on 64bit systems. The file format
     * will only use one or the other.
     */
    static inline double hex2double(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(*reinterpret_cast<const double *>(ba.left(sizeof(double)).constData()));
    }

    static inline double hex2float(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(*reinterpret_cast<const float *>(ba.left(sizeof(float)).constData()));
    }

    static inline unsigned short hex2ushort(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(*reinterpret_cast<const unsigned short *>(ba.left(sizeof(ushort)).constData()));
    }

    static inline int hex2int(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(*reinterpret_cast<const int *>(ba.left(sizeof(int)).constData()));
    }

    static inline int hex2long(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(*reinterpret_cast<const long *>(ba.left(sizeof(long)).constData()));
    }


    static inline int char2int(const QByteArray &ba, bool * ok = 0)
    {
        Q_UNUSED(ok);
        return qFromLittleEndian(static_cast<int>(*ba.mid(0, 1).constData()));
    }


    /*
     * returns a recursive list of files in "root" and subdirs, with extension "filters".
     * first string: filename relative to root, second string: absolute file name.
     */
    static QMap<QString,QString> getFileList(QDir root, QStringList filters);

    /*
     * returns the system's location for temporary files
     */
    static QString getTempLocation();

    /*
     * (Only used for debug and reverse engineering purposes)
     *
     * This function dumps the data read from a binary file to the return string
     * and interprets the bytes as various variable types.
     *
     * Warning: This generates return strings of enormous size!
     */
    static QString decode(const QByteArray &, int pos = 0, bool bStr = true, bool bChr = true, bool bSht = true, bool bInt = true, bool bLng = true, bool bFlt = true, bool bDbl = true);
};

#endif // BGMNFILEIO_H
