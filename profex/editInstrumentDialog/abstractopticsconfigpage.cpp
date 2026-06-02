/***************************************************************************
                          abstractopticsconfigpage.cpp  -  description
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

#include "abstractopticsconfigpage.h"

AbstractOpticsConfigPage::AbstractOpticsConfigPage(const QString &t, QWidget *parent) :
    QWidget(parent), tag(t)
{
    settings = SettingsManager::getInstance();

    installed = false;
    css = QString("<style>"
                  "h1 {"
                  "  font-size: x-large;"
                  "}"
                  "h2 {"
                  "  font-size: large;"
                  "}"
                  "h3 {"
                  "  font-size: medium;"
                  "}"
                  "p {"
                  "  font-size: medium;"
                  "}"
                  "</style>");
}

AbstractOpticsConfigPage::~AbstractOpticsConfigPage()
{
}

void AbstractOpticsConfigPage::setInstalled(bool b)
{
    installed = b;
    setEnabled(b);
}
