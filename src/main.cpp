#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QTimer>
#include <QPushButton>
#include <QGroupBox>
#include <QFileInfo>
#include <cstdio>

int main(int argc, char *argv[])
{
    bool headless = false;
    for (int i = 1; i < argc; ++i) if (QByteArray(argv[i]) == "--headless") headless = true;
    if (headless) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication a(argc, argv);
    if (headless && (argc != 3 || QByteArray(argv[2]) != "--headless" || !QFileInfo(QString::fromLocal8Bit(argv[1])).isFile())) {
        std::fprintf(stderr, "マップファイルを指定してください\n");
        return 2;
    }
    if (headless) {
        auto *timer = new QTimer(&a);
        QObject::connect(timer, &QTimer::timeout, [&a, timer]() {
            for (auto *widget : QApplication::topLevelWidgets()) {
                auto *cool = widget->findChild<QGroupBox *>("CoolGroupBox");
                auto *hot = widget->findChild<QGroupBox *>("HotGroupBox");
                auto *start = widget->findChild<QPushButton *>("ServerStartButton");
                if (!cool || !hot || !start) continue;
                auto *c = cool->findChild<QPushButton *>("ConnectButton");
                auto *h = hot->findChild<QPushButton *>("ConnectButton");
                if (!c || !h) { a.exit(2); return; }
                if (!c->isChecked()) c->click();
                if (!h->isChecked()) h->click();
                if (start->isEnabled()) { timer->stop(); start->click(); }
            }
        });
        timer->start(100);
    }
    // ダークテーマに変わらないように設定
    // (デフォルト:windows11 -> windowsvista)
    a.setStyle("windowsvista");

    QTranslator qtTranslator;

    // 返り値を無視
    (void) qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    MainWindow w;
    if (!headless) w.show();

    return a.exec();
}
