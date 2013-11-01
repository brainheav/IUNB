#include "iunb.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    bool con = true;
    while (con)
    {
        con = false;

        QApplication a(argc, argv);
        IUNB window;
        window.show();

        try
        {
            return a.exec();
        }
        catch (std::exception & ref)
        {
            QMessageBox::critical(nullptr, "App will be reloaded",
                                  ref.what());
            con = true;
        }
    }
}

