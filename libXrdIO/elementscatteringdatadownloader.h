#ifndef ELEMENTSCATTERINGDATADOWNLOADER_H
#define ELEMENTSCATTERINGDATADOWNLOADER_H

#include <QString>
#include <QMap>
#include "elementscatteringdata.h"

class ElementScatteringDataDownloader
{
public:
    ElementScatteringDataDownloader();
    QMap<int, ElementScatteringData> getDataMap();
    void createBinaryFile(const QString &, const QMap<int, ElementScatteringData> &) const;

private:
    QMap<int, ElementScatteringData> readFiles();
    ElementScatteringData parseFile(const QStringList &);
};

#endif // ELEMENTSCATTERINGDATADOWNLOADER_H
