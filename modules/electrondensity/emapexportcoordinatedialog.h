/***************************************************************************
                          emapexportcoordinatedialog.h  -  description
                             -------------------
    begin                : Fri Jul 15 18:00:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#ifndef EMAPEXPORTCOORDINATEDIALOG_H
#define EMAPEXPORTCOORDINATEDIALOG_H

#include <QDialog>

namespace Ui {
class EMapExportCoordinateDialog;
}

class EMapExportCoordinateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EMapExportCoordinateDialog(QWidget *parent = nullptr);
    ~EMapExportCoordinateDialog();

    int dataMode();
    int coordinateMode();

private:
    Ui::EMapExportCoordinateDialog *ui;
};

#endif // EMAPEXPORTCOORDINATEDIALOG_H
