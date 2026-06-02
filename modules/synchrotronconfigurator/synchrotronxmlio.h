/***************************************************************************
                          synchrotronxmlio.h  -  description
                             -------------------
    begin                : Wed Feb 19 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef SYNCHROTRONXMLIO_H
#define SYNCHROTRONXMLIO_H

#include <QString>
#include <QMap>
#include <QXmlStreamWriter>
#include "parameterstorage.h"

class SynchrotronXmlIO
{
public:
    SynchrotronXmlIO(const QString &f);

    inline void setParameters(const ParameterStorage *p) {_parameters = p;}
    void writeFile();
    inline void setSupportPeaks(const QList<synchro::SupportPeak> &l) {_supportPeaks = l;}
    inline void setProfiles(const QList<synchro::Profile> &l) {_profiles = l;}
    bool readFile(ParameterStorage *parameters,
                  QList<synchro::SupportPeak> &supportPeaks,
                  QList<synchro::Profile> &profiles);

private:
    QString _file;
    const ParameterStorage *_parameters;
    QList<synchro::SupportPeak> _supportPeaks;
    QList<synchro::Profile> _profiles;

    void writeSupportPeak(QXmlStreamWriter &xmlWriter, const synchro::SupportPeak &sp);
    void writeProfile(QXmlStreamWriter &xmlWriter, const synchro::Profile &pr);
    void writeDetector(QXmlStreamWriter &xmlwriter);
    QString valuesToBase64(const QList<double> &);
    QList<double> base64ToValues(const QString &);
};

#endif // SYNCHROTRONXMLIO_H
