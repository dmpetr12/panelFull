#include <QCoreApplication>
#include <QDebug>

#include "BackendController.h"
#include "../core/AppConfig.h"
#include "../core/LocalIpcServer.h"
#include "../core/web/WebApiServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    BackendController backend;
    if (!backend.start())
        return 1;

    LocalIpcServer ipc(&backend);
    if (!ipc.start())
        return 2;

    WebApiServer web(&backend);
    AppConfig *config = backend.config();
    const bool webEnabled = config ? config->webEnabled() : true;
    const int webPort = config ? config->webPort() : 8080;

    if (webEnabled) {
        if (!web.start(static_cast<quint16>(webPort), "./web/dist")) {
            qCritical() << "Web API failed to start on port" << webPort;
            return 3;
        }
    } else {
        qInfo() << "Web API disabled by config";
    }

    return app.exec();
}
