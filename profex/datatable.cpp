/***************************************************************************
                          datatable.cpp  -  description
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

#include "datatable.h"

DataTable::DataTable(const QStringList &lst)
{
    parseData(lst);
}

void DataTable::parseData(const QStringList &lines)
{
    dt_header.clear();
    dt_data.clear();
    dt_rows = 0;
    dt_columns = 0;

    if (lines.size() < 2) {
        return;
    }

    dt_header = lines.first().split(";");
    dt_rows = lines.size() - 1;
    dt_columns = dt_header.size();

    for (int r = 1; r < lines.size(); ++r) {
        QStringList cells = lines.at(r).split(";");

        for (int c = 0; c < dt_header.size(); ++c) {
            if (c < cells.size()) dt_data[dt_header.at(c)].append(cells.at(c));
            else                  dt_data[dt_header.at(c)].append(QString());
        }
    }
}

QString DataTable::value(const QString &param, int line) const
{
    if (dt_data.contains(param)) {
        if (dt_data[param].size() > line) {
            return dt_data[param].at(line);
        }
    }

    return QString();
}

QString DataTable::dump()
{
    QString out;

    out += dt_header.join("\t") + "\n";

    for (int i = 0; i < dt_rows; ++i) {
        QStringList line;
        for (int j = 0; j < dt_header.size(); ++j) {
            line.append(dt_data[dt_header.at(j)].at(i));
        }

        out += line.join("\t") + "\n";
    }

    return out;
}
