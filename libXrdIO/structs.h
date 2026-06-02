/***************************************************************************
                          structs.h  -  description
                             -------------------
    begin                : Mon Jun 30 14:16:07 CEST 2014
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

#ifndef STRUCTS_H
#define STRUCTS_H

#include <QList>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <QString>
#include <QStringView>
#include <QFileInfo>
#include <QChar>
#include <QPointF>
#include <QColor>
#include <QPen>
#include <QRegularExpression>

namespace global {

/** Types of files supported by the IO system. */
enum FileType {
    DEVICE_ASCII,
    DEVICE_BINARY,
    STRUCTURE,
    TEMPLATE,
    TEXT,
    BINARY
};

/** Units for peak position values. */
enum PositionUnit {
    TWOTHETA,   /**< Degrees 2-theta. */
    DNM,        /**< d-spacing in nanometers. */
    DANGSTROM   /**< d-spacing in Angstroms. */
};

/** Item types for report tree structure (parent, child, or page break). */
enum ReportItemType {
    PARENT,
    CHILD,
    PAGEBREAK
};

/** Current state of a refinement process. */
enum RefinementStatus {
    IDLE,           /**< No refinement running. */
    RUNNING,        /**< Refinement is in progress. */
    MATCHING,       /**< Pattern matching is in progress. */
    COMPLETED,      /**< Refinement completed successfully. */
    SCHEDULED,      /**< Refinement is scheduled to run. */
    ABORTED,        /**< Refinement was aborted by user. */
    FAILURE,        /**< Refinement failed. */
    CRASH,          /**< Refinement process crashed. */
    FITSCHEDULED,   /**< Fit scheduled to run. */
    FITRUNNING      /**< Fit is in progress. */
};

/** Controls which part of the UI is updated when results change. */
enum ViewUpdateMode{
    RESULTS,    /**< Update only the results display. */
    DISPLAY     /**< Update the graphical display. */
};

/** Describes a highlighted region on a plot, with optional center line and label. */
struct HighlightRegion {
    QString name;           /**< Display name for the region. */
    QPointF start;          /**< Start point of the highlighted area. */
    QPointF end;            /**< End point of the highlighted area. */
    bool centerLine;        /**< Whether to draw a center line through the region. */
    QColor fillColor;       /**< Fill color for the highlighted area. */
    QPen centerLinePen;     /**< Pen used for the center line. */
    QRectF boundingRect;    /**< Bounding rectangle of the region. */
    QRectF labelBoundingRect; /**< Bounding rectangle for the label. */

    HighlightRegion(QString n,
                    QPointF s,
                    QPointF e,
                    bool b,
                    QColor cf,
                    QPen cl)
        : name(n),
        start(s),
        end(e),
        centerLine(b),
        fillColor(cf),
        centerLinePen(cl) {}
};

/** Metadata for a BGMN preset file including its path and type. */
struct BgmnPresetFile {
    QFileInfo fileInfo;       /**< File information for the preset. */
    QString relativePath;     /**< Relative path to the preset file. */
    QString md5Hash;          /**< MD5 hash for integrity checking. */
    FileType type;            /**< Type of the preset file. */
};

/** Defines a range over which peak search or background is evaluated. */
struct PeakRange {
    QString name;         /**< Name or label of the range. */
    double start;         /**< Start position of the range. */
    double end;           /**< End position of the range. */
    int background;       /**< Background order or identifier for this range. */
};

/** Represents a variable used in curve fitting with its constraints. */
struct CurveFitVariable {
    QString name;         /**< Name of the variable. */
    QString value;        /**< Current value of the variable. */
    QString lowerLimit;   /**< Lower bound for the variable. */
    QString upperLimit;   /**< Upper bound for the variable. */
    QString checkState;   /**< Check state (e.g., "unchecked", "fixed", "refined"). */
};

/** Parameters for the SNIP background estimation algorithm. */
struct BaseLineSnip {
    int m;      /**< Window width parameter for the SNIP algorithm. */
    int mode;   /**< Clipping mode (e.g., linear or quadratic). */

    BaseLineSnip() : m(), mode() {}
    BaseLineSnip(int _m, int _mode) : m(_m), mode(_mode) {}
};

/** Wyckoff position information for a crystallographic site. */
struct WyckoffPosition {
    int itNum;                     /**< International Tables number for the space group. */
    int setting;                   /**< Setting number of the space group. */
    QString name;                  /**< Wyckoff letter designation. */
    QVector<QStringList> operators; /**< Symmetry operators for this position. */
};

/** A single refinement result with name, value, estimated standard deviation and error text. */
struct Result{
    QString name;         /**< Name or label of the result. */
    double value;         /**< Numeric value of the result. */
    double esd;           /**< Estimated standard deviation. */
    QString error;        /**< Error message if the result could not be computed. */
    int precision;         /**< Number of decimal places for display. */

    Result() : name(), value(), esd(), error(), precision() {}
    Result(QString n, double v, double e) : name(n), value(v), esd(e), error(QString()), precision(-1) {}
    Result(QString n, double v, double e, QString r) : name(n), value(v), esd(e), error(r), precision(-1) {}
    Result(QString n, double v, double e, QString r, int p) : name(n), value(v), esd(e), error(r), precision(p) {}
};

/** An oxide component with name, weight fraction and its estimated standard deviation. */
struct Oxide{
    QString name;     /**< Chemical formula of the oxide (e.g., "SiO2"). */
    double weight;    /**< Weight fraction of the oxide. */
    double esd;       /**< Estimated standard deviation of the weight. */

    Oxide() : name(), weight(), esd() {}
    Oxide(QString _o, double _w, double _e) : name(_o), weight(_w), esd(_e) {}
};

/** Describes an Excel import/export object linking a spreadsheet cell to a phase parameter. */
struct AxObjectExcel{
    QString file;        /**< Path to the Excel file. */
    int worksheet;       /**< Worksheet index (0-based). */
    int row;             /**< Row number in the worksheet. */
    int col;             /**< Column number in the worksheet. */
    QString phase;       /**< Name of the associated phase. */
    QString parameter;   /**< Name of the associated parameter. */
    QString filter;      /**< Filter string for selecting data. */
    QString output;      /**< Output caption for the imported value. */

    AxObjectExcel() : file(), worksheet(), row(), col(), phase(), parameter(), filter(), output() {}
    AxObjectExcel(QString _f, int _w, int _r, int _c, QString _ph, QString _pa)
        : file(_f), worksheet(_w), row(_r), col(_c), phase(_ph), parameter(_pa) {}
    AxObjectExcel(QString _f, int _w, int _r, int _c, QString _ph, QString _pa, QString _fi, QString _cap)
        : file(_f), worksheet(_w), row(_r), col(_c), phase(_ph), parameter(_pa), filter(_fi), output(_cap) {}
};

/** Characteristic X-ray wavelengths for an element (K-alpha1, K-alpha2, K-beta). */
struct CharWaveLength{
    QString element;    /**< Element symbol. */
    double ka1;         /**< K-alpha1 wavelength in Angstroms. */
    double ka2;         /**< K-alpha2 wavelength in Angstroms. */
    double kb;          /**< K-beta wavelength in Angstroms. */

    CharWaveLength(QString _e, double _ka1, double _ka2, double _kb) : element(_e), ka1(_ka1), ka2(_ka2), kb(_kb) {}
};

/** Nine-parameter atomic scattering factor function (f = sum(a_i * exp(-b_i * s^2)) + c). */
struct ScatteringFactorFunction{
    QString element;    /**< Element or ion symbol. */
    double a1;          /**< First coefficient a1. */
    double b1;          /**< First exponent coefficient b1. */
    double a2;          /**< Second coefficient a2. */
    double b2;          /**< Second exponent coefficient b2. */
    double a3;          /**< Third coefficient a3. */
    double b3;          /**< Third exponent coefficient b3. */
    double a4;          /**< Fourth coefficient a4. */
    double b4;          /**< Fourth exponent coefficient b4. */
    double c;           /**< Constant term c. */

    ScatteringFactorFunction() : element(), a1(), b1(), a2(), b2(), a3(), b3(), a4(), b4(), c() {}
    ScatteringFactorFunction(QString _e, double _a1, double _b1, double _a2, double _b2, double _a3, double _b3, double _a4, double _b4, double _c)
        : element(_e), a1(_a1), b1(_b1), a2(_a2), b2(_b2), a3(_a3), b3(_b3), a4(_a4), b4(_b4), c(_c) {}
};

/** Multiplier used for converting between internal and display coordinate representations. */
static const double dMult(10000000.0);

/**
 * Periodic table atom data string containing a sequential list of:
 * "Atom number;Atom Name;Molecular Weight;Standard Oxide Name;Molecular Weight Standard Oxide;"
 * Split into a string list at ";" and process in chunks of 5 elements to access the data.
 */
const QString atoms = QStringLiteral(
        "1;H;1.0079;H2O;18.0152;"
        "2;He;4.0026;He;4.0026;"
        "3;Li;6.941;Li2O;29.8814;"
        "4;Be;9.0122;BeO;25.0116;"
        "5;B;10.811;B2O3;69.6202;"
        "6;C;12.0107;CO2;44.0095;"
        "7;N;14.0067;N;14.0067;"
        "8;O;15.9994;O;15.9994;"
        "9;F;18.9984;F;18.9984;"
        "10;Ne;20.1797;Ne;20.1797;"
        "11;Na;22.9897;Na2O;61.9788;"
        "12;Mg;24.305;MgO;40.3044;"
        "13;Al;26.9815;Al2O3;101.9612;"
        "14;Si;28.0855;SiO2;60.0843;"
        "15;P;30.9738;P2O5;141.9446;"
        "16;S;32.065;SO3;80.0632;"
        "17;Cl;35.453;Cl;35.453;"
        "18;K;39.0983;K2O;94.196;"
        "19;Ar;39.948;Ar;39.948;"
        "20;Ca;40.078;CaO;56.0774;"
        "21;Sc;44.9559;Sc2O3;137.91;"
        "22;Ti;47.867;TiO2;79.8658;"
        "23;V;50.9415;V2O5;181.88;"
        "24;Cr;51.9961;Cr2O3;151.9904;"
        "25;Mn;54.938;MnO;70.9374;"
        "26;Fe;55.845;Fe2O3;159.6882;"
        "27;Ni;58.6934;NiO;74.6928;"
        "28;Co;58.9332;CoO;74.9326;"
        "29;Cu;63.546;CuO;79.5454;"
        "30;Zn;65.39;ZnO;81.3894;"
        "31;Ga;69.723;Ga2O3;187.4442;"
        "32;Ge;72.64;GeO2;104.6388;"
        "33;As;74.9216;As2O3;197.8414;"
        "34;Se;78.96;SeO2;110.9588;"
        "35;Br;79.904;Br;79.904;"
        "36;Kr;83.8;Kr;83.8;"
        "37;Rb;85.4678;Rb2O;186.935;"
        "38;Sr;87.62;SrO;103.6194;"
        "39;Y;88.9059;Y2O3;225.81;"
        "40;Zr;91.224;ZrO2;123.2228;"
        "41;Nb;92.9064;Nb2O5;265.8098;"
        "42;Mo;95.94;MoO3;143.9382;"
        "43;Tc;98.0;Tc;98.0;"
        "44;Ru;101.07;RuO2;133.0688;"
        "45;Rh;102.9055;Rh2O3;253.8092;"
        "46;Pd;106.42;PdO;122.4194;"
        "47;Ag;107.8682;Ag2O;231.7358;"
        "48;Cd;112.411;CdO;128.4104;"
        "49;In;114.818;In2O3;277.6342;"
        "50;Sn;118.71;SnO2;150.7088;"
        "51;Sb;121.76;Sb2O3;291.5182;"
        "52;I;126.9045;I;126.9045;"
        "53;Te;127.6;TeO2;159.5988;"
        "54;Xe;131.293;Xe;131.293;"
        "55;Cs;132.9055;Cs2O;281.8104;"
        "56;Ba;137.327;BaO;153.3264;"
        "57;La;138.9055;La2O3;325.8092;"
        "58;Ce;140.116;CeO2;172.1148;"
        "59;Pr;140.9077;Pr6O11;1021.4396;"
        "60;Nd;144.24;Nd2O3;336.4782;"
        "61;Pm;145.0;Pm;145.0;"
        "62;Sm;150.36;Sm2O3;348.7182;"
        "63;Eu;151.964;Eu2O3;351.9262;"
        "64;Gd;157.25;Gd2O3;362.4982;"
        "65;Tb;158.9253;Tb2O3;365.8488;"
        "66;Dy;162.5;Dy2O3;372.9982;"
        "67;Ho;164.9303;Ho2O3;377.8588;"
        "68;Er;167.259;Er2O3;382.5162;"
        "69;Tm;168.9342;Tm2O3;385.8666;"
        "70;Yb;173.04;Yb2O3;394.0782;"
        "71;Lu;174.967;Lu2O3;397.9322;"
        "72;Hf;178.49;HfO2;210.4888;"
        "73;Ta;180.9479;Ta2O5;441.8928;"
        "74;W;183.84;WO3;231.8382;"
        "75;Re;186.207;Re2O3;420.4122;"
        "76;Os;190.23;OsO4;254.2276;"
        "77;Ir;192.217;IrO2;224.2158;"
        "78;Pt;195.078;PtO2;227.0768;"
        "79;Au;196.9665;Au2O;409.9324;"
        "80;Hg;200.59;HgO;216.5894;"
        "81;Tl;204.3833;Tl2O3;456.7648;"
        "82;Pb;207.2;PbO;223.1994;"
        "83;Bi;208.9804;Bi2O3;465.959;"
        "84;Po;209;Po;209;"
        "85;At;210;At;210;"
        "86;Rn;222;Rn;222;"
        "87;Fr;223;Fr;223;"
        "88;Ra;226;Ra;226;"
        "89;Ac;227;Ac;227;"
        "90;Pa;231.0359;Pa;231.0359;"
        "91;Th;232.0381;ThO2;264.0369;"
        "92;Np;237;Np;237;"
        "93;U;238.0289;U3O8;842.0819;"
        "94;Am;243;Am;243;"
        "95;Pu;244;Pu;244;"
        "96;Cm;247;Cm;247;"
        "97;Bk;247;Bk;247;"
        "98;Cf;251;Cf;251;"
        "99;Es;252;Es;252;"
        "100;Fm;257;Fm;257;"
        "101;Md;258;Md;258;"
        "102;No;259;No;259;"
        "103;Lr;262.11;Lr;262.11;"
        "104;Rf;267.12;Rf;267.12;"
        "105;Db;270.13;Db;270.13;"
        "106;Sg;269.13;Sg;269.13;"
        "107;Bh;270.13;Bh;270.13;"
        "108;Hs;269.13;Hs;269.13;"
        "109;Mt;278.16;Mt;278.16;"
        "110;Ds;281.17;Ds;281.17;"
        "111;Rg;281.17;Rg;281.17;"
        "112;Cn;285.18;Cn;285.18;"
        "113;Nh;286.18;Nh;286.18;"
        "114;Fl;289.19;Fl;289.19;"
        "115;Mc;289.20;Mc;289.20;"
        "116;Lv;293.20;Lv;293.20;"
        "117;Ts;293.21;Ts;293.21;"
        "118;Og;294.21;Og;294.21"
);

/** Regular expression matching line endings (CR, LF, or CR+LF). */
const QRegularExpression rxLineEnding("\\r\\n?|\\n");
/** String pattern for line ending matching. */
const QString rxLineEndingPattern("\\r\\n?|\\n");
/** Regular expression for matching floating-point numbers. */
const QRegularExpression rxDouble("[+-]?\\d*\\.?\\d+(?:[eE][+-]?\\d+)?");
/** String pattern for floating-point number matching. */
const QString rxDoublePattern("[+-]?\\d*\\.?\\d+(?:[eE][+-]?\\d+)?");

/**
 * All element symbols supported by BGMN capitalized in a regexp pattern.
 * Dual letter symbols must appear at the beginning of the list, else "C" would also match "CA".
 */
const QString rxElements = QStringLiteral("HE|LI|BE|NE|NA|MG|AL|SI|CL|AR|CA|SC|TI|CR|MN|FE|"
                         "NI|CO|CU|ZN|GA|GE|AS|SE|BR|KR|RB|SR|ZR|NB|MO|TC|RU|RH|PD|AG|CD|IN|"
                         "SN|SB|TE|XE|CS|BA|LA|CE|PR|ND|PM|SM|EU|GD|TB|DY|HO|ER|TM|YB|LU|HF|"
                         "TA|RE|OS|IR|PT|AU|HG|TL|PB|BI|PO|AT|RN|FR|RA|AC|PA|TH|NP|AM|PU|CM|"
                         "BK|CF|ES|FM|MD|NO|LR|RF|DB|SG|BH|HS|MT|DS|RG|CN|NH|FL|MC|LV|TS|OG|"
                         "H|B|C|N|O|F|P|S|K|V|Y|I|W|U"
);

/** Default global goals pattern for BGMN refinement (sum, amorphous, Q, and Qabs goals). */
const QString defaultBgmnGlobalGoals = QStringLiteral("^[^\\/]+\\/sum$\n^Amorph$\n^Q\\S+$\n^Qabs\\S+$");
/** Default local goals pattern for BGMN refinement (phase-specific parameters). */
const QString defaultBgmnLocalGoals = QStringLiteral("^A$\n^B$\n^C$\n^ALPHA$\n^BETA$\n^GAMMA$\n^UNIT$\n^GrainSize\\(-?\\d+,-?\\d+,-?\\d+\\)$");

const QStringList BgmnScatteringFactorSymbols = {
    {"H"}, {"HE"}, {"LI"}, {"LI+1"}, {"BE"}, {"BE+2"}, {"B"}, {"C"}, {"N"},
    {"O"}, {"O-1"}, {"O-2"}, {"F"}, {"F-1"}, {"NE"}, {"NA"}, {"NA+1"},
    {"MG"}, {"MG+2"}, {"AL"}, {"AL+3"}, {"SI"}, {"SI+4"}, {"P"}, {"S"},
    {"CL"}, {"CL-1"}, {"AR"}, {"K"}, {"K+1"}, {"CA"}, {"CA+2"}, {"SC"},
    {"SC+3"}, {"TI"}, {"TI+3"}, {"TI+4"}, {"V"}, {"V+2"}, {"V+3"},
    {"V+5"}, {"CR"}, {"CR+2"}, {"CR+3"}, {"MN"}, {"MN+2"}, {"MN+3"},
    {"MN+4"}, {"FE"}, {"FE+2"}, {"FE+3"}, {"CO"}, {"CO+2"}, {"CO+3"},
    {"NI"}, {"NI+2"}, {"NI+3"}, {"CU"}, {"CU+1"}, {"CU+2"}, {"ZN"},
    {"ZN+2"}, {"GA"}, {"GA+3"}, {"GE"}, {"AS"}, {"SE"}, {"BR"}, {"BR-1"},
    {"KR"}, {"RB"}, {"RB+1"}, {"SR"}, {"SR+2"}, {"Y"}, {"Y+3"}, {"ZR"},
    {"ZR+4"}, {"NB"}, {"NB+3"}, {"NB+5"}, {"MO"}, {"MO+3"}, {"MO+5"},
    {"MO+6"}, {"TC"}, {"RU"}, {"RU+3"}, {"RU+4"}, {"RH"}, {"RH+3"},
    {"RH+4"}, {"PD"}, {"PD+2"}, {"PD+4"}, {"AG"}, {"AG+1"}, {"AG+2"},
    {"CD"}, {"CD+2"}, {"IN"}, {"IN+3"}, {"SN"}, {"SN+2"}, {"SN+4"},
    {"SB"}, {"SB+3"}, {"SB+5"}, {"TE"}, {"I"}, {"I-1"}, {"XE"}, {"CS"},
    {"CS+1"}, {"BA"}, {"BA+2"}, {"LA"}, {"LA+3"}, {"CE"}, {"CE+3"},
    {"CE+4"}, {"PR"}, {"PR+3"}, {"PR+4"}, {"ND"}, {"ND+3"}, {"PM"},
    {"PM+3"}, {"SM"}, {"SM+3"}, {"EU"}, {"EU+2"}, {"EU+3"}, {"GD"},
    {"GD+3"}, {"TB"}, {"TB+3"}, {"DY"}, {"DY+3"}, {"HO"}, {"HO+3"},
    {"ER"}, {"ER+3"}, {"TM"}, {"TM+3"}, {"YB"}, {"YB+2"}, {"YB+3"},
    {"LU"}, {"LU+3"}, {"HF"}, {"TA"}, {"W"}, {"RE"}, {"OS"}, {"IR"},
    {"PT"}, {"AU"}, {"HG"}, {"TL"}, {"PB"}, {"BI"}, {"PO"}, {"AT"},
    {"RN"}, {"FR"}, {"RA"}, {"AC"}, {"TH"}, {"PA"}, {"U"}, {"NP"}, {"PU"},
    {"AM"}, {"CM"}, {"BK"}, {"CF"}, {"ES"}, {"FM"}, {"MD"}, {"NO"}, {"LW"}
};

/*
 * atomic form factor function parameters imported from BGMN's AFAPARAM.DAT file
 */
const QHash<QString, ScatteringFactorFunction> ScatteringFactors = {
    {"e-1", ScatteringFactorFunction("e-1", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)},
    {"H", ScatteringFactorFunction("H", 0.48992, 20.6593, 0.26200, 7.74039, 0.19677, 49.5519, 0.04988, 2.20159, 0.00130)},
    {"HE", ScatteringFactorFunction("HE", 0.76844, 10.9071, 0.72694, 4.30779, 0.27631, 1.33127, 0.21572, 25.6848, 0.01249)},
    {"LI", ScatteringFactorFunction("LI", 0.99279, 4.33979, 0.87402, 1.26006, 0.84240, 98.7088, 0.23101, 212.088, 0.05988)},
    {"LI+1", ScatteringFactorFunction("LI+1", 6.08475, 0.00498, 0.86773, 1.53730, 0.80588, 4.28524, 0.17720, 9.81413, -5.93560)},
    {"BE", ScatteringFactorFunction("BE", 2.22744, 0.04965, 1.55249, 42.9165, 1.40060, 1.66379, 0.58290, 100.361, -1.76339)},
    {"BE+2", ScatteringFactorFunction("BE+2", 5.69034, -0.01336, 1.19706, 0.39000, 1.03057, 1.97441, 0.20150, 4.90642, -6.11950)},
    {"B", ScatteringFactorFunction("B", 2.03876, 23.0888, 1.41491, 0.97848, 1.11609, 59.8985, 0.73273, 0.08538, -0.30409)},
    {"C", ScatteringFactorFunction("C", 1.93019, 12.7188, 1.87812, 28.6498, 1.57415, 0.59645, 0.37108, 65.0337, 0.24637)},
    {"N", ScatteringFactorFunction("N", 12.7913, 0.02064, 3.28546, 10.7018, 1.76483, 30.7773, 0.54709, 1.48044, -11.3926)},
    {"O", ScatteringFactorFunction("O", 2.95648, 13.8964, 2.45240, 5.91765, 1.50510, 0.34537, 0.78135, 34.0811, 0.30413)},
    {"O-1", ScatteringFactorFunction("O-1", 3.22563, 18.4991, 3.01717, 6.65680, 1.42553, 0.40589, 0.90525, 61.1889, 0.42362)},
    {"O-2", ScatteringFactorFunction("O-2", 3.7504, 16.5131, 2.9429, 6.5920, 1.5430, 0.3192, 1.6209, 43.3486, 0.2421)},
    {"F", ScatteringFactorFunction("F", 3.30393, 11.2651, 3.01753, 4.66504, 1.35754, 0.33760, 0.83645, 27.9898, 0.48398)},
    {"F-1", ScatteringFactorFunction("F-1", 3.63220, 5.27756, 3.51057, 14.7353, 1.26064, 0.44226, 0.94071, 47.3437, 0.65340)},
    {"NE", ScatteringFactorFunction("NE", 3.71272, 3.91091, 3.52631, 9.63126, 1.19237, 0.40483, 0.83080, 23.9546, 0.73728)},
    {"NA", ScatteringFactorFunction("NA", 5.26400, 4.02579, 2.17549, 10.4796, 1.36690, 0.84222, 1.08859, 133.617, 1.09912)},
    {"NA+1", ScatteringFactorFunction("NA+1", 3.99479, 3.11047, 3.37245, 7.14318, 1.13877, 0.40692, 0.65118, 15.7319, 0.84267)},
    {"MG", ScatteringFactorFunction("MG", 5.59229, 4.41142, 2.68206, 1.36549, 1.72235, 93.4885, 0.73055, 32.5281, 1.26883)},
    {"MG+2", ScatteringFactorFunction("MG+2", 4.30491, 2.55961, 3.14719, 5.60660, 1.12859, 0.41574, 0.49034, 11.4840, 0.92893)},
    {"AL", ScatteringFactorFunction("AL", 5.35047, 3.48665, 2.92451, 1.20535, 2.27309, 42.6051, 1.16531, 107.170, 1.28489)},
    {"AL+3", ScatteringFactorFunction("AL+3", 4.17448, 1.93816, 3.38760, 4.14553, 1.20296, 0.22875, 0.52814, 8.28524, 0.70679)},
    {"SI", ScatteringFactorFunction("SI", 5.79411, 2.57104, 3.22390, 34.1775, 2.42795, 0.86937, 1.32149, 85.3410, 1.23139)},
    {"SI+4", ScatteringFactorFunction("SI+4", 4.43918, 1.64167, 3.20345, 3.43757, 1.19453, 0.21490, 0.41653, 6.65365, 0.74630)},
    {"P", ScatteringFactorFunction("P", 6.92073, 1.83778, 4.14396, 27.0198, 2.01697, 0.21318, 1.53860, 67.1086, 0.37870)},
    {"S", ScatteringFactorFunction("S", 7.18742, 1.43280, 5.88671, 0.02865, 5.15858, 22.1101, 1.64403, 55.4651, -3.87732)},
    {"CL", ScatteringFactorFunction("CL", 9.83957, -0.00053, 7.53181, 1.11119, 6.07100, 18.0846, 1.87128, 45.3666, -8.31430)},
    {"CL-1", ScatteringFactorFunction("CL-1", 18.0842, 0.00129, 7.47202, 1.12976, 6.46337, 19.3079, 2.43918, 59.0633, -16.4654)},
    {"AR", ScatteringFactorFunction("AR", 16.8752, -0.01456, 8.32256, 0.83310, 6.91326, 14.9177, 2.18515, 37.2256, -16.2972)},
    {"K", ScatteringFactorFunction("K", 8.11756, 12.6684, 7.48062, 0.76409, 1.07795, 211.222, 0.97218, 37.2727, 1.35009)},
    {"K+1", ScatteringFactorFunction("K+1", 9.70659, 0.59947, 7.37245, 11.8765, 5.67228, -0.08359, 1.90688, 26.7668, -6.65819)},
    {"CA", ScatteringFactorFunction("CA", 8.60272, 10.2636, 7.50769, 0.62794, 1.75117, 149.301, 0.96216, 60.2274, 1.17430)},
    {"CA+2", ScatteringFactorFunction("CA+2", 13.2063, 0.39466, 11.0586, -0.08204, 7.73221, 9.62976, 1.72057, 20.3341, -15.7176)},
    {"SC", ScatteringFactorFunction("SC", 9.06482, 8.77431, 7.55526, 0.53306, 2.05017, 123.880, 1.28745, 36.8890, 1.03849)},
    {"SC+3", ScatteringFactorFunction("SC+3", 13.4008, 0.29854, 8.02730, 7.96290, 1.65943, -0.28604, 1.57936, 16.0662, -6.66668)},
    {"TI", ScatteringFactorFunction("TI", 9.54969, 7.60579, 7.60067, 0.45899, 2.17223, 109.099, 1.75438, 27.5715, 0.91762)},
    {"TI+3", ScatteringFactorFunction("TI+3", 17.7344, 0.22061, 8.73816, 7.04716, 5.25691, -0.15762, 1.92134, 15.9768, -14.6519)},
    {"TI+4", ScatteringFactorFunction("TI+4", 19.5114, 0.17885, 8.23473, 6.67018, 2.01341, -0.29263, 1.52080, 12.9464, -13.2803)},
    {"V", ScatteringFactorFunction("V", 10.0661, 6.67721, 7.61420, 0.40322, 2.23551, 98.5954, 2.23170, 22.5720, 0.84574)},
    {"V+2", ScatteringFactorFunction("V+2", 9.34513, 6.49985, 7.68833, 0.39491, 2.94531, 15.9868, 0.26998, 41.0832, 0.75143)},
    {"V+3", ScatteringFactorFunction("V+3", 9.43141, 6.39535, 7.74190, 0.38339, 2.15343, 15.1908, 0.01686, 63.9690, 0.65657)},
    {"V+5", ScatteringFactorFunction("V+5", 15.6887, 0.67900, 8.14208, 5.40135, 2.03081, 9.97278, -9.57602, 0.94046, 1.71430)},
    {"CR", ScatteringFactorFunction("CR", 10.4757, 6.01658, 7.51402, 0.37426, 3.50115, 19.0654, 1.54902, 97.4599, 0.95226)},
    {"CR+2", ScatteringFactorFunction("CR+2", 9.54034, 5.66078, 7.75090, 0.34426, 3.58274, 13.3075, 0.50911, 32.4224, 0.61690)},
    {"CR+3", ScatteringFactorFunction("CR+3", 9.68090, 5.59463, 7.81136, 0.33439, 2.87603, 12.8288, 0.11357, 32.8761, 0.51827)},
    {"MN", ScatteringFactorFunction("MN", 11.2519, 5.34818, 7.36935, 0.34373, 3.04107, 17.4089, 2.27703, 84.2139, 1.05195)},
    {"MN+2", ScatteringFactorFunction("MN+2", 9.78094, 4.98303, 7.79153, 0.30421, 4.18544, 11.4399, 0.72736, 27.7750, 0.51454)},
    {"MN+3", ScatteringFactorFunction("MN+3", 9.84521, 4.91797, 7.87194, 0.29439, 3.56531, 10.8171, 0.32361, 24.1281, 0.39397)},
    {"MN+4", ScatteringFactorFunction("MN+4", 9.96253, 4.84850, 7.97057, 0.28330, 2.76067, 10.4852, 0.05445, 27.5730, 0.25188)},
    {"FE", ScatteringFactorFunction("FE", 11.9185, 4.87394, 7.04848, 0.34023, 3.34326, 15.9330, 2.27228, 79.0339, 1.40818)},
    {"FE+2", ScatteringFactorFunction("FE+2", 10.1270, 4.44133, 7.78007, 0.27418, 4.71825, 10.1451, 0.89547, 24.8302, 0.47888)},
    {"FE+3", ScatteringFactorFunction("FE+3", 10.0333, 4.36001, 7.90625, 0.26250, 4.20562, 9.35847, 0.55048, 20.4105, 0.30429)},
    {"CO", ScatteringFactorFunction("CO", 12.6158, 4.48994, 6.62642, 0.35459, 3.57722, 14.8402, 2.25644, 74.7352, 1.91452)},
    {"CO+2", ScatteringFactorFunction("CO+2", 10.5942, 4.00858, 7.67791, 0.25410, 5.15947, 9.21931, 1.01440, 22.7516, 0.55358)},
    {"CO+3", ScatteringFactorFunction("CO+3", 10.3380, 3.90969, 7.88173, 0.23867, 4.76795, 8.35583, 0.72559, 18.3491, 0.28667)},
    {"NI", ScatteringFactorFunction("NI", 13.3239, 4.17742, 6.18746, 0.38682, 3.74792, 14.0123, 2.23195, 71.1195, 2.49899)},
    {"NI+2", ScatteringFactorFunction("NI+2", 11.1650, 3.65944, 7.45636, 0.24397, 5.51106, 8.52596, 1.09496, 21.1647, 0.77218)},
    {"NI+3", ScatteringFactorFunction("NI+3", 10.7806, 3.54770, 7.75868, 0.22314, 5.22746, 7.64468, 0.84711, 16.9673, 0.38604)},
    {"CU", ScatteringFactorFunction("CU", 13.9352, 3.97779, 5.84833, 0.44555, 4.64221, 13.3971, 1.44753, 74.1605, 3.11686)},
    {"CU+1", ScatteringFactorFunction("CU+1", 12.4655, 3.54270, 6.63111, 0.28920, 5.76679, 9.31140, 1.34230, 26.9799, 1.79285)},
    {"CU+2", ScatteringFactorFunction("CU+2", 11.8168, 3.37484, 7.11181, 0.24408, 5.78135, 7.98760, 1.14523, 19.8970, 1.14431)},
    {"ZN", ScatteringFactorFunction("ZN", 14.6744, 3.71486, 5.62816, 0.50033, 3.92540, 12.8862, 2.16398, 65.4071, 3.59838)},
    {"ZN+2", ScatteringFactorFunction("ZN+2", 12.5225, 3.13961, 6.68507, 0.25431, 5.98382, 7.55544, 1.17317, 18.8453, 1.63497)},
    {"GA", ScatteringFactorFunction("GA", 15.3412, 3.63868, 5.74150, 0.65640, 3.10733, 16.0719, 2.52764, 70.7609, 4.26842)},
    {"GA+3", ScatteringFactorFunction("GA+3", 12.6920, 2.81262, 6.69883, 0.22789, 6.06692, 6.36441, 1.00660, 14.4122, 1.53545)},
    {"GE", ScatteringFactorFunction("GE", 15.4378, 3.39715, 6.00432, 0.73097, 3.05158, 18.9533, 2.93572, 63.7969, 4.56068)},
    {"AS", ScatteringFactorFunction("AS", 15.4043, 3.07517, 6.13723, 0.74113, 3.74679, 21.0014, 3.01390, 57.7446, 4.69149)},
    {"SE", ScatteringFactorFunction("SE", 15.5372, 2.71530, 5.98288, 0.68962, 4.83996, 21.0079, 2.93549, 52.4308, 4.70026)},
    {"BR", ScatteringFactorFunction("BR", 15.9934, 2.35651, 6.02439, 19.7393, 5.51599, 0.58143, 2.88716, 47.3323, 4.57602)},
    {"BR-1", ScatteringFactorFunction("BR-1", 15.4080, 2.43532, 6.78083, 22.0832, 6.00715, 0.68621, 2.99332, 64.9193, 4.80234)},
    {"KR", ScatteringFactorFunction("KR", 16.8494, 2.01856, 7.19790, 18.0409, 4.92564, 0.39741, 2.91606, 42.5054, 4.10864)},
    {"RB", ScatteringFactorFunction("RB", 11.4809, 1.08140, 9.46904, 18.2800, 9.16981, 2.38825, 1.42608, 185.293, 5.43921)},
    {"RB+1", ScatteringFactorFunction("RB+1", 17.8943, 1.71750, 8.59341, 0.09258, 7.91428, 15.4484, 2.47499, 32.5110, -0.87756)},
    {"SR", ScatteringFactorFunction("SR", 11.6164, 1.85574, 9.73009, 14.6109, 8.68081, 0.89852, 2.60986, 139.830, 5.34841)},
    {"SR+2", ScatteringFactorFunction("SR+2", 18.2430, 1.51215, 8.90811, 13.6536, 1.69192, 27.8238, -32.1118, -0.01488, 39.2691)},
    {"Y", ScatteringFactorFunction("Y", 19.0567, 1.24615, 6.50783, 9.68019, 4.81524, 18.8903, 2.84786, 121.353, 5.76121)},
    {"Y+3", ScatteringFactorFunction("Y+3", 18.4202, 1.34457, 9.75213, 12.0631, 1.05270, 25.1684, -33.4755, -0.01023, 40.2513)},
    {"ZR", ScatteringFactorFunction("ZR", 19.2273, 1.15488, 10.1378, 10.7877, 2.48177, 120.126, 2.42892, 33.3722, 5.71886)},
    {"ZR+4", ScatteringFactorFunction("ZR+4", 19.1301, 1.16051, 10.1098, 10.4084, 0.98896, 20.7214, -0.00004, -3.20442, 5.77164)},
    {"NB", ScatteringFactorFunction("NB", 19.3496, 1.06626, 10.8737, 10.5977, 3.47687, 32.6174, 1.64516, 120.397, 5.65073)},
    {"NB+3", ScatteringFactorFunction("NB+3", 19.1248, 1.07235, 18.2989, 0.00315, 11.0121, 10.3385, 2.04325, 25.9292, -12.4799)},
    {"NB+5", ScatteringFactorFunction("NB+5", 19.0175, 1.06028, 10.7591, 9.36239, 1.09900, 0.03765, 0.48469, 20.9764, 4.64045)},
    {"MO", ScatteringFactorFunction("MO", 19.3885, 0.97877, 11.8308, 10.0885, 3.75919, 31.9738, 1.46772, 117.932, 5.55047)},
    {"MO+3", ScatteringFactorFunction("MO+3", 19.6761, 0.95118, 18.0893, -0.00669, 11.7086, 9.61097, 2.50624, 24.0356, -12.9813)},
    {"MO+5", ScatteringFactorFunction("MO+5", 19.6054, 0.94029, 17.9292, -0.00795, 11.3451, 8.76715, 1.04247, 19.3690, -12.9217)},
    {"MO+6", ScatteringFactorFunction("MO+6", 19.4800, 0.94043, 17.6328, -0.00723, 11.0940, 8.29745, 0.37154, 18.9700, -12.5778)},
    {"TC", ScatteringFactorFunction("TC", 19.3597, 0.89356, 12.8087, 9.27497, 3.41372, 32.3513, 1.99926, 107.406, 5.41556)},
    {"RU", ScatteringFactorFunction("RU", 19.4316, 0.82092, 13.7309, 8.97737, 4.26537, 28.2621, 1.28720, 111.501, 5.28192)},
    {"RU+3", ScatteringFactorFunction("RU+3", 20.8024, 0.74711, 13.2995, 8.36626, 3.27542, 20.6179, 2.21026, -0.14664, 1.41087)},
    {"RU+4", ScatteringFactorFunction("RU+4", 41.5821, 0.61466, 12.9936, 7.99801, 2.71276, 18.1564, -24.2593, 0.43857, 6.97025)},
    {"RH", ScatteringFactorFunction("RH", 19.4524, 0.75019, 14.6845, 8.42622, 4.50240, 26.1564, 1.24740, 107.780, 5.11007)},
    {"RH+3", ScatteringFactorFunction("RH+3", 25.0958, 0.61346, 14.1510, 7.80244, 3.64428, 19.0932, -12.5768, 0.13532, 11.6838)},
    {"RH+4", ScatteringFactorFunction("RH+4", 41.5236, 0.52905, 13.8272, 7.49419, 3.07969, 16.9498, -25.9694, 0.32686, 8.53824)},
    {"PD", ScatteringFactorFunction("PD", 19.5123, 0.68583, 15.3800, 7.95714, 5.38330, 23.1808, 0.81015, 65.9295, 4.91427)},
    {"PD+2", ScatteringFactorFunction("PD+2", 19.4652, 0.68159, 15.5805, 7.80880, 4.04748, 20.9573, 0.02216, 110.020, 4.88510)},
    {"PD+4", ScatteringFactorFunction("PD+4", 51.1288, 0.43734, 14.6979, 7.03139, 3.41607, 15.8623, -38.2678, 0.26589, 11.0241)},
    {"AG", ScatteringFactorFunction("AG", 19.5284, 0.62387, 16.5811, 7.39504, 4.99150, 22.2282, 1.21404, 100.226, 4.68114)},
    {"AG+1", ScatteringFactorFunction("AG+1", 19.5416, 0.62273, 16.4239, 7.39663, 5.12995, 20.5530, 0.24053, 59.0604, 4.66470)},
    {"AG+2", ScatteringFactorFunction("AG+2", 19.5152, 0.62050, 16.4852, 7.30347, 4.32525, 19.3673, 0.02777, 92.9184, 4.64695)},
    {"CD", ScatteringFactorFunction("CD", 19.5528, 0.56604, 17.5717, 6.79630, 4.47374, 21.2907, 1.98562, 85.2777, 4.41158)},
    {"CD+2", ScatteringFactorFunction("CD+2", 19.5901, 0.56389, 17.3740, 6.83082, 4.62594, 17.8856, 0.03770, 76.2909, 4.37269)},
    {"IN", ScatteringFactorFunction("IN", 19.5872, 0.51510, 18.7169, 6.29430, 4.02722, 22.7308, 2.51452, 88.5675, 4.14542)},
    {"IN+3", ScatteringFactorFunction("IN+3", 19.6698, 0.50926, 18.1942, 6.28098, 4.09851, 15.4189, 0.00365, 160.227, 4.03396)},
    {"SN", ScatteringFactorFunction("SN", 19.6527, 0.46604, 19.5108, 5.76321, 3.86895, 24.0627, 3.14764, 78.1533, 3.81227)},
    {"SN+2", ScatteringFactorFunction("SN+2", 19.7166, 0.46027, 18.9265, 5.66448, 3.79775, 17.7248, 1.86248, 42.8086, 3.69648)},
    {"SN+4", ScatteringFactorFunction("SN+4", 19.7914, 0.45879, 18.9162, 5.76682, 3.64761, 13.3733, -0.0, -0.0, 3.64494)},
    {"SB", ScatteringFactorFunction("SB", 20.0759, 5.24328, 19.7766, 0.41858, 4.30389, 26.0178, 3.44952, 70.1646, 3.38881)},
    {"SB+3", ScatteringFactorFunction("SB+3", 19.8617, 0.41409, 19.5199, 5.18292, 3.73465, 16.8529, 1.61027, 35.1406, 3.27356)},
    {"SB+5", ScatteringFactorFunction("SB+5", 19.9613, 0.41262, 19.5889, 5.30028, 3.24333, 11.7603, -0.0, -0.0, 3.20701)},
    {"TE", ScatteringFactorFunction("TE", 20.4608, 4.74225, 20.0336, 0.37041, 5.38664, 27.3458, 3.33079, 65.0573, 2.78462)},
    {"I", ScatteringFactorFunction("I", 20.7492, 4.27091, 20.5640, 0.31960, 6.86158, 27.3186, 2.97589, 61.5375, 1.84739)},
    {"I-1", ScatteringFactorFunction("I-1", 20.8307, 4.29514, 20.4454, 0.32402, 7.52618, 29.8990, 3.18616, 81.4344, 2.00513)},
    {"XE", ScatteringFactorFunction("XE", 21.6679, 0.26422, 21.0085, 3.83526, 8.43382, 26.2297, 2.62265, 58.4830, 0.26635)},
    {"CS", ScatteringFactorFunction("CS", 22.3163, 0.23092, 21.1792, 3.49464, 10.7382, 25.1864, 1.46163, 232.829, -0.70709)},
    {"CS+1", ScatteringFactorFunction("CS+1", 23.9649, 0.20446, 21.2204, 3.43876, 9.76727, 23.4941, 1.61550, 49.7057, -2.56728)},
    {"BA", ScatteringFactorFunction("BA", 27.7489, 0.15152, 21.3777, 3.09817, 11.0400, 20.6774, 2.68186, 178.819, -6.85854)},
    {"BA+2", ScatteringFactorFunction("BA+2", 29.2996, 0.14047, 21.4669, 3.08785, 10.9209, 20.8818, 0.80126, 46.8842, -8.48753)},
    {"LA", ScatteringFactorFunction("LA", 33.2109, 0.11040, 21.7181, 2.83641, 11.6222, 19.3886, 3.17239, 144.438, -12.7404)},
    {"LA+3", ScatteringFactorFunction("LA+3", 43.6346, 0.07854, 21.7192, 2.78360, 11.7264, 18.4930, 0.32945, 49.2222, -23.4085)},
    {"CE", ScatteringFactorFunction("CE", 29.4100, 0.12335, 22.2428, 2.74837, 11.9818, 18.3794, 3.19259, 139.603, -8.84560)},
    {"CE+3", ScatteringFactorFunction("CE+3", 49.1105, 0.06535, 22.3499, 2.67229, 11.8399, 17.2040, 0.67455, 38.1904, -28.9739)},
    {"CE+4", ScatteringFactorFunction("CE+4", 66.7693, 0.04464, 21.8563, 2.53711, 12.2486, 16.4477, 0.09617, 64.4675, -46.9691)},
    {"PR", ScatteringFactorFunction("PR", 22.9220, 2.78604, 22.2518, 0.18015, 12.2269, 17.6663, 2.72431, 160.915, -1.13930)},
    {"PR+3", ScatteringFactorFunction("PR+3", 49.4655, 0.06197, 22.9705, 2.57634, 11.8015, 16.0371, 1.12179, 32.3673, -29.3586)},
    {"PR+4", ScatteringFactorFunction("PR+4", 62.6752, 0.04586, 22.4952, 2.45900, 12.4946, 15.5713, 0.20294, 46.5889, -42.8667)},
    {"ND", ScatteringFactorFunction("ND", 23.4069, 2.71587, 19.7073, 0.20950, 12.5016, 16.9122, 2.72850, 196.556, 1.64038)},
    {"ND+3", ScatteringFactorFunction("ND+3", 49.4292, 0.05936, 23.6116, 2.48611, 11.6190, 14.9366, 1.68986, 28.4515, -29.3493)},
    {"PM", ScatteringFactorFunction("PM", 23.8480, 2.65746, 17.5535, 0.24780, 12.7324, 16.2463, 2.72975, 152.682, 4.12018)},
    {"PM+3", ScatteringFactorFunction("PM+3", 49.2699, 0.05709, 24.2700, 2.40099, 11.3481, 13.9124, 2.32869, 25.6906, -29.2165)},
    {"SM", ScatteringFactorFunction("SM", 24.2242, 2.60993, 15.9132, 0.29475, 12.9238, 15.6554, 2.72836, 149.221, 6.19355)},
    {"SM+3", ScatteringFactorFunction("SM+3", 36.3271, 0.07823, 24.8502, 2.33602, 11.3426, 13.1872, 2.62300, 24.3996, -16.1429)},
    {"EU", ScatteringFactorFunction("EU", 24.9148, 2.97255, 14.8058, 0.34930, 13.0799, 15.1280, 2.72477, 146.103, 7.85731)},
    {"EU+2", ScatteringFactorFunction("EU+2", 25.6516, 2.36073, 23.9387, 0.13260, 10.5738, 12.6495, 4.05853, 25.0026, -3.22358)},
    {"EU+3", ScatteringFactorFunction("EU+3", 33.2862, 0.08350, 29.5041, 2.26275, 11.1494, 12.3883, 3.13496, 22.8351, -13.0748)},
    {"GD", ScatteringFactorFunction("GD", 24.4004, 2.47491, 14.0308, 0.40238, 13.1754, 14.4670, 3.24472, 119.738, 9.12488)},
    {"GD+3", ScatteringFactorFunction("GD+3", 29.0290, 0.09521, 26.1387, 2.19696, 11.0510, 11.7141, 3.52244, 21.6929, -8.74150)},
    {"TB", ScatteringFactorFunction("TB", 24.3736, 2.46637, 13.8649, 0.47517, 13.2510, 14.0424, 3.24435, 117.446, 10.2420)},
    {"TB+3", ScatteringFactorFunction("TB+3", 26.7821, 2.13333, 25.9463, 0.10597, 10.9724, 11.0974, 3.88172, 20.7042, -5.58307)},
    {"DY", ScatteringFactorFunction("DY", 24.6193, 2.52208, 14.2735, 0.54556, 13.3567, 13.8487, 2.70316, 138.385, 11.0290)},
    {"DY+3", ScatteringFactorFunction("DY+3", 27.3805, 2.07832, 22.2062, 0.12643, 10.9975, 10.5960, 4.10030, 19.9671, -1.68516)},
    {"HO", ScatteringFactorFunction("HO", 24.3162, 2.52724, 14.9012, 0.61572, 13.3895, 13.5041, 2.69309, 136.246, 11.6817)},
    {"HO+3", ScatteringFactorFunction("HO+3", 27.9956, 2.02324, 19.9560, 0.14275, 11.0106, 10.1165, 4.33205, 19.2589, 0.70499)},
    {"ER", ScatteringFactorFunction("ER", 23.8201, 2.54419, 15.8796, 0.68445, 13.3938, 13.1932, 2.68190, 134.282, 12.2062)},
    {"ER+3", ScatteringFactorFunction("ER+3", 28.5315, 1.97796, 17.4316, 0.17182, 11.1113, 9.73821, 4.43156, 18.7294, 3.49325)},
    {"TM", ScatteringFactorFunction("TM", 23.1386, 2.57320, 17.1707, 0.74948, 13.3703, 12.9126, 2.66981, 132.468, 12.6322)},
    {"TM+3", ScatteringFactorFunction("TM+3", 29.0215, 1.93707, 15.6168, 0.20467, 11.2288, 9.40342, 4.49403, 18.2607, 5.63812)},
    {"YB", ScatteringFactorFunction("YB", 22.3028, 2.61393, 18.7202, 0.80868, 13.3200, 12.6590, 2.65701, 130.783, 12.9818)},
    {"YB+2", ScatteringFactorFunction("YB+2", 29.1313, 1.99979, 13.5855, 0.32335, 11.4132, 9.59277, 4.69659, 20.3507, 9.17182)},
    {"YB+3", ScatteringFactorFunction("YB+3", 29.4761, 1.89879, 14.4357, 0.23793, 11.3446, 9.09408, 4.54681, 17.8206, 7.19600)},
    {"LU", ScatteringFactorFunction("LU", 21.1866, 0.88654, 20.1760, 2.68610, 13.0532, 12.2746, 3.21190, 107.128, 13.3489)},
    {"LU+3", ScatteringFactorFunction("LU+3", 29.8480, 1.86596, 13.6268, 0.27623, 11.4750, 8.82479, 4.56009, 17.4364, 8.48923)},
    {"HF", ScatteringFactorFunction("HF", 24.6725, 0.97400, 17.2295, 2.89038, 12.8069, 12.2897, 3.55970, 93.4381, 13.7049)},
    {"TA", ScatteringFactorFunction("TA", 28.1757, 1.04034, 14.4288, 3.20784, 12.6412, 12.5054, 3.74436, 85.0183, 13.9824)},
    {"W", ScatteringFactorFunction("W", 31.0935, 1.07885, 12.5273, 12.8331, 12.3769, 3.63298, 3.79138, 79.7647, 14.1842)},
    {"RE", ScatteringFactorFunction("RE", 33.2961, 1.09315, 12.3497, 13.2559, 11.2819, 4.16736, 3.72367, 76.6562, 14.3239)},
    {"OS", ScatteringFactorFunction("OS", 34.8667, 1.08840, 11.9524, 13.8042, 11.1851, 4.79179, 3.56436, 75.1399, 14.4097)},
    {"IR", ScatteringFactorFunction("IR", 35.9454, 1.06924, 11.9980, 5.43443, 11.2501, 14.4983, 3.34312, 74.7918, 14.4449)},
    {"PT", ScatteringFactorFunction("PT", 36.8102, 1.04422, 13.0747, 6.07340, 11.3323, 15.7018, 2.31421, 73.8375, 14.4526)},
    {"AU", ScatteringFactorFunction("AU", 37.3027, 1.00810, 14.9306, 6.52550, 10.3425, 16.5100, 2.01229, 76.9117, 14.3992)},
    {"HG", ScatteringFactorFunction("HG", 37.5186, 0.96455, 17.0353, 6.65786, 8.51121, 16.8438, 2.63340, 76.7228, 14.2911)},
    {"TL", ScatteringFactorFunction("TL", 37.6947, 0.92263, 19.7195, 6.78248, 6.38290, 19.2435, 3.00960, 85.9267, 14.1800)},
    {"PB", ScatteringFactorFunction("PB", 37.7383, 0.87755, 21.3394, 6.58964, 5.17527, 21.2437, 3.71604, 78.8094, 14.0203)},
    {"BI", ScatteringFactorFunction("BI", 37.7143, 0.83222, 22.4542, 6.27051, 4.84549, 24.4693, 4.14816, 72.1558, 13.8301)},
    {"PO", ScatteringFactorFunction("PO", 37.6297, 0.78640, 23.1323, 5.86644, 5.59203, 27.8678, 4.04218, 68.1617, 13.5991)},
    {"AT", ScatteringFactorFunction("AT", 37.4971, 0.74012, 23.5635, 5.42694, 7.15953, 29.8350, 3.45924, 66.3564, 13.3183)},
    {"RN", ScatteringFactorFunction("RN", 37.3308, 0.69354, 23.8933, 4.98696, 9.02222, 30.0338, 2.77349, 65.5799, 12.9796)},
    {"FR", ScatteringFactorFunction("FR", 37.1902, 0.65303, 24.1306, 4.61305, 11.5026, 29.2597, 1.47980, 257.965, 12.6868)},
    {"RA", ScatteringFactorFunction("RA", 36.9820, 0.60394, 24.2495, 4.17857, 11.8719, 24.3782, 2.72428, 200.024, 12.1642)},
    {"AC", ScatteringFactorFunction("AC", 36.8705, 0.56458, 24.7131, 3.88776, 12.3889, 23.1506, 3.26501, 161.726, 11.7484)},
    {"TH", ScatteringFactorFunction("TH", 36.7754, 0.52510, 25.2506, 3.61658, 13.0681, 22.3410, 3.63791, 139.164, 11.2497)},
    {"PA", ScatteringFactorFunction("PA", 37.1457, 0.52020, 25.2998, 3.66300, 13.7846, 20.6539, 3.29611, 150.973, 11.4561)},
    {"U", ScatteringFactorFunction("U", 37.2808, 0.90239, 25.6563, 3.58562, 14.3501, 19.6342, 3.30732, 146.633, 11.3864)},
    {"NP", ScatteringFactorFunction("NP", 37.3968, 0.48676, 26.0671, 3.52325, 14.8366, 18.7419, 3.31586, 142.798, 11.3632)},
    {"PU", ScatteringFactorFunction("PU", 37.6407, 0.47976, 26.5603, 3.57178, 15.4492, 17.9814, 2.79814, 165.232, 11.5358)},
    {"AM", ScatteringFactorFunction("AM", 37.6909, 0.46617, 27.1436, 3.52195, 15.7842, 17.3069, 2.79600, 161.931, 11.5685)},
    {"CM", ScatteringFactorFunction("CM", 37.5543, 0.44932, 27.6657, 3.38713, 15.8858, 16.6498, 3.32758, 133.547, 11.5431)},
    {"BK", ScatteringFactorFunction("BK", 37.5273, 0.43930, 28.3202, 3.35014, 16.1181, 16.1000, 3.32793, 131.027, 11.6823)},
    {"CF", ScatteringFactorFunction("CF", 37.6111, 0.43255, 29.2465, 3.39285, 16.4566, 15.6791, 2.78216, 153.766, 11.8853)},
    {"ES", ScatteringFactorFunction("ES", 37.4979, 0.42353, 30.0495, 3.35234, 16.5881, 15.2381, 2.77596, 151.474, 12.0698)},
    {"FM", ScatteringFactorFunction("FM", 37.3380, 0.41562, 30.8936, 3.31193, 16.6818, 14.8362, 2.76929, 149.344, 12.2983)},
    {"MD", ScatteringFactorFunction("MD", 37.1301, 0.40883, 31.7721, 3.27132, 16.7422, 14.4683, 2.76232, 147.353, 12.5741)},
    {"NO", ScatteringFactorFunction("NO", 36.8731, 0.40324, 32.6784, 3.23045, 16.7732, 14.1302, 2.75513, 145.481, 12.9008)},
    {"LW", ScatteringFactorFunction("LW", 36.3813, 0.40165, 33.1999, 3.13608, 16.6469, 13.7255, 3.31406, 119.377, 13.4313)}
};

/*
 * Atomic radii in Angstrom
 *
 * Clementi, E.; Raimond, D. L.; Reinhardt, W. P. (1967). "Atomic Screening Constants from SCF
 * Functions. II. Atoms with 37 to 86 Electrons". Journal of Chemical Physics. 47 (4): 1300–1307.
 * Bibcode:1967JChPh..47.1300C.
 * doi:10.1063/1.1712084
 */
const QMap<QString, double> ionicRadii {
    {"H+1", -0.0400},
    {"H", 0.5300},
    {"HE", 0.3100},
    {"LI+1", 0.9000},
    {"LI", 1.6700},
    {"BE+2", 0.5900},
    {"BE", 1.1200},
    {"B+3", 0.4100},
    {"B", 0.8700},
    {"C+4", 0.3000},
    {"C", 0.6700},
    {"N-3", 1.3200},
    {"N+3", 0.3000},
    {"N+5", 0.2700},
    {"N", 0.5600},
    {"O-2", 1.2600},
    {"O", 0.4800},
    {"F-1", 1.1900},
    {"F+7", 0.2200},
    {"F", 0.4200},
    {"NE", 0.3800},
    {"NA+1", 1.1600},
    {"NA", 1.9000},
    {"MG+2", 0.8600},
    {"MG", 1.4500},
    {"AL+3", 0.6750},
    {"AL", 1.1800},
    {"SI+4", 0.5400},
    {"SI", 1.1100},
    {"P+3", 0.5800},
    {"P+5", 0.5200},
    {"P", 0.9800},
    {"S-2", 1.7000},
    {"S+4", 0.5100},
    {"S+6", 0.4300},
    {"S", 0.8800},
    {"CL-1", 1.6700},
    {"CL+5", 0.2600},
    {"CL+7", 0.4100},
    {"CL", 0.7900},
    {"AR", 0.7100},
    {"K+1", 1.5200},
    {"K", 2.4300},
    {"CA+2", 1.1400},
    {"CA", 1.9400},
    {"SC+3", 0.8850},
    {"SC", 1.8400},
    {"TI+2", 1.0000},
    {"TI+3", 0.8100},
    {"TI+4", 0.7450},
    {"TI", 1.7600},
    {"V+2", 0.9300},
    {"V+3", 0.7800},
    {"V+4", 0.7200},
    {"V+5", 0.6800},
    {"V", 1.7100},
    {"CR+2", 0.8700},
    {"CR+3", 0.7550},
    {"CR+4", 0.6900},
    {"CR+5", 0.6300},
    {"CR+6", 0.5800},
    {"CR", 1.6600},
    {"MN+2", 0.8100},
    {"MN+3", 0.7200},
    {"MN+4", 0.6700},
    {"MN+5", 0.4700},
    {"MN+6", 0.3950},
    {"MN+7", 0.6000},
    {"MN", 1.6100},
    {"FE+2", 0.7500},
    {"FE+3", 0.6900},
    {"FE+4", 0.7250},
    {"FE+6", 0.3900},
    {"FE", 1.5600},
    {"CO+2", 0.7900},
    {"CO+3", 0.6850},
    {"CO", 1.5200},
    {"NI+2", 0.8300},
    {"NI+3", 0.7000},
    {"NI+4", 0.6200},
    {"NI", 1.4900},
    {"CU+1", 0.9100},
    {"CU+2", 0.8700},
    {"CU+3", 0.6800},
    {"CU", 1.4500},
    {"ZN+2", 0.8800},
    {"ZN", 1.4200},
    {"GA+3", 0.7600},
    {"GA", 1.3600},
    {"GE+2", 0.8700},
    {"GE+4", 0.6700},
    {"GE", 1.2500},
    {"AS+3", 0.7200},
    {"AS+5", 0.6000},
    {"AS", 1.1400},
    {"SE-2", 1.8400},
    {"SE+4", 0.6400},
    {"SE+6", 0.5600},
    {"SE", 1.0300},
    {"BR-1", 1.8200},
    {"BR+3", 0.7300},
    {"BR+5", 0.4500},
    {"BR+7", 0.5300},
    {"BR", 0.9400},
    {"KR", 0.8800},
    {"RB+1", 1.6600},
    {"RB", 2.6500},
    {"SR+2", 1.3200},
    {"SR", 2.1900},
    {"Y+3", 1.0400},
    {"Y", 2.1200},
    {"ZR+4", 0.8600},
    {"ZR", 2.0600},
    {"NB+3", 0.8600},
    {"NB+4", 0.8200},
    {"NB+5", 0.7800},
    {"NB", 1.9800},
    {"MO+3", 0.8300},
    {"MO+4", 0.7900},
    {"MO+5", 0.7500},
    {"MO+6", 0.7300},
    {"MO", 1.9000},
    {"TC+4", 0.7850},
    {"TC+5", 0.7400},
    {"TC+7", 0.7000},
    {"TC", 1.8300},
    {"RU+3", 0.8200},
    {"RU+4", 0.7600},
    {"RU+5", 0.7050},
    {"RU+7", 0.5200},
    {"RU+8", 0.5000},
    {"RU", 1.7800},
    {"RH+3", 0.8050},
    {"RH+4", 0.7400},
    {"RH+5", 0.6900},
    {"RH", 1.7300},
    {"PD+1", 0.7300},
    {"PD+2", 1.0000},
    {"PD+3", 0.9000},
    {"PD+4", 0.7550},
    {"PD", 1.6900},
    {"AG+1", 1.2900},
    {"AG+2", 1.0800},
    {"AG+3", 0.8900},
    {"AG", 1.6500},
    {"CD+2", 1.0900},
    {"CD", 1.6100},
    {"IN+3", 0.9400},
    {"IN", 1.5600},
    {"SN+4", 0.8300},
    {"SN", 1.4500},
    {"SB+3", 0.9000},
    {"SB+5", 0.7400},
    {"SB", 1.3300},
    {"TE-2", 2.0700},
    {"TE+4", 1.1100},
    {"TE+6", 0.7000},
    {"TE", 1.2300},
    {"I-1", 2.0600},
    {"I+5", 1.0900},
    {"I+7", 0.6700},
    {"I", 1.1500},
    {"XE+8", 0.6200},
    {"XE", 1.0800},
    {"CS+1", 1.8100},
    {"CS", 2.9800},
    {"BA+2", 1.4900},
    {"BA", 2.5300},
    {"LA+3", 1.1720},
    {"LA", 1.9500},
    {"CE+3", 1.1500},
    {"CE+4", 1.0100},
    {"CE", 1.8500},
    {"PR+3", 1.1300},
    {"PR+4", 0.9900},
    {"PR", 2.4700},
    {"ND+2", 1.4300},
    {"ND+3", 1.1230},
    {"ND", 2.0600},
    {"PM+3", 1.1100},
    {"PM", 2.0500},
    {"SM+2", 1.3600},
    {"SM+3", 1.0980},
    {"SM", 2.3800},
    {"EU+2", 1.3100},
    {"EU+3", 1.0870},
    {"EU", 2.3100},
    {"GD+3", 1.0780},
    {"GD", 2.3300},
    {"TB+3", 1.0630},
    {"TB+4", 0.9000},
    {"TB", 2.2500},
    {"DY+2", 1.2100},
    {"DY+3", 1.0520},
    {"DY", 2.2800},
    {"HO+3", 1.0410},
    {"HO", 2.2600},
    {"ER+3", 1.0300},
    {"ER", 2.2600},
    {"TM+2", 1.1700},
    {"TM+3", 1.0200},
    {"TM", 2.2200},
    {"YB+2", 1.1600},
    {"YB+3", 1.0080},
    {"YB", 2.2200},
    {"LU+3", 1.0010},
    {"LU", 2.1700},
    {"HF+4", 0.8500},
    {"HF", 2.0800},
    {"TA+3", 0.8600},
    {"TA+4", 0.8200},
    {"TA+5", 0.7800},
    {"TA", 2.0000},
    {"W+4", 0.8000},
    {"W+5", 0.7600},
    {"W+6", 0.7400},
    {"W", 1.9300},
    {"RE+4", 0.7700},
    {"RE+5", 0.7200},
    {"RE+6", 0.6900},
    {"RE+7", 0.6700},
    {"RE", 1.8800},
    {"OS+4", 0.7700},
    {"OS+5", 0.7150},
    {"OS+6", 0.6850},
    {"OS+7", 0.6650},
    {"OS+8", 0.5300},
    {"OS", 1.8500},
    {"IR+3", 0.8200},
    {"IR+4", 0.7650},
    {"IR+5", 0.7100},
    {"IR", 1.8000},
    {"PT+2", 0.9400},
    {"PT+4", 0.7650},
    {"PT+5", 0.7100},
    {"PT", 1.7700},
    {"AU+1", 1.5100},
    {"AU+3", 0.9900},
    {"AU+5", 0.7100},
    {"AU", 1.7400},
    {"HG+1", 1.3300},
    {"HG+2", 1.1600},
    {"HG", 1.7100},
    {"TL+1", 1.6400},
    {"TL+3", 1.0250},
    {"TL", 1.5600},
    {"PB+2", 1.3300},
    {"PB+4", 0.9150},
    {"PB", 1.5400},
    {"BI+3", 1.1700},
    {"BI+5", 0.9000},
    {"BI", 1.4300},
    {"PO+4", 1.0800},
    {"PO+6", 0.8100},
    {"PO", 1.3500},
    {"AT+7", 0.7600},
    {"AT", 1.2700},
    {"RN", 1.2000},
    {"FR+1", 1.9400},
    {"RA+2", 1.6200},
    {"AC+3", 1.2600},
    {"TH+4", 1.0800},
    {"PA+3", 1.1600},
    {"PA+4", 1.0400},
    {"PA+5", 0.9200},
    {"U+3", 1.1650},
    {"U+4", 1.0300},
    {"U+5", 0.9000},
    {"U+6", 0.8700},
    {"NP+2", 1.2400},
    {"NP+3", 1.1500},
    {"NP+4", 1.0100},
    {"NP+5", 0.8900},
    {"NP+6", 0.8600},
    {"NP+7", 0.8500},
    {"PU+3", 1.1400},
    {"PU+4", 1.0000},
    {"PU+5", 0.8800},
    {"PU+6", 0.8500},
    {"AM+2", 1.4000},
    {"AM+3", 1.1150},
    {"AM+4", 0.9900},
    {"CM+3", 1.1100},
    {"CM+4", 0.9900},
    {"BK+3", 1.1000},
    {"BK+4", 0.9700},
    {"CF+3", 1.0900},
    {"CF+4", 0.9610},
    {"ES+3", 0.9280}
};


struct AnchorPoint{
    double angle;
    double intensity;
    bool highlight;

    AnchorPoint() : angle(), intensity(), highlight() {}
    AnchorPoint(double a, double i, bool h) : angle(a), intensity(i), highlight(h) {}
};

static const QChar Alpha(0x0391);
static const QChar Beta(0x0392);
static const QChar Gamma(0x0393);
static const QChar Delta(0x0394);
static const QChar Epsilon(0x0395);
static const QChar Zeta(0x0396);
static const QChar Eta(0x0397);
static const QChar Theta(0x0398);
static const QChar Iota(0x0399);
static const QChar Kappa(0x039A);
static const QChar Lambda(0x039B);
static const QChar Mu(0x039C);
static const QChar Nu(0x039D);
static const QChar Xi(0x039E);
static const QChar Omicron(0x039F);
static const QChar Pi(0x03A0);
static const QChar Rho(0x03A1);
static const QChar Sigma(0x03A3);
static const QChar Tau(0x03A4);
static const QChar Upsilon(0x03A5);
static const QChar Phi(0x03A6);
static const QChar Chi(0x03A7);
static const QChar Psi(0x03A8);
static const QChar Omega(0x03A9);

static const QChar alpha(0x03B1);
static const QChar beta(0x03B2);
static const QChar gamma(0x03B3);
static const QChar delta(0x03B4);
static const QChar epsilon(0x03B5);
static const QChar zeta(0x03B6);
static const QChar eta(0x03B7);
static const QChar theta(0x03B8);
static const QChar iota(0x03B9);
static const QChar kappa(0x03BA);
static const QChar lambda(0x03BB);
static const QChar mu(0x03BC);
static const QChar nu(0x03BD);
static const QChar xi(0x03BE);
static const QChar omicron(0x03BF);
static const QChar pi(0x03C0);
static const QChar rho(0x03C1);
static const QChar finalsigma(0x03C2);
static const QChar sigma(0x03C3);
static const QChar tau(0x03C4);
static const QChar upsilon(0x03C5);
static const QChar phi(0x03C6);
static const QChar chi(0x03C7);
static const QChar psi(0x03C8);
static const QChar omega(0x03C9);

static const QChar minusDash(0x2212);
static const QChar enDash(0x2013);
static const QChar emDash(0x2014);

static const QChar superMinus(0x207B);
static const QChar superOne(0x00B9);
static const QChar superTwo(0x00B2);
static const QChar superThree(0x00B3);

static const QChar subOne(0x2081);
static const QChar subTwo(0x2082);
static const QChar subThree(0x2083);
static const QChar subFour(0x2084);
static const QChar subFive(0x2085);
static const QChar subSix(0x2086);
static const QChar subSeven(0x2087);
static const QChar subEight(0x2088);
static const QChar subNine(0x2089);
static const QChar subZero(0x2080);

static const QChar subM(0x2098);

static const QChar angstrom(0x00C5);
static const QChar degree(0x00B0);
static const QChar micro(0x00B5);
static const QChar errorSymb(0x21E8);

static const int defaultTextBlockNumber = 3;
static const QString defaultTextBlocks[2*defaultTextBlockNumber] =
{
    "Bi-modal crystallite size (STR)",
        "RefMult=2\nPARAM=pG=0.75_0.5^0.99\n"
        "\n"
        "GEWICHT[1]=pG*GEWICHT\n"
        "GEWICHT[2]=(1-pG)*GEWICHT\n"
        "\n"
        "PARAM=pB1=2_1^100\n"
        "B1[1]=B1\n"
        "B1[2]=pB1*B1\n",
    "Override SPHAR limits (STR)",
        "LIMIT8=0\n"
        "LIMIT6=0\n"
        "LIMIT4=0\n"
        "LIMIT2=0\n",
    "Sample height displacement (SAV)",
        "% To calculate the refined zero-point error (degree 2 theta) and sample displacement (mm) for a BRAGG-BRENTANO setup in a control file\n"
        "% helper variable pi\n"
        "pi=2*acos(0)\n"
        "% definition of the gonimeter radius in mm, must be actualized\n"
        "R=280\n"
        "% definition of a new variable for conversion zero point error in deg 2theta\n"
        "twothetazerodeg=EPS1*180/pi*2\n"
        "% definition of a new variable for calculation of sample displacement in mm\n"
        "displacementmm=-EPS2*R\n"
        "GOAL[]=twothetazerodeg\n"
        "GOAL[]=displacementmm\n\n"
};

const QMap<QString, double> tungstenLines {
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2084)), 1.30162},  // WLb4
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2083)), 1.26269},  // WLb3
    {QString("WL%1%2").arg(global::beta).arg(global::subOne), 1.281809}, // WLb1
    {QString("WL%1%2").arg(global::alpha).arg(QChar(0x2082)), 1.48743},  // WLa2
    {QString("WL%1%2").arg(global::alpha).arg(global::subOne), 1.47639},  // WLa1
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2086)), 1.28989},  // WLb6
    {QString("WL%1%2%3").arg(global::beta).arg(QChar(0x2081)).arg(QChar(0x2085)), 1.24631},  // WLb15
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2082)), 1.24460},  // WLb2
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2087)), 1.22400},  // WLb7
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2085)), 1.21545},  // WLb5
    {QString("WL%1%2%3").arg(global::beta).arg(QChar(0x2081)).arg(QChar(0x2080)), 1.21218},  // WLb10
    {QString("WL%1%2").arg(global::beta).arg(QChar(0x2089)), 1.20479}  // WLb9
};

struct ProfileCurveData {
    double center;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    QVector<double> x;
    QVector<double> y;
};

struct ProfileL12PCurve{
    double b1;
    double k1;
    double k2;
    double dInv;
    double waveLength;
    QVector<double> x;
    QVector<double> y;
};

struct ProfileL1SubCurve
{
    QString l;
    double g;
    double e;
    double q;
    QVector<double> x;
    QVector<double> y;

    ProfileL1SubCurve(): l(), g(), e(), q(), x(), y() {}
    ProfileL1SubCurve(QString _l, double _g, double _e, double _q): l(_l), g(_g), e(_e), q(_q), x(), y() {}
};

struct ProfileL2SubCurve
{
    double g;
    double e;
    double q;
    QVector<double> x;
    QVector<double> y;

    ProfileL2SubCurve(): g(), e(), q(), x(), y() {}
    ProfileL2SubCurve(double _g, double _e, double _q): g(_g), e(_e), q(_q), x(), y() {}
};

} // end of namespace global

#endif // STRUCTS_H
