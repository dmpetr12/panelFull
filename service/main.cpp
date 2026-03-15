#include <QCoreApplication>

#include "BackendController.h"
#include "../core/LocalIpcServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    BackendController backend;
    if (!backend.start())
        return 1;

    LocalIpcServer ipc(&backend);
    if (!ipc.start())
        return 2;

    return app.exec();
}
