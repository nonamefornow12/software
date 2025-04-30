#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QEvent>
#include <QPoint>
#include <QSvgRenderer>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QFontDatabase>

// Helper class to render SVG
class SvgPainter : public QObject
{
    Q_OBJECT
public:
    SvgPainter(QSvgRenderer* renderer, QObject* parent = nullptr)
        : QObject(parent), renderer(renderer) {}

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::Paint) {
            QWidget* widget = static_cast<QWidget*>(watched);
            QPainter painter(widget);
            renderer->render(&painter, QRectF(0, 0, widget->width(), widget->height()));
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QSvgRenderer* renderer;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogoDownloaded(QNetworkReply *reply);
    void onProfilePictureClicked();
    void onLogoClicked();
    void onMenuButtonClicked();
    void onMenuTextClicked();
    void onMinimizeClicked();
    void updateCenterContent(const QString &tabName);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void createSidebar();
    void setupFonts();
    void downloadLogo();
    void applyProfilePicture(const QString &imagePath);
    void loadProfilePicture();
    QPushButton* createMenuButton(const QString &icon, const QString &text, bool isHomeIcon = false);
    QWidget* createFreeSubscriptionBadge(bool small = false);
    QWidget* createModernDivider();
    QWidget* createThreeDotsButton(bool smaller = false);
    void activateTab(const QString &tabName);
    void collapseSidebar();
    void expandSidebar();
    void updateMinimizeButtonIcon(bool minimized);

    // Network manager for downloading logo
    QNetworkAccessManager *networkManager;

    // Sidebar components
    QFrame *sidebarFrame;
    QVBoxLayout *sidebarLayout;
    int expandedSidebarWidth = 200;
    int collapsedSidebarWidth = 70;
    bool isCollapsed = false;

    // Font IDs
    int poppinsBoldId = -1;
    int poppinsMediumId = -1;
    QString poppinsBoldFamily;
    QString poppinsMediumFamily;

    // Content area
    QFrame *contentArea;
    QLabel *contentTitleLabel;

    // UI elements we need to access later
    QLabel *logoLabel;
    QPushButton *profilePicBtn;
    QWidget *minimizeBtn;
    QWidget *threeDots;
    QLabel *appNameLabel;
    QString currentTab;
    QMap<QString, QPushButton*> menuButtons;
    QMap<QString, QLabel*> menuTexts;
    QFrame *subscriptionPanel;
    QLabel *emailLabel;
    QLabel *usernameLabel;
    QWidget *profileSettingsBtn;
    QWidget *freeSubscriptionLabel;
    QWidget *subscriptionDotsBtn;
    QWidget *modernDivider;
    QPropertyAnimation *sidebarAnim;
    QPropertyAnimation *minBtnAnim;
    QPropertyAnimation *iconAnim;

    // For window dragging
    QPoint dragPosition;
    // SVG Content
    QString minimizeSvgContent;
    QString expandSvgContent;
};

#endif // MAINWINDOW_H
