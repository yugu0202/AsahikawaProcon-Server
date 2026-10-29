#include <QtTest>
#include <QFile>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>

class CliTest : public QObject {
    Q_OBJECT
    QString binary() { return qEnvironmentVariable("CHASER_SERVER_BINARY"); }
    QString map(const QString &dir) {
        QString path = dir + "/test.map";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        file.write("N:test\nT:2\nS:15,17\n");
        for (int y = 0; y < 17; ++y) file.write("D:0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n");
        file.write("C:1,1\nH:13,15\n");
        return path;
    }
private slots:
    void invalid_data() { QTest::addColumn<int>("unused"); QTest::newRow("存在しないマップはエラーで終了する") << 0; }
    void invalid() {
        QFETCH(int, unused); Q_UNUSED(unused);
        QProcess p; p.start(binary(), {"/not-found.map", "--headless"});
        QVERIFY(p.waitForFinished(5000)); QCOMPARE(p.exitCode(), 2);
    }
    void occupied_data() { QTest::addColumn<int>("unused"); QTest::newRow("使用中のポートではエラーで終了する") << 0; }
    void occupied() {
        QFETCH(int, unused); Q_UNUSED(unused);
        QTemporaryDir dir; QVERIFY(dir.isValid());
        QTcpServer listener; QVERIFY(listener.listen(QHostAddress::Any, 2009));
        QProcess p; p.setWorkingDirectory(dir.path()); p.start(binary(), {map(dir.path()), "--headless"});
        QVERIFY(p.waitForFinished(10000)); QCOMPARE(p.exitCode(), 2);
    }
    void match_data() { QTest::addColumn<int>("unused"); QTest::newRow("固定マップの対戦が完了し結果を出力する") << 0; }
    void match() {
        QFETCH(int, unused); Q_UNUSED(unused);
        QTemporaryDir dir; QVERIFY(dir.isValid());
        QProcess p; p.setWorkingDirectory(dir.path()); p.start(binary(), {map(dir.path()), "--headless"});
        QVERIFY(p.waitForStarted());
        QTcpSocket sockets[2]; int phase[2] = {0, 0}; bool done[2] = {false, false};
        QTimer retry;
        connect(&retry, &QTimer::timeout, this, [&]() {
            for (int i = 0; i < 2; ++i)
                if (!done[i] && sockets[i].state() == QAbstractSocket::UnconnectedState)
                    sockets[i].connectToHost(QHostAddress::LocalHost, 2009 + i);
        });
        for (int i = 0; i < 2; ++i) {
            connect(&sockets[i], &QTcpSocket::connected, this, [&, i]() { sockets[i].write("test\r\n"); });
            connect(&sockets[i], &QTcpSocket::readyRead, this, [&, i]() {
                while (sockets[i].canReadLine()) {
                    QByteArray line = sockets[i].readLine();
                    if (phase[i] == 0) {
                        if (line != "@\r\n") { done[i] = true; return; }
                        sockets[i].write("gr\r\n"); phase[i] = 1;
                    } else if (phase[i] == 1) {
                        sockets[i].write("lu\r\n");
                        if (line.startsWith('0')) done[i] = true;
                        else phase[i] = 2;
                    } else {
                        sockets[i].write("#\r\n");
                        if (line.startsWith('0')) done[i] = true;
                        else phase[i] = 0;
                    }
                }
            });
        }
        retry.start(100);
        QTRY_VERIFY_WITH_TIMEOUT(p.state() == QProcess::NotRunning, 35000);
        QCOMPARE(p.exitCode(), 0);
        QVERIFY2(p.readAllStandardOutput().contains("RESULT DRAW 0 0"), "対戦結果がありません");
    }
};
QTEST_GUILESS_MAIN(CliTest)
#include "cli_test.moc"
