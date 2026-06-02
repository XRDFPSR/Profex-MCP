#include "elementscatteringdatadownloader.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDataStream>
#include <QDebug>

ElementScatteringDataDownloader::ElementScatteringDataDownloader()
{
    /*
        This is not actually a downloader. It expects to find 92 html files downloaded from NIST.
        Check the function readFiles() for the hard-coded location of the html files.

        Normally, the scattering data is read from a binary file in the ressources.
        This class helps re-creating the binary file, but it is not very user friendly
        and uses hard-coded paths and names for the html files.

        To download the html files for all elements, use the bash below:
    */

    /*
        #! /bin/bash

        for n in {1..92}
        do

        wget 'https://physics.nist.gov/cgi-bin/ffast/ffast.pl?Z='$n'&Formula=&gtype=4&range=U&lower=&upper=&density=&frames=no'
        mv ffast* $n.html

        done
    */
}

QMap<int, ElementScatteringData> ElementScatteringDataDownloader::getDataMap()
{
    return readFiles();
}

/*
 * this function writes the ElementScatteringData list to a binary file that can
 * be added to the resources.
 *
 * only use this function to re-create the binary file when the hard-coded scattering
 * data has changed.
 */
void ElementScatteringDataDownloader::createBinaryFile(const QString &s, const QMap<int, ElementScatteringData> &data) const
{
    QFile f(s);

    if (!f.open(QIODevice::WriteOnly)) {
        qDebug() << QString("ElementScatteringDataDownloader::createBinaryFile(): Could not open file for writing, exiting (%1)").arg(s);
        return;
    }

    QDataStream out(&f);

    QMapIterator<int, ElementScatteringData> it(data);

    while (it.hasNext()) {
        it.next();
        QByteArray ba = it.value().serialize();
        out << qint32(ba.size()) << ba;
    }

    f.close();
}

QMap<int, ElementScatteringData> ElementScatteringDataDownloader::readFiles()
{
    QMap<int, ElementScatteringData> map;
    static QRegularExpression rxSplit("\\r\\n?|\\n");
    QString dataDir = QString("/home/doebelinn/Develop/ParseScatFac/NISTscatfact");

    for (int i = 1; i <= 92; ++i) {
        QFile f(QString("%1/%2.html").arg(dataDir).arg(i));

        if (!f.exists()) continue;
        if (!f.open(QIODevice::ReadOnly)) continue;

        QTextStream stream(&f);
        QString s = stream.readAll();
        f.close();

        ElementScatteringData el = parseFile(s.split(rxSplit));
        if (!el.element().isEmpty()) map.insert(el.z(), el);
    }

    return map;
}

ElementScatteringData ElementScatteringDataDownloader::parseFile(const QStringList &l)
{
    static QRegularExpression rxEl("^<b>\\s+([A-Za-z]{1,2})&#160;\\(Z\\s+=\\s+([0-9]{1,2})\\)$");
    static QRegularExpression rxDensity(".+Nominal\\s+density.+(\\d+\\.\\d+(?:E[+-]?\\d+)?)\\s*$");
    static QRegularExpression rxHead("^\\s+keV\\s+<i>e</i>\\s+atom.*");
    static QRegularExpression rxLine("^(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})\\s+"
                              "(\\d\\.\\d+E[\\+-]\\d{2,})$");

    QRegularExpressionMatch rm;

    double density = -1.0;
    QString m_element;
    int m_z;

    QVector<double> m_energy;
    QVector<double> m_f0;
    QVector<double> m_f1;
    QVector<double> m_f2;
    QVector<double> m_macTotal;
    QVector<double> m_lac;

    int lEl = l.indexOf(rxEl);
    int lDe = l.indexOf(rxDensity);
    int lTl = l.indexOf(rxHead) + 1;

    if ((lEl < 0) || (lDe < 0) || (lTl < 0)) {
        return ElementScatteringData();
    }

    rm = rxEl.match(l.at(lEl));

    if (rm.hasMatch()) {
        m_element = rm.captured(1);
        m_z = rm.captured(2).toInt();
    } else {
        return ElementScatteringData();
    }

    rm = rxDensity.match(l.at(lDe));

    if (rm.hasMatch()) {
        density = rm.captured(1).toDouble();
    } else {
        return ElementScatteringData();
    }

    for (int i = lTl; i < l.size(); ++i) {
        rm = rxLine.match(l.at(i));
        if (!rm.hasMatch()) continue;

        double f1 = rm.captured(2).toDouble();
        double mac = rm.captured(6).toDouble();

        m_energy.append(rm.captured(1).toDouble());
        m_f0.append(f1 + double(m_z));
        m_f1.append(f1);
        m_f2.append(rm.captured(3).toDouble());
        m_macTotal.append(mac);
        m_lac.append(mac * density);
    }

    ElementScatteringData el(m_element, m_z);
    el.dataEnergy() = m_energy;
    el.dataF0() = m_f0;
    el.dataF1() = m_f1;
    el.dataF2() = m_f2;
    el.dataMac() = m_macTotal;
    el.dataLac() = m_lac;

    return el;
}

