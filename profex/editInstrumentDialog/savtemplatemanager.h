/***************************************************************************
                          savtemplatemanager.h  -  description
                             -------------------
    begin                : Mon Nov 22 19:02:00 CEST 2021
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

#ifndef SAVTEMPLATEMANAGER_H
#define SAVTEMPLATEMANAGER_H

#include <QString>
#include <QFileInfo>

class SavTemplateManager
{
public:
    SavTemplateManager();
    void loadTemplateFile(const QString &);
    void generateDefaultTemplate();
    void saveTemplate(const QString &);

    inline QString fileName() const {return _fileName.absoluteFilePath();}
    double getSynchrotron() const;
    double getNeutrons() const;
    QString getLambda() const;
    QString getBackgroundC() const;
    QString getBackground() const;
    QString getTubeTails() const;
    int getRU() const;

    void setSynchrotron(double);
    void setLambda(const QString &);
    void setNeutrons(double);
    void setBackgroundC(const QString &);
    void setBackground(const QString &);
    void setTubeTails(const QString &);
    void setRU(int);
    void setPolarization(bool, const QString &);

private:
    QFileInfo _fileName;
    QString _content;

    QString defaultContent();
    void insertBeforePhases(const QString &);
    QString getStrParameter(const QString &, bool &) const;
    int getIntParameter(const QString &, bool &) const;
    double getDoubleParameter(const QString &, bool &) const;
};

#endif // SAVTEMPLATEMANAGER_H
