#include "cli.h"
#include "storage.h"
#include "terminal.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

#include <cstdio>
#include <unistd.h>

namespace {

constexpr auto RESET = "\033[0;32m";
constexpr auto CYAN = "\033[1;92m";
constexpr auto GREEN = "\033[1;32m";
constexpr auto DIM = "\033[0;32m";

QString quoteForBash(QString text)
{
    text.replace("'", "'\\''");
    return QStringLiteral("'") + text + QStringLiteral("'");
}
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

bool ask(QTextStream &in, QTextStream &out,
         const QString &label, QString &answer)
{
    Q_UNUSED(in);
    out << CYAN << "  " << label << RESET;
    out.flush();
    const auto line = readBlinkingLine();
    if (!line) return false;
    answer = QString::fromUtf8(line->data(), static_cast<qsizetype>(line->size()));
    return true;
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
        << "  v0.2.0                          made by reiethereal\n"
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
        out << (message.startsWith("[!]") ? "\033[1;31m" : "\033[1;92m")
            << "\n  " << displayText(message) << RESET << '\n';
    }

    out << '\n';
}

}

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
        out << "Alias Manager 0.2.0\n\n"
            << "Usage: alias-manager [OPTION]\n\n"
            << "Run without arguments to open the interactive menu.\n\n"
            << "Options:\n"
            << "  -h, --help   Show help\n"
            << "  --version    Show version\n"
            << "  --here       Run in the current terminal\n\n"
            << "The 'export' command is reserved for Bash integration.\n";
        return 0;
    }

    if (args.size() == 1 && args.first() == "--version") {
        out << "alias-manager 0.2.0\n";
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

    struct TerminalState {
        TerminalState() {
            std::fputs("\033[?1049h\033[0;32m\033[6 q\033[?25h", stdout);
            std::fflush(stdout);
        }
        ~TerminalState() {
            std::fputs("\033[?25h\033[0m\033[0 q\033[?1049l", stdout);
            std::fflush(stdout);
        }
    } terminalState;
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
        const qlonglong selected = number.trimmed().toLongLong(&validNumber);
        const qlonglong index = selected > 0 ? selected - 1 : -1;
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