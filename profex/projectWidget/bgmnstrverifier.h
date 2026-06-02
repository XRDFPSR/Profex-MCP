/***************************************************************************
                          bgmnstrindexer.h  -  description
                             -------------------
    begin                : Sun Oct 31 13:14:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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


#ifndef BGMNSTRVERIFIER_H
#define BGMNSTRVERIFIER_H

#include <QObject>
#include "../libXrdIO/hkl.h"
#include "../libXrdIO/hklphasedata.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/settingsmanager.h"
#include "../projectWidget/bgmnhandler.h"
#include "../projectWidget/indexedhklstructure.h"

class BgmnStrVerifier : public QObject
{
    Q_OBJECT

public:
    explicit BgmnStrVerifier(QObject *parent = nullptr);
    ~BgmnStrVerifier();

    void runStrVerification(const QString &);
    inline HklPhaseData getLastIndexedHkl() const {return indexedStructure.hklPhase();}
    inline QString getLastIndexMessage() const    {return indexedStructure.lastMessage();}

private:
    SettingsManager *settings;
    IndexedHklStructure indexedStructure;
    BgmnHandler *bgmnVerifyHandler;
    QString lastLogMessage;

    void runVerification();
    QVector<Hkl> parseParFile(const QString &parFile);
    QMap<QString, QVariant> parseLstFile(const QString &lstFile);
    QString getComment(const QString &);

private slots:
    void verifyProcessComplete();

signals:
    void signalVerifyComplete();

};

#endif // BGMNSTRVERIFIER_H
