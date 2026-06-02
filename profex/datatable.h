/***************************************************************************
                          datatable.h  -  description
                             -------------------
    begin                : Thu Jun 16 18:15:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef DATATABLE_H
#define DATATABLE_H

#include <QString>
#include <QStringList>
#include <QMap>

class DataTable
{
public:
    // the stringlist contains lines, first line is the header, cells
    // within lines are separated by ";"
    DataTable(const QStringList &);

    inline QStringList header() const {return dt_header;}
    inline int rows() const {return dt_rows;}
    inline int columns() const {return dt_columns;}

    QString value(const QString &, int) const;
    QString dump();

private:
    QStringList dt_header;
    QMap<QString, QStringList> dt_data;
    int dt_rows;
    int dt_columns;

    void parseData(const QStringList &);
};

#endif // DATATABLE_H
