#include <LingmoSession/Component.h>
#include <QTest>
class tst_Component : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testCreate() {
        Lingmo::Component comp(QStringLiteral("test"), QStringLiteral("/bin/true"));
        QCOMPARE(comp.name(), QStringLiteral("test"));
        QCOMPARE(comp.command(), QStringLiteral("/bin/true"));
        QVERIFY(comp.dependencies().isEmpty());
        QCOMPARE(comp.state(), Lingmo::ComponentState::Stopped);
    }
    void testDependencies() {
        Lingmo::Component comp(QStringLiteral("app"),
            QStringLiteral("/bin/true"),
            QStringList() << QStringLiteral("dep1") << QStringLiteral("dep2"));
        QCOMPARE(comp.dependencies().size(), 2);
    }
    void testDependenciesSatisfied() {
        Lingmo::Component comp(QStringLiteral("app"),
            QStringLiteral("/bin/true"),
            QStringList() << QStringLiteral("dep1"));
        QVERIFY(!comp.dependenciesSatisfied(QStringList()));
        QVERIFY(comp.dependenciesSatisfied(QStringList() << QStringLiteral("dep1")));
    }
    void testDependenciesEmptyAlwaysSatisfied() {
        Lingmo::Component comp(QStringLiteral("app"), QStringLiteral("/bin/true"));
        QVERIFY(comp.dependenciesSatisfied(QStringList()));
    }
};
QTEST_MAIN(tst_Component)
#include "tst_Component.moc"
