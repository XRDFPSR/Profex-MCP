/***************************************************************************
                          peaklistfilterform.cpp  -  description
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

#include "peaklistfilterform.h"
#include "structs.h"
#include "peaklistfilterhkldelegate.h"
#include "peaklistfilterdoubledelegate.h"
#include <QHeaderView>

PeakListFilterForm::PeakListFilterForm(QWidget *parent) :
    QTreeWidget(parent)
{
    settings = SettingsManager::getInstance();
    comboBoxPhases = nullptr;

    setColumnCount(3);
    setHeaderLabels(QStringList() << QString("Filter parameter") << QString("Range start") << QString("Range end"));

    QString angleLabel = QString("Angle (%1 2%2)").arg(global::degree, global::theta);
    QString dLabel("d (nm)");
    QString absIntLabel("Intensity (deg*cts)");
    QString relIntLabel("Rel. intensity (%)");

    it_idx_h       = new PeakListFilterItemHkl(this, "h");
    it_idx_k       = new PeakListFilterItemHkl(this, "k");
    it_idx_l       = new PeakListFilterItemHkl(this, "l");
    it_range_tt    = new PeakListFilterItemDouble(this, angleLabel);
    it_range_d     = new PeakListFilterItemDouble(this, dLabel);
    it_intens_abs  = new PeakListFilterItemDouble(this, absIntLabel);
    it_intens_rel  = new PeakListFilterItemDouble(this, relIntLabel);
    it_profile_tex = new PeakListFilterItemDouble(this, "Texture");
    it_profile_b1  = new PeakListFilterItemDouble(this, "B1");
    it_profile_b2  = new PeakListFilterItemDouble(this, "B2");

    setItemDelegateForRow(0, new PeakListFilterHklDelegate(0, 0, this));
    setItemDelegateForRow(1, new PeakListFilterHklDelegate(0, 0, this));
    setItemDelegateForRow(2, new PeakListFilterHklDelegate(0, 0, this));
    setItemDelegateForRow(3, new PeakListFilterDoubleDelegate(2, 0.0, 0.0, 0.1, this));
    setItemDelegateForRow(4, new PeakListFilterDoubleDelegate(4, 0.0, 0.0, 0.01, this));
    setItemDelegateForRow(5, new PeakListFilterDoubleDelegate(2, 0.0, 0.0, 1.0, this));
    setItemDelegateForRow(6, new PeakListFilterDoubleDelegate(2, 0.0, 100.0, 1.0, this));
    setItemDelegateForRow(7, new PeakListFilterDoubleDelegate(4, 0.0, 0.0, 0.1, this));
    setItemDelegateForRow(8, new PeakListFilterDoubleDelegate(6, 0.0, 0.0, 0.001, this));
    setItemDelegateForRow(9, new PeakListFilterDoubleDelegate(6, 0.0, 0.0, 0.001, this));

    proxyModel = nullptr;

    header()->restoreState(settings->value("peakListWidget/filterHeader", QByteArray()).toByteArray());

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(filterParametersChanged()));
    connect(this->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(saveFilterHeaderState()));
}

PeakListFilterForm::~PeakListFilterForm()
{
}

void PeakListFilterForm::setComboBox(QComboBox *c)
{
    comboBoxPhases = c;
    connect(comboBoxPhases, SIGNAL(currentIndexChanged(int)), this, SLOT(filterParametersChanged()));
}

void PeakListFilterForm::filterParametersChanged()
{
    if (!proxyModel) return;
    if (!comboBoxPhases) return;

    QString pattern = comboBoxPhases->currentData(Qt::UserRole).toString();

    if (pattern.isEmpty()) {
        // manually entered regexp patterns do not have UserRoles. Use the entered text directly instead
        pattern = comboBoxPhases->currentText();
    } else {
        // use the pre-defined pattern from the UserRole strictly
        pattern = "^" + pattern + "$";
    }

    QMap<QString, QVariant> filterParams;

    if (it_idx_h->isChecked()) {
        filterParams["hlow"]  = it_idx_h->getMin();
        filterParams["hhigh"] = it_idx_h->getMax();
    }

    if (it_idx_k->isChecked()) {
        filterParams["klow"]  = it_idx_k->getMin();
        filterParams["khigh"] = it_idx_k->getMax();
    }

    if (it_idx_l->isChecked()) {
        filterParams["llow"]  = it_idx_l->getMin();
        filterParams["lhigh"] = it_idx_l->getMax();
    }

    if (it_range_tt->isChecked()) {
        filterParams["2tmin"] = it_range_tt->getMin();
        filterParams["2tmax"] = it_range_tt->getMax();
    }

    if (it_range_d->isChecked()) {
        filterParams["dmin"] = it_range_d->getMin();
        filterParams["dmax"] = it_range_d->getMax();
    }

    if (it_intens_abs->isChecked()) {
        filterParams["iabsmin"] = it_intens_abs->getMin();
        filterParams["iabsmax"] = it_intens_abs->getMax();
    }

    if (it_intens_rel->isChecked()) {
        filterParams["irelmin"] = it_intens_rel->getMin();
        filterParams["irelmax"] = it_intens_rel->getMax();
    }

    if (it_profile_tex->isChecked()) {
        filterParams["texmin"] = it_profile_tex->getMin();
        filterParams["texmax"] = it_profile_tex->getMax();
    }

    if (it_profile_b1->isChecked()) {
        filterParams["b1min"] = it_profile_b1->getMin();
        filterParams["b1max"] = it_profile_b1->getMax();
    }

    if (it_profile_b2->isChecked()) {
        filterParams["b2min"] = it_profile_b2->getMin();
        filterParams["b2max"] = it_profile_b2->getMax();
    }

    // also set it if the map is empty, otherwise unsetting all filters may fail
    proxyModel->setFilters(filterParams, pattern);
}

void PeakListFilterForm::setHklLimits(int hmin, int hmax, int kmin, int kmax, int lmin, int lmax)
{
    hklLimits["idx_h_min"] = qMin(hmin, hmax);
    hklLimits["idx_h_max"] = qMax(hmin, hmax);
    hklLimits["idx_k_min"] = qMin(kmin, kmax);
    hklLimits["idx_k_max"] = qMax(kmin, kmax);
    hklLimits["idx_l_min"] = qMin(lmin, lmax);
    hklLimits["idx_l_max"] = qMax(lmin, lmax);

    if (!it_idx_h->isChecked()) {
        it_idx_h->setText(1, QString("%1").arg(hklLimits.value("idx_h_min")));
        it_idx_h->setText(2, QString("%1").arg(hklLimits.value("idx_h_max")));
    }

    if (!it_idx_k->isChecked()) {
        it_idx_k->setText(1, QString("%1").arg(hklLimits.value("idx_k_min")));
        it_idx_k->setText(2, QString("%1").arg(hklLimits.value("idx_k_max")));
    }

    if (!it_idx_l->isChecked()) {
        it_idx_l->setText(1, QString("%1").arg(hklLimits.value("idx_l_min")));
        it_idx_l->setText(2, QString("%1").arg(hklLimits.value("idx_l_max")));
    }

    PeakListFilterHklDelegate *hdel = qobject_cast<PeakListFilterHklDelegate*>(itemDelegateForRow(0));
    PeakListFilterHklDelegate *kdel = qobject_cast<PeakListFilterHklDelegate*>(itemDelegateForRow(1));
    PeakListFilterHklDelegate *ldel = qobject_cast<PeakListFilterHklDelegate*>(itemDelegateForRow(2));

    if (hdel) hdel->setLimits(hklLimits.value("idx_h_min"), hklLimits.value("idx_h_max"));
    if (kdel) kdel->setLimits(hklLimits.value("idx_k_min"), hklLimits.value("idx_k_max"));
    if (ldel) ldel->setLimits(hklLimits.value("idx_l_min"), hklLimits.value("idx_l_max"));
}

void PeakListFilterForm::setRangeLimits(double ttMin, double ttMax, double dMin, double dMax)
{
    rangeLimits["range_tt_min"]   = qMin(ttMin, ttMax);
    rangeLimits["range_tt_max"]   = qMax(ttMin, ttMax);
    rangeLimits["range_d_min"]    = qMin(dMin, dMax);
    rangeLimits["range_d_max"]    = qMax(dMin, dMax);

    if (!it_range_tt->isChecked()) {
        it_range_tt->setText(1,   QString("%1").arg(rangeLimits.value("range_tt_min"), 0, 'f', 2));
        it_range_tt->setText(2,   QString("%1").arg(rangeLimits.value("range_tt_max"), 0, 'f', 2));
    }

    if (!it_range_d->isChecked()) {
        it_range_d->setText(1,    QString("%1").arg(rangeLimits.value("range_d_min"), 0, 'f', 6));
        it_range_d->setText(2,    QString("%1").arg(rangeLimits.value("range_d_max"), 0, 'f', 6));
    }

    PeakListFilterDoubleDelegate *tdel = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(3));
    PeakListFilterDoubleDelegate *ddel = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(4));

    if (tdel) tdel->setLimits(rangeLimits.value("range_tt_min"), rangeLimits.value("range_tt_max"));
    if (ddel) ddel->setLimits(rangeLimits.value("range_d_min"),  rangeLimits.value("range_d_max"));
}

void PeakListFilterForm::setIntensityLimits(double iMin, double iMax, double riMin, double riMax)
{
    rangeLimits["range_int_min"]  = qMin(iMin, iMax);
    rangeLimits["range_int_max"]  = qMax(iMin, iMax);
    rangeLimits["range_rint_min"] = qMin(riMin, riMax);
    rangeLimits["range_rint_max"] = qMax(riMin, riMax);

    if (!it_intens_abs->isChecked()) {
        it_intens_abs->setText(1, QString("%1").arg(rangeLimits.value("range_int_min"), 0, 'f', 2));
        it_intens_abs->setText(2, QString("%1").arg(rangeLimits.value("range_int_max"), 0, 'f', 2));
    }

    if (!it_intens_rel->isChecked()) {
        it_intens_rel->setText(1, QString("%1").arg(rangeLimits.value("range_rint_min"), 0, 'f', 2));
        it_intens_rel->setText(2, QString("%1").arg(rangeLimits.value("range_rint_max"), 0, 'f', 2));
    }

    PeakListFilterDoubleDelegate *idel = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(5));
    PeakListFilterDoubleDelegate *ridel = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(6));

    if (idel)   idel->setLimits(rangeLimits.value("range_int_min"),  rangeLimits.value("range_int_max"));
    if (ridel) ridel->setLimits(rangeLimits.value("range_rint_min"), rangeLimits.value("range_rint_max"));
}

void PeakListFilterForm::setProfileLimits(double tMin, double tMax, double b1Min, double b1Max, double b2Min, double b2Max)
{
    rangeLimits["profile_tex_min"] = qMin(tMin, tMax);
    rangeLimits["profile_tex_max"] = qMax(tMin, tMax);
    rangeLimits["profile_b1_min"]  = qMin(b1Min, b1Max);
    rangeLimits["profile_b1_max"]  = qMax(b1Min, b1Max);
    rangeLimits["profile_b2_min"]  = qMin(b2Min, b2Max);
    rangeLimits["profile_b2_max"]  = qMax(b2Min, b2Max);

    if (!it_profile_tex->isChecked()) {
        it_profile_tex->setText(1, QString("%1").arg(rangeLimits.value("profile_tex_min"), 0, 'f', 4));
        it_profile_tex->setText(2, QString("%1").arg(rangeLimits.value("profile_tex_max"), 0, 'f', 4));
    }

    if (!it_profile_b1->isChecked()) {
        it_profile_b1->setText(1, QString("%1").arg(rangeLimits.value("profile_b1_min"), 0, 'f', 4));
        it_profile_b1->setText(2, QString("%1").arg(rangeLimits.value("profile_b1_max"), 0, 'f', 4));
    }

    if (!it_profile_b2->isChecked()) {
        it_profile_b2->setText(1, QString("%1").arg(rangeLimits.value("profile_b2_min"), 0, 'f', 4));
        it_profile_b2->setText(2, QString("%1").arg(rangeLimits.value("profile_b2_max"), 0, 'f', 4));
    }

    PeakListFilterDoubleDelegate *texdel = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(7));
    PeakListFilterDoubleDelegate *b1del  = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(8));
    PeakListFilterDoubleDelegate *b2del  = qobject_cast<PeakListFilterDoubleDelegate*>(itemDelegateForRow(9));

    if (texdel) texdel->setLimits(rangeLimits.value("profile_tex_min"), rangeLimits.value("profile_tex_max"));
    if (b1del)  b1del->setLimits(rangeLimits.value("profile_b1_min"), rangeLimits.value("profile_b1_max"));
    if (b2del)  b2del->setLimits(rangeLimits.value("profile_b2_min"), rangeLimits.value("profile_b2_max"));
}

void PeakListFilterForm::saveFilterHeaderState()
{
    settings->setValue("peakListWidget/filterHeader", header()->saveState());
}

void PeakListFilterForm::saveRegexpHeaderState()
{
    settings->setValue("peakListWidget/regexpFilterHeader", header()->saveState());
}

QDomElement PeakListFilterForm::getFilterParameters() const
{
    QDomDocument doc("peakListFilters");
    QDomElement el = doc.createElement("peakListFilters");
    doc.appendChild(el);

    QDomElement elPhase = doc.createElement("phasePattern");
    elPhase.setAttribute("name", comboBoxPhases->currentText());
    elPhase.setAttribute("pattern", comboBoxPhases->currentData(Qt::UserRole).toString());
    el.appendChild(elPhase);

    if (it_idx_h->isChecked()) {
        QDomElement elH = doc.createElement("idxRangeH");
        elH.setAttribute("min", it_idx_h->getMin());
        elH.setAttribute("max", it_idx_h->getMax());
        el.appendChild(elH);
    }

    if (it_idx_k->isChecked()) {
        QDomElement elK = doc.createElement("idxRangeK");
        elK.setAttribute("min", it_idx_k->getMin());
        elK.setAttribute("max", it_idx_k->getMax());
        el.appendChild(elK);
    }

    if (it_idx_l->isChecked()) {
        QDomElement elL = doc.createElement("idxRangeL");
        elL.setAttribute("min", it_idx_l->getMin());
        elL.setAttribute("max", it_idx_l->getMax());
        el.appendChild(elL);
    }

    if (it_range_tt->isChecked()) {
        QDomElement elTT = doc.createElement("dataRangeTwoTheta");
        elTT.setAttribute("min", it_range_tt->getMin());
        elTT.setAttribute("max", it_range_tt->getMax());
        el.appendChild(elTT);
    }

    if (it_range_d->isChecked()) {
        QDomElement elD = doc.createElement("dataRangeD");
        elD.setAttribute("min", it_range_d->getMin());
        elD.setAttribute("max", it_range_d->getMax());
        el.appendChild(elD);
    }

    if (it_intens_abs->isChecked()) {
        QDomElement elIntA = doc.createElement("intensityRangeAbs");
        elIntA.setAttribute("min", it_intens_abs->getMin());
        elIntA.setAttribute("max", it_intens_abs->getMax());
        el.appendChild(elIntA);
    }

    if (it_intens_rel->isChecked()) {
        QDomElement elIntR = doc.createElement("intensityRangeRel");
        elIntR.setAttribute("min", it_intens_rel->getMin());
        elIntR.setAttribute("max", it_intens_rel->getMax());
        el.appendChild(elIntR);
    }

    if (it_profile_tex->isChecked()) {
        QDomElement elTex = doc.createElement("profileRangeTexture");
        elTex.setAttribute("min", it_profile_tex->getMin());
        elTex.setAttribute("max", it_profile_tex->getMax());
        el.appendChild(elTex);
    }

    if (it_profile_b1->isChecked()) {
        QDomElement elB1 = doc.createElement("profileRangeB1");
        elB1.setAttribute("min", it_profile_b1->getMin());
        elB1.setAttribute("max", it_profile_b1->getMax());
        el.appendChild(elB1);
    }

    if (it_profile_b2->isChecked()) {
        QDomElement elB2 = doc.createElement("profileRangeB2");
        elB2.setAttribute("min", it_profile_b2->getMin());
        elB2.setAttribute("max", it_profile_b2->getMax());
        el.appendChild(elB2);
    }

    // it always contains the "phasePattern" child node, but we only
    // return the element if it contains at least one filter
    if (el.childNodes().size() > 1) return el;
    return QDomElement();
}

void PeakListFilterForm::setFilterParameters(const QDomElement &el)
{
    if (el.tagName() != QString("peakListFilters")) return;

    QDomElement elPhase = el.firstChildElement("phasePattern");
    QDomElement elH     = el.firstChildElement("idxRangeH");
    QDomElement elK     = el.firstChildElement("idxRangeK");
    QDomElement elL     = el.firstChildElement("idxRangeL");
    QDomElement elTT    = el.firstChildElement("dataRangeTwoTheta");
    QDomElement elD     = el.firstChildElement("dataRangeD");
    QDomElement elIabs  = el.firstChildElement("intensityRangeAbs");
    QDomElement elIrel  = el.firstChildElement("intensityRangeRel");
    QDomElement elTex   = el.firstChildElement("profileRangeTexture");
    QDomElement elB1    = el.firstChildElement("profileRangeB1");
    QDomElement elB2    = el.firstChildElement("profileRangeB2");

    if (elPhase.isNull()) {
        comboBoxPhases->setCurrentIndex(0);
    } else {
        QString pName = elPhase.attribute("name");
        QString pPtrn = elPhase.attribute("pattern");
        int ptrnIdx = comboBoxPhases->findData(pPtrn, Qt::UserRole);
        int pnmeIdx = comboBoxPhases->findText(pName);

        if (ptrnIdx > -1) {
            comboBoxPhases->setCurrentIndex(ptrnIdx);
        } else if (pnmeIdx > -1) {
            comboBoxPhases->setCurrentIndex(pnmeIdx);
        } else {
            comboBoxPhases->addItem(pName, pPtrn);
            comboBoxPhases->setCurrentIndex(comboBoxPhases->count() - 1);
        }
    }

    if (elH.isNull()) {
        it_idx_h->setCheckState(0, Qt::Unchecked);
    } else {
        it_idx_h->setCheckState(0, Qt::Checked);
        it_idx_h->setText(1, elH.attribute("min"));
        it_idx_h->setText(2, elH.attribute("max"));
    }

    if (elK.isNull()) {
        it_idx_k->setCheckState(0, Qt::Unchecked);
    } else {
        it_idx_k->setCheckState(0, Qt::Checked);
        it_idx_k->setText(1, elK.attribute("min"));
        it_idx_k->setText(2, elK.attribute("max"));
    }

    if (elL.isNull()) {
        it_idx_l->setCheckState(0, Qt::Unchecked);
    } else {
        it_idx_l->setCheckState(0, Qt::Checked);
        it_idx_l->setText(1, elL.attribute("min"));
        it_idx_l->setText(2, elL.attribute("max"));
    }

    if (elTT.isNull()) {
        it_range_tt->setCheckState(0, Qt::Unchecked);
    } else {
        it_range_tt->setCheckState(0, Qt::Checked);
        it_range_tt->setText(1, elTT.attribute("min"));
        it_range_tt->setText(2, elTT.attribute("max"));
    }

    if (elD.isNull()) {
        it_range_d->setCheckState(0, Qt::Unchecked);
    } else {
        it_range_d->setCheckState(0, Qt::Checked);
        it_range_d->setText(1, elD.attribute("min"));
        it_range_d->setText(2, elD.attribute("max"));
    }

    if (elIabs.isNull()) {
        it_intens_abs->setCheckState(0, Qt::Unchecked);
    } else {
        it_intens_abs->setCheckState(0, Qt::Checked);
        it_intens_abs->setText(1, elIabs.attribute("min"));
        it_intens_abs->setText(2, elIabs.attribute("max"));
    }

    if (elIrel.isNull()) {
        it_intens_rel->setCheckState(0, Qt::Unchecked);
    } else {
        it_intens_rel->setCheckState(0, Qt::Checked);
        it_intens_rel->setText(1, elIrel.attribute("min"));
        it_intens_rel->setText(2, elIrel.attribute("max"));
    }

    if (elTex.isNull()) {
        it_profile_tex->setCheckState(0, Qt::Unchecked);
    } else {
        it_profile_tex->setCheckState(0, Qt::Checked);
        it_profile_tex->setText(1, elTex.attribute("min"));
        it_profile_tex->setText(2, elTex.attribute("max"));
    }

    if (elB1.isNull()) {
        it_profile_b1->setCheckState(0, Qt::Unchecked);
    } else {
        it_profile_b1->setCheckState(0, Qt::Checked);
        it_profile_b1->setText(1, elB1.attribute("min"));
        it_profile_b1->setText(2, elB1.attribute("max"));
    }

    if (elB2.isNull()) {
        it_profile_b2->setCheckState(0, Qt::Unchecked);
    } else {
        it_profile_b2->setCheckState(0, Qt::Checked);
        it_profile_b2->setText(1, elB2.attribute("min"));
        it_profile_b2->setText(2, elB2.attribute("max"));
    }

    filterParametersChanged();
}
