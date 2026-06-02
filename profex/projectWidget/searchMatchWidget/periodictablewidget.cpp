/***************************************************************************
                          periodictablewidget.cpp  -  description
                             -------------------
    begin                : Mon Aug 03 18:21:00 CEST 2020
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

#include "periodictablewidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

PeriodicTableWidget::PeriodicTableWidget(QWidget *parent) : QWidget(parent)
{
    buttonMap.insert(1,  new PeriodicTableButton("H",   1,  1, this));
    buttonMap.insert(2,  new PeriodicTableButton("HE",  1, 19, this));
    buttonMap.insert(3,  new PeriodicTableButton("LI",  2,  1, this));
    buttonMap.insert(4,  new PeriodicTableButton("BE",  2,  2, this));
    buttonMap.insert(5,  new PeriodicTableButton("B",   2, 14, this));
    buttonMap.insert(6,  new PeriodicTableButton("C",   2, 15, this));
    buttonMap.insert(7,  new PeriodicTableButton("N",   2, 16, this));
    buttonMap.insert(8,  new PeriodicTableButton("O",   2, 17, this));
    buttonMap.insert(9,  new PeriodicTableButton("F",   2, 18, this));
    buttonMap.insert(10, new PeriodicTableButton("NE",  2, 19, this));
    buttonMap.insert(11, new PeriodicTableButton("NA",  3,  1, this));
    buttonMap.insert(12, new PeriodicTableButton("MG",  3,  2, this));
    buttonMap.insert(13, new PeriodicTableButton("AL",  3, 14, this));
    buttonMap.insert(14, new PeriodicTableButton("SI",  3, 15, this));
    buttonMap.insert(15, new PeriodicTableButton("P",   3, 16, this));
    buttonMap.insert(16, new PeriodicTableButton("S",   3, 17, this));
    buttonMap.insert(17, new PeriodicTableButton("CL",  3, 18, this));
    buttonMap.insert(18, new PeriodicTableButton("AR",  3, 19, this));
    buttonMap.insert(19, new PeriodicTableButton("K",   4,  1, this));
    buttonMap.insert(20, new PeriodicTableButton("CA",  4,  2, this));
    buttonMap.insert(21, new PeriodicTableButton("SC",  4,  4, this));
    buttonMap.insert(22, new PeriodicTableButton("TI",  4,  5, this));
    buttonMap.insert(23, new PeriodicTableButton("V",   4,  6, this));
    buttonMap.insert(24, new PeriodicTableButton("CR",  4,  7, this));
    buttonMap.insert(25, new PeriodicTableButton("MN",  4,  8, this));
    buttonMap.insert(26, new PeriodicTableButton("FE",  4,  9, this));
    buttonMap.insert(27, new PeriodicTableButton("CO",  4, 10, this));
    buttonMap.insert(28, new PeriodicTableButton("NI",  4, 11, this));
    buttonMap.insert(29, new PeriodicTableButton("CU",  4, 12, this));
    buttonMap.insert(30, new PeriodicTableButton("ZN",  4, 13, this));
    buttonMap.insert(31, new PeriodicTableButton("GA",  4, 14, this));
    buttonMap.insert(32, new PeriodicTableButton("GE",  4, 15, this));
    buttonMap.insert(33, new PeriodicTableButton("AS",  4, 16, this));
    buttonMap.insert(34, new PeriodicTableButton("SE",  4, 17, this));
    buttonMap.insert(35, new PeriodicTableButton("BR",  4, 18, this));
    buttonMap.insert(36, new PeriodicTableButton("KR",  4, 19, this));
    buttonMap.insert(37, new PeriodicTableButton("RB",  5,  1, this));
    buttonMap.insert(38, new PeriodicTableButton("SR",  5,  2, this));
    buttonMap.insert(39, new PeriodicTableButton("Y",   5,  4, this));
    buttonMap.insert(40, new PeriodicTableButton("ZR",  5,  5, this));
    buttonMap.insert(41, new PeriodicTableButton("NB",  5,  6, this));
    buttonMap.insert(42, new PeriodicTableButton("MO",  5,  7, this));
    buttonMap.insert(43, new PeriodicTableButton("TC",  5,  8, this));
    buttonMap.insert(44, new PeriodicTableButton("RU",  5,  9, this));
    buttonMap.insert(45, new PeriodicTableButton("RH",  5, 10, this));
    buttonMap.insert(46, new PeriodicTableButton("PD",  5, 11, this));
    buttonMap.insert(47, new PeriodicTableButton("AG",  5, 12, this));
    buttonMap.insert(48, new PeriodicTableButton("CD",  5, 13, this));
    buttonMap.insert(49, new PeriodicTableButton("IN",  5, 14, this));
    buttonMap.insert(50, new PeriodicTableButton("SN",  5, 15, this));
    buttonMap.insert(51, new PeriodicTableButton("SB",  5, 16, this));
    buttonMap.insert(52, new PeriodicTableButton("TE",  5, 17, this));
    buttonMap.insert(53, new PeriodicTableButton("I",   5, 18, this));
    buttonMap.insert(54, new PeriodicTableButton("XE",  5, 19, this));
    buttonMap.insert(55, new PeriodicTableButton("CS",  6,  1, this));
    buttonMap.insert(56, new PeriodicTableButton("BA",  6,  2, this));
    buttonMap.insert(57, new PeriodicTableButton("LA",  9,  4, this));
    buttonMap.insert(58, new PeriodicTableButton("CE",  9,  5, this));
    buttonMap.insert(59, new PeriodicTableButton("PR",  9,  6, this));
    buttonMap.insert(60, new PeriodicTableButton("ND",  9,  7, this));
    buttonMap.insert(61, new PeriodicTableButton("PM",  9,  8, this));
    buttonMap.insert(62, new PeriodicTableButton("SM",  9,  9, this));
    buttonMap.insert(63, new PeriodicTableButton("EU",  9, 10, this));
    buttonMap.insert(64, new PeriodicTableButton("GD",  9, 11, this));
    buttonMap.insert(65, new PeriodicTableButton("TB",  9, 12, this));
    buttonMap.insert(66, new PeriodicTableButton("DY",  9, 13, this));
    buttonMap.insert(67, new PeriodicTableButton("HO",  9, 14, this));
    buttonMap.insert(68, new PeriodicTableButton("ER",  9, 15, this));
    buttonMap.insert(69, new PeriodicTableButton("TM",  9, 16, this));
    buttonMap.insert(70, new PeriodicTableButton("YB",  9, 17, this));
    buttonMap.insert(71, new PeriodicTableButton("LU",  6,  4, this));
    buttonMap.insert(72, new PeriodicTableButton("HF",  6,  5, this));
    buttonMap.insert(73, new PeriodicTableButton("TA",  6,  6, this));
    buttonMap.insert(74, new PeriodicTableButton("W",   6,  7, this));
    buttonMap.insert(75, new PeriodicTableButton("RE",  6,  8, this));
    buttonMap.insert(76, new PeriodicTableButton("OS",  6,  9, this));
    buttonMap.insert(77, new PeriodicTableButton("IR",  6, 10, this));
    buttonMap.insert(78, new PeriodicTableButton("PT",  6, 11, this));
    buttonMap.insert(79, new PeriodicTableButton("AU",  6, 12, this));
    buttonMap.insert(80, new PeriodicTableButton("HG",  6, 13, this));
    buttonMap.insert(81, new PeriodicTableButton("TL",  6, 14, this));
    buttonMap.insert(82, new PeriodicTableButton("PB",  6, 15, this));
    buttonMap.insert(83, new PeriodicTableButton("BI",  6, 16, this));
    buttonMap.insert(84, new PeriodicTableButton("PO",  6, 17, this));
    buttonMap.insert(85, new PeriodicTableButton("AT",  6, 18, this));
    buttonMap.insert(86, new PeriodicTableButton("RN",  6, 19, this));
    buttonMap.insert(87, new PeriodicTableButton("FR",  7,  1, this));
    buttonMap.insert(88, new PeriodicTableButton("RA",  7,  2, this));
    buttonMap.insert(89, new PeriodicTableButton("AC", 10,  4, this));
    buttonMap.insert(90, new PeriodicTableButton("TH", 10,  5, this));
    buttonMap.insert(91, new PeriodicTableButton("PA", 10,  6, this));
    buttonMap.insert(92, new PeriodicTableButton("U",  10,  7, this));
    buttonMap.insert(93, new PeriodicTableButton("NP", 10,  8, this));
    buttonMap.insert(94, new PeriodicTableButton("PU", 10,  9, this));
    buttonMap.insert(95, new PeriodicTableButton("AM", 10, 10, this));
    buttonMap.insert(96, new PeriodicTableButton("CM", 10, 11, this));
    buttonMap.insert(97, new PeriodicTableButton("BK", 10, 12, this));
    buttonMap.insert(98, new PeriodicTableButton("CF", 10, 13, this));

    toolTipMap.insert(1, QString("1: Hydrogen (1.008)"));
    toolTipMap.insert(2, QString("2: Helium (4.002)"));
    toolTipMap.insert(3, QString("3: Lithium (6.941)"));
    toolTipMap.insert(4, QString("4: Beryllium (9.012)"));
    toolTipMap.insert(5, QString("5: Boron (10.811)"));
    toolTipMap.insert(6, QString("6: Carbon (12.011)"));
    toolTipMap.insert(7, QString("7: Nitrogen (14.007)"));
    toolTipMap.insert(8, QString("8: Oxygen (15.999)"));
    toolTipMap.insert(9, QString("9: Fluorine (18.998)"));
    toolTipMap.insert(10, QString("10: Neon (20.18)"));
    toolTipMap.insert(11, QString("11: Sodium (22.99)"));
    toolTipMap.insert(12, QString("12: Magnesium (24.305)"));
    toolTipMap.insert(13, QString("13: Aluminum (26.982)"));
    toolTipMap.insert(14, QString("14: Silicon (28.086)"));
    toolTipMap.insert(15, QString("15: Phosphorus (30.974)"));
    toolTipMap.insert(16, QString("16: Sulfur (32.065)"));
    toolTipMap.insert(17, QString("17: Chlorine (35.453)"));
    toolTipMap.insert(18, QString("18: Argon (39.948)"));
    toolTipMap.insert(19, QString("19: Potassium (39.098)"));
    toolTipMap.insert(20, QString("20: Calcium (40.078)"));
    toolTipMap.insert(21, QString("21: Scandium (44.956)"));
    toolTipMap.insert(22, QString("22: Titanium (47.867)"));
    toolTipMap.insert(23, QString("23: Vanadium (50.942)"));
    toolTipMap.insert(24, QString("24: Chromium (51.996)"));
    toolTipMap.insert(25, QString("25: Manganese (54.938)"));
    toolTipMap.insert(26, QString("26: Iron (55.845)"));
    toolTipMap.insert(27, QString("27: Cobalt (58.933)"));
    toolTipMap.insert(28, QString("28: Nickel (58.693)"));
    toolTipMap.insert(29, QString("29: Copper (63.546)"));
    toolTipMap.insert(30, QString("30: Zinc (65.38)"));
    toolTipMap.insert(31, QString("31: Gallium (69.723)"));
    toolTipMap.insert(32, QString("32: Germanium (72.64)"));
    toolTipMap.insert(33, QString("33: Arsenic (74.922)"));
    toolTipMap.insert(34, QString("34: Selenium (78.96)"));
    toolTipMap.insert(35, QString("35: Bromine (79.904)"));
    toolTipMap.insert(36, QString("36: Krypton (83.798)"));
    toolTipMap.insert(37, QString("37: Rubidium (85.468)"));
    toolTipMap.insert(38, QString("38: Strontium (87.62)"));
    toolTipMap.insert(39, QString("39: Yttrium (88.906)"));
    toolTipMap.insert(40, QString("40: Zirconium (91.224)"));
    toolTipMap.insert(41, QString("41: Niobium (92.906)"));
    toolTipMap.insert(42, QString("42: Molybdenum (95.96)"));
    toolTipMap.insert(43, QString("43: Technetium (98)"));
    toolTipMap.insert(44, QString("44: Ruthenium (101.07)"));
    toolTipMap.insert(45, QString("45: Rhodium (102.906)"));
    toolTipMap.insert(46, QString("46: Palladium (106.42)"));
    toolTipMap.insert(47, QString("47: Silver (107.868)"));
    toolTipMap.insert(48, QString("48: Cadmium (112.411)"));
    toolTipMap.insert(49, QString("49: Indium (114.818)"));
    toolTipMap.insert(50, QString("50: Tin (118.71)"));
    toolTipMap.insert(51, QString("51: Antimony (121.76)"));
    toolTipMap.insert(52, QString("52: Tellurium (127.6)"));
    toolTipMap.insert(53, QString("53: Iodine (126.904)"));
    toolTipMap.insert(54, QString("54: Xenon (131.293)"));
    toolTipMap.insert(55, QString("55: Cesium (132.905)"));
    toolTipMap.insert(56, QString("56: Barium (137.327)"));
    toolTipMap.insert(57, QString("57: Lanthanum (138.905)"));
    toolTipMap.insert(58, QString("58: Cerium (140.116)"));
    toolTipMap.insert(59, QString("59: Praseodymium (140.908)"));
    toolTipMap.insert(60, QString("60: Neodymium (144.242)"));
    toolTipMap.insert(61, QString("61: Promethium (145)"));
    toolTipMap.insert(62, QString("62: Samarium (150.36)"));
    toolTipMap.insert(63, QString("63: Europium (151.964)"));
    toolTipMap.insert(64, QString("64: Gadolinium (157.25)"));
    toolTipMap.insert(65, QString("65: Terbium (158.925)"));
    toolTipMap.insert(66, QString("66: Dysprosium (162.5)"));
    toolTipMap.insert(67, QString("67: Holmium (164.93)"));
    toolTipMap.insert(68, QString("68: Erbium (167.259)"));
    toolTipMap.insert(69, QString("69: Thulium (168.934)"));
    toolTipMap.insert(70, QString("70: Ytterbium (173.054)"));
    toolTipMap.insert(71, QString("71: Lutetium (174.967)"));
    toolTipMap.insert(72, QString("72: Hafnium (178.49)"));
    toolTipMap.insert(73, QString("73: Tantalum (180.948)"));
    toolTipMap.insert(74, QString("74: Wolfram (183.84)"));
    toolTipMap.insert(75, QString("75: Rhenium (186.207)"));
    toolTipMap.insert(76, QString("76: Osmium (190.23)"));
    toolTipMap.insert(77, QString("77: Iridium (192.217)"));
    toolTipMap.insert(78, QString("78: Platinum (195.084)"));
    toolTipMap.insert(79, QString("79: Gold (196.967)"));
    toolTipMap.insert(80, QString("80: Mercury (200.59)"));
    toolTipMap.insert(81, QString("81: Thallium (204.383)"));
    toolTipMap.insert(82, QString("82: Lead (207.2)"));
    toolTipMap.insert(83, QString("83: Bismuth (208.98)"));
    toolTipMap.insert(84, QString("84: Polonium (210)"));
    toolTipMap.insert(85, QString("85: Astatine (210)"));
    toolTipMap.insert(86, QString("86: Radon (222)"));
    toolTipMap.insert(87, QString("87: Francium (223)"));
    toolTipMap.insert(88, QString("88: Radium (226)"));
    toolTipMap.insert(89, QString("89: Actinium (227)"));
    toolTipMap.insert(90, QString("90: Thorium (232.038)"));
    toolTipMap.insert(91, QString("91: Protactinium (231.036)"));
    toolTipMap.insert(92, QString("92: Uranium (238.029)"));
    toolTipMap.insert(93, QString("93: Neptunium (237)"));
    toolTipMap.insert(94, QString("94: Plutonium (244)"));
    toolTipMap.insert(95, QString("95: Americium (243)"));
    toolTipMap.insert(96, QString("96: Curium (247)"));
    toolTipMap.insert(97, QString("97: Berkelium (247)"));
    toolTipMap.insert(98, QString("98: Californium (251)"));

    QMapIterator<int, PeriodicTableButton*> ttIt(buttonMap);
    while (ttIt.hasNext()) {
        ttIt.next();
        ttIt.value()->setToolTip(toolTipMap.value(ttIt.key()));
    }

    QGridLayout *layout = new QGridLayout;

    for (int c = 1; c < 20; ++c) {
        if (c == 3) continue;
        PeriodicTableToggleGroupButton *p = new PeriodicTableToggleGroupButton("c", 0, c, this);
        connect(p, SIGNAL(groupButtonClicked(QString,int,int)), this, SLOT(toggleGroup(QString,int,int)));
        layout->addWidget(p, 0, c);
    }

    for (int r = 1; r < 11; ++r) {
        if (r == 8) continue;
        PeriodicTableToggleGroupButton *p = new PeriodicTableToggleGroupButton("r", r, 0, this);
        connect(p, SIGNAL(groupButtonClicked(QString,int,int)), this, SLOT(toggleGroup(QString,int,int)));
        layout->addWidget(p, r, 0);
    }

    QMapIterator<int, PeriodicTableButton*> it(buttonMap);

    while (it.hasNext()) {
        it.next();
        layout->addWidget(it.value(), it.value()->row(), it.value()->col());
    }

    QLabel *lblLant1 = new QLabel("*");
    QLabel *lblAct1  = new QLabel("**");
    QLabel *lblLant2 = new QLabel("*");
    QLabel *lblAct2  = new QLabel("**");

    lblLant1->setAlignment(Qt::AlignHCenter);
    lblAct1->setAlignment(Qt::AlignHCenter);
    lblLant2->setAlignment(Qt::AlignHCenter);
    lblAct2->setAlignment(Qt::AlignHCenter);

    layout->addWidget(lblLant1,  6,  3);
    layout->addWidget(lblAct1, 7,  3);
    layout->addWidget(lblLant2,  9,  3);
    layout->addWidget(lblAct2, 10, 3);

    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    this->setLayout(layout);
}

PeriodicTableWidget::~PeriodicTableWidget()
{
}

QStringList PeriodicTableWidget::getAll()
{
    return getButtons(1);
}

QStringList PeriodicTableWidget::getOne()
{
    return getButtons(2);
}

QStringList PeriodicTableWidget::getNone()
{
    return getButtons(3);
}

QStringList PeriodicTableWidget::getButtons(int st)
{
    QStringList out;

    QMapIterator<int, PeriodicTableButton*> it(buttonMap);

    while (it.hasNext()) {
        it.next();
        if (it.value()->status() == st) out.append(it.value()->element());
    }

    return out;
}

void PeriodicTableWidget::toggleGroup(QString dir, int num, int status)
{
    QMapIterator<int, PeriodicTableButton*> it(buttonMap);

    while (it.hasNext()) {
        it.next();
        if      ((dir == "r") && (it.value()->row() == num)) it.value()->setStatus(status);
        else if ((dir == "c") && (it.value()->col() == num)) it.value()->setStatus(status);
    }
}

void PeriodicTableWidget::allOptional()
{
    setAllStatus(0);
}

void PeriodicTableWidget::allMandatory()
{
    setAllStatus(1);
}

void PeriodicTableWidget::allOne()
{
    setAllStatus(2);
}

void PeriodicTableWidget::allDisabled()
{
    setAllStatus(3);
}

void PeriodicTableWidget::setAllStatus(int s)
{
    QMapIterator<int, PeriodicTableButton*> it(buttonMap);

    while (it.hasNext()) {
        it.next();
        it.value()->setStatus(s);
    }
}

void PeriodicTableWidget::disableOptionals()
{
    QMapIterator<int, PeriodicTableButton*> it(buttonMap);

    while (it.hasNext()) {
        it.next();
        if (it.value()->status() == 0) it.value()->setStatus(3);
    }
}
