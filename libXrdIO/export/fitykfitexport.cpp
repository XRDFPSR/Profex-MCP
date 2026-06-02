/***************************************************************************
                          fitykcorundumexport.cpp  -  description
                             -------------------
    begin                : Tue Jul 09 15:06:07 CEST 2014
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

#include <QDateTime>
#include <QFileInfo>
#include <QStringBuilder>
#include <QInputDialog>

#include "fitykfitexport.h"

FitykFitExport::FitykFitExport(QObject *parent, bool iactive) :
    GenericExport(parent), interactive(iactive)
{
    uId = "FITYK_FIT";
}

int FitykFitExport::save(const QString &file, Scan &scan)
{
    QString fout = file;
    QFileInfo fi(fout);

    if (fi.suffix().isEmpty()) {
        fout = QString("%1%2.fit").arg(fi.absolutePath()).arg(fi.completeBaseName());
    }

    int mode = 0;
    int ret = -1;

    if (interactive) {
        QMap<QString, int> options;
        options.insert(QStringLiteral("Entire file"), 0);
        options.insert(QStringLiteral("Corundum (104) peak (34.0 - 36.2 °2theta)"), 1);
        options.insert(QStringLiteral("b-TCP range (27.0 - 35.0 °2theta)"), 2);
        options.insert(QStringLiteral("CaO range (36.0 - 38.8 °2theta)"), 3);
        options.insert(QStringLiteral("HA range (37.5 - 59.5 °2theta)"), 4);
        options.insert(QStringLiteral("All CaP ranges to separate files"), 5);

        bool ok;
        QString smode = QInputDialog::getItem(0, tr("Fityk export"), tr("Export Mode"), options.keys(), 0, false, &ok);

        if (!ok) {
            // dialog aborted
            return ret;
        }

        mode = options[smode];
    }

    QString output;

    if (mode == 0) {
        output = getFullRange(file, scan);
        ret = writeFile(fout, output);
    }

    if (mode == 1) {
        output = getCorundumRange(file, scan);
        ret = writeFile(fout, output);
    }

    if (mode == 2) {
        output = getBTcpRange(file, scan);
        ret = writeFile(fout, output);
    }

    if (mode == 3) {
        output = getCaORange(file, scan);
        ret = writeFile(fout, output);
    }

    if (mode == 4) {
        output = getHARange(file, scan);
        ret = writeFile(fout, output);
    }

    if (mode == 5) {
        // write output to different files
        QString fbTcp = QString("%1%2_bTCP.fit").arg(fi.absolutePath()).arg(fi.completeBaseName());
        output = getBTcpRange(file, scan);
        ret = writeFile(fbTcp, output);

        QString fCaO = QString("%1%2_CaO.fit").arg(fi.absolutePath()).arg(fi.completeBaseName());
        output = getCaORange(file, scan);
        ret += writeFile(fCaO, output);

        QString fHA = QString("%1%2_HA.fit").arg(fi.absolutePath()).arg(fi.completeBaseName());
        output = getHARange(file, scan);
        ret += writeFile(fHA, output);
    }

    return ret;
}

int FitykFitExport::save(const QString &file, QVector<Scan> &scanHeap)
{
    if (scanHeap.size() == 1) {
        // in that case, do not modify the file name
        return save(file, scanHeap[0]);
    }

    int ret = 0;

    for (int i = 0; i < scanHeap.size(); ++i) {
        QFileInfo fi(file);
        QString fout = QString("%1_%2.%3").arg(fi.completeBaseName()).arg(i).arg(fi.suffix());
        ret += save(fout, scanHeap[i]);
    }

    return ret;
}

QString FitykFitExport::getFullRange(const QString &file, Scan &scan)
{
    QString values;

    double minang = scan.startAngle();
    double maxang = scan.endAngle();
    double maxint = scan.maxIntensity();

    for (int i = 0; i < scan.size(); ++i) {
        values += QString("X[%1]=%2, Y[%1]=%3, S[%1]=%4, A[%1]=1\n")
                .arg(i)
                .arg(scan.angle(i), 0, 'f', 6)
                .arg(scan.intensity(i), 0, 'f', 6)
                .arg(1.0/sqrt(scan.intensity(i)), 0, 'f', 6);
    }

    // compile the hard-coded header
    QString ostring = compileHeader(file, scan.size(), maxang);

    ostring += values
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  (un)defines  ------------\n")
            % definesString()
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  variables and functions  ------------\n")
            % QStringLiteral("\n")
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  models  ------------\n")
            % QStringLiteral("\n")
            % QString("plot [%1:%2] [%3:%4]\n").arg(minang, 0, 'f', 6).arg(maxang, 0, 'f', 6).arg(-maxint/10.0, 0, 'f', 1).arg(maxint*1.2, 0, 'f', 6)
            % QStringLiteral("use @0\n")
            % QStringLiteral("set autoplot = 1\n")
            % QStringLiteral("set verbosity = 0\n");

    return ostring;
}

QString FitykFitExport::getCorundumRange(const QString &file, Scan &scan)
{
    double loLimit = 34.0;
    double upLimit = 36.2;

    QString values;

    int n = 0;
    double maxang = 0.0;
    double maxint = 0.0;

    for (int i = 0; i < scan.size(); ++i) {
        double a = scan.angle(i);

        if (a < loLimit) continue;

        double c = scan.intensity(i);
        double s = 1.0/sqrt(c);

        values += QString("X[%1]=%2, Y[%1]=%3, S[%1]=%4, A[%1]=1\n").arg(n).arg(a, 0, 'f', 6).arg(c, 0, 'f', 6).arg(s, 0, 'f', 6);

        maxang = qMax(maxang, a);
        maxint = qMax(maxint, c);

        ++n;

        // we stop here, but after writing the first line > upLimit to the string
        if (a > upLimit) break;
    }

    // compile the hard-coded header
    QString ostring = compileHeader(file, n, maxang);

    ostring += values
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  (un)defines  ------------\n")
            % definesString()
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  variables and functions  ------------\n")
            % QStringLiteral("$_1 = ~500.0\n")   // linear y-pos
            % QStringLiteral("$_2 = ~-10.0\n")   // linear slope
            % QStringLiteral("\n")
            % QStringLiteral("%_1 = Linear($_1, $_2)\n")
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  models  ------------\n")
            % QStringLiteral("@0: F = %_1\n")
            % QStringLiteral("\n")
            % QString("plot [%1:%2] [%3:%4]\n").arg(loLimit, 0, 'f', 6).arg(upLimit, 0, 'f', 6).arg(-maxint/10.0, 0, 'f', 1).arg(maxint*1.2, 0, 'f', 6)
            % QStringLiteral("use @0\n")
            % QStringLiteral("set autoplot = 1\n")
            % QStringLiteral("set verbosity = 0\n\n")
            % QStringLiteral("# ------------  automatic fitting  ------------\n")
            % QStringLiteral("A = a and not (x > 34.2 and x <= 36.0)\n")
            % QStringLiteral("fit\n")
            % QStringLiteral("p sum(y-%_1(x) if x > 34.4 and x <= 35.9)\n");

    return ostring;
}

QString FitykFitExport::getBTcpRange(const QString &file, Scan &scan)
{
    double loLimit = 29.7;
    double upLimit = 35.0;

    QString values;

    int n = 0;
    double maxang = 0.0;
    double maxint = 0.0;

    for (int i = 0; i < scan.size(); ++i) {
        double a = scan.angle(i);

        if (a < loLimit) continue;

        double c = scan.intensity(i);
        double s = 1.0/sqrt(c);

        values += QString("X[%1]=%2, Y[%1]=%3, S[%1]=%4, A[%1]=1\n").arg(n).arg(a, 0, 'f', 6).arg(c, 0, 'f', 6).arg(s, 0, 'f', 6);

        maxang = qMax(maxang, a);
        maxint = qMax(maxint, c);

        ++n;

        // we stop here, but after writing the first line > upLimit to the string
        if (a > upLimit) break;
    }

    // compile the hard-coded header
    QString ostring = compileHeader(file, n, maxang);

    ostring += values
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  (un)defines  ------------\n")
            % definesString()
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  variables and functions  ------------\n")

            % QStringLiteral("$_1 = ~230.0\n")    // linear y-pos
            % QStringLiteral("$_2 = ~-5.0\n")     // linear slope
            % QStringLiteral("$_3 = ~0.1\n")      // pearson7 hwhm
            % QStringLiteral("$_4 = ~2.0\n")      // pearson7 shape

            % QStringLiteral("$_5 = ~1150.0\n")   // peak 1 intensity
            % QStringLiteral("$_6 = ~31.78\n")    // peak 1 pos
            % QStringLiteral("$_7 = ~525.0\n")    // peak 2 intensity
            % QStringLiteral("$_8 = ~32.10\n")    // peak 2 pos
            % QStringLiteral("\n")
            % QStringLiteral("%_1 = Linear($_1, $_2)\n")
            % QStringLiteral("%_2 = Pearson7($_5, $_6, $_3, $_4)\n")
            % QStringLiteral("%_3 = Pearson7($_7, $_8, $_3, $_4)\n")

            % QStringLiteral("\n")
            % QStringLiteral("# ------------  models  ------------\n")
            % QStringLiteral("@0: F = %_1 + %_2 + %_3\n")
            % QStringLiteral("\n")
            % QString("plot [%1:%2] [%3:%4]\n").arg(loLimit, 0, 'f', 6).arg(upLimit, 0, 'f', 6).arg(-maxint/10.0, 0, 'f', 1).arg(maxint*1.2, 0, 'f', 6)
            % QStringLiteral("use @0\n")
            % QStringLiteral("set autoplot = 1\n")
            % QStringLiteral("set verbosity = 0\n\n")
            % QStringLiteral("# ------------  automatic fitting  ------------\n")
            % QStringLiteral("A = a and not (x > 30.4 and x <= 34.6)\n")
            % QStringLiteral("A = a or (x > 31.4 and x <= 32.5)\n")
            % QStringLiteral("fit\n")
            % QStringLiteral("p sum(y-F(x) if x > 30.5 and x <= 31.5)\n");

    return ostring;
}

QString FitykFitExport::getCaORange(const QString &file, Scan &scan)
{
    double loLimit = 36.0;
    double upLimit = 38.8;

    QString values;

    int n = 0;
    double maxang = 0.0;
    double maxint = 0.0;

    for (int i = 0; i < scan.size(); ++i) {
        double a = scan.angle(i);

        if (a < loLimit) continue;

        double c = scan.intensity(i);
        double s = 1.0/sqrt(c);

        values += QString("X[%1]=%2, Y[%1]=%3, S[%1]=%4, A[%1]=1\n").arg(n).arg(a, 0, 'f', 6).arg(c, 0, 'f', 6).arg(s, 0, 'f', 6);

        maxang = qMax(maxang, a);
        maxint = qMax(maxint, c);

        ++n;

        // we stop here, but after writing the first line > upLimit to the string
        if (a > upLimit) break;
    }

    // compile the hard-coded header
    QString ostring = compileHeader(file, n, maxang);

    ostring += values
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  (un)defines  ------------\n")
            % definesString()
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  variables and functions  ------------\n")
            % QStringLiteral("$_1 = ~165.0\n")   // linear y-pos
            % QStringLiteral("$_2 = ~-3.0\n")    // linear slope

            % QStringLiteral("\n")
            % QStringLiteral("%_1 = Linear($_1, $_2)\n")
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  models  ------------\n")
            % QStringLiteral("@0: F = %_1")
            % QStringLiteral("\n")
            % QString("plot [%1:%2] [%3:%4]\n").arg(loLimit, 0, 'f', 6).arg(upLimit, 0, 'f', 6).arg(-maxint/10.0, 0, 'f', 1).arg(maxint*1.2, 0, 'f', 6)
            % QStringLiteral("use @0\n")
            % QStringLiteral("set autoplot = 1\n")
            % QStringLiteral("set verbosity = 0\n\n")
            % QStringLiteral("# ------------  automatic fitting  ------------\n")
            % QStringLiteral("fit\n")
            % QStringLiteral("print if x > 37.0 and x <= 38.5: x, y-%_1(x)\n")
            % QStringLiteral("p sum(y-%_1(x) if x > 37.0 and x <= 38.5)\n");

    return ostring;
}

QString FitykFitExport::getHARange(const QString &file, Scan &scan)
{
    double loLimit = 37.5;
    double upLimit = 59.5;

    QString values;

    int n = 0;
    double maxang = 0.0;
    double maxint = 0.0;

    for (int i = 0; i < scan.size(); ++i) {
        double a = scan.angle(i);

        if (a < loLimit) continue;

        double c = scan.intensity(i);
        double s = 1.0/sqrt(c);

        values += QString("X[%1]=%2, Y[%1]=%3, S[%1]=%4, A[%1]=1\n").arg(n).arg(a, 0, 'f', 6).arg(c, 0, 'f', 6).arg(s, 0, 'f', 6);

        maxang = qMax(maxang, a);
        maxint = qMax(maxint, c);

        ++n;

        // we stop here, but after writing the first line > upLimit to the string
        if (a > upLimit) break;
    }

    // compile the hard-coded header
    QString ostring = compileHeader(file, n, maxang);

    ostring += values
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  (un)defines  ------------\n")
            % definesString()
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  variables and functions  ------------\n")
            % QStringLiteral("$_1 = ~115.0\n")   // linear y-pos
            % QStringLiteral("$_2 = ~-1.5\n")    // linear slope

            % QStringLiteral("\n")
            % QStringLiteral("%_1 = Linear($_1, $_2)\n")

            % QStringLiteral("\n")
            % QStringLiteral("# ------------  models  ------------\n")
            % QStringLiteral("@0: F = %_1\n")
            % QString("plot [%1:%2] [%3:%4]\n").arg(loLimit, 0, 'f', 6).arg(upLimit, 0, 'f', 6).arg(-maxint/10.0, 0, 'f', 1).arg(maxint*1.2, 0, 'f', 6)
            % QStringLiteral("use @0\n")
            % QStringLiteral("set autoplot = 1\n")
            % QStringLiteral("set verbosity = 0\n\n")
            % QStringLiteral("# ------------  automatic fitting  ------------\n")
            % QStringLiteral("A = a and not (x > 38.3 and x <= 59.2)\n")
            % QStringLiteral("fit\n")
            % QStringLiteral("p sum(y-%_1(x) if x > 38.5 and x <= 59.0)\n");

    return ostring;
}

QString FitykFitExport::compileHeader(const QString &f, double m, double max)
{
    QFileInfo fi(f);

    // the header is always the same
    QString header = QString("# Fityk script. Fityk version: 0.9.8. Created: %1 by Profex\n\n")
               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    // using a QStringBuilder to compose the header string
    header += QStringLiteral("set verbosity = -1 #the rest of the file is not shown\n")
            % QStringLiteral("set autoplot = 0\n")
            % QStringLiteral("reset\n")
            % QStringLiteral("# ------------  settings  ------------\n")
            % QStringLiteral("set default_sigma = sqrt\n")
            % QStringLiteral("set domain_percent = 30\n")
            % QStringLiteral("set epsilon = 1e-012\n")
            % QStringLiteral("set exit_on_warning = 0\n")
            % QStringLiteral("set fit_replot = 0\n")
            % QStringLiteral("set fitting_method = levenberg_marquardt\n")
            % QStringLiteral("set function_cutoff = 0\n")
            % QStringLiteral("set guess_uses_weights = 1\n")
            % QStringLiteral("set height_correction = 1\n")
            % QStringLiteral("set lm_lambda_down_factor = 10\n")
            % QStringLiteral("set lm_lambda_start = 0.001\n")
            % QStringLiteral("set lm_lambda_up_factor = 10\n")
            % QStringLiteral("set lm_max_lambda = 1e+015\n")
            % QStringLiteral("set lm_stop_rel_change = 0.0001\n")
            % QStringLiteral("set log_full = 0\n")
            % QStringLiteral("set logfile = ''\n")
            % QStringLiteral("set max_wssr_evaluations = 1000\n")
            % QStringLiteral("set nm_convergence = 0.0001\n")
            % QStringLiteral("set nm_distribution = bound\n")
            % QStringLiteral("set nm_move_all = 0\n")
            % QStringLiteral("set nm_move_factor = 1\n")
            % QStringLiteral("set numeric_format = '%g'\n")
            % QStringLiteral("set pseudo_random_seed = 0\n")
            % QStringLiteral("set refresh_period = 4\n")
            % QStringLiteral("set width_correction = 1\n")
            % QStringLiteral("\n")
            % QStringLiteral("# ------------  datasets ------------\n")
            % QStringLiteral("use @0\n")
            % QString("title = '%1'\n").arg(fi.completeBaseName())
            % QString("M=%1\n").arg(m)
            % QString("X=%1# =max(x), prevents sorting.\n\n").arg(max, 0, 'f', 6);

    return header;
}

QString FitykFitExport::definesString()
{
    QString str = QStringLiteral("# define Constant(a=avgy) = a\n")
                % QStringLiteral("# define Linear(a0=intercept, a1=slope) = a0 + a1 * x\n")
                % QStringLiteral("# define Quadratic(a0=intercept, a1=slope, a2=0) = a0 + a1*x + ")
                % QStringLiteral("a2*x^2\n")
                % QStringLiteral("# define Cubic(a0=intercept, a1=slope, a2=0, a3=0) = a0 + a1*x + ")
                % QStringLiteral("a2*x^2 + a3*x^3\n")
                % QStringLiteral("# define Polynomial4(a0=intercept, a1=slope, a2=0, a3=0, a4=0) = ")
                % QStringLiteral("a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4\n")
                % QStringLiteral("# define Polynomial5(a0=intercept, a1=slope, a2=0, a3=0, a4=0, ")
                % QStringLiteral("a5=0) = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5\n")
                % QStringLiteral("# define Polynomial6(a0=intercept, a1=slope, a2=0, a3=0, a4=0, ")
                % QStringLiteral("a5=0, a6=0) = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6\n")
                % QStringLiteral("# define Gaussian(height, center, hwhm) = height*exp(-ln(2)*")
                % QStringLiteral("((x-center)/hwhm)^2)\n")
                % QStringLiteral("# define SplitGaussian(height, center, hwhm1=hwhm, hwhm2=hwhm) ")
                % QStringLiteral("= x<center ? Gaussian(height,center,hwhm1) : Gaussian(height,center,hwhm2)\n")
                % QStringLiteral("# define Lorentzian(height, center, hwhm) = height/(1+((x-center)/hwhm)^2)\n")
                % QStringLiteral("# define Pearson7(height, center, hwhm, shape=2) = height/(1+((x-center)")
                % QStringLiteral("/hwhm)^2*(2^(1/shape)-1))^shape\n")
                % QStringLiteral("# define SplitPearson7(height, center, hwhm1=hwhm, hwhm2=hwhm, shape1=2, ")
                % QStringLiteral("shape2=2) = x < center ? Pearson7(height, center, hwhm1, shape1) : ")
                % QStringLiteral("Pearson7(height, center, hwhm2, shape2)\n")
                % QStringLiteral("# define PseudoVoigt(height, center, hwhm, shape=0.5) = height*((1-shape)*")
                % QStringLiteral("exp(-ln(2)*((x-center)/hwhm)^2)+shape/(1+((x-center)/hwhm)^2))\n")
                % QStringLiteral("# define Voigt(height, center, gwidth=hwhm*0.8, shape=0.1) = convolution ")
                % QStringLiteral("of Gaussian and Lorentzian #\n")
                % QStringLiteral("# define VoigtA(area, center, gwidth=hwhm*0.8, shape=0.1) = convolution of ")
                % QStringLiteral("Gaussian and Lorentzian #\n")
                % QStringLiteral("# define EMG(a=height, b=center, c=hwhm*0.8, d=hwhm*0.08) = a*c*(2*pi)^0.5/(2*d) ")
                % QStringLiteral("* exp((b-x)/d + c^2/(2*d^2)) * (abs(d)/d - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))\n")
                % QStringLiteral("# define DoniachSunjic(h=height, a=0.1, f=1, e=center) = h * cos(pi*a/2 + ")
                % QStringLiteral("(1-a)*atan((x-e)/f)) / (f^2+(x-e)^2)^((1-a)/2)\n")
                % QStringLiteral("# define PielaszekCube(a=height*0.016, center, r=300, s=150) = ...#\n")
                % QStringLiteral("# define LogNormal(height, center, width=2*hwhm, asym=0.1) = height*exp(-ln(2)")
                % QStringLiteral("*(ln(2.0*asym*(x-center)/width+1)/asym)^2)\n")
                % QStringLiteral("# define Spline() = cubic spline #\n")
                % QStringLiteral("# define Polyline() = linear interpolation #\n")
                % QStringLiteral("# define ExpDecay(a=0, t=1) = a*exp(-x/t)\n")
                % QStringLiteral("# define GaussianA(area, center, hwhm) = Gaussian(area/hwhm/sqrt(pi/ln(2)), ")
                % QStringLiteral("center, hwhm)\n")
                % QStringLiteral("# define LogNormalA(area, center, width=2*hwhm, asym=0.1) = LogNormal(sqrt(ln(2)/pi)*")
                % QStringLiteral("(2*area/width)*exp(-asym^2/4/ln(2)), center, width, asym)\n")
                % QStringLiteral("# define LorentzianA(area, center, hwhm) = Lorentzian(area/hwhm/pi, center, hwhm)\n")
                % QStringLiteral("# define Pearson7A(area, center, hwhm, shape=2) = Pearson7(area/(hwhm*")
                % QStringLiteral("exp(lgamma(shape-0.5)-lgamma(shape))*sqrt(pi/(2^(1/shape)-1))), center, hwhm, shape)\n")
                % QStringLiteral("# define PseudoVoigtA(area, center, hwhm, shape=0.5) = GaussianA(area*(1-shape), ")
                % QStringLiteral("center, hwhm) + LorentzianA(area*shape, center, hwhm)\n")
                % QStringLiteral("# define SplitLorentzian(height, center, hwhm1=hwhm, hwhm2=hwhm) = x < center ? ")
                % QStringLiteral("Lorentzian(height, center, hwhm1) : Lorentzian(height, center, hwhm2)\n")
                % QStringLiteral("# define SplitPseudoVoigt(height, center, hwhm1=hwhm, hwhm2=hwhm, shape1=0.5, ")
                % QStringLiteral("shape2=0.5) = x < center ? PseudoVoigt(height, center, hwhm1, shape1) : ")
                % QStringLiteral("PseudoVoigt(height, center, hwhm2, shape2)\n")
                % QStringLiteral("# define SplitVoigt(height, center, hwhm1=hwhm, hwhm2=hwhm, shape1=0.5, shape2=0.5) ")
                % QStringLiteral("= x < center ? Voigt(height, center, hwhm1, shape1) : Voigt(height, center, hwhm2, shape2)\n");

    return str;
}
