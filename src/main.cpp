#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QScopedPointer>

#include <sailfishapp.h>

#include "walletcoreprobe.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    qmlRegisterType<WalletCoreProbe>(
        "org.sailfishos.sailvault", 1, 0, "WalletCoreProbe");

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
