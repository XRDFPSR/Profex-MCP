#ifndef STRUCIMPORTBGMNMESSAGES_H
#define STRUCIMPORTBGMNMESSAGES_H

#include <QString>

class StrucImportBgmnMessages
{
public:
    StrucImportBgmnMessages();

    static QString showError(const QString &);
    static QString showWarning(const QString &);
    static QString showComment(const QString &);
    static QString showSuccess(const QString &);
};

#endif // STRUCIMPORTBGMNMESSAGES_H
