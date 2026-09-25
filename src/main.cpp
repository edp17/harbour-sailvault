#include <QCoreApplication>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QScopedPointer>

#include <sailfishapp.h>

#include "walletcoreprobe.h"
#include "walletvault.h"
#include "portfolioservice.h"
#include "portfoliohistoryservice.h"
#include "apptools.h"
#include "activityservice.h"
#include "tokenservice.h"
#include "priceservice.h"
#include "sessionsecurity.h"
#include "settingsstore.h"
#include "transactiondetailservice.h"
#include "watchonlyservice.h"
#include "addressbookservice.h"
#include "qrimageprovider.h"
#include "networkhealthservice.h"
#include "networkfeeservice.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    QCoreApplication::setOrganizationName(QStringLiteral("harbour-sailvault"));
    QCoreApplication::setApplicationName(QStringLiteral("harbour-sailvault"));

    SailVaultSettings::initialize();

    qmlRegisterType<WalletCoreProbe>(
        "org.sailfishos.sailvault", 1, 0, "WalletCoreProbe");
    qmlRegisterType<WalletVault>(
        "org.sailfishos.sailvault", 1, 0, "WalletVault");
    qmlRegisterType<PortfolioService>(
        "org.sailfishos.sailvault", 1, 0, "PortfolioService");
    qmlRegisterType<PortfolioHistoryService>(
        "org.sailfishos.sailvault", 1, 0, "PortfolioHistoryService");
    qmlRegisterType<AppTools>(
        "org.sailfishos.sailvault", 1, 0, "AppTools");
    qmlRegisterType<ActivityService>(
        "org.sailfishos.sailvault", 1, 0, "ActivityService");
    qmlRegisterType<TokenService>(
        "org.sailfishos.sailvault", 1, 0, "TokenService");
    qmlRegisterType<PriceService>(
        "org.sailfishos.sailvault", 1, 0, "PriceService");
    qmlRegisterType<SessionSecurity>(
        "org.sailfishos.sailvault", 1, 0, "SessionSecurity");
    qmlRegisterType<TransactionDetailService>(
        "org.sailfishos.sailvault", 1, 0, "TransactionDetailService");
    qmlRegisterType<WatchOnlyService>(
        "org.sailfishos.sailvault", 1, 0, "WatchOnlyService");
    qmlRegisterType<AddressBookService>(
        "org.sailfishos.sailvault", 1, 0, "AddressBookService");
    qmlRegisterType<NetworkHealthService>(
        "org.sailfishos.sailvault", 1, 0, "NetworkHealthService");
    qmlRegisterType<NetworkFeeService>(
        "org.sailfishos.sailvault", 1, 0, "NetworkFeeService");

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // Offline QR rendering. The provider receives public addresses only.
    view->engine()->addImageProvider(
        QStringLiteral("sailvaultqr"),
        new QrImageProvider());

    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
