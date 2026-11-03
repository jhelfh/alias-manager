#include "storage.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    QString error;
    QJsonObject aliases;
    const QString directory = temporary.path();
    const QString path = directory + "/aliases.json";
    auto check = [](bool condition, const char *message) {
        if (!condition) QTextStream(stderr) << message << '\n';
        return condition;
    };
    if (!check(temporary.isValid(), "Temporary directory failed")) return 1;
    if (!check(changeAlias(directory, "add", "", "test", "echo 'hello'", error), "Add failed")) return 1;
    if (!check(!changeAlias(directory, "add", "", "test", "duplicate", error), "Duplicate accepted")) return 1;
    if (!check(!changeAlias(directory, "add", "", "bad name", "echo no", error), "Invalid name accepted")) return 1;
    if (!check(!changeAlias(directory, "add", "", "empty", " ", error), "Empty command accepted")) return 1;
    if (!check(changeAlias(directory, "edit", "test", "renamed", "ls -al", error), "Edit failed")) return 1;
    if (!check(loadAliases(path, aliases, error), "Read failed")) return 1;
    if (!check(!aliases.contains("test") && aliases.value("renamed").toString() == "ls -al", "Edit mismatch")) return 1;
    if (!check(changeAlias(directory, "delete", "renamed", "", "", error), "Delete failed")) return 1;
    if (!check(loadAliases(path, aliases, error) && aliases.isEmpty(), "Delete mismatch")) return 1;
    QFile broken(path);
    if (!broken.open(QIODevice::WriteOnly)) return 1;
    broken.write("not JSON");
    broken.close();
    if (!check(!changeAlias(directory, "add", "", "test", "echo no", error), "Corrupt JSON overwritten")) return 1;
    if (!broken.open(QIODevice::ReadOnly)) return 1;
    return check(broken.readAll() == "not JSON", "Corrupt file changed") ? 0 : 1;
}
