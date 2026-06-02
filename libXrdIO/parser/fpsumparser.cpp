/***************************************************************************
                          sumparser.cpp  -  description
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

#include "fpsumparser.h"


FpSumParser::FpSumParser(QString f)
{
	setFile(f);
}

void FpSumParser::setFile(const QString &f)
{
    fileName = f;
    content.clear();

    QFile fi(fileName);
	if (!fi.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << QString("SumParser: Could not open file %1").arg(f);
		return;
	}

	QTextStream istream(&fi);
	while (!istream.atEnd()) {
		QString line = istream.readLine();
        content.append(line);
	}
}

QStringList FpSumParser::getContents()
{
    return content;
}

QList<phase> FpSumParser::getPhases()
{
	QList<phase> plist;

    QFileInfo fi(fileName);

    for (int i = 0; i < content.size(); ++i) {
        QString line = content.at(i);
		if (line.left(13) == " => Phase No.") {
			phase p;
			p.sample = fi.completeBaseName();
			p.number = line.mid(13, 3).toInt();
			p.name = line.mid(17, 40);
			p.crystsize = 0.0;

			// check if the phase is already present (can happen sometimes.
			// Bug in fullprof?)
			bool present = false;
			for (int j = 0; j < plist.size(); ++j) {
				if (plist.at(j).number == p.number) {
					present = true;
				}
			}

			// only append the phase if it is not already present
			if (!present) {
				plist.append(p);
			}
		}
	}

    for (int i = 0; i < content.size(); ++i) {
        QString line = content.at(i);
		if (line.left(10) == " => Phase:") {
			int n = line.mid(10, 3).toInt();
			if (n > plist.size()) {
				continue;
			}

			++i;
            if (i >= content.size()) {
				break;
			}

            line = content.at(i);

			if (line.left(19) != " => Bragg R-factor:") {
				continue;
			}

			phase *p = &plist[n-1];
			p->rbragg = line.mid(20, 5).toFloat();
			p->fraction = line.mid(65, 7).toFloat();
			p->volume = line.mid(37, 8).toFloat();

			++i;
            if (i >= content.size()) {
				break;
			}

            line = content.at(i);

			if (line.left(14) != " => Rf-factor=") {
				continue;
			}

			p->atz = line.mid(37, 16).toFloat();
		}
	}

	return plist;
}


QString FpSumParser::getFormattedPhases()
{
	QString str;
	QList<phase> plist = getPhases();
    QFileInfo fi(fileName);

	QString thcolor = "#9999ff";
	QString ccolor1 = "#cccccc";
	QString ccolor2 = "#aaaaaa";

	str += "<b>Results for file " + fi.completeBaseName() + "</b>\n\n";
	str += "<table border=\"0\">\n  <tr>\n    <th bgcolor=\"" + thcolor + "\">Phase</th>    <th align=\"right\"  bgcolor=\"" + thcolor + "\">R<sub>Bragg</sub></th>\n";
    str += "    <th align=\"right\"  bgcolor=\"" + thcolor + "\">Quantity (wt-%)</th>\n  </tr>\n";

	for (int i = 0; i < plist.size(); ++i) {
		QString col = ccolor1;

		// check if i is even or odd
		if (2 * (i / 2) == i) {
			col = ccolor2;
		}

		str += "  <tr bgcolor=\"" + col + "\">\n";
		str += "    <td>" + plist.at(i).name + "</td>\n";
		str += "    <td align=\"right\">" + QString::number(plist.at(i).rbragg) + "</td>\n";
		str += "    <td align=\"right\">" + QString::number(plist.at(i).fraction) + "</td>\n";
		str += "  <\tr>\n";
	}

	str += "</table><br><br>\n\n";

	return str;
}

