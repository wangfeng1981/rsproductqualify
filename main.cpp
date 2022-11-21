#include "mainwindow.h"

#include <QApplication>
#include "wprocessqueue.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    wProcessQueueAutoRelease autorelease ;
    MainWindow w;
    w.show();
    return a.exec();
}
