/***************************************************************************
                          abstractopticsconfigpage.h  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef ABSTRACTOPTICSCONFIGPAGE_H
#define ABSTRACTOPTICSCONFIGPAGE_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QVariant>
#include <QMap>
#include <QDebug>

#include "../libXrdIO/settingsmanager.h"

class AbstractOpticsConfigPage : public QWidget
{
    Q_OBJECT

public:
    explicit AbstractOpticsConfigPage(const QString &t, QWidget *parent = nullptr);
    ~AbstractOpticsConfigPage();

    inline QString configTag() {return tag;}
    virtual QMap<QString, QString> setParameters(const QMap<QString, QString> &) = 0;
    virtual QMap<QString, QString> getParameters() = 0;
    virtual void saveSettings() {/* override if needed */}
    virtual QString helpText() {return QString();}
    void setInstalled(bool b);
    bool isInstalled() const {return installed;}

protected:
    SettingsManager *settings;
    QString tag;
    QString css;
    bool installed;

signals:
    void activeStatusChanged(QString, bool);
    void updateLayout(QString, QVariant);
};

#endif // ABSTRACTOPTICSCONFIGPAGE_H
