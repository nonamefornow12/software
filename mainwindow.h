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
#include <QResizeEvent>
#include <QHash>
#include <QVariant>

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
    void storeProfilePosition();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void createSidebar();
    void setupFonts();
    void downloadLogo();
    void applyProfilePicture(const QString &imagePath);
    void loadProfilePicture();
    void prepareMinimizeButtonImages();
    QPushButton* createMenuButton(const QString &icon, const QString &text, bool isHomeIcon = false);
    QLabel* createCrispMinimizeButton(bool forExpandedMode);
    QWidget* createFreeSubscriptionBadge(bool small = false);
    QWidget* createModernDivider(bool minimized = false);
    QWidget* createThreeDotsButton(bool smaller = false);
    void activateTab(const QString &tabName);
    void collapseSidebar();
    void expandSidebar();

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

    // Pre-rendered button images for crisp display
    QPixmap expandButtonImage;
    QPixmap minimizeButtonImage;

    // UI elements we need to access later
    QLabel *logoLabel;
    QWidget *profilePicContainer;
    QPushButton *profilePicBtn;
    QLabel *expandMinimizeBtn;    // Ultra-sharp minimize button for expanded mode
    QLabel *collapseMinimizeBtn;  // Ultra-sharp expand button for collapsed mode
    QWidget *minimizeButtonContainer;  // Container for collapsed sidebar minimize button
    QWidget *buttonsContainer;         // Container for the buttons in expanded sidebar
    QHBoxLayout *buttonLayout;         // Layout for the buttons in expanded sidebar
    QWidget *threeDots;
    QLabel *appNameLabel;
    QString currentTab;
    QMap<QString, QPushButton*> menuButtons;
    QMap<QString, QLabel*> menuTexts;
    QFrame *subscriptionPanel;
    QHBoxLayout *panelContainer;
    QHBoxLayout *profileLayout;
    QLabel *emailLabel;
    QLabel *usernameLabel;
    QWidget *profileSettingsBtn;
    QWidget *freeSubscriptionLabel;
    QWidget *subscriptionDotsBtn;
    QWidget *modernDivider;
    QWidget *minimizedDivider;
    QPropertyAnimation *sidebarAnim;
    QPropertyAnimation *minBtnAnim;
    QPropertyAnimation *iconAnim;
    QWidget *collapsedContainer;   // Container for collapsed sidebar elements

    // Profile position tracking
    QHash<QString, QVariant> profilePicOrig;
    QWidget* originalProfileParent = nullptr; // Original parent widget

    // SVG content
    QString minimizeSvgContent;
    QString expandSvgContent;
    QString freeSubscriptionSvgContent;

    // For window dragging
    QPoint dragPosition;
};

#endif // MAINWINDOW_H
