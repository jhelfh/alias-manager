#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Alias Manager");
    window.resize(700, 450);
    window.show();

    return app.exec();
}