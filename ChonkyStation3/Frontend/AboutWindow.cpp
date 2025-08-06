#include "AboutWindow.hpp"


AboutWindow::AboutWindow(QWidget* parent) : QWidget(parent, Qt::Window) {
    ui.setupUi(this);

    setWindowTitle("About ChonkyStation3");
    hide();
}
