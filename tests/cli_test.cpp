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
    QString map(const QString &dir, int width = 15, int rows = 17) {
        QString path = dir + "/test.map";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        file.write(QString("N:test\nT:2\nS:%1,17\n").arg(width).toUtf8());
        for (int y = 0; y < rows; ++y) file.write(("D:" + (QString("0,").repeated(width - 1) + "0") + "\n").toUtf8());
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
    void malformed_data() { QTest::addColumn<int>("rows"); QTest::newRow("行数不足を拒否する") << 16; QTest::newRow("行数超過を拒否する") << 18; }
    void malformed() { QFETCH(int, rows); QTemporaryDir dir; QProcess p; p.start(binary(), {map(dir.path(), 15, rows), "--headless"}); QVERIFY(p.waitForFinished(5000)); QCOMPARE(p.exitCode(), 2); }
    void malformedHeader_data() { QTest::addColumn<QByteArray>("header"); QTest::newRow("空の名前を拒否する") << QByteArray("N:\n"); QTest::newRow("予約された名前を拒否する") << QByteArray("N:D:0\n"); QTest::newRow("不正な接頭辞を拒否する") << QByteArray("Cjunk\n"); QTest::newRow("空行を拒否する") << QByteArray("\n"); QTest::newRow("長すぎる行を拒否する") << (QByteArray("N:") + QByteArray(1024, 'x') + "\n"); }
    void malformedHeader() {
        QFETCH(QByteArray, header); QTemporaryDir dir; const QString path = map(dir.path());
        QFile file(path); QVERIFY(file.open(QIODevice::Append)); file.write(header); file.close();
        QProcess p; p.start(binary(), {path, "--headless"}); QVERIFY(p.waitForFinished(5000)); QCOMPARE(p.exitCode(), 2);
    }
    void match_data() { QTest::addColumn<int>("width"); QTest::addColumn<bool>("failure"); QTest::newRow("15幅の対戦が完了する") << 15 << false; QTest::newRow("21幅の対戦が完了する") << 21 << false; QTest::newRow("不正な応答をクライアント異常として報告する") << 15 << true; }
    void match() {
        QFETCH(int, width); QFETCH(bool, failure);
        QTemporaryDir dir; QVERIFY(dir.isValid());
        QProcess p; p.setWorkingDirectory(dir.path()); p.start(binary(), {map(dir.path(), width), "--headless"});
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
                        sockets[i].write(failure && i == 0 ? "xx\r\n" : "gr\r\n"); phase[i] = 1;
                    } else if (phase[i] == 1) {
                        if (line.startsWith('0')) done[i] = true;
                        else { sockets[i].write("lu\r\n"); phase[i] = 2; }
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
        if (failure) {
            QCOMPARE(p.exitCode(), 3);
            QVERIFY(p.readAllStandardError().contains("ERROR CLIENT_FAILURE COOL"));
            QVERIFY(!p.readAllStandardOutput().contains("RESULT "));
        } else {
            QCOMPARE(p.exitCode(), 0);
            QVERIFY2(p.readAllStandardOutput().contains("RESULT DRAW 0 0"), "対戦結果がありません");
        }
    }
};
QTEST_GUILESS_MAIN(CliTest)
#include "cli_test.moc"
