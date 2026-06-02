/***************************************************************************
                          philipsudfexport.cpp  -  description
                             -------------------
    begin                : May 21, 2013
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

#include "philipsudfexport.h"

PhilipsUdfExport::PhilipsUdfExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "PHILIPS_UDF";
}

int PhilipsUdfExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
  QString ostr;
  QTextStream stream( &ostr );

  stream << QString("SampleIdent, %1,/").arg(scan.name()) << '\r' << '\n';
  stream << QString("Title1, %1,/").arg(scan.comment().left(72)) << '\r' << '\n';
  stream << QString("Title2, %1,/").arg(scan.comment().mid(72, -1)) << '\r' << '\n';
  
  /* some hardcoded stuff */
  stream << QString("DiffrType,0000000000007031,/") << '\r' << '\n';
  stream << QString("DiffrNumber,  0,/") << '\r' << '\n';
  stream << QString("Anode,Cu,/") << '\r' << '\n';
  /* end of hardcoded stuff */
  
  stream << QString("LabdaAlpha1,%1,/").arg(scan.waveLength(), 8, 'f', 5) << '\r' << '\n';

  /* some hardcoded stuff */
  stream << QString("LabdaAlpha2, 1.54443,/") << '\r' << '\n';
  stream << QString("RatioAlpha21, 0.50000,/") << '\r' << '\n';
  stream << QString("DivergenceSlit,Automatic,10,/") << '\r' << '\n';
  stream << QString("ReceivingSlit, 0.07,/") << '\r' << '\n';
  stream << QString("MonochromatorUsed,NO,/") << '\r' << '\n';
  stream << QString("GeneratorVoltage,  40,/") << '\r' << '\n';
  stream << QString("TubeCurrent,  40,/") << '\r' << '\n';
  stream << QString("FileDateTime, %1,/").arg(QDateTime::currentDateTime().toString("dd.MM.yyy hh:mm:ss")) << '\r' << '\n';
  /* end of hardcoded stuff */

  stream << QString("DataAngleRange,%1,%2,/").arg(scan.startAngle(), 9, 'f', 4).arg(scan.endAngle(), 9, 'f', 4) << '\r' << '\n';
  stream << QString("ScanStepSize,%1,/").arg(scan.stepSize(), 8, 'f', 4) << '\r' << '\n';

  /* some more hardcoded stuff */
  stream << QString("ScanType,Continuous,/") << '\r' << '\n';
  stream << QString("ScanStepTime,  10.00,/") << '\r' << '\n';
  /* end of hardcoded stuff */

  stream << QString("RawScan");

  QVector<double> v;
  QString s;

  for (int i = 0; i < scan.size(); i++) {
      v.push_back(scan.intensity(i));

      if (i % 8 == 7) {
          stream << '\r' << '\n';
          s += QString("%1").arg(v.at(0), 8, 'f', 0, ' ');

          for (int j = 1; j < (int)v.size(); ++j) {
              s += QString(",%1").arg(v.at(j), 8, 'f', 0, ' ');
          }

          stream << s;
          v.clear();
          s.clear();
      }
  }

  // in case the last line does not contain 8 elements, process the rest
  if (v.size()) {
        s.clear();
        s += QString("%1").arg(v.at(0), 8, 'f', 0, ' ');

        for (int j = 1; j < (int)v.size(); ++j) {
              s += QString(",%1").arg(v.at(j), 8, 'f', 0, ' ');
        }

        stream << '\r' << '\n';
        stream << s;
  }

  stream << ",/" << '\r' << '\n';

  return writeFile(file, ostr);
}

int PhilipsUdfExport::save(const QString &file, const QVector<Scan> &vec, const QMap<QString, QVariant> &)
{
    int n = 0; // count successful writings

    // if only one scan is found, export to file
    if (vec.size() == 1) {
        return save(file, vec[0]);
    }


    // in case of several scans, append _i to file name
    // loop over vector and save all scans
    for (int i = 0; i < (int)vec.size(); ++i) {
        QFileInfo fi(file);
        QString fn = QString("%1/%2_%3.%4").arg(fi.absolutePath()).arg(fi.completeBaseName()).arg(i).arg(fi.suffix());
        n += save(fn, vec[i]);
    }

    return n;
}
