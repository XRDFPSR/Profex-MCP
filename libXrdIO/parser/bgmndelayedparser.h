/***************************************************************************
                          bgmndelayedparser.h  -  description
                             -------------------
    begin                : Jul 15 17:25:02 CEST 2025
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

#ifndef BGMNDELAYEDPARSER_H
#define BGMNDELAYEDPARSER_H

#include <QString>
#include "../settingsmanager.h"

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnDelayedParser
{
public:
    explicit BgmnDelayedParser();
    explicit BgmnDelayedParser(const QString &, bool &);
    virtual ~BgmnDelayedParser();

    bool load(const QString &);
    bool reload();
    inline QString getFileName() const {return fileName;}
    static bool readSafely(const QString &f, QString &out, int maxRetries = 3);
    static bool readSafely(const QString &f, QList<QByteArray> &out, int maxRetries = 3);

private:
    SettingsManager *settings;
    QString fileName;

    virtual bool privateLoad(const QString &) = 0;
    virtual bool privateReload() = 0;

    bool waitForStable(const QString &, int intervalMS, int timeoutMS);
};

#endif // BGMNDELAYEDPARSER_H
