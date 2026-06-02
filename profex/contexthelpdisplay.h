/***************************************************************************
                          contexthelpdisplay.h  -  description
                             -------------------
    begin                : Thu Oct 20 08:00:00 CEST 2013
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

#ifndef CONTEXTHELPDISPLAY_H
#define CONTEXTHELPDISPLAY_H

#include <QTextBrowser>
#include <QtXml>
#include "../libXrdIO/settingsmanager.h"

class ContextHelpDisplay : public QTextBrowser
{
    Q_OBJECT
public:
    explicit ContextHelpDisplay(QWidget *parent = 0);

    void setKeyword(const QString &, const QString &);
    void parseHelpFiles();
    void setText(const QString &);

private:
    bool readFile(const QString &, QString &);
    bool parseXml(const QString &, QMap<QString, QString> &);
    QString getHelpText(const QMap<QString, QString> &, const QString &);

    QMap<QString, QString> strHelp;
    QMap<QString, QString> savHelp;
    QString sourceDir;
    SettingsManager *settings;
};

#endif // CONTEXTHELPDISPLAY_H
