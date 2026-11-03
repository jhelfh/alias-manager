#include "cli.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

#include <cstdio>
#include <unistd.h>

namespace {

constexpr auto RESET = "\033[0m";
constexpr auto CYAN = "\033[1;36m";
constexpr auto GREEN = "\033[1;32m";
constexpr auto DIM = "\033[2m";

bool validName(const QString &name)
{
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z_][A-Za-z0-9_-]*$")
    );

    return pattern.match(name).hasMatch();
}

QString quoteForBash(QString text)
{
    text.replace("'", "'\\''");
    return QStringLiteral("'") + text + QStringLiteral("'");
}

// Do not print control characters from stored commands into the terminal.
QString displayText(const QString &text)
{
    QString result;

    for (const QChar ch : text) {
        if (ch == '\n') {
            result += "\\n";
        } else if (ch == '\r') {
            result += "\\r";
        } else if (ch == '\t') {
            result += "\\t";
        } else if (ch.category() == QChar::Other_Control ||
                   ch.category() == QChar::Other_Format) {
            result += '?';
        } else {
            result += ch;
        }
    }

    return result;
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

// Lock only while updating storage, not while waiting for user input.
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

    // Read the latest version after obtaining the lock.
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

bool ask(QTextStream &in,
         QTextStream &out,
         const QString &label,
         QString &answer)
{
    out << CYAN << "  " << label << RESET;
    out.flush();

    return in.readLineInto(&answer);
}

void drawScreen(QTextStream &out,
                const QJsonObject &aliases,
                const QString &message)
{
    out << "\033[2J\033[H" << CYAN;

    out << R"(
     _    _     ___    _    ____
    / \  | |   |_ _|  / \  / ___|
   / _ \ | |    | |  / _ \ \___ \
  / ___ \| |___ | | / ___ \ ___) |
 /_/   \_\_____|___/_/   \_\____/

               M A N A G E R
)";

    out << RESET
        << DIM
        << "  v0.1.0                          made by reiethereal\n"
        << RESET
        << "\n  YOUR ALIASES\n"
        << DIM
        << "  --------------------------------------------------------\n"
        << RESET;

    const QStringList names = aliases.keys();

    if (names.isEmpty()) {
        out << DIM << "  No aliases yet. Press 1 to add one.\n" << RESET;
    } else {
        for (qsizetype i = 0; i < names.size(); ++i) {
            const QString &name = names.at(i);

            out << DIM
                << "  " << QString::number(i + 1).rightJustified(2, '0')
                << "  "
                << RESET << GREEN
                << name.leftJustified(16)
                << RESET
                << displayText(aliases.value(name).toString())
                << '\n';
        }
    }

    out << DIM
        << "  --------------------------------------------------------\n"
        << RESET
        << "\n  [1] Add alias\n"
        << "  [2] Edit alias\n"
        << "  [3] Delete alias\n"
        << "  [4] Refresh\n"
        << "  [0] Exit\n";

    if (!message.isEmpty()) {
        out << "\n  " << displayText(message) << '\n';
    }

    out << '\n';
}

} // namespace

int runCli(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("alias-manager");

    QTextStream in(stdin);
    QTextStream out(stdout);
    QTextStream err(stderr);

    const QStringList args = app.arguments().mid(1);

    if (args.size() == 1 &&
        (args.first() == "--help" || args.first() == "-h")) {
        out << "Alias Manager 0.1.0\n\n"
            << "Usage: alias-manager [OPTION]\n\n"
            << "Run without arguments to open the interactive menu.\n\n"
            << "Options:\n"
            << "  -h, --help   Show help\n"
            << "  --version    Show version\n\n"
            << "The 'export' command is reserved for Bash integration.\n";
        return 0;
    }

    if (args.size() == 1 && args.first() == "--version") {
        out << "alias-manager 0.1.0\n";
        return 0;
    }

    const bool exporting =
        args.size() == 1 && args.first() == "export";

    if (!args.isEmpty() && !exporting) {
        err << "alias-manager: unexpected arguments\n"
            << "Run without arguments to open the menu.\n";
        return 1;
    }

    if (!exporting &&
        (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))) {
        err << "alias-manager: interactive mode requires a terminal\n";
        return 1;
    }

    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        err << "alias-manager: cannot create configuration directory\n";
        return 1;
    }

    const QString path = directory + "/aliases.json";

    if (exporting) {
        QJsonObject aliases;
        QString error;

        if (!loadAliases(path, aliases, error)) {
            err << "alias-manager: " << error << '\n';
            return 1;
        }

        for (const QString &name : aliases.keys()) {
            out << "alias " << name << "="
                << quoteForBash(aliases.value(name).toString()) << '\n';
        }

        return 0;
    }

    QString message;

    while (true) {
        QJsonObject aliases;
        QString error;

        if (!loadAliases(path, aliases, error)) {
            err << "alias-manager: " << error << '\n';
            return 1;
        }

        drawScreen(out, aliases, message);
        message.clear();

        QString choice;

        if (!ask(in, out, "alias-manager > ", choice)) {
            out << '\n';
            return 0;
        }

        choice = choice.trimmed().toLower();

        if (choice == "0" || choice == "q") {
            QString username = qEnvironmentVariable("USER");

            if (username.isEmpty()) {
            username = qEnvironmentVariable("LOGNAME");
            }

            if (username.isEmpty()) {
            out << "\n  Bye.\n";
            } else {
            out << "\n  Bye, " << displayText(username) << ".\n";
            }

    return 0;
}

        if (choice == "4") {
            continue;
        }

        if (choice != "1" && choice != "2" && choice != "3") {
            message = "[!] Choose 1, 2, 3, 4 or 0.";
            continue;
        }

        if (choice == "1") {
            QString name;
            QString command;

            out << "\n  ADD ALIAS — empty name cancels\n";

            if (!ask(in, out, "Alias   > ", name)) {
                return 0;
            }

            name = name.trimmed();

            if (name.isEmpty()) {
                continue;
            }

            if (!ask(in, out, "Command > ", command)) {
                return 0;
            }

            if (changeAlias(directory, "add", "", name, command, error)) {
                message = "[+] Added: " + name;
            } else {
                message = "[!] " + error;
            }

            continue;
        }

        QString number;

        out << "\n  Enter the alias number from the list."
            << "\n  Empty input cancels.\n";

        if (!ask(in, out, "Number > ", number)) {
            return 0;
        }

        if (number.trimmed().isEmpty()) {
            continue;
        }

        bool validNumber = false;
        const int index = number.trimmed().toInt(&validNumber) - 1;
        const QStringList names = aliases.keys();

        if (!validNumber || index < 0 || index >= names.size()) {
            message = "[!] Invalid alias number.";
            continue;
        }

        const QString oldName = names.at(index);

        if (choice == "3") {
            QString confirmation;

            out << "\n  Delete: " << oldName << '\n';

            if (!ask(in, out, "Confirm [y/N] > ", confirmation)) {
                return 0;
            }

            if (confirmation.trimmed().toLower() != "y") {
                message = "[-] Cancelled.";
                continue;
            }

            if (changeAlias(directory, "delete", oldName, "", "", error)) {
                message = "[-] Deleted: " + oldName;
            } else {
                message = "[!] " + error;
            }

            continue;
        }

        QString name;
        QString command;

        out << "\n  EDIT: " << oldName
            << "\n  Current command: "
            << displayText(aliases.value(oldName).toString())
            << "\n  Press Enter to keep the current value.\n\n";

        if (!ask(in, out, "New alias   > ", name)) {
            return 0;
        }

        name = name.trimmed();

        if (name.isEmpty()) {
            name = oldName;
        }

        if (!ask(in, out, "New command > ", command)) {
            return 0;
        }

        if (command.isEmpty()) {
            command = aliases.value(oldName).toString();
        }

        if (changeAlias(directory, "edit", oldName, name, command, error)) {
            message = "[*] Updated: " + oldName + " -> " + name;
        } else {
            message = "[!] " + error;
        }
    }
}