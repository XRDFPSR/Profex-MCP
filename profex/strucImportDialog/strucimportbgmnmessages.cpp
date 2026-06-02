#include "strucimportbgmnmessages.h"

StrucImportBgmnMessages::StrucImportBgmnMessages()
{

}

QString StrucImportBgmnMessages::showError(const QString &s)
{
    return QString("<font color=\"%1\">%2</font><br>").arg("#FF0000").arg(s);
}

QString StrucImportBgmnMessages::showWarning(const QString &s)
{
    return QString("<font color=\"%1\">%2</font><br>").arg("#990000").arg(s);
}

QString StrucImportBgmnMessages::showComment(const QString &s)
{
    return QString("<font color=\"%1\">%2</font><br>").arg("#0000CC").arg(s);
}

QString StrucImportBgmnMessages::showSuccess(const QString &s)
{
    return QString("<font color=\"%1\">%2</font><br>").arg("#009900").arg(s);
}
