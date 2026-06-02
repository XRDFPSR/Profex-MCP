/***************************************************************************
                          opticsitem.h  -  description
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

#ifndef OPTICSITEM_H
#define OPTICSITEM_H

#include <QGraphicsSvgItem>

class OpticsItem : public QGraphicsSvgItem
{
    Q_OBJECT

public:
    OpticsItem(const QString &fa, const QString &fi, const QString &m, QGraphicsItem *parent = nullptr);

    inline QString moduleName() const {return _module;}
    void setInstalled(bool);
    void setHighlighted(bool);
    inline void setDarkMode(bool b) {_darkMode = b;}
    inline bool isHighlighted() const  {return _isHighlighted;}
    void loadShape(const QString &);
    inline void setData(const QString &s, const QVariant &v) {_data[s] = v;}
    inline QVariant data(const QString &s, const QVariant &v = QVariant()) const {return _data.value(s, v);}

private:
    QString _activeFile;
    QString _inactiveFile;
    QString _module;
    QMap<QString, QVariant> _data;
    bool _isHighlighted;
    bool _darkMode;

private slots:
    void posChanged();
};

#endif // OPTICSITEM_H
