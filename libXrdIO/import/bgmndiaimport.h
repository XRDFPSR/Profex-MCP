/***************************************************************************
                          bgmndiaimport.h  -  description
                             -------------------
    begin                : Thu Jan 26, 2012
    copyright            : (C) 2012 by Nicola Doebelin
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

#ifndef BGMNDIAIMPORT_H
#define BGMNDIAIMPORT_H

#include "genericimport.h"

#include <QStringList>
#include <QColor>
#include <QRegularExpression>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

struct LineDataColumns {
    bool valid;
    double x;
    QVector<double> yValues;
};

class XRDIO_EXPORT BgmnDiaImport : public GenericImport
{
    Q_OBJECT

    public:
        explicit BgmnDiaImport(QObject *parent = 0);

        bool isSupported(const QByteArray &);
        int load(const QString &, QVector<Scan> &, bool minimal = false);
        int loadAndCheck(const QString &, QVector<Scan> &, int &ll, int &tl, bool minimal = false);
        QString uniqueId() {return "BGMN_DIA";}

    private:
        QByteArray _header;
        QList<QVector<double> > _data;
        int _totalDataLines;

        int checkSize(int &ll, int &tl);
        int createScans(QVector<Scan> &, const QString &);
        double getWaveLength(const QByteArray &);
        QStringList getStructureNames(const QByteArray &);
        bool parseBinaryData(QList<QList<double> > &, const QList<QByteArray> &);

        bool parseDouble(const char *&ptr, const char *end, double &value);
        LineDataColumns parseLineColumns(const QByteArray &line);

    signals:
        void maxSteps(int);
        void progress(int);

};

#endif // BGMNDIAIMPORT_H
