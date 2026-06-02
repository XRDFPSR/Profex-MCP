/***************************************************************************
                          coddbmanager.h  -  description
                             -------------------
    begin                : Wed Apr 05 18:10:07 CET 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef CODDBMANAGER_H
#define CODDBMANAGER_H

#include "settingsmanager.h"
#include <QObject>
#include <QSqlQuery>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

/* singleton pattern */

class XRDIO_EXPORT CodDbManager
{
public:
    static CodDbManager* getInstance();

    void destroy();
    bool init(QString *err = nullptr);
    bool init(const QString &s, QString *err = nullptr);
    void closeDb();
    QString probeDb(bool *ok = nullptr);
    void shutdown();
    inline bool isConnected() const {return connectionOk;}

    QSqlQuery query(const QString &, bool &) const;

private:
    static bool instanceFlag;
    static CodDbManager *instance;

    CodDbManager();
    ~CodDbManager();

    SettingsManager *settings;
    QString localDbFile;
    bool connectionOk;
};

#endif // CODDBMANAGER_H
