/***************************************************************************
                          msoexportexcel.h  -  description
                             -------------------
    begin                : Sun Aug 09 08:25:00 CEST 2020
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

#ifndef MSOEXPORTEXCEL_H
#define MSOEXPORTEXCEL_H

#include "../libXrdIO/structs.h"
#include "projectWidget/graphWidget/graphdatacontroller.h"
#include "msoeditexcelexportdialog.h"

#include <QObject>
#include <QString>
#include <QDomDocument>

#ifdef Q_OS_WIN
    #include <ActiveQt/qaxobject.h>
    #include <ActiveQt/qaxbase.h>
#endif

class MsoExportExcel : public QObject
{
    Q_OBJECT
public:
    explicit MsoExportExcel(GraphDataController *, QObject *parent = nullptr);

    void getPreset(QDomDocument &);
    void applyPreset(const QDomElement &);

    bool hasData() const;

    QList<global::AxObjectExcel> getConfig();
    void initStructure(const QList<global::AxObjectExcel> &);
    inline bool isEmpty() {return _config.isEmpty();}
    void exportData(const QString &savFile, const QString &lstFile);
    void showEditor(const QString &savFile, const QString &lstFile);

private:
    GraphDataController *graphControl;
    QList<global::AxObjectExcel> _config;
    static const QChar letters[];

    QString cellIdxToString(int, int);
    QMap<QString, QList<global::Result> > parseLstFile(const QString &);
    QMap<QString, QVariant> parseSavFile(const QString &);

    bool lstContains(const QMap<QString, QList<global::Result> > &, const QString &, const QString &);
    double getLstValue(const QMap<QString, QList<global::Result> > &, const QString &, const QString &);
    QString getFilteredVal(const QString &val, const QString &fil, const QString &cap);

#ifdef Q_OS_WIN
    void writeScanToWorksheet(const global::AxObjectExcel &);

    QAxObject* m_excelApplication;
    QAxObject* m_workbooks;
    QAxObject* m_workbook;
    QAxObject* m_sheets;
    QAxObject* m_sheet;
#endif

signals:

};

#endif // MSOEXPORTEXCEL_H
