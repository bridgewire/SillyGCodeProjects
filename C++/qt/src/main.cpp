#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    int retv = 0;

    QApplication a(argc, argv);

    mainwindow w;

    QCommandLineParser parser;
    parser.setApplicationDescription("Viewer");
    parser.addHelpOption();
    QCommandLineOption threadopt( QStringList() << "t" << "threads", "The number of worker threads to use.", "threadcnt");
    parser.addOption(threadopt);
    parser.process(a);

    bool isOk = true;
    int threadcnt = parser.value(threadopt).toInt(&isOk);
    if( isOk )
    {
        printf( "setting option: threadcnt: %d\n", threadcnt );
        w.set_threadcnt( threadcnt );
    }

    try
    {
        w.show();
        w.toggle_timer();
        retv = a.exec();
    }
    catch( const char * errstr )
    {
        printf( "died ... caught error: %s\n", errstr );
    }

    printf( "main is returning: %d\n", retv );

    return retv;
}
