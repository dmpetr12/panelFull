#include <QGuiApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCursor>
#include <QFont>

#include "PanelFacade.h"
#include "systemObjects.h"
#include "TimeAjst.h"
#include "line.h"
#include "linesmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setApplicationName("Panel");

    QQuickStyle::setStyle("Fusion");

    QFont font("Ubuntu");
    font.setPixelSize(30);
    app.setFont(font);

    app.setOverrideCursor(QCursor(Qt::BlankCursor));

    PanelFacade panel;

    qmlRegisterType<TimeAjst>("App", 1, 0, "TimeAjst");
    qmlRegisterType<Line>("App", 1, 0, "Line");
    qmlRegisterType<LinesModel>("App", 1, 0, "LinesModel");
    qmlRegisterType<Network>("App", 1, 0, "Network");
    qmlRegisterType<System>("App", 1, 0, "System");
    qmlRegisterType<Mode>("App", 1, 0, "Mode");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("panel", &panel);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app,
                     [url](QObject *obj, const QUrl &objUrl)
                     {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     },
                     Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
