/***************************************************************************
                          bgmninstrumentsavparser.cpp  -  description
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

#ifndef BGMNINSTRUMENTSAVPARSER_H
#define BGMNINSTRUMENTSAVPARSER_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QMap>
#include <QVariant>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnInstrumentSavParser
{
public:
    explicit BgmnInstrumentSavParser(const QMap<QString, QString> &par, const QString &header, const QString &fn);
    explicit BgmnInstrumentSavParser(const QString &str, const QString &name);
    explicit BgmnInstrumentSavParser(const QString &file, bool * ok = nullptr);

    bool loadFile(const QString &);
    QString getText();
    bool checkOutputNames(const QString &);
    void fixOutputNames(const QString &);
    QString getHeader();
    QMap<QString, QString> getParameters();
    QString getTubeTailsFile();
    bool hasTubeTails();

private:
    QString fileName;
    QStringList content;

    void setContent(const QString &);
    void compose(const QMap<QString, QString> &, const QString &);
};

#endif // BGMNINSTRUMENTSAVPARSER_H
