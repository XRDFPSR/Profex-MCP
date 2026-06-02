/***************************************************************************
                          coddbmanager.cpp  -  description
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

#include "coddbmanager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QDebug>

bool CodDbManager::instanceFlag = false;
CodDbManager* CodDbManager::instance = nullptr;

CodDbManager::CodDbManager()
{
    settings = SettingsManager::getInstance();
    connectionOk = false;
}

CodDbManager::~CodDbManager()
{
    instanceFlag = false;
}

CodDbManager* CodDbManager::getInstance()
{
    if (!instanceFlag) {
        instance = new CodDbManager();
        instanceFlag = true;
        return instance;
    } else {
        return instance;
    }
}

void CodDbManager::destroy()
{
    if (instanceFlag) delete instance;
}

void CodDbManager::closeDb()
{
    if (QSqlDatabase::contains("cod")) {
        QSqlDatabase::database("cod").close();
        QSqlDatabase::removeDatabase("cod");
        qDebug() << QString("CodDbManager::closeDb(): Database closed");
    }

    connectionOk = false;
}

bool CodDbManager::init(const QString &s, QString *err)
{
    localDbFile = s;
    return init(err);
}

bool CodDbManager::init(QString *err)
{
    closeDb();
    QString errString;

    if (localDbFile.isEmpty()) {
        // using the deprecated setting value as fallback
        QString oldDbSetting = settings->value("codQueryDialog/localDbFile", QString()).toString();
        localDbFile = settings->value("config/localCodDbFile", oldDbSetting).toString();
    }

    if (!QFile::exists(localDbFile)) {
        if (localDbFile.isEmpty()) {
            errString = QString("Select a database file");
        } else {
            errString = QString("Database file does not exist: %1").arg(localDbFile);
        }

        qDebug() << QString("CodDbManager::init(): %1").arg(errString);
        if (err) *err = errString;
        return connectionOk;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cod");
    db.setDatabaseName(localDbFile);
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP=25;QSQLITE_OPEN_READONLY=1"); // cache size 25 is the default value

    qDebug() << QString("CodDbManager::init(): Connecting to SQLITE3 database: %1").arg(localDbFile);

    // Warning: QSqlDatabase::open() always seems to return true, even for illegal files.
    // use the PRAGMA schema_version check instead (reports > 0 if the database file was opened successfully)

    if (QSqlDatabase::database("cod").open()) {
        QSqlQuery query("PRAGMA schema_version", db);

        if (query.first()) {
            if (query.value(0).toInt() > 0) {
                connectionOk = true;
                errString = QString("Database opened");
                qDebug() << QString("CodDbManager::init(): Database opened successfully");
             } else {
                errString = QString("Not a valid SQLITE3 database file: %1").arg(localDbFile);
                qDebug() << QString("CodDbManager::init(): Error opening database. PRAGMA schema_version returned 0");
            }
        } else {
            errString = QString("Not a valid SQLITE3 database file: %1").arg(localDbFile);
            qDebug() << QString("CodDbManager::init(): Error opening database. PRAGMA schema_version returned 0");
        }
    } else {
        QString dberr = QSqlDatabase::database("cod").lastError().text();
        errString = QString("Error opening database: %1").arg(dberr);
        qDebug() << QString("CodDbManager::init(): Opening database failed");
        qDebug() << QString("                      %1").arg(dberr);
    }

    if (err) *err = errString;
    return connectionOk;
}

void CodDbManager::shutdown()
{
    localDbFile = QString();
    closeDb();
}

QString CodDbManager::probeDb(bool *ok)
{
    QSqlDatabase db = QSqlDatabase::database("cod");

    if (!db.isOpen()) {
        qDebug() << QString("CodDbManager::probeDb(): Database is closed: %1").arg(localDbFile);
        connectionOk = false;
        if (ok) *ok = false;
        return QString("Database is closed: %1").arg(localDbFile);
    }

    QSqlQuery query("SELECT (COUNT(*)) FROM data", db);

    if (!query.first()) {
        qDebug() << QString("CodDbManager::probeDb(): No valid queries received from database: %1").arg(localDbFile);
        if (ok) *ok = false;
        return QString("No valid queries received from database:\n%1").arg(localDbFile);
    }

    QString str = QString("Connected to database:\n%1\n\n").arg(localDbFile);
    str += QString("The database contains %1 structure records.\n").arg(query.value(0).toInt());

    qDebug() << QString("CodDbManager::probeDb(): %1").arg(str);
    if (ok) *ok = true;
    return str;
}

QSqlQuery CodDbManager::query(const QString &s, bool &ok) const
{
    QSqlQuery query(QSqlDatabase::database("cod"));
    ok = query.exec(s);
    return query;
}
