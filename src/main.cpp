#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QTimer>
#include <QPushButton>
#include <QGroupBox>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
#include <cstdio>

static bool validMap(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
    if (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
    int width = 0, height = 0, rows = 0, turns = 0;
    bool sizeSeen = false, turnSeen = false, coolSeen = false, hotSeen = false;
    QPoint cool(-1, -1), hot(-1, -1);
    for (QString line : lines) {
        if (line.endsWith('\r')) line.chop(1);
        if (line.isEmpty() || line.toUtf8().size() >= 1022) return false;
        if (line != line.trimmed()) return false;
        if (line.startsWith("N:")) {
            const QString name = line.mid(2);
            if (name.isEmpty() || QString("TDCH").contains(name[0])) return false;
        } else if (line.startsWith("S:")) {
            if (sizeSeen || rows) return false;
            sizeSeen = true;
            const auto parts = line.mid(2).split(',');
            if (parts.size() != 2) return false;
            bool a, b; width = parts[0].toInt(&a); height = parts[1].toInt(&b);
            if (!a || !b) return false;
        } else if (line.startsWith("T:")) {
            if (turnSeen) return false;
            turnSeen = true;
            bool ok; turns = line.mid(2).toInt(&ok); if (!ok) return false;
        } else if (line.startsWith("D:")) {
            const auto cells = line.mid(2).split(',');
            if (cells.size() != width || (width != 15 && width != 21)) return false;
            for (const auto &cell : cells) if (cell != "0" && cell != "2" && cell != "3") return false;
            ++rows;
        } else if (line.startsWith("C:") || line.startsWith("H:")) {
            bool &seen = line[0] == 'C' ? coolSeen : hotSeen;
            if (seen) return false;
            seen = true;
            const auto parts = line.mid(2).split(',');
            if (parts.size() != 2) return false;
            bool a, b; int x = parts[0].toInt(&a), y = parts[1].toInt(&b);
            if (!a || !b) return false;
            (line[0] == 'C' ? cool : hot) = QPoint(x, y);
        } else {
            return false;
        }
    }
    const auto inside = [width, height](QPoint p) { return p.x() >= 0 && p.x() < width && p.y() >= 0 && p.y() < height; };
    return (width == 15 || width == 21) && height == 17 && rows == height && turns > 0 && inside(cool) && inside(hot) && cool != hot;
}

int main(int argc, char *argv[])
{
    bool headless = false;
    for (int i = 1; i < argc; ++i) if (QByteArray(argv[i]) == "--headless") headless = true;
    if (headless) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication a(argc, argv);
    if (headless && (argc != 3 || QByteArray(argv[2]) != "--headless" || !validMap(QString::fromLocal8Bit(argv[1])))) {
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
