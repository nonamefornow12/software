#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application style
    app.setStyle("Fusion");

    // Load Maven Pro Bold 700 font as requested
    QString fontPath = ":/assets/fonts/MavenPro-VariableFont_wght.ttf";
    if (QFile::exists(fontPath)) {
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
            if (!fontFamilies.empty()) {
                QFont font(fontFamilies.at(0));
                font.setWeight(QFont::Bold); // Set to bold weight (700)
                QApplication::setFont(font);
            }
        }
    }

    // Create directories for assets if they don't exist
    QDir assetsDir(QApplication::applicationDirPath() + "/assets");
    if (!assetsDir.exists()) {
        assetsDir.mkpath(".");
    }

    MainWindow w;
    w.show();

    return app.exec();
}
