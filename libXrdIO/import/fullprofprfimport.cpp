/***************************************************************************
                          fullprofprfimport.cpp  -  description
                             -------------------
    begin                : Mon Jan 16, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMap>

#include "fullprofprfimport.h"
#include "fullprofsubimport.h"

FullprofPrfImport::FullprofPrfImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "prf";
    descr = QLatin1String("Fullprof refined profile type 3");
}

FullprofPrfImport::~FullprofPrfImport()
{
}

/************* PUBLIC FUNCTIONS *************/

/* - If the load function returns -1, don't use any of the 
     get* functions. They are bound to break!

   - Always make sure that the load function was called at least once
     before using the get* functions. Otherwise you'll get an error message. */

bool FullprofPrfImport::isSupported(const QByteArray &ba)
{
    QStringList header = QString(ba).split(global::rxLineEnding);

    if (header.size() < 3) {
        qDebug() << QString("FullprofPrfImport::isSupported(): Less than 3 lines provided");
        return false;
    }

    QRegularExpression rx("^\\s*2Theta\\s+Yobs\\s+Ycal\\s+Yobs-Ycal\\s+Backg\\s+Posr\\s+\\(hkl\\)\\s+K\\s*$");
    QRegularExpressionMatch rm;

    for (int i = 2; i < header.size(); ++i ) {
        rm = rx.match(header.at(i));

        if (rm.hasMatch()) {
                return true;
        }
    }

    return false;
}

int FullprofPrfImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("FullprofPrfImport::load(): Loading file %1").arg(file);

    QByteArray ba;
    if (!BgmnFileIO::readBinaryFile(file, ba)) {
        qDebug() << QString("FullprofPrfImport::load(): Could not read file %1").arg(file);
        return -1;

    }

    _contents = ba.split('\n');

    // do some checks to see if the file is valid
    if (_contents.size() < 5) {
        qDebug() << QString("FullprofPrfImport::load(): PRF file is too short %1").arg(file);
        return -1;
    }

    getScans(scanHeap, file);

    // read the hkl indices
    if (scanHeap.size()) {
        QVector<Hkl> &v = scanHeap[0].pDataHkl();
        getReflections(v);
    }

    getSubScans(file, scanHeap);

    return scanHeap.size();
}

/************* PRIVATE FUNCTIONS *************/

void FullprofPrfImport::getScans(QVector<Scan> &scanHeap, const QString &file)
{
	scanHeap.clear();

    QVector<double> vec_a;
    Scan vec_o("I observed", QColor(0, 0, 0), 1);
    Scan vec_c("I calculated", QColor(255, 0, 0), 1);
    Scan vec_d("I difference", QColor(187, 187, 187), 1);
    Scan vec_x("Background", QColor(0, 0, 255), 1);

    vec_o.setSourceFileName(file);
    vec_c.setSourceFileName(file);
    vec_d.setSourceFileName(file);
    vec_x.setSourceFileName(file);

    vec_o.setTypes(Scan::XY | Scan::MEASURED | Scan::ISABOVEBACKGROUND);
    vec_c.setTypes(Scan::XY | Scan::REFINED | Scan::ISABOVEBACKGROUND);
    vec_d.setTypes(Scan::XY | Scan::REFINED | Scan::DIFF);
    vec_x.setTypes(Scan::XY | Scan::REFINED | Scan::BACKGROUND);

	// loop through the lines containing the scans
    int start = startIndexReflections();
    int end = start + getNumberOfDataPoints();

    for ( int i = start; i < end; ++i)
	{
        QList<QByteArray> l = _contents.at(i).simplified().split(' ');

		// break if the line does not contain all values we need
		if (l.size() < 5) {
			break;
		}

        vec_a.push_back(l.at(0).toDouble());
        vec_o.pDataIntensity().append(l.at(1).toDouble());
        vec_c.pDataIntensity().append(l.at(2).toDouble());
        vec_d.pDataIntensity().append(l.at(3).toDouble());
        vec_x.pDataIntensity().append(l.at(4).toDouble());
	}

	vec_o.setDataAng(vec_a);
	vec_c.setDataAng(vec_a);
	vec_d.setDataAng(vec_a);
	vec_x.setDataAng(vec_a);

	// this is only needed if the difference curve is calculated instead of read from the file
// 	double diff_offset = -1.2 * vec_d.maxIntensity();
// 	vec_d.setYoffset(diff_offset);

    double wl = getWavelength();
    vec_o.setWaveLength(wl);
    vec_c.setWaveLength(wl);
    vec_d.setWaveLength(wl);
    vec_x.setWaveLength(wl);

	scanHeap.push_back(vec_o);
	scanHeap.push_back(vec_c);
	scanHeap.push_back(vec_d);
	scanHeap.push_back(vec_x);
}

/*
 * locates all sub phase files with the same basename as the PRF file (+ incrementing number)
 * and loads them, too
 */
void FullprofPrfImport::getSubScans(const QString &file, QVector<Scan> &scanHeap)
{
    // compile a filter list for sub files
    QFileInfo fi(file);
    QStringList subFilter;
    subFilter << QString("%1*.sub").arg(fi.completeBaseName()) << QString("%1*.SUB").arg(fi.completeBaseName());

    // get the file names
    QDir subDir(fi.absoluteDir());
    QFileInfoList subFiles = subDir.entryInfoList(subFilter, QDir::Files);

    // qDebug() << QString("FullprofPrfImport::getSubScans(): Found %1 files with filter %2").arg(subFiles.size()).arg(subFilter.at(0));

    // load the files
    int pfiles = scanHeap.size();     // number of scans from the normal prf file
    int phases = getNumberOfPhases(); // number of phases, because we don't want to load old sub files from previous refinements,
                                      // we must not iterate over files beyond the number of phases.
    for (int i = 0; i < qMin(subFiles.size(), phases); ++i) {
        FullprofSubImport simp;
        simp.load(subFiles.at(i).absoluteFilePath(), scanHeap);

        // add the background intensity
        if (scanHeap.size() > 4) {
            int s = scanHeap.size() - 1;
            Scan *bg = &scanHeap[3];
            Scan *sb = &scanHeap[s];

            for (int i = 0; i < qMin(bg->size(), sb->size()); ++i) {
                sb->pDataIntensity()[i] += bg->pDataIntensity()[i];
            }
        }
    }

    // qDebug() << QString("FullprofPrfImport::getSubScans(): %1 sub files imported").arg(scanHeap.size() - pfiles);

    // check the size again, because we are going to use pointers and don't want to crash
    // due to invalid pointers
    if (!scanHeap.size()) {
        // qDebug() << QString("FullprofPrfImport::getSubScans(): scanHeap is empty, exiting");
        return;
    }

    // in the Iobs scan, all hkl indices are stored, but the only way to separate them is their
    // vposition. So we will distribute all Hkl indices on new vectors stored in a map.
    QVector<Hkl> &vec = scanHeap[0].pDataHkl();
    QMap<int, QVector<Hkl> > map;

    // Hkl.vPosition() will be the key, vector<Hkl> the value
    for (int i = 0; i < (int)vec.size(); ++i) {
        map[-vec[i].vPosition()].push_back(vec[i]);
    }

    // now that all hkl are stored in new vectors, we clear the original one from Iobs
    vec.clear();

    // assign the new vectors to the subphases
    QMap<int, QVector<Hkl> >::const_iterator it = map.constBegin();

    while (it != map.constEnd()) {
        if (pfiles < scanHeap.size()) {
            // copy the vector<Hkl> to the scanHeap's pDataHkl
            QVector<Hkl> &v = scanHeap[pfiles].pDataHkl();
            v = it.value();
        }

        ++pfiles;
        ++it;
    }
}

void FullprofPrfImport::getExclRegs(QVector<double> &exclRegs)
{
    if (_contents.size() < 4) {
        // qDebug() << QString("FullprofPrfImport::getExclRegs(): Call FullprofPrfImport::load() before trying to access PRF data.");
		return;
	}

	exclRegs.clear();

	int n = getNumberOfExcludedRegions();

	if (n == 0) {
		return;
	}

	for (int i = 0; i < n; ++i) {
        exclRegs.push_back(_contents.at(i+3).left(14).toFloat());
        exclRegs.push_back(_contents.at(i+3).right(14).toFloat());
	}
}


void FullprofPrfImport::getReflections(QVector<Hkl> &vec)
{
    if (_contents.size() < 4) {
        // qDebug() << QString("FullprofPrfImport::getReflections(): Call FullprofPrfImport::load() before trying to access PRF data.");
		return;
	}

    vec.clear();

    int start = startIndexHkl();
    int end = start + getNumberOfHkl();

    for (int i = start; i < end; ++i) {
        if (_contents.at(i).indexOf("(", 0) == -1) {
			// the line must contain a "(", else it is not valid
            continue;
		}

        QRegularExpression rx("(\\d+\\.?\\d*)\\s+(-?\\d+)\\s+(\\([\\+-\\d\\s]{9,9}\\))");
        QRegularExpressionMatch rm = rx.match(_contents.at(i));

        if (!rm.hasMatch()) {
            qDebug() << QString("FullprofPrfImport::getReflections(): HKL data not found in line %1").arg(i);
            break;
        }

        double pos = rm.captured(1).toDouble();
        int vpos = rm.captured(2).toInt();	// the vertical position

        // try to get rid of the parentheses by extracting the part in between
        QString s = rm.captured(3);

        QString hkl = QString("%1 %2 %3").arg(s.mid(1, 3)).arg(s.mid(4, 3)).arg(s.mid(7, 3));

        vec.push_back(Hkl(pos, hkl, vpos, "", QColor(Qt::black)));
	}
}

/************* PRIVATE FUNCTIONS *************/

double FullprofPrfImport::getWavelength()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
#endif
    bool ok;
    double wl = line.at(2).toDouble(&ok);

    if (!ok) return 0.0;
	return wl;
}

int FullprofPrfImport::getNumberOfPhases()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
#endif
    return line.at(0).toInt();
}

int FullprofPrfImport::getNumberOfExcludedRegions()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList line = QString(_contents.at(2)).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList line = QString(_contents.at(2)).trimmed().split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
#endif

    return line.last().toInt();
}

int FullprofPrfImport::getNumberOfDataPoints()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList line = QString(_contents.at(1)).trimmed().split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
#endif

    return line.at(1).toInt();
}

int FullprofPrfImport::getNumberOfHkl()
{
	int p = getNumberOfPhases();
	int r = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList l = QString(_contents.at(2)).trimmed().split(QRegularExpression("\\s"), Qt::SkipEmptyParts);
#else
    QStringList l = QString(_contents.at(2)).trimmed().split(QRegularExpression("\\s"), QString::SkipEmptyParts);
#endif

	// check if the range of the for-loop is valid
	if (l.size() < p) {
        return 0;
	}

	for (int i = 0; i < p; ++i) {
        r += l.at(i).toInt();
	}

    return r;
}

int FullprofPrfImport::startIndexReflections()
{
    for (int i = 0; i < _contents.size(); ++i) {
        if (_contents.at(i).contains("2Theta")) {
			return i + 1;
		}
	}

	return 0;
}

int FullprofPrfImport::startIndexHkl()
{
    return startIndexReflections() + getNumberOfDataPoints();
}
