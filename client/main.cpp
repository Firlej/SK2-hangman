#include <QApplication>
#include "window.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Window Win;
    Win.show();
    return a.exec();
}
