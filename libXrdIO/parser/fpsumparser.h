/***************************************************************************
                          sumparser.h  -  description
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

#ifndef SUMPARSER_H
#define SUMPARSER_H

#include <QtCore>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

struct phase {
	int number;
	QString sample;
	QString name;
	double rbragg;
	double fraction;
	double crystsize;
	double atz;
	double scale;
	double volume;
	double mp;
};

class XRDIO_EXPORT FpSumParser
{
	public:
                FpSumParser(QString);             // constructur, pass the file name


                QStringList getContents();      // returns the content line by line stored in a string list
                void setFile(const QString &);  // set a new file name
                QList<phase> getPhases();       // returns all phases in a list
                QString getFormattedPhases();   // returns all phases as a formatted string

	private:
		QString fileName;
		QStringList content;


};

#endif
