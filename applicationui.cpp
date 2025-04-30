#include "applicationui.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

ApplicationUI::ApplicationUI(QObject *parent) : QObject(parent)
{
}

void ApplicationUI::openFileDialog()
{
    // Open native file dialog
    QString picturePath = QFileDialog::getOpenFileName(
        nullptr,
        tr("Select Profile Picture"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        tr("Image Files (*.png *.jpg *.jpeg)")
        );

    if (!picturePath.isEmpty()) {
        QUrl fileUrl = QUrl::fromLocalFile(picturePath);
        setProfileImage(fileUrl);
    }
}

void ApplicationUI::setProfileImage(const QUrl &imageUrl)
{
    // Process and emit the new profile image URL
    // Here you would typically resize/crop the image if needed
    qDebug() << "New profile image set:" << imageUrl.toString();
    emit profileImageChanged(imageUrl);
}
