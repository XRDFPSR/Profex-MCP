/***************************************************************************
                          peaklistfilterform.h  -  description
                             -------------------
    begin                : Thu Mar 23 21:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef PEAKLISTFILTERFORM_H
#define PEAKLISTFILTERFORM_H

#include <QTreeWidget>
#include <QDomElement>
#include <QComboBox>
#include "peaklistfilteritemhkl.h"
#include "peaklistfilteritemdouble.h"
#include "hklsortfilterproxymodel.h"
#include "../libXrdIO/settingsmanager.h"

class PeakListFilterForm : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PeakListFilterForm(QWidget *parent = nullptr);
    ~PeakListFilterForm();

    inline void setProxyModel(HklSortFilterProxyModel *m) {proxyModel = m;}
    void setComboBox(QComboBox *);

    // set force to true if the filter state is changes programmatically.
    // if the peak data has changed, set force to false
    void setHklLimits(int, int, int, int, int, int);
    void setRangeLimits(double, double, double, double);
    void setIntensityLimits(double, double, double, double);
    void setProfileLimits(double, double, double, double, double, double);

    inline double getRangeTtMin() {return rangeLimits.value("range_tt_min", 0.0);}
    inline double getRangeTtMax() {return rangeLimits.value("range_tt_max", 180.0);}
    inline double getRangeDMin()  {return rangeLimits.value("range_d_min", 0.0);}
    inline double getRangeDMax()  {return rangeLimits.value("range_d_max", 1.0);}

    inline double getRangeTexMin() {return rangeLimits.value("profile_tex_min", 0.0);}
    inline double getRangeTexMax() {return rangeLimits.value("profile_tex_max", 0.0);}
    inline double getRangeB1Min()  {return rangeLimits.value("profile_b1_min", 0.0);}
    inline double getRangeB1Max()  {return rangeLimits.value("profile_b1_max", 0.0);}
    inline double getRangeB2Min()  {return rangeLimits.value("profile_b2_min", 0.0);}
    inline double getRangeB2Max()  {return rangeLimits.value("profile_b2_max", 0.0);}

    inline int getRangeHmin() {return hklLimits.value("idx_h_min", 0);}
    inline int getRangeHmax() {return hklLimits.value("idx_h_max", 0);}
    inline int getRangeKmin() {return hklLimits.value("idx_k_min", 0);}
    inline int getRangeKmax() {return hklLimits.value("idx_k_max", 0);}
    inline int getRangeLmin() {return hklLimits.value("idx_l_min", 0);}
    inline int getRangeLmax() {return hklLimits.value("idx_l_max", 0);}

    QDomElement getFilterParameters() const;
    void setFilterParameters(const QDomElement &);

private:
    SettingsManager *settings;
    HklSortFilterProxyModel *proxyModel;
    QMap<QString, int> hklLimits;
    QMap<QString, double> rangeLimits;
    QComboBox *comboBoxPhases;

    PeakListFilterItemHkl *it_idx_h;
    PeakListFilterItemHkl *it_idx_k;
    PeakListFilterItemHkl *it_idx_l;

    PeakListFilterItemDouble *it_range_tt;
    PeakListFilterItemDouble *it_range_d;
    PeakListFilterItemDouble *it_intens_abs;
    PeakListFilterItemDouble *it_intens_rel;
    PeakListFilterItemDouble *it_profile_tex;
    PeakListFilterItemDouble *it_profile_b1;
    PeakListFilterItemDouble *it_profile_b2;

public slots:
    void filterParametersChanged();

private slots:
    void saveFilterHeaderState();
    void saveRegexpHeaderState();
};

#endif // PEAKLISTFILTERFORM_H
