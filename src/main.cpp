// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QIcon>

#include <KLocalizedContext>
#include <KLocalizedString>
#include <KAboutData>

#include "fontlistmodel.h"
#include "terminalsettings.h"
#include "savedcommandsmodel.h"
#include "terminaltabmodel.h"
#include "version.h"
#include "util.h"
#include "shellcommand.h"

constexpr auto URI = "org.kde.qmlkonsole";

QCommandLineParser *createParser()
{
    QCommandLineParser *parser = new QCommandLineParser;
    parser->addOption(QCommandLineOption(QStringLiteral("e"), i18n("Command to execute"), QStringLiteral("command")));
    parser->addOption(QCommandLineOption(QStringLiteral("workdir"), i18n("Set the initial working directory to 'dir'"), QStringLiteral("dir")));
    parser->addVersionOption();
    parser->addHelpOption();
    return parser;
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
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

    QObject::connect(TerminalSettings::self(), &TerminalSettings::configChanged, QApplication::instance(), [] {
        TerminalSettings::self()->save();
        qDebug() << "saving config";
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    
    // ~~~~ Parse command line arguments ~~~~
    {
        QScopedPointer<QCommandLineParser> parser(createParser());
        parser->process(app);

        engine.rootContext()->setContextProperty("INITIAL_COMMAND", parser->isSet(QStringLiteral("e")) ? parser->value(QStringLiteral("e")) : "");
        engine.rootContext()->setContextProperty("INITIAL_WORK_DIR", parser->isSet(QStringLiteral("workdir")) ? parser->value(QStringLiteral("workdir")) : QDir::currentPath());
    }
    
    qmlRegisterSingletonType<TerminalTabModel>(URI, 1, 0, "SavedCommandsModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return SavedCommandsModel::self();
    });
    qmlRegisterSingletonType<TerminalTabModel>(URI, 1, 0, "TerminalTabModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return TerminalTabModel::self();
    });
    qmlRegisterSingletonType<FontListSearchModel>(URI, 1, 0, "FontListSearchModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return FontListSearchModel::self();
    });
    qmlRegisterSingletonType<Util>(URI, 1, 0, "Util", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return Util::self();
    });
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
