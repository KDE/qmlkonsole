#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <KLocalizedContext>

#include "terminalsettings.h"

constexpr auto URI = "org.kde.qmlkonsole";

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("KDE");
    QCoreApplication::setOrganizationDomain("kde.org");
    QCoreApplication::setApplicationName("QMLKonsole");

    qmlRegisterSingletonInstance<TerminalSettings>(URI, 1, 0, "TerminalSettings", TerminalSettings::self());

    QObject::connect(QApplication::instance(), &QCoreApplication::aboutToQuit, QApplication::instance(), [] {
        TerminalSettings::self()->save();
        qDebug() << "saving config";
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    int ret = app.exec();
    return ret;
}
