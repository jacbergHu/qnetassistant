#include "qnetassistant.h"
#include <QApplication>
#include <QFile>
#include <QtGlobal>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

#ifdef Q_OS_LINUX
    QFile file(QString(":/images/stylesheet/LinuxStyle.qss"));
#endif
#ifdef Q_OS_WIN
    QFile file(QString(":/images/stylesheet/WindowsStyle.qss"));
#endif
    if(file.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }

    QNetAssistant w;
    w.show();

    return a.exec();
}
