#pragma once
#include <QJsonObject>
#include <QString>
bool loadAliases(const QString &path, QJsonObject &aliases, QString &error);
bool changeAlias(const QString &directory, const QString &action,
                 const QString &oldName, const QString &name,
                 const QString &command, QString &error);
