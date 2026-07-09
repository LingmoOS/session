#include <LingmoSession/SessionManager.h>
#include <LingmoSession/Component.h>
#include <QTest>
#include <QSignalSpy>
class tst_SessionManager : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testInstance() {
        auto *mgr = Lingmo::SessionManager::instance();
        QVERIFY(mgr != nullptr);
        QCOMPARE(Lingmo::SessionManager::instance(), mgr);
    }
    void testRegisterComponent() {
        auto *mgr = new Lingmo::SessionManager(this);
        auto *comp = new Lingmo::Component(QStringLiteral("test"), QStringLiteral("/bin/true"), {}, mgr);
        mgr->registerComponent(comp);
        QCOMPARE(mgr->componentNames().size(), 1);
        QCOMPARE(mgr->component(QStringLiteral("test")), comp);
        delete mgr;
    }
    void testStartSession() {
        auto *mgr = new Lingmo::SessionManager(this);
        QSignalSpy readySpy(mgr, &Lingmo::SessionManager::sessionReady);
        auto *comp = new Lingmo::Component(QStringLiteral("test"), QStringLiteral("/bin/true"), {}, mgr);
        mgr->registerComponent(comp);
        mgr->startSession();
        QCOMPARE(mgr->state(), Lingmo::SessionState::Running);
        delete mgr;
    }
    void testStopSession() {
        auto *mgr = new Lingmo::SessionManager(this);
        auto *comp = new Lingmo::Component(QStringLiteral("test"), QStringLiteral("/bin/true"), {}, mgr);
        mgr->registerComponent(comp);
        mgr->startSession();
        QSignalSpy stoppedSpy(mgr, &Lingmo::SessionManager::sessionStopped);
        mgr->stopSession();
        QCOMPARE(stoppedSpy.count(), 1);
        delete mgr;
    }
    void testComponentsRunning() {
        auto *mgr = new Lingmo::SessionManager(this);
        auto *c1 = new Lingmo::Component(QStringLiteral("c1"), QStringLiteral("/bin/true"), {}, mgr);
        auto *c2 = new Lingmo::Component(QStringLiteral("c2"), QStringLiteral("/bin/true"), {}, mgr);
        mgr->registerComponent(c1);
        mgr->registerComponent(c2);
        mgr->startSession();
        QCOMPARE(mgr->componentsRunning(), 2);
        delete mgr;
    }
};
QTEST_MAIN(tst_SessionManager)
#include "tst_SessionManager.moc"
