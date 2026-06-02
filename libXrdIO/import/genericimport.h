/***************************************************************************
                          genericimport.h  -  description
                             -------------------
    begin                : Thu June 01, 2013
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

#ifndef GENERICIMPORT_H
#define GENERICIMPORT_H

#include "../scan.h"
#include "../bgmnfileio.h"
#include "../structs.h"
#include <QObject>
#include <QDebug>
#include <QtCore>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT GenericImport : public QObject
{
    Q_OBJECT
public:
    explicit GenericImport(QObject *parent = 0);

    /*
     * Loads all scans stored in file "QString" into "QVector<Scan>".
     * If "minimal = true" only the absolutely necessary data will be
     * parsed, all optional AuxInfo will be skipped for performance reasons.
     *
     * Compulsory data that will never be skipped include:
     *
     * - data points (2theta / intensity)
     * - primary wavelength
     * - scan name
     *
     * However, even though wavelength and scan name are compulsory, some
     * file formats may not contain this information, so they still CAN be
     * empty.
     */
    virtual int load(const QString &, QVector<Scan> &, bool minimal = false) = 0;

    /*
     * Returns a string with all file extensions belonging to the specific file
     * format (in lower and upper case). This string can be used as file filter
     * in QFileDialogs without further processing.
     */
    virtual QString filter();

    /*
     * Returns all file extensions belonging to the specific format in lower case.
     */
    virtual QStringList extensions();

    /*
     * Returns a description string for the specific file format, to be used in
     * QFileDialogs for example (in conjunction with the "::filter()" string).
     */
    virtual QString description();

    /*
     * Returns a unique identifier string for the specific format. This is used to
     * distinguish between different formats sharing the same file extension, e.g.
     * "RAW". Use this string rather than the ::extension() to refer to a specific
     * file format.
     */
    virtual QString uniqueId() = 0;

    /*
     * Returns true if the header of the scan file appears to be of the specific
     * file format, else returns false. It is recommended to use the first 512
     * bytes of the data for the QByteArray.
     *
     * The implementation of this method should be as efficient as possible,
     * because the import handler has to call ::isSupported() of all existing
     * import filter in order to find the correct one.
     */
    virtual bool isSupported(const QByteArray &) = 0;

    /*
     * Sets the default wavelength, which is the fallback value stored in the
     * resulting Scan() if no other information was read from the raw data file.
     */
    inline void setDefaultWl(double d) {defaultWl = d;}


protected:
    QStringList extens;
    QString descr;
    double defaultWl;

    QString baToString(const QByteArray &, int, int);
};

#endif // GENERICIMPORT_H
