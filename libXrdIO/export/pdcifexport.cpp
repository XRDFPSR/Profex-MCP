/***************************************************************************
                          pdcifexport.cpp  -  description
                             -------------------
    begin                : Mon Jul 04 14:06:07 CEST 2016
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

#include "pdcifexport.h"

PdCifExport::PdCifExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "CIF_PD";
}

/*
 * if a single scan is provided, it will be saved with name:
 * "projectName-scanName.rtv.cif"
 */
int PdCifExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &)
{
    QFileInfo fi(file);
    QString suffix = fi.completeSuffix();
    if (suffix.isEmpty()) suffix = QString("cif");

    QString fname = QDir::toNativeSeparators(QString("%1/%2-%3.rtv.%4")
                                             .arg(fi.absolutePath())
                                             .arg(fi.completeBaseName())
                                             .arg(scan.name())
                                             .arg(suffix));

    return saveSingle(fname, scan, fi.completeBaseName());
}

/*
 * if the complete scanHeap is provided, it will be tested whether it contains a DIA file.
 *
 * if yes: a rietveld refinement cif file will be created, only containing iobs, icalc, ibkgr
 *         file name: "projectName.rtv.cif"
 *
 * if no:  one cif file per scan will be created
 *         file name: "projectName-scanName.rtv.cif"
 */
int PdCifExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &)
{
    QFileInfo fi(file);
    QString suffix = fi.completeSuffix();
    if (suffix.isEmpty()) suffix = QString("cif");

    if (scanHeap.size() >= 4) {
        if (scanHeap[0].name().simplified().toLower() == QString("i observed")
                && scanHeap[1].name().simplified().toLower() == QString("i calculated")
                && scanHeap[2].name().simplified().toLower() == QString("i difference")
                && scanHeap[3].name().simplified().toLower() == QString("background")) {

            QString fname = QDir::toNativeSeparators(QString("%1/%2.rtv.%3")
                                                     .arg(fi.absolutePath())
                                                     .arg(fi.completeBaseName())
                                                     .arg(suffix));

            return saveRietveldSet(fname, scanHeap, fi.completeBaseName());
        }
    }

    int n = 0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        QString fname = QDir::toNativeSeparators(QString("%1/%2-%3.rtv.%4")
                                                 .arg(fi.absolutePath())
                                                 .arg(fi.completeBaseName())
                                                 .arg(scanHeap[i].name())
                                                 .arg(suffix));

        n += saveSingle(fname, scanHeap[i], fi.completeBaseName());
    }

    return n;
}

int PdCifExport::saveSingle(const QString &file, const Scan &scan, const QString &difId)
{
    qDebug() << QString("PdCifExport::saveSingle(): Saving file %1").arg(file);

    QString cif;

    cif = QStringLiteral("###############################################################################\n");
    cif += QString(       "# Powder diffraction data for sample %1, phase %2\n").arg(difId).arg(scan.name());
    cif += QString(       "# Created with Profex %1.%2.%3\n").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
    cif += QString(       "# Export date: %1\n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy - hh:mm"));
    cif += QStringLiteral("###############################################################################\n\n");

    QFileInfo fi(scan.sourceFileName());

    cif += QString("data_%1\n").arg(fi.completeBaseName());
    cif += QString("_pd_block_id %1\n").arg(difId);

    bool addXoffset = !qFuzzyIsNull(scan.xOffset());
    bool addYoffset = !qFuzzyIsNull(scan.yOffset());
    bool addScale = !qFuzzyCompare(scan.scaleFactor(), 1.0);

    QString aTag("_pd_proc_2theta_corrected");
    QString iTag("_pd_meas_counts_total");

    if (addXoffset || addYoffset || addScale) {
        iTag = QString("_pd_proc_intensity_net");
    }

    cif += QString("\nloop_\n");
    cif += QString("    %1\n").arg(aTag);
    cif += QString("    %1\n").arg(iTag);

    for (int i = 0; i < scan.size(); i++) {
        double a = scan.angle(i);
        double c = scan.intensity(i);

        if (addXoffset) a += scan.xOffset();
        if (addScale)   c *= scan.scaleFactor();
        if (addYoffset) c += scan.yOffset();

        cif += QString("%1 %2\n").arg(a, 12, 'f', 6).arg(c, 12, 'f', 4);
    }

    cif += QStringLiteral("\n# The following lines are used to test the character set of files sent by\n");
    cif += QStringLiteral("# network email or other means. They are not part of the CIF data set.\n");
    cif += QStringLiteral("# abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n");
    cif += QString("# !@#$%^&*()_+{}:\"~<>?|\\-=[];'`,./ \n");

    QFile data(file);
    if (!data.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << QString("PdCifExport::save(): Could not open file %1 for writing").arg(file);
        return 0;
    }

    QTextStream out(&data);
    out << cif;
    data.close();
    return 1;
}

int PdCifExport::saveRietveldSet(const QString &file, const QVector<Scan> &scanHeap, const QString &difId)
{
    qDebug() << QString("PdCifExport::saveRietveldSet(): Saving file %1").arg(file);

    if (scanHeap.size() < 4) {
        return 0;
    }

    QString cif;

    cif =  QStringLiteral("###############################################################################\n");
    cif += QString(       "# Rietveld refinement results for sample %1\n").arg(difId);
    cif += QString(       "# Created with Profex %1.%2.%3\n").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
    cif += QString(       "# Export date: %1\n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy - hh:mm"));
    cif += QStringLiteral("###############################################################################\n\n");

    QFileInfo fi(scanHeap[0].sourceFileName());

    cif += QString("data_%1\n").arg(fi.completeBaseName());
    cif += QString("_pd_block_id %1\n").arg(difId);


    cif += QString("\nloop_\n");
    cif += QString("    _pd_proc_point_id\n");
    cif += QString("    _pd_proc_2theta_corrected\n");
    cif += QString("    _pd_proc_intensity_total\n");
    cif += QString("    _pd_calc_intensity_total\n");
    cif += QString("    _pd_proc_intensity_bkg_calc\n");

    int size = qMin(scanHeap[0].size(), qMin(scanHeap[1].size(), qMin(scanHeap[2].size(), scanHeap[3].size())));

    for (int i = 0; i < size; i++) {

        cif += QString("    %1 %2 %3 %4 %5\n")
                  .arg(i+1, 8)
                  .arg(scanHeap[0].angle(i), 12, 'f', 6)
                .arg(scanHeap[0].intensity(i), 12, 'f', 4)
                .arg(scanHeap[1].intensity(i), 12, 'f', 4)
                .arg(scanHeap[3].intensity(i), 12, 'f', 4);
    }

    cif += QStringLiteral("\n# The following lines are used to test the character set of files sent by\n");
    cif += QStringLiteral("# network email or other means. They are not part of the CIF data set.\n");
    cif += QStringLiteral("# abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n");
    cif += QString("# !@#$%^&*()_+{}:\"~<>?|\\-=[];'`,./ \n");

    QFile data(file);
    if (!data.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << QString("PdCifExport::saveRietveldSet(): Could not open file %1 for writing").arg(file);
        return 0;
    }

    QTextStream out(&data);
    out << cif;
    data.close();
    return 1;
}
