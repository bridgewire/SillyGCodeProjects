#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    int retv = 0;

    try
    {
        QApplication a(argc, argv);
        mainwindow w;
        w.show();
        retv = a.exec();
    }
    catch( const char * errstr )
    {
        printf( "died ... caught error: %s\n", errstr );
    }

    return retv;
}
