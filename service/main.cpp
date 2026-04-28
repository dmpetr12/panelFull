#include <QCoreApplication>

#include "BackendController.h"
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
    if (!web.start(8080, "./web/dist")) {
        qCritical() << "Web API failed to start";
        return 3;
    }

    return app.exec();
}
