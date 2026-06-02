/***************************************************************************
                          emapdatahandler.h  -  description
                             -------------------
    begin                : Tue Jul 12 17:49:22 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "emapdatahandler.h"

EMapDataHandler::EMapDataHandler()
{

}

/*
 * matrix adapted from:
 * http://www.ruppweb.org/Xray/tutorial/Coordinate%20system%20transformation.htm
 */
QMatrix4x4 EMapDataHandler::createF2CMatrix(const UnitCell &uc, projection_t prj)
{
    float data[16] = {0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 1.0};

    // cosines
    float ca = uc.alpha == 90.0 ? 0.0 : cos(uc.alpha * M_PI / 180.0);
    float cb = uc.beta  == 90.0 ? 0.0 : cos(uc.beta  * M_PI / 180.0);
    float cc = uc.gamma == 90.0 ? 0.0 : cos(uc.gamma * M_PI / 180.0);

    // sines
    float sa = uc.alpha == 90.0 ? 1.0 : sin(uc.alpha * M_PI / 180.0);
    float sb = uc.beta  == 90.0 ? 1.0 : sin(uc.beta  * M_PI / 180.0);
    float sc = uc.gamma == 90.0 ? 1.0 : sin(uc.gamma * M_PI / 180.0);

    float v = uc.a * uc.b * uc.c * sqrt(1.0 - pow(ca, float(2.0)) - pow(cb, float(2.0)) - pow(cc, float(2.0))
                   + 2.0 * ca * cb * cc);

    // reciprocal axes
    float ra = uc.b * uc.c * sa / v;
    float rb = uc.a * uc.c * sb / v;
    float rc = uc.a * uc.b * sc / v;

    // reciprocal angle cosines
    float rca = (cb * cc - ca)/(sb * sc);
    float rcb = (ca * cc - cb)/(sa * sc);
    float rcc = (ca * cb - cc)/(sa * sb);
    Q_UNUSED(rcc);

    // write the matrix elements
    if (prj == AB_PLANE) {
        data[0] = uc.a;
        data[1] = uc.b * cc;
        data[2] = uc.c * cb;
        data[5] = -uc.b * sc; // negative, to keep the origin bottom-left instead of top-left
        data[6] = -uc.c * rca * sb;
        data[10] = 1.0 / rc;
    }

    if (prj == AC_PLANE) {
        data[0] = uc.a;
        data[1] = uc.c * cb;
        data[2] = uc.b * cc;
        data[5] = -uc.c * sb; // negative, to keep the origin bottom-left instead of top-left
        data[6] = -uc.b * rca * sc;
        data[10] = 1.0 / rb;
    }

    if (prj == BC_PLANE) {
        data[0] = uc.b;
        data[1] = uc.c * ca;
        data[2] = uc.a * cc;
        data[5] = -uc.c * sa; // negative, to keep the origin bottom-left instead of top-left
        data[6] = -uc.a * rcb * sc;
        data[10] = 1.0 / ra;
    }

    QMatrix4x4 m(data);
    return m;
}

QString EMapDataHandler::mapToCsvFractional(const EDataMap &vmap, bool *ok)
{
    *ok = checkEMapSize(vmap);
    if (!ok) return QString();

    int nz = vmap.size();
    int ny = vmap.first().size();
    int nx = vmap.first().first().size();

    QString out;

    for (int z = 0; z < vmap.size(); ++z) {
        for (int y = 0; y < vmap.at(z).size(); ++y) {
            for (int x = 0; x < vmap.at(z).at(y).size(); ++x) {
                double fx = double(x)/double(nx);
                double fy = double(y)/double(ny);
                double fz = double(z)/double(nz);
                out.append(QString("%1 %2 %3 %4\n").arg(fx, 0, 'f', 6).arg(fy, 0, 'f', 6).arg(fz, 0, 'f', 6).arg(vmap.at(z).at(y).at(x), 0, 'f', 6));
            }
        }
    }

    return out;
}

QString EMapDataHandler::mapToCsvCartesian(const EDataMap &vmap, const UnitCell &uc, bool *ok)
{
    *ok = checkEMapSize(vmap);
    if (!ok) return QString();

    QString out;
    int nz = vmap.size();
    int ny = vmap.first().size();
    int nx = vmap.first().first().size();

    // get the matrix fractional coordinates -> cartesian Angstrom
    // and flip y to get the origin bottom left (instead of top left)
    QMatrix4x4 cellMatrix = EMapDataHandler::createF2CMatrix(uc, AB_PLANE);
    QMatrix4x4 mirror(1.0,  0.0, 0.0, 0.0,
                      0.0, -1.0, 0.0, 0.0,
                      0.0,  0.0, 1.0, 0.0,
                      0.0,  0.0, 0.0, 1.0);

    cellMatrix = mirror * cellMatrix;

    // size of one voxel in fractional coordinates
    double sz = 1.0 / (double)nz;
    double sy = 1.0 / (double)ny;
    double sx = 1.0 / (double)nx;

    for (int z = 0; z < nz; ++z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                double val = vmap.at(z).at(y).at(x);
                QVector3D pos = cellMatrix.map(QVector3D(sx * double(x), sy * double(y), sz * double(z)));
                out.append(QString("%1 %2 %3 %4\n").arg(pos.x(), 0, 'f', 6).arg(pos.y(), 0, 'f', 6).arg(pos.z(), 0, 'f', 6).arg(val, 0, 'f', 6));
            }
        }
    }

    return out;
}

QString EMapDataHandler::layerToCsvFractional(const EDataMap &vmap, projection_t proj, int layer, bool *ok)
{
    *ok = checkEMapSize(vmap);
    if (!ok) return QString();

    int nz = vmap.size();
    int ny = vmap.first().size();
    int nx = vmap.first().first().size();

    QString out;

    if (proj == AB_PLANE) {
        for (int y = 0; y < vmap.at(layer).size(); ++y) {
            for (int x = 0; x < vmap.at(layer).at(y).size(); ++x) {
                double fx = double(x)/double(nx);
                double fy = double(y)/double(ny);
                out.append(QString("%1 %2 %3\n").arg(fx, 0, 'f', 6).arg(fy, 0, 'f', 6).arg(vmap.at(layer).at(y).at(x), 0, 'f', 6));
            }
        }
    } else if (proj == AC_PLANE) {
        for (int z = 0; z < vmap.size(); ++z) {
            for (int x = 0; x < vmap.at(z).at(layer).size(); ++x) {
                double fx = double(x)/double(nx);
                double fz = double(z)/double(nz);
                out.append(QString("%1 %2 %3\n").arg(fx, 0, 'f', 6).arg(fz, 0, 'f', 6).arg(vmap.at(z).at(layer).at(x), 0, 'f', 6));
            }
        }
    } else {
        for (int z = 0; z < vmap.size(); ++z) {
            for (int y = 0; y < vmap.at(z).size(); ++y) {
                double fy = double(y)/double(ny);
                double fz = double(z)/double(nz);
                out.append(QString("%1 %2 %3\n").arg(fy, 0, 'f', 6).arg(fz, 0, 'f', 6).arg(vmap.at(z).at(y).at(layer), 0, 'f', 6));
            }
        }
    }

    return out;
}

QString EMapDataHandler::layerToCsvCartesian(const EDataMap &vmap, const UnitCell &uc, projection_t proj, int layer, bool *ok)
{
    *ok = checkEMapSize(vmap);
    if (!ok) return QString();

    QString out;
    int nz = vmap.size();
    int ny = vmap.first().size();
    int nx = vmap.first().first().size();

    // get the matrix fractional coordinates -> cartesian Angstrom
    // and flip y to get the origin bottom left (instead of top left)
    QMatrix4x4 cellMatrix = EMapDataHandler::createF2CMatrix(uc, AB_PLANE);
    QMatrix4x4 mirror(1.0,  0.0, 0.0, 0.0,
                      0.0, -1.0, 0.0, 0.0,
                      0.0,  0.0, 1.0, 0.0,
                      0.0,  0.0, 0.0, 1.0);

    cellMatrix = mirror * cellMatrix;

    // size of one voxel in fractional coordinates
    double sz = 1.0 / (double)nz;
    double sy = 1.0 / (double)ny;
    double sx = 1.0 / (double)nx;

    if (proj == AB_PLANE) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                double val = vmap.at(layer).at(y).at(x);
                QVector3D pos = cellMatrix.map(QVector3D(sx * double(x), sy * double(y), sz * double(layer)));
                out.append(QString("%1 %2 %3\n").arg(pos.x(), 0, 'f', 6).arg(pos.y(), 0, 'f', 6).arg(val, 0, 'f', 6));
            }
        }
    } else if (proj == AC_PLANE) {
        for (int z = 0; z < nz; ++z) {
            for (int x = 0; x < nx; ++x) {
                double val = vmap.at(z).at(layer).at(x);
                QVector3D pos = cellMatrix.map(QVector3D(sx * double(x), sy * double(layer), sz * double(z)));
                out.append(QString("%1 %2 %3\n").arg(pos.x(), 0, 'f', 6).arg(pos.z(), 0, 'f', 6).arg(val, 0, 'f', 6));
            }
        }
    } else {
        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                double val = vmap.at(z).at(y).at(layer);
                QVector3D pos = cellMatrix.map(QVector3D(sx * double(layer), sy * double(y), sz * double(z)));
                out.append(QString("%1 %2 %3\n").arg(pos.y(), 0, 'f', 6).arg(pos.z(), 0, 'f', 6).arg(val, 0, 'f', 6));
            }
        }
    }

    return out;
}

bool EMapDataHandler::checkEMapSize(const EDataMap &vmap)
{
    if (!vmap.size())                      return false;
    else if (!vmap.first().size())         return false;
    else if (!vmap.first().first().size()) return false;

    return true;
}

bool EMapDataHandler::checkEMapSize(const EDataMap *vmap)
{
    if (!vmap) return false;

    if (!vmap->size())                      return false;
    else if (!vmap->first().size())         return false;
    else if (!vmap->first().first().size()) return false;

    return true;
}

int EMapDataHandler::numberOfImages(const EDataMap *vmap, projection_t prj)
{
    if (!EMapDataHandler::checkEMapSize(vmap)) return 0;
    if ((prj == AB_PLANE) || (prj == C_AXIS)) return vmap->size();
    if ((prj == AC_PLANE) || (prj == B_AXIS)) return vmap->first().size();
    if ((prj == BC_PLANE) || (prj == A_AXIS)) return vmap->first().first().size();

    return 0;

}
