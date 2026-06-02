/***************************************************************************
                          indexedhklstructure.h  -  description
                             -------------------
    begin                : Sun Oct 31 13:14:00 CEST 2021
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

#ifndef INDEXEDHKLSTRUCTURE_H
#define INDEXEDHKLSTRUCTURE_H

#include <QFileInfo>
#include "../libXrdIO/hklphasedata.h"

class IndexedHklStructure
{
public:
    IndexedHklStructure();

        inline void setHklPhase(const HklPhaseData &p) {_hklData = p;}
        inline void setStrSourceFile(const QString &s) {_strSourceFile = QFileInfo(s);}
        inline void setStrFile(const QString &s)       {_strFile = QFileInfo(s);}
        inline void setParFile(const QString &s)       {_parFile = QFileInfo(s);}
        inline void setLstFile(const QString &s)       {_lstFile = QFileInfo(s);}
        inline void setRemove(bool b)                  {_doRemove = b;}
        inline void setMessage(const QString &s)       {_message = s;}

        inline HklPhaseData hklPhase() const           {return _hklData;}
        inline HklPhaseData & hklPhase()               {return _hklData;}
        inline QFileInfo strSourceFile() const         {return _strSourceFile;}
        inline QFileInfo strFile() const               {return _strFile;}
        inline QFileInfo parFile() const               {return _parFile;}
        inline QFileInfo lstFile() const               {return _lstFile;}
        inline bool doRemove() const                   {return _doRemove;}
        inline QString lastMessage() const             {return _message;}

        void reset();

    private:
        HklPhaseData _hklData;
        QFileInfo _strFile;
        QFileInfo _parFile;
        QFileInfo _lstFile;
        QFileInfo _strSourceFile;
        bool _doRemove;
        QString _message;
};

#endif // INDEXEDHKLSTRUCTURE_H
