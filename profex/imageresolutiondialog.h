/***************************************************************************
                          imageresolutiondialog.h  -  description
                             -------------------
    begin                : Tue Jan 07 16:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef IMAGERESOLUTIONDIALOG_H
#define IMAGERESOLUTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ImageResolutionDialog;
}

class ImageResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageResolutionDialog(QWidget *parent = 0);
    ~ImageResolutionDialog();

    void setPixelWidth(int);
    void setPixelHeight(int);
    int pixelWidth();
    int pixelHeight();

private:
    Ui::ImageResolutionDialog *ui;
};

#endif // IMAGERESOLUTIONDIALOG_H
