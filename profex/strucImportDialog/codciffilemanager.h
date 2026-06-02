/***************************************************************************
                          codciffilemanager.h  -  description
                             -------------------
    begin                : Thu Jul 20 18:30:00 CEST 2023
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

#ifndef CODCIFFILEMANAGER_H
#define CODCIFFILEMANAGER_H

#include <QObject>

struct CifFile {
    QString record;
    QString content;
    CifFile(QString r, QString c) : record(r), content(c) {}
};

class CodCifFileManager : public QObject
{
    Q_OBJECT
public:
    explicit CodCifFileManager(QObject *parent = nullptr);

    virtual void startCifDownload(const QStringList &, const QString &) = 0;

signals:
    void cifDownloadComplete(QList<CifFile>&, QStringList&);
    void progress(int);

public slots:
    virtual void cancel() = 0;
};

#endif // CODCIFFILEMANAGER_H
