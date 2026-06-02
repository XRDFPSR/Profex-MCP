/***************************************************************************
                          pcrparser.cpp  -  description
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

#include "fppcrparser.h"
#include "../libXrdIO/structs.h"
#include <math.h>

FpPcrParser::FpPcrParser(QString f)
{
	setFile(f);
}

FpPcrParser::FpPcrParser()
{

}

void FpPcrParser::setFile(const QString &f)
{
    fileName = f;
    content.clear();

    QFile fi(fileName);
	if (!fi.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << QString("PcrParser: Could not open file %1").arg(f);
		return;
	}

	QTextStream istream(&fi);
	while (!istream.atEnd()) {
		QString line = istream.readLine();
        content.append(line);
	}
}

/*
  accepts a string and splits it at every newline. this
  can be used to send the content of a pcr file as a string
  rather than reading it from a file
*/
void FpPcrParser::setContentsString(const QString &s)
{
    content = s.split(global::rxLineEnding);
}

void FpPcrParser::setComment(const QString &c)
{
    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).indexOf(QRegularExpression("^COMM\\b"), 0) >= 0) {
            content[i] = QString("COMM %1").arg(c);
			return;
		}
	}
	qDebug() << "PcrParser: Could not set the comment string";
}

/*
   sets the number of cycles "NCY"
   NOTE: It is recommended to start looping over _contents at position 1, which
       skips the COMM line. The COMM line can, in theory, contain strings of
       variable names (e.g. !NCY), which would cause the looping algorithms to read
       the wrong line. So starting at _contents.at(1) is safer.
*/
void FpPcrParser::setCycles(int c)
{
    changeParam("NCY", QString::number(c), "NCY");
}

void FpPcrParser::setNumberOfParameters(int p)
{
        for (int i = 1; i < content.size(); ++i) {
        if (content.at(i).contains(QString("!Number of refined parameters"))) {
            content[i] = QString("%1    !Number of refined parameters").arg(p);
			return;
		}
	}

	qDebug() << "PcrParser: Could not set the number of refined parameters";
}

void FpPcrParser::clearChiSquare()
{
        for (int i = 1; i < content.size(); ++i) {
        if (content.at(i).contains(QString("Current global Chi2"))) {
            content[i] = QString("! Current global Chi2 (Bragg contrib.) =  ");
			return;
		}
	}

	qDebug() << "PcrParser: Could not reset ChiSquare";
}

QString FpPcrParser::getContentsString()
{
    return content.join("\n");
}

void FpPcrParser::save()
{
    QFile fi(fileName);
	if (!fi.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	QTextStream ostream(&fi);
    QStringList::iterator it = content.begin();

    while (it != content.end()) {
		ostream << (*it) << "\n";
		++it;
	}
}

// returns a vector containing all crystallite sizes (gaussian size) of all phases in nm.
// wl is the wave length of the X-radiation. k is the Scherrer constant.
QVector<double> FpPcrParser::getCrystSizes(double wl, double k)
{
    QVector<double> vecGS = getAllDoubleValues("GauSiz", "GauSiz\\s+LorSiz");
    QVector<double> vecNm;

    for (int i = 0; i < vecGS.size(); ++i) {
        if (vecGS.at(i) == 0.0) {
            vecNm.append(0.0);
        } else {
            double f = 180.0 * wl * k / (M_PI * sqrt(vecGS.at(i)));
            vecNm.append(f/10.0);
        }
    }

    return vecNm;
}

QVector<double> FpPcrParser::getScales()
{
    return getAllDoubleValues("Scale", "Scale\\s+Shape1");
}

// returns the current number of cycles in the PCR file
int FpPcrParser::getCycles()
{
    return getIntValue("NCY", "NCY");
}

// returns the current number of phases in the PCR file
int FpPcrParser::getNumberOfPhases()
{
    return getIntValue("Nph", "Nph");
}

/*
 * Generic parameter reading function. Returns the value of parameter "Key", read from
 * the next line at the position of key, as an integer.
 *
 * Example: Calling getIntValue("Nph", "Nph\\s+Dum\\s+Ias") returns 2:
 *
 * !Nph Dum Ias Nre Cry Opt Aut
 *   2   0   0   0   0   0   0
 *
 * cline is a characteristic pattern that only occurs in the line of interest, so as to
 * unambiguously identify the correct line
 */
int FpPcrParser::getIntValue(const QRegularExpression &key, const QRegularExpression &cline)
{
    for (int i = 1; i < content.size(); ++i) {
            if (content.at(i).contains(cline)) {
                QStringList lstKey = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
                int n = lstKey.indexOf(key);

                // read next line
                ++i;

                // check if there is still data available
                if (i >= content.size()) {
                    qDebug() << "PcrParser::getIntValue(): Could not read the number of phases";
                    return 0;
                }

                // isolate the third word
                QStringList l = content.at(i).split(QRegularExpression("[!\\s]+"), Qt::SkipEmptyParts);
                return l.at(n).toInt();
            }
    }

    qDebug() << "PcrParser::getIntValue(): Could not read the number of phases";
    return 0;
}

/*
 * convenience functino
 */
int FpPcrParser::getIntValue(const QString &s, const QString &cline)
{
    return getIntValue(QRegularExpression(s), QRegularExpression(cline));
}

/*
 * Generic parameter reading function. Returns the value of parameter "Key", read from
 * the next line at the position of key, as a double.
 *
 * Example: Calling getDoubleValue("Cthm") returns 0.7998:
 *
 * ! Lambda1  Lambda2    Ratio    Bkpos    Wdt    Cthm     muR   AsyLim   Rpolarz  2nd-muR -> Patt# 1
 * 1.540560 1.544390  0.50000   40.000 20.0000  0.7998  0.0000   50.00    0.0000  0.0000
 */
double FpPcrParser::getDoubleValue(const QRegularExpression &key, const QRegularExpression &cline)
{
    for (int i = 1; i < content.size() - 1; ++i) {
            if (content.at(i).contains(cline)) {
                QStringList lstKey = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
                int n = lstKey.indexOf(key);

                // read next line
                ++i;

                QStringList l = content.at(i).split(QRegularExpression("[!\\s]+"), Qt::SkipEmptyParts);

                if ((n >= 0) && (n < l.size())) return l.at(n).toDouble();
                else return 0.0;
            }
    }

    return 0.0;
}

/*
 * convenience function
 */
double FpPcrParser::getDoubleValue(const QString &s, const QString &cline)
{
    return getDoubleValue(QRegularExpression(s), QRegularExpression(cline));
}

/*
 * returns a vector containing all occurrences of parameter "key"
 */
QVector<int> FpPcrParser::getAllIntValues(const QRegularExpression &key, const QRegularExpression &cline)
{
    QVector<int> vec;

    for (int i = 1; i < content.size(); ++i) {
        if (content.at(i).contains(cline)) {
            QStringList lstKey = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
            int n = lstKey.indexOf(key);

            // read next line
            ++i;

            // check if there is still data available
            if (i >= content.size()) {
                qDebug() << "FpPcrParser::getAllDoubleValues(): Could not parse Scale factor";
                return vec;
            }

            // isolate the relevant part from the line
            QStringList l = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
            double f = l.at(n).trimmed().toInt();

            // store the value
            vec.push_back(f);
        }
    }

    return vec;
}

/*
 * convenience function
 */
QVector<int> FpPcrParser::getAllIntValues(const QString &s, const QString &cline)
{
    return getAllIntValues(QRegularExpression(s), QRegularExpression(cline));
}

/*
 * returns a vector containing all occurrences of parameter "key"
 */
QVector<double> FpPcrParser::getAllDoubleValues(const QRegularExpression &key, const QRegularExpression &cline)
{
    QVector<double> vec;

        for (int i = 1; i < content.size(); ++i) {
        if (content.at(i).contains(cline)) {
            QStringList lstKey = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
            int n = lstKey.indexOf(key);

            // read next line
            ++i;

            // check if there is still data available
            if (i >= content.size()) {
                qDebug() << "FpPcrParser::getAllDoubleValues(): Could not parse Scale factor";
                return vec;
            }

            // isolate the relevant part from the line
            QStringList l = content.at(i).split(QRegularExpression("[\\s!]+"), Qt::SkipEmptyParts);
            double f = l.at(n).trimmed().toDouble();

            // store the value
            vec.push_back(f);
        }
    }

    return vec;
}

/*
 * convenience function
 */
QVector<double> FpPcrParser::getAllDoubleValues(const QString &s, const QString &cline)
{
    return getAllDoubleValues(QRegularExpression(s), QRegularExpression(cline));
}

// sets the number of phases in the PCR file
void FpPcrParser::setNumberOfPhases(int p)
{
    changeParam("Nph", QString::number(p, 'd', 0), "Nph", 0);
}

/* i = 0: returns the primary wavelength (Ka1)
   i = 1: returns the secondary wavelength (Ka2)
   i = 2: returns the ratio
*/
double FpPcrParser::getWavelength(int n)
{
    if (n == 0) return getDoubleValue("[Ll]ambda1", "[Ll]ambda1\\s+[Ll]ambda2\\s+Ratio");
    if (n == 1) return getDoubleValue("[Ll]ambda2", "[Ll]ambda1\\s+[Ll]ambda2\\s+Ratio");
    if (n == 2) return getDoubleValue("Ratio", "[Ll]ambda1\\s+[Ll]ambda2\\s+Ratio");
    return 0.0;
}

void FpPcrParser::setDevice(double u, double v, double w, double x, double y,
                            double sl, double dl, double shape, double asyLim, double cthm, double npr)
{
    changeParam("Cthm", QString::number(cthm, 'f', 4), "Wdt\\s+Cthm\\s+muR");
    changeParam("AsyLim", QString::number(asyLim, 'f', 4), "Wdt\\s+Cthm\\s+muR");
    changeParam("Npr", QString::number(int(npr)), "Job\\s+Npr\\+Nba");

    qDebug() << QString("Setting U   = %1").arg(u);
    qDebug() << QString("Setting V   = %1").arg(v);
    qDebug() << QString("Setting W   = %1").arg(w);
    qDebug() << QString("Setting X   = %1").arg(x);
    qDebug() << QString("Setting Y   = %1").arg(y);
    qDebug() << QString("Setting S_L = %1").arg(sl);
    qDebug() << QString("Setting D_L = %1").arg(dl);
    qDebug() << QString("Setting Shp = %1").arg(shape);
    qDebug() << QString("Setting Asy = %1").arg(asyLim);
    qDebug() << QString("Setting Cth = %1").arg(cthm);
    qDebug() << QString("Setting Npr = %1").arg(npr);

    int l;

    l = changeParam("Npr",  QString::number(int(npr)), "Irf\\s+Npr\\s+Jtyp", 0);
    while (l > 0) l = changeParam("Npr", QString::number(int(npr)), "Irf\\s+Npr\\s+Jtyp", l + 1);

    l = changeParam("U", QString::number(u, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", 0);
    while (l > 0) l = changeParam("U", QString::number(u, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", l + 1);

    l = changeParam("V", QString::number(v, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", 0);
    while (l > 0) l = changeParam("V", QString::number(v, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", l + 1);

    l = changeParam("W", QString::number(w, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", 0);
    while (l > 0) l = changeParam("W", QString::number(w, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", l + 1);

    l = changeParam("X", QString::number(x, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", 0);
    while (l > 0) l = changeParam("X", QString::number(x, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", l + 1);

    l = changeParam("Y", QString::number(y, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", 0);
    while (l > 0) l = changeParam("Y", QString::number(y, 'f', 6), "U\\s+V\\s+W\\s+X\\s+Y", l + 1);

    if (int(npr) == 7) {
        // for Npr == 7 the S_L and D_L parameters are in the normal Asymmetry line
        // with numbers in the line below the parameters:
        //
        // !  Pref1    Pref2      Asy1     Asy2     Asy3     Asy4      S_L      D_L
        //  1.00000  0.00000   0.00000  0.00000  0.00000  0.00000  0.00000  0.00000
        //     0.00     0.00      0.00     0.00     0.00     0.00     0.00     0.00

        // first remove the S_L and D_L parameters
        for (int i = 0; i < content.size() - 2; ++i) {
            if (content.at(i).contains(QRegularExpression("!\\s*Pref1\\s+Pref2\\s+Asy1\\s+Asy2\\s+Asy3\\s+Asy4\\s+S_L\\s+D_L"))) {
                content[i]     = QString("!  Pref1    Pref2      Asy1     Asy2     Asy3     Asy4");
                content[i + 1] = QString(" 1.00000  0.00000   0.00000  0.00000  0.00000  0.00000");
                content[i + 2] = QString("    0.00     0.00      0.00     0.00     0.00     0.00");
            }
        }

        // now use a trick: Set the Asy4 parameter to "0.0000 S_L.0000 D_L.0000"
        l = changeParam("Asy4", QString("0.00000  %1  %2").arg(sl, 0, 'f', 6).arg(dl, 0, 'f', 6), "Pref1\\s+Pref2\\s+Asy1\\s+Asy2", 0);
        while (l > 0) l = changeParam("Asy4", QString("0.00000  %1  %2").arg(sl, 0, 'f', 6).arg(dl, 0, 'f', 6), "Pref1\\s+Pref2\\s+Asy1\\s+Asy2", l + 1);

        // write back the comment line and adjust the numbering line to contain S_L and D_L
        for (int i = 0; i < content.size() - 2; ++i) {
            if (content.at(i).contains(QRegularExpression("!\\s*Pref1\\s+Pref2\\s+Asy1\\s+Asy2\\s+Asy3\\s+Asy4"))) {
                content[i]     = QString("!  Pref1    Pref2      Asy1     Asy2     Asy3     Asy4      S_L      D_L");
                content[i + 2] = QString("    0.00     0.00      0.00     0.00     0.00     0.00     0.00     0.00");
            }
        }
    }

    if (int(npr) == 12) {
        // for Npr == 12 the S_L and D_L parameters are in a separate line with title
        // with numbers behind the parameters:
        //
        // !Additional asymmetry parameters (S_L, D_L)
        // 0.0000 0.0 0.0000 0.0
        //
        // instead of using a separate function to handle this, we pretend to change
        // the parameters "Additional" and "parameters".

        l = changeParam("Additional", QString::number(sl, 'f', 6), "Additional asymmetry parameters", 0);
        while (l > 0) l = changeParam("Additional", QString::number(sl, 'f', 6), "Additional asymmetry parameters", l + 1);

        l = changeParam("parameters", QString::number(dl, 'f', 6), "Additional asymmetry parameters", 0);
        while (l > 0) l = changeParam("parameters", QString::number(dl, 'f', 6), "Additional asymmetry parameters", l + 1);
    } else{
        // only Npr == 12 needs the following line
        l = content.lastIndexOf("!Additional asymmetry parameters (S_L, D_L)");
        while (l >= 0) {
            if (l < content.size() - 1) {
                content.removeAt(l + 1);
                content.removeAt(l);
                l = content.lastIndexOf("!Additional asymmetry parameters (S_L, D_L)");
            }
        }
    }

    l = changeParam("Shape1", QString::number(shape, 'f', 6), "Scale\\s+Shape1\\s+Bov", 0);
    while (l > 0) l = changeParam("Shape1", QString::number(shape, 'f', 6), "Scale\\s+Shape1\\s+Bov", l + 1);
}

QStringList FpPcrParser::getPhases()
{
    QStringList l;

    if (!content.size()) {
        return l;
    }

    QRegularExpression rxPhaseStart("(!\\s+Data\\s+for\\s+PHASE\\s+number:\\s+)(\\d+)");
    QRegularExpression rxFileEnd("!\\s+2Th1/TOF1\\s+2Th2/TOF2\\s+Pattern\\s+#\\s+\\d+");
    QRegularExpression rxFrame("(!-+\\s*$)");

    int i = 1;

    while (i < content.size()) {
        if (content.at(i).indexOf(rxPhaseStart) > -1) {
                QStringList p;
                p.append(content.at(i-1));
                p.append(content.at(i));
                p.append(content.at(i-1));  // use the first line, so we don't need to check
                                              // for _contents.size() again.

                i += 2;                       // skip the next "!----" line, we already appended it

                // loop through the rest of the phase block
                while (i < content.size()) {

                    // we reached the next phase's first "!----" line
                    if (content.at(i).indexOf(rxFrame) > -1) {
                        break;
                    }

                    // we reached the next phase
                    // we should not arrive here, because the if() above should capture
                    // the next phase. But just in case...
                    if (content.at(i).indexOf(rxPhaseStart) > -1) {
                        --i; // this line already belongs to the next phase, so set back the counter
                        break;
                    }

                    // we reached the end of the phase blocks
                    if (content.at(i).indexOf(rxFileEnd) > -1) {
                        break;
                    }

                    p.append(content.at(i));
                    ++i;
                }

                l.append(p.join("\n"));
        }
        ++i;
    }

    return l;
}

/*
 * returns the name of a specific phase block, e.g. obtained from ::getPhases()
 */
QString FpPcrParser::getPhaseName(const QString &s)
{
    QStringList l = s.split(global::rxLineEnding);
    QRegularExpression rxFrame("(!-+\\s*$)");

    for (int i = 0; i < l.size(); ++i) {
        if (l.at(i).indexOf(rxFrame) > -1) {

            i += 3; // jump to the line containing the phase name

            // if we reach the end of the file, return whatever we have got so far and exit
            if (i >= l.size()) {
                qDebug() << "PcrParser::getPhaseName(): End of text block reached";
                return QString();
            }

            // found the name
            qDebug() << QString("PcrParser::getPhaseName(): Returning phase name: %1").arg(l.at(i).trimmed());
            return l.at(i).trimmed();
        }
    }

    // if we arrive here, we couldn't find the name. Return empty string
    qDebug() << "PcrParser::getPhaseName(): No phase name found";
    return QString("");
}

QString FpPcrParser::sampleID()
{
    QRegularExpression rx("COMM\\s(.*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1);
    }

    return QString();
}

QStringList FpPcrParser::getPhaseNames()
{
    QStringList l;
    QStringList p = getPhases();

    for (int i = 0; i < p.size(); ++i) {
        l.append(getPhaseName(p.at(i)));
    }

    return l;
}

QString FpPcrParser::getHeader()
{
    QStringList l;
    QRegularExpression rxPhaseStart("(!\\s+Data\\s+for\\s+PHASE\\s+number:\\s+)(\\d+)");

    for (int i = 2; i < content.size(); ++i) {
        if (content.at(i).indexOf(rxPhaseStart) > -1) {
            return l.join("\n");
        }

        l.append(content.at(i - 1));
    }

    return l.join("\n");
}

void FpPcrParser::getDevice(double &u, double &v, double &w, double &x, double &y, double &sl, double &dl, double &shape, double &asyLim, double &cthm, double &npr)
{
    u      = getDoubleValue("U", "U\\s+V\\s+W\\s+X\\s+Y");
    v      = getDoubleValue("V", "U\\s+V\\s+W\\s+X\\s+Y");
    w      = getDoubleValue("W", "U\\s+V\\s+W\\s+X\\s+Y");
    x      = getDoubleValue("X", "U\\s+V\\s+W\\s+X\\s+Y");
    y      = getDoubleValue("Y", "U\\s+V\\s+W\\s+X\\s+Y");
    shape  = getDoubleValue("Shape1", "Scale\\s+Shape1\\s+Bov");
    asyLim = getDoubleValue("AsyLim", "Cthm\\s+muR\\s+AsyLim");
    cthm   = getDoubleValue("Cthm", "Cthm\\s+muR\\s+AsyLim");
    npr    = getDoubleValue("Npr", "Npr\\s+Nba\\s+Nex");

    if (int(npr) == 7) {
        sl = getDoubleValue("S_L", "S_L\\s+D_L");
        dl = getDoubleValue("D_L", "S_L\\s+D_L");
    }

    if (int(npr) == 12) {
        sl = getDoubleValue("Additional", "Additional asymmetry parameters");
        dl = getDoubleValue("parameters", "Additional asymmetry parameters");
    }

    qDebug() << QString("Getting U   = %1").arg(u);
    qDebug() << QString("Getting V   = %1").arg(v);
    qDebug() << QString("Getting W   = %1").arg(w);
    qDebug() << QString("Getting X   = %1").arg(x);
    qDebug() << QString("Getting Y   = %1").arg(y);
    qDebug() << QString("Getting Shp = %1").arg(shape);
    qDebug() << QString("Getting Asy = %1").arg(asyLim);
    qDebug() << QString("Getting Cth = %1").arg(cthm);
    qDebug() << QString("Getting Npr = %1").arg(npr);
}

void FpPcrParser::appendPhase(const QString &s)
{
    if (!content.size()) {
        qDebug() << "PcrParser::appendPhase(): Content string is empty, aborting";
        return;
    }

    int np = getNumberOfPhases();

    // clean up the end of the content stringlist
    while (content.last().simplified().isEmpty() || content.last().simplified() == " ") {
        content.removeLast();
    }

    QString thTof;
    QString thTofVal;

    // the following comment and value lines must always appear at the very end
    if (content.at(content.size() - 2).simplified().contains("! 2Th1/TOF1 2Th2/TOF2 Pattern")) {
        thTofVal = content.takeLast();
        thTof = content.takeLast();
    }

    // append the new phase and the lines appearing at the very end
    QStringList l = s.trimmed().split(global::rxLineEnding);
    content.append(l);
    content.append(thTof);
    content.append(thTofVal);

    setNumberOfPhases(np+1);
}

void FpPcrParser::removePhase(const QString &s)
{
    QString header = getHeader();
    QStringList oldPhases = getPhases();
    QStringList newPhases;
    double d[11] = {0.0};
    getDevice(d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);

    for (int i = 0; i < oldPhases.size(); ++i) {
        if (getPhaseName(oldPhases.at(i)).simplified() != s.simplified()) {
            newPhases.append(oldPhases.at(i));
        }
    }

    // rewrite the entire content
    content = header.split(global::rxLineEnding);

    for (int i = 0; i < newPhases.size(); ++i) {
        content.append(newPhases.at(i).split(global::rxLineEnding));
    }

    setNumberOfPhases(newPhases.size());
    setDevice(d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);
}

/*
 * changes parameter "key" to value "val". Note that val must be given as String in the
 * correct format, because here we don't know what format "key" expects
 */
void FpPcrParser::changeParam(const QString &key, const QString &val, const QString &cline)
{
    changeParam(QRegularExpression(QString("\\b!?%1\\b").arg(key)), val, QRegularExpression(cline), 0);
}

int FpPcrParser::changeParam(const QString &key, const QString &val, const QString &cline, int line)
{
    return changeParam(QRegularExpression(QString("\\b!?%1\\b").arg(key)), val, QRegularExpression(cline), line);
}

int FpPcrParser::changeParam(const QRegularExpression &key, const QString &val, const QRegularExpression &cline, int line)
{
    if ((line < 0) || (line >= content.size())) {
        qDebug() << "PcrParser::changeParam(): Trying to access a line outside the file";
        return -1;
    }

    for (int i = line; i < content.size(); ++i) {
        if (content.at(i).indexOf(cline, 0) >= 0) {
            // store the position of the parameter's keyword
            QStringList lineP = content.at(i).split(QRegularExpression("[!\\s]+"), Qt::SkipEmptyParts);
            int idx = lineP.indexOf(key);

            if (idx < 0) {
                qDebug() << QString("PcrParser::changeParam(): Could not locate keyword %1").arg(key.pattern());
                return -1;
            }

            // read next line without comment sign
            while(i <= content.size() && content.at(i).trimmed().left(1) == "!") {
                ++i;
            }

            // check if there is still data available
            if (i >= content.size()) {
                qDebug() << QString("PcrParser::changeParam(): Reached end of file while searching parameter %1").arg(key.pattern());
                return -1;
            }

            // split the line into segments
            QStringList lineV = content.at(i).split(QRegularExpression("[!\\s]+"), Qt::SkipEmptyParts);

            if (idx >= lineV.size()) {
                qDebug() << QString("PcrParser::changeParam(): Line too short for parameter %1").arg(key.pattern());
                return -1;
            }

            lineV[idx] = val;
            content[i] = lineV.join(" ");
            return i;
        }
    }

    return -1;
}

void FpPcrParser::setDataFileName(const QString &fn)
{
    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).trimmed().left(35) == "!File names of data(patterns) files") {
            ++i;

            if (i > content.size() - 1) {
                qDebug() << QString("PcrParser::setDataFileName(): Could not find line for data file name");
                return;
            }

            qDebug() << QString("PcrParser::setDataFileName(): Setting data file name to %1").arg(fn);

            if (content.at(i).left(1) == "!") content.insert(i, fn);
            else content[i] = fn;

            return;
        }
    }

    qDebug() << QString("PcrParser::setDataFileName(): Could not find line for data file name");
}
