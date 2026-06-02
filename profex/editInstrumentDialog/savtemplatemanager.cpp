/***************************************************************************
                          savtemplatemanager.cpp  -  description
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

#include <QRegularExpression>
#include <QFile>

#include "savtemplatemanager.h"
#include "../libXrdIO/bgmnfileio.h"

SavTemplateManager::SavTemplateManager()
{

}

void SavTemplateManager::loadTemplateFile(const QString &s)
{
    if (!QFile::exists(s)) {
        _content = defaultContent();
        _fileName = QFileInfo();
    } else {
        _content = BgmnFileIO::readTextFile(s);
        _fileName = QFileInfo(s);
    }
}

void SavTemplateManager::generateDefaultTemplate()
{
    _content = defaultContent();
    _fileName = QFileInfo();
}

void SavTemplateManager::saveTemplate(const QString &s)
{
    if (!s.isEmpty()) {
        _fileName = QFileInfo(s);
    }

    BgmnFileIO::writeTextFile(_fileName.absoluteFilePath(), _content);
}

QString SavTemplateManager::getLambda() const
{
    bool ok;
    QString lam = getStrParameter("LAMBDA", ok);

    if (ok) return lam;
    return QString();
}

double SavTemplateManager::getSynchrotron() const
{
    bool ok;
    double syn = getDoubleParameter("SYNCHROTRON", ok);

    if (ok) return syn;
    return -1.0;
}

double SavTemplateManager::getNeutrons() const
{
    bool ok;
    double neu = getDoubleParameter("NEUTRONS", ok);

    if (ok) return neu;
    return -1.0;
}

QString SavTemplateManager::getBackground() const
{
    bool ok;
    QString unt = getStrParameter("UNT", ok);

    if (ok) return unt;
    return QString();
}

QString SavTemplateManager::getBackgroundC() const
{
    bool ok;
    QString untc = getStrParameter("UNTC", ok);

    if (ok) return untc;
    return QString();
}

QString SavTemplateManager::getTubeTails() const
{
    bool ok;
    QString tt = getStrParameter("TubeTails", ok);

    if (ok) return tt;
    return QString();
}

int SavTemplateManager::getRU() const
{
    bool ok;
    int ru = getIntParameter("RU", ok);

    if (ok) return ru;
    return -1;
}

QString SavTemplateManager::getStrParameter(const QString &s, bool &ok) const
{
    // commented lines are ignored
    QRegularExpression rx(QString("(?<!%)\\s*%1=([^\\n]+)\\n").arg(s));
    QRegularExpressionMatch rm = rx.match(_content);

    ok = rm.hasMatch();
    return rm.captured(1).trimmed();
}

int SavTemplateManager::getIntParameter(const QString &s, bool &ok) const
{
    // commented lines are ignored
    QRegularExpression rx(QString("(?<!%)\\s*%1=(\\d+)").arg(s));
    QRegularExpressionMatch rm = rx.match(_content);

    ok = rm.hasMatch();
    return rm.captured(1).toInt();
}

double SavTemplateManager::getDoubleParameter(const QString &s, bool &ok) const
{
    // commented lines are ignored
    QRegularExpression rx(QString("(?<!%)\\s*%1=(\\d+\\.?\\d*)").arg(s));
    QRegularExpressionMatch rm = rx.match(_content);

    ok = rm.hasMatch();
    return rm.captured(1).toDouble();
}


void SavTemplateManager::setSynchrotron(double d)
{
    static QRegularExpression rx("(?:%\\s+Wavelength\\n)?(?:LAMBDA|SYNCHROTRON|NEUTRONS)=\\S*\\n");

    if (_content.contains(rx)) {
        _content.replace(rx, QString("% Wavelength\nSYNCHROTRON=%1\n").arg(d, 0, 'd', 8));
    } else {
        insertBeforePhases(QString("% Wavelength\nSYNCHROTRON=%1\n").arg(d, 0, 'd', 8));
    }
}

void SavTemplateManager::setLambda(const QString &s)
{
    static QRegularExpression rx("(?:%\\s+Wavelength\\n)?(?:LAMBDA|SYNCHROTRON|NEUTRONS)=\\S*\\n");

    if (_content.contains(rx)) {
        _content.replace(rx, QString("% Wavelength\nLAMBDA=%1\n").arg(s));
    } else {
        insertBeforePhases(QString("% Wavelength\nLAMBDA=%1\n").arg(s));
    }
}

void SavTemplateManager::setNeutrons(double d)
{
    static QRegularExpression rx("(?:%\\s+Wavelength\\n)?(?:LAMBDA|SYNCHROTRON|NEUTRONS)=\\S*\\n");

    if (_content.contains(rx)) {
        _content.replace(rx, QString("% Wavelength\nNEUTRONS=%1\n").arg(d, 0, 'd', 8));
    } else {
        insertBeforePhases(QString("% Wavelength\nNEUTRONS=%1\n").arg(d, 0, 'd', 8));
    }
}

/* send empty string to remove the line */
void SavTemplateManager::setBackgroundC(const QString &s)
{
    static QRegularExpression rxCom("%\\s+Measured background\\n");
    static QRegularExpression rxVal("(?:%\\s*)?UNTC?=\\S*\\n");

    QFileInfo fi(s);

    _content.remove(rxCom);
    _content.remove(rxVal);

    if (fi.exists()) {
        insertBeforePhases(QString("% Measured background\nUNTC=%1\n").arg(fi.fileName()));
    }
}

/* send empty string to remove the line */
void SavTemplateManager::setBackground(const QString &s)
{
    static QRegularExpression rxCom("%\\s+Measured background\\n");
    static QRegularExpression rxVal("(?:%\\s*)?UNTC?=\\S*\\n");

    QFileInfo fi(s);

    _content.remove(rxCom);
    _content.remove(rxVal);

    if (fi.exists()) {
        insertBeforePhases(QString("% Measured background\nUNT=%1\n").arg(fi.fileName()));
    }
}

/* send empty string to remove the line */
void SavTemplateManager::setTubeTails(const QString &s)
{
    static QRegularExpression rxCom("%\\s+Measured tube tails\\n");
    static QRegularExpression rxVal("(?:%\\s*)?TubeTails=\\S*\\n");

    QFileInfo fi(s);

    _content.remove(rxCom);
    _content.remove(rxVal);

    if (fi.exists()) {
        insertBeforePhases(QString("% Measured tube tails\nTubeTails=%1\n").arg(fi.fileName()));
    }
}

/* set to negative value to remove the line */
void SavTemplateManager::setRU(int n)
{
    static QRegularExpression rxCom("%\\s+Background coefficients\\n");
    static QRegularExpression rxVal("(?:%\\s*)?RU=\\d*\\n");

    _content.remove(rxCom);
    _content.remove(rxVal);

    if (n >= 0) {
        insertBeforePhases(QString("% Background coefficients\nRU=%1\n").arg(n));
    }
}

void SavTemplateManager::setPolarization(bool b, const QString &ang)
{
    static QRegularExpression rxCom("%\\s+Polari[sz]ation[^\\n]*\\n");
    static QRegularExpression rxVal("(?:%\\s*)?POL=[^\\n]+\\n");
    static QRegularExpression rxPi("(?:%\\s*)?pi=2\\*acos\\(0\\)\\n");

    _content.remove(rxCom);
    _content.remove(rxVal);
    _content.remove(rxPi);

    if (b) {
        insertBeforePhases("% Polarization\n");
        insertBeforePhases(QString("POL=%1\n").arg(ang));
        insertBeforePhases("pi=2*acos(0)\n");
    }
}

void SavTemplateManager::insertBeforePhases(const QString &s)
{
    static QRegularExpression rxPha("%\\s+Phases|STRUC\\[\\d+\\]");

    int idx = _content.indexOf(rxPha);
    _content.insert(idx < 0 ? 0 : idx, s);
}

QString SavTemplateManager::defaultContent()
{
    static QString str("% Theoretical instrumental function\n"
                       "VERZERR=\n"
                       "% Wavelength\n"
                       "LAMBDA=\n"
                       "% Phases\n"
                       "% Measured background\n"
                       "% Measured data\n"
                       "VAL[1]=\n"
                       "% Minimum Angle (2theta)\n"
                       "% WMIN=10\n"
                       "% Maximum Angle (2theta)\n"
                       "% WMAX=60\n"
                       "% Result list output\n"
                       "LIST=\n"
                       "% Peak list output\n"
                       "OUTPUT=\n"
                       "% Diagram output\n"
                       "DIAGRAMM=\n"
                       "% Global parameters for zero point and sample displacement\n"
                       "EPS1=0\n"
                       "PARAM[1]=EPS2=0_-0.01^0.01\n"
                       "EPS3=0\n"
                       "alpha3ratio=0.020\n"
                       "betaratio=0.005\n"
                       "NTHREADS=8\n"
                       "PROTOKOLL=Y\n"
                       "SAVE=N\n");

    return str;
}
