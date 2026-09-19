// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "commandline.h"

#include <KLocalizedString>

void configureCommandLineParser(QCommandLineParser &parser)
{
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addOption(QCommandLineOption(QStringLiteral("e"), i18n("Execute the following command and arguments")));
    parser.addPositionalArgument(QStringLiteral("command"), i18n("Command to execute, followed by its arguments"), QStringLiteral("[command [arguments...]]"));
    parser.addOption(QCommandLineOption(QStringLiteral("workdir"), i18n("Set the initial working directory to 'dir'"), QStringLiteral("dir")));

    // Add a no-op compatibility option to make Konsole compatible with
    // Debian's policy on X terminal emulators.
    // -T is technically meant to set a title, that is not really meaningful
    // for Konsole as we have multiple user-facing options controlling
    // the title and overriding whatever is set elsewhere.
    // https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=532029
    // https://www.debian.org/doc/debian-policy/ch-customized-programs.html#s11.8.3
    // --title is used by the VirtualBox Guest Additions installer
    auto titleOption =
        QCommandLineOption({QStringLiteral("T"), QStringLiteral("title")}, QStringLiteral("Debian policy compatibility, not used"), QStringLiteral("value"));
    titleOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(titleOption);

    parser.addVersionOption();
    parser.addHelpOption();
}

QString initialCommand(const QCommandLineParser &parser)
{
    if (!parser.isSet(QStringLiteral("e"))) {
        return {};
    }

    auto arguments = parser.positionalArguments();
    if (arguments.size() == 1) {
        return arguments.first();
    }
    // Single-quote each argument so the shell preserves contents literally
    for (auto &argument : arguments) {
        argument.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        argument = QLatin1Char('\'') + argument + QLatin1Char('\'');
    }
    return arguments.join(QLatin1Char(' '));
}
