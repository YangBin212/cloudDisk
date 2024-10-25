#include "tcpclient.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFont font("Times", 24, QFont::Bold);
    a.setFont(font);

    TcpClient::getInstance().show();

    return a.exec();
}
