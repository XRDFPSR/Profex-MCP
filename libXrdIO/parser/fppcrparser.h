/***************************************************************************
                          pcrparser.h  -  description
                             -------------------
    begin                : Mon Jan 20 14:16:07 CEST 2009
    copyright            : (C) 2005 by Nicola Doebelin
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

#ifndef PCRPARSER_H
#define PCRPARSER_H

#include <QtCore>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT FpPcrParser
{
	public:
		FpPcrParser(QString);
        FpPcrParser();

        void setComment(const QString &);
		void setCycles(int);
		void setNumberOfParameters(int);
		void clearChiSquare();
        void setNumberOfPhases(int);
        void setDevice(double u, double v, double w, double x, double y, double sl, double dl, double shape, double asyLim, double cthm, double npr);
        void getDevice(double &u, double &v, double &w, double &x, double &y, double &sl, double &dl, double &shape, double &asyLim, double &cthm, double &npr);

        QString getContentsString();
        QVector<double> getCrystSizes(double wl, double k = 1.0);
		QVector<double> getScales();
		int getCycles();
        int getNumberOfPhases();
        QStringList getPhases();
        QStringList getPhaseNames();
        QString getPhaseName(const QString &);
        QString getHeader();
        QString sampleID();
        double getWavelength(int n = 0);
        void appendPhase(const QString &);
        void removePhase(const QString &);

		void setFile(const QString &);
		void save();
        void setContentsString(const QString &);
        void setDataFileName(const QString &);

	private:
		QString fileName;
		QStringList content;

        void changeParam(const QString &key, const QString &val, const QString &cline);
        int changeParam(const QString &key, const QString &val, const QString &cline, int line);
        int changeParam(const QRegularExpression &key, const QString &val, const QRegularExpression &cline, int line);
        int getIntValue(const QRegularExpression &key, const QRegularExpression &cline);
        int getIntValue(const QString &key, const QString &cline);
        double getDoubleValue(const QRegularExpression &key, const QRegularExpression &cline);
        double getDoubleValue(const QString &key, const QString &cline);
        QVector<int> getAllIntValues(const QRegularExpression &key, const QRegularExpression &cline);
        QVector<int> getAllIntValues(const QString &key, const QString &cline);
        QVector<double> getAllDoubleValues(const QRegularExpression &key, const QRegularExpression &cline);
        QVector<double> getAllDoubleValues(const QString &key, const QString &cline);
};

#endif
