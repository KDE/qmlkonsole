// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtQml>

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "fontlistmodel.h"
#include "savedcommandsmodel.h"
#include "shellcommand.h"
#include "terminalsettings.h"
#include "terminaltabmodel.h"
#include "util.h"
#include "version.h"

#include <KonsoleQML.h>

constexpr auto URI = "org.kde.qmlkonsole";

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("qmlkonsole");
    KAboutData aboutData(QStringLiteral("qmlkonsole"),
                         QStringLiteral("QMLKonsole"),
                         QStringLiteral(QMLKONSOLE_VERSION_STRING),
                         QStringLiteral("Mobile terminal application"),
                         KAboutLicense::GPL,
                         i18n("© 2020-2022 KDE Community"));
    aboutData.setBugAddress("https://bugs.kde.org/enter_bug.cgi?product=QMLKonsole");
    aboutData.addAuthor(i18n("Jonah Brüchert"), QString(), QStringLiteral("jbb@kaidan.im"));
    aboutData.addAuthor(i18n("Devin Lin"), QString(), QStringLiteral("devin@kde.org"));
    KAboutData::setApplicationData(aboutData);

    qmlRegisterSingletonInstance<TerminalSettings>(URI, 1, 0, "TerminalSettings", TerminalSettings::self());
    qmlRegisterSingletonType<ShellCommand>(URI, 1, 0, "ShellCommand", [](QQmlEngine *, QJSEngine *) {
        return new ShellCommand();
    });

    KonsoleQML::registerTypes("org.kde.konsoleqml");

    QObject::connect(TerminalSettings::self(), &TerminalSettings::configChanged, QApplication::instance(), [] {
        TerminalSettings::self()->save();
        qDebug() << "saving config";
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    Util util;
    {
        QCommandLineParser parser;
        parser.addOption(QCommandLineOption(QStringLiteral("e"), i18n("Command to execute"), QStringLiteral("command")));
        parser.addOption(QCommandLineOption(QStringLiteral("workdir"), i18n("Set the initial working directory to 'dir'"), QStringLiteral("dir")));

        // Add a no-op compatibility option to make Konsole compatible with
        // Debian's policy on X terminal emulators.
        // -T is technically meant to set a title, that is not really meaningful
        // for Konsole as we have multiple user-facing options controlling
        // the title and overriding whatever is set elsewhere.
        // https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=532029
        // https://www.debian.org/doc/debian-policy/ch-customized-programs.html#s11.8.3
        // --title is used by the VirtualBox Guest Additions installer
        auto titleOption = QCommandLineOption(
            {QStringLiteral("T"), QStringLiteral("title")},
            QStringLiteral("Debian policy compatibility, not used"),
            QStringLiteral("value"));
        titleOption.setFlags(QCommandLineOption::HiddenFromHelp);
        parser.addOption(titleOption);

        parser.addVersionOption();
        parser.addHelpOption();
        parser.process(app);

        QString initialCommand = parser.isSet(QStringLiteral("e")) ? parser.value(QStringLiteral("e")) : "";
        QString initialWorkDir = parser.isSet(QStringLiteral("workdir")) ? parser.value(QStringLiteral("workdir")) : QDir::currentPath();
        util.setInitialCommand(std::move(initialCommand));
        util.setInitialWorkDir(std::move(initialWorkDir));
    }

    qmlRegisterSingletonType<SavedCommandsModel>(URI, 1, 0, "SavedCommandsModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        const auto self = SavedCommandsModel::self();
        QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
        return self;
    });
    qmlRegisterSingletonType<TerminalTabModel>(URI, 1, 0, "TerminalTabModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        const auto self = TerminalTabModel::self();
        QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
        return self;
    });
    qmlRegisterSingletonType<FontListSearchModel>(URI, 1, 0, "FontListSearchModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        const auto self = FontListSearchModel::self();
        QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
        return self;
    });
    qmlRegisterSingletonInstance(URI, 1, 0, "Util", &util);
    qmlRegisterSingletonType(URI, 1, 0, "About", [](QQmlEngine *, QJSEngine *engine) {
        return engine->toScriptValue(KAboutData::applicationData());
    });

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    // required for X11
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.qmlkonsole")));

    return app.exec();
}
