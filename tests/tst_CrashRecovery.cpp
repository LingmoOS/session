#include <LingmoSession/CrashRecovery.h>
#include <QTest>
#include <QSignalSpy>
class tst_CrashRecovery : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testRecordCrash() {
        Lingmo::CrashRecovery cr;
        cr.recordCrash(QStringLiteral("comp1"));
        QCOMPARE(cr.crashCount(QStringLiteral("comp1")), 1);
        cr.recordCrash(QStringLiteral("comp1"));
        QCOMPARE(cr.crashCount(QStringLiteral("comp1")), 2);
    }
    void testCrashLimit() {
        Lingmo::CrashRecovery cr;
        cr.setMaxCrashes(2);
        QSignalSpy spy(&cr, &Lingmo::CrashRecovery::crashLimitExceeded);
        cr.recordCrash(QStringLiteral("comp1"));
        QCOMPARE(spy.count(), 0);
        cr.recordCrash(QStringLiteral("comp1"));
        QCOMPARE(spy.count(), 1);
        QVERIFY(cr.isCrashLimitExceeded(QStringLiteral("comp1")));
    }
    void testReset() {
        Lingmo::CrashRecovery cr;
        cr.recordCrash(QStringLiteral("comp1"));
        cr.resetCrashCount(QStringLiteral("comp1"));
        QCOMPARE(cr.crashCount(QStringLiteral("comp1")), 0);
    }
    void testResetAll() {
        Lingmo::CrashRecovery cr;
        cr.recordCrash(QStringLiteral("a"));
        cr.recordCrash(QStringLiteral("b"));
        cr.resetAll();
        QCOMPARE(cr.crashCount(QStringLiteral("a")), 0);
        QCOMPARE(cr.crashCount(QStringLiteral("b")), 0);
    }
};
QTEST_MAIN(tst_CrashRecovery)
#include "tst_CrashRecovery.moc"
