#include <QtGui>
#include <QtWebKit>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWebView view;
//    view.show();
    view.showFullScreen();
    view.setUrl(QUrl("http://127.0.0.1"));
    return app.exec();
}

