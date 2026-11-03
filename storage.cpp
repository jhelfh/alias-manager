#include "storage.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>

bool validName(const QString &name)
{
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z_][A-Za-z0-9_-]*$")
    );

    return pattern.match(name).hasMatch();
}

bool loadAliases(const QString &path, QJsonObject &aliases, QString &error)
{
    aliases = QJsonObject();
    QFile file(path);

    if (!file.exists()) {
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();

    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        error = "Invalid aliases.json. File was not changed.";
        return false;
    }

    aliases = document.object();

    for (auto it = aliases.constBegin(); it != aliases.constEnd(); ++it) {
        if (!validName(it.key()) ||
            !it.value().isString() ||
            it.value().toString().trimmed().isEmpty() ||
            it.value().toString().contains(QChar(0))) {
            error = "Invalid alias data. File was not changed.";
            return false;
        }
    }

    return true;
}

bool saveAliases(const QString &path,
                 const QJsonObject &aliases,
                 QString &error)
{
    QSaveFile file(path);

    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }

    const QByteArray data =
        QJsonDocument(aliases).toJson(QJsonDocument::Indented);

    if (file.write(data) != data.size()) {
        error = file.errorString();
        return false;
    }

    if (!file.commit()) {
        error = file.errorString();
        return false;
    }

    return true;
}
bool changeAlias(const QString &directory,
                 const QString &action,
                 const QString &oldName,
                 const QString &name,
                 const QString &command,
                 QString &error)
{
    QLockFile lock(directory + "/aliases.lock");

    if (!lock.tryLock(1000)) {
        error = "Storage is busy. Try again.";
        return false;
    }

    const QString path = directory + "/aliases.json";
    QJsonObject aliases;
    if (!loadAliases(path, aliases, error)) {
        return false;
    }

    if (action == "delete") {
        if (!aliases.contains(oldName)) {
            error = "Alias not found: " + oldName;
            return false;
        }

        aliases.remove(oldName);
    } else {
        if (!validName(name)) {
            error = "Use letters, digits, _ and -. "
                    "Start with a letter or underscore.";
            return false;
        }

        if (command.trimmed().isEmpty() || command.contains(QChar(0))) {
            error = "Command is empty or contains a null character.";
            return false;
        }

        if (action == "edit" && !aliases.contains(oldName)) {
            error = "Alias not found: " + oldName;
            return false;
        }

        if (aliases.contains(name) &&
            (action == "add" || name != oldName)) {
            error = "Alias already exists: " + name;
            return false;
        }

        if (action == "edit") {
            aliases.remove(oldName);
        }

        aliases.insert(name, command);
    }

    return saveAliases(path, aliases, error);
}
