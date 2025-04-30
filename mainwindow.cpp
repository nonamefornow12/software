#include "mainwindow.h"
#include <QPixmap>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMouseEvent>
#include <QDateTime>
#include <QSvgRenderer>
#include <QSettings>
#include <QTimer>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QFile>
#include <QDesktopWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)), currentTab("Dashboard")
{
    // Remove title bar but keep window frame
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // Set a stronger visible border around the main window (1px solid gray border)
    setStyleSheet("MainWindow { border: 1px solid #999999; }");

    // Set fixed border using a frame
    QFrame* borderFrame = new QFrame(this);
    borderFrame->setFrameShape(QFrame::Box);
    borderFrame->setFrameShadow(QFrame::Plain);
    borderFrame->setLineWidth(1); // 1px line width
    borderFrame->setStyleSheet("QFrame { border: 1px solid #999999; }"); // Darker gray for visibility
    borderFrame->setGeometry(0, 0, width(), height());

    // Setup custom fonts
    setupFonts();

    setupUi();
    createSidebar();
    downloadLogo();
    loadProfilePicture(); // Load saved profile picture on startup

    // Install event filter to resize frame when window resizes
    installEventFilter(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupFonts()
{
    // Load Poppins SemiBold and Medium fonts
    QString fontPath = "C:/Users/sem/Documents/untitled8/poppins.semibold.tff";
    if (QFile::exists(fontPath)) {
        poppinsFontId = QFontDatabase::addApplicationFont(fontPath);
        if (poppinsFontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(poppinsFontId);
            if (!families.isEmpty()) {
                poppinsFontFamily = families.at(0);
            }
        }
    }
}

void MainWindow::setupUi()
{
    setWindowTitle("rhynecsecurity");
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int width = screenGeometry.width() * 0.8;
    int height = screenGeometry.height() * 0.8;
    resize(width, height);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    sidebarFrame = new QFrame(centralWidget);
    sidebarFrame->setObjectName("sidebarFrame");
    sidebarFrame->setStyleSheet("QFrame#sidebarFrame { background-color: white; border-right: 1px solid #e0e0e0; }");
    sidebarFrame->setFixedWidth(expandedSidebarWidth);

    contentArea = new QFrame(centralWidget);
    contentArea->setObjectName("contentArea");
    contentArea->setStyleSheet("QFrame#contentArea { background-color: white; }");

    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(10);

    contentTitleLabel = new QLabel(currentTab, contentArea);
    QFont titleFont = contentTitleLabel->font();
    titleFont.setWeight(QFont::Bold);
    titleFont.setPixelSize(32);
    contentTitleLabel->setFont(titleFont);
    contentTitleLabel->setAlignment(Qt::AlignCenter);
    contentTitleLabel->setStyleSheet("color: #333333;");

    contentLayout->addWidget(contentTitleLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);

    sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(10, 25, 8, 15);
    sidebarLayout->setSpacing(9);

    mainLayout->addWidget(sidebarFrame);
    mainLayout->addWidget(contentArea, 1);

    setCentralWidget(centralWidget);
}

QPushButton* MainWindow::createMenuButton(const QString &icon, const QString &text, bool isHomeIcon)
{
    QPushButton *button = new QPushButton("", sidebarFrame);

    QString normalIcon = icon;
    QString activeIcon = icon;
    activeIcon.replace(".svg", "-2.svg");

    button->setProperty("normalIcon", normalIcon);
    button->setProperty("activeIcon", activeIcon);
    button->setProperty("isActive", false);
    button->setProperty("tabName", text);

    button->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   border-radius: 4px;"
        "   background-color: #f8f8f8;"
        "   padding: 8px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e8e8e8; }"
        );

    button->setCursor(Qt::PointingHandCursor);

    connect(button, &QPushButton::clicked, this, &MainWindow::onMenuButtonClicked);

    QFileInfo check_file(normalIcon);
    if (!check_file.exists()) {
        qDebug() << "Initial icon file not found:" << normalIcon;
    }

    QSvgRenderer renderer(normalIcon);

    QSize iconSize;
    if (isHomeIcon) {
        iconSize = QSize(30, 30);
    } else {
        iconSize = QSize(28, 28);
    }

    qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap(iconSize * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRectF viewBox = renderer.viewBoxF();

    qreal xScale = iconSize.width() / viewBox.width();
    qreal yScale = iconSize.height() / viewBox.height();
    qreal scaleFactor = qMin(xScale, yScale) * 0.9;

    qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
    qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

    painter.translate(xOffset, yOffset);
    painter.scale(scaleFactor, scaleFactor);

    renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
    painter.end();

    button->setIcon(QIcon(pixmap));
    button->setIconSize(iconSize);
    button->setProperty("iconSize", iconSize);

    return button;
}

void MainWindow::onMenuButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;

    QString tabName = button->property("tabName").toString();
    if (!tabName.isEmpty()) {
        activateTab(tabName);
    }

    bool isActive = true;
    button->setProperty("isActive", isActive);

    QString iconPath = button->property("activeIcon").toString();

    QFileInfo check_file(iconPath);
    if (!check_file.exists()) {
        qDebug() << "Icon file not found:" << iconPath;
        return;
    }

    QSvgRenderer renderer(iconPath);
    QSize iconSize = button->property("iconSize").toSize();

    qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap(iconSize * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRectF viewBox = renderer.viewBoxF();

    qreal xScale = iconSize.width() / viewBox.width();
    qreal yScale = iconSize.height() / viewBox.height();
    qreal scaleFactor = qMin(xScale, yScale) * 0.9;

    qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
    qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

    painter.translate(xOffset, yOffset);
    painter.scale(scaleFactor, scaleFactor);

    renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
    painter.end();

    button->setIcon(QIcon(pixmap));
}

void MainWindow::onMenuTextClicked()
{
    QLabel* label = qobject_cast<QLabel*>(sender());
    if (!label)
        return;

    QString tabName = label->text();
    activateTab(tabName);
}

void MainWindow::activateTab(const QString &tabName)
{
    foreach (const QString &key, menuButtons.keys()) {
        QPushButton* btn = menuButtons[key];
        QString normalIcon = btn->property("normalIcon").toString();

        if (key != tabName) {
            btn->setProperty("isActive", false);

            QFileInfo check_file(normalIcon);
            if (check_file.exists()) {
                QSvgRenderer renderer(normalIcon);
                QSize iconSize = btn->property("iconSize").toSize();

                qreal dpr = qApp->devicePixelRatio();
                QPixmap pixmap(iconSize * dpr);
                pixmap.fill(Qt::transparent);
                pixmap.setDevicePixelRatio(dpr);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                QRectF viewBox = renderer.viewBoxF();

                qreal xScale = iconSize.width() / viewBox.width();
                qreal yScale = iconSize.height() / viewBox.height();
                qreal scaleFactor = qMin(xScale, yScale) * 0.9;

                qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
                qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

                painter.translate(xOffset, yOffset);
                painter.scale(scaleFactor, scaleFactor);

                renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
                painter.end();

                btn->setIcon(QIcon(pixmap));
            }
        } else {
            btn->setProperty("isActive", true);

            QString activeIcon = btn->property("activeIcon").toString();

            QFileInfo check_file(activeIcon);
            if (check_file.exists()) {
                QSvgRenderer renderer(activeIcon);
                QSize iconSize = btn->property("iconSize").toSize();

                qreal dpr = qApp->devicePixelRatio();
                QPixmap pixmap(iconSize * dpr);
                pixmap.fill(Qt::transparent);
                pixmap.setDevicePixelRatio(dpr);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                QRectF viewBox = renderer.viewBoxF();

                qreal xScale = iconSize.width() / viewBox.width();
                qreal yScale = iconSize.height() / viewBox.height();
                qreal scaleFactor = qMin(xScale, yScale) * 0.9;

                qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
                qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

                painter.translate(xOffset, yOffset);
                painter.scale(scaleFactor, scaleFactor);

                renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
                painter.end();

                btn->setIcon(QIcon(pixmap));
            }
        }
    }

    currentTab = tabName;
    updateCenterContent(tabName);
}

void MainWindow::updateCenterContent(const QString &tabName)
{
    contentTitleLabel->setText(tabName);
}

QWidget* MainWindow::createFreeSubscriptionBadge()
{
    QWidget* badgeWidget = new QWidget();
    badgeWidget->setFixedSize(60, 24);

    // SVG content for FREE badge with text "FREE"
    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"24\" viewBox=\"0 0 60 24\">"
        "  <rect x=\"1\" y=\"1\" width=\"58\" height=\"22\" rx=\"8\" ry=\"8\" stroke=\"#222\" stroke-width=\"2\" fill=\"white\"/>"
        "  <text x=\"50%\" y=\"58%\" font-family=\"Arial, sans-serif\" font-size=\"13\" font-weight=\"bold\" text-anchor=\"middle\" fill=\"#111\" stroke=\"#111\" stroke-width=\"0.2\">FREE</text>"
        "</svg>";

    QVBoxLayout* layout = new QVBoxLayout(badgeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSvgRenderer* renderer = new QSvgRenderer(svgContent.toUtf8(), badgeWidget);
    QWidget* svgWidget = new QWidget();
    svgWidget->setFixedSize(60, 24);
    svgWidget->installEventFilter(new SvgPainter(renderer, svgWidget));

    layout->addWidget(svgWidget);

    badgeWidget->setCursor(Qt::PointingHandCursor);
    return badgeWidget;
}

QWidget* MainWindow::createModernDivider()
{
    QWidget* container = new QWidget();
    container->setFixedHeight(16);

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(8, 6, 8, 6);

    QFrame* divider = new QFrame();
    divider->setFixedHeight(1);
    divider->setStyleSheet(
        "QFrame {"
        "   background-color: #e0e0e0;"
        "   border: none;"
        "}"
        );

    layout->addWidget(divider);
    return container;
}

QWidget* MainWindow::createThreeDotsButton(bool smaller)
{
    QWidget* container = new QWidget();
    int size = smaller ? 16 : 28;
    container->setFixedSize(size, size);

    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg width=\"" + QString::number(size) + "px\" height=\"" + QString::number(size) + "px\" viewBox=\"0 0 24 24\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">"
                                                                             "<circle cx=\"12\" cy=\"5\" r=\"2\" fill=\"#222\"/>"
                                                                             "<circle cx=\"12\" cy=\"12\" r=\"2\" fill=\"#222\"/>"
                                                                             "<circle cx=\"12\" cy=\"19\" r=\"2\" fill=\"#222\"/>"
                                                                             "</svg>";

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSvgRenderer* renderer = new QSvgRenderer(svgContent.toUtf8(), container);
    QWidget* svgWidget = new QWidget();
    svgWidget->setFixedSize(size, size);
    svgWidget->installEventFilter(new SvgPainter(renderer, svgWidget));

    layout->addWidget(svgWidget);

    container->setCursor(Qt::PointingHandCursor);
    return container;
}

void MainWindow::updateMinimizeButtonIcon()
{
    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?><svg width=\"32\" height=\"32\" viewBox=\"0 0 32 32\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\"><rect x=\"8\" y=\"15\" width=\"16\" height=\"2.5\" rx=\"1.25\" fill=\"#333\"/></svg>";

    int size = 32;

    if (minimizeBtn->layout() == nullptr) {
        QVBoxLayout* layout = new QVBoxLayout(minimizeBtn);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    } else {
        QLayoutItem *item;
        while ((item = minimizeBtn->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }

    QSvgRenderer* minimizeRenderer = new QSvgRenderer(svgContent.toUtf8(), minimizeBtn);
    QWidget* minimizeSvgWidget = new QWidget();
    minimizeSvgWidget->setFixedSize(size, size);
    minimizeSvgWidget->installEventFilter(new SvgPainter(minimizeRenderer, minimizeSvgWidget));

    minimizeBtn->layout()->addWidget(minimizeSvgWidget);
}

void MainWindow::createSidebar()
{
    // Modern divider and subscription panel at the top
    QWidget* modernDividerWidget = createModernDivider();
    sidebarLayout->addWidget(modernDividerWidget);

    subscriptionPanel = new QFrame(sidebarFrame);
    subscriptionPanel->setObjectName("subscriptionPanel");
    subscriptionPanel->setFixedHeight(55); // Smaller height

    QHBoxLayout* panelContainer = new QHBoxLayout();
    panelContainer->setContentsMargins(8, 0, 8, 0);

    subscriptionPanel->setStyleSheet(
        "QFrame#subscriptionPanel {"
        "   background-color: white;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 15px;"
        "}"
        );

    QVBoxLayout* subscriptionPanelLayout = new QVBoxLayout(subscriptionPanel);
    subscriptionPanelLayout->setContentsMargins(8, 4, 8, 4);
    subscriptionPanelLayout->setSpacing(1);

    QHBoxLayout* topSubscriptionRow = new QHBoxLayout();
    topSubscriptionRow->setContentsMargins(0, 0, 0, 0);
    topSubscriptionRow->setSpacing(0);

    subscriptionDotsBtn = createThreeDotsButton(true);
    topSubscriptionRow->addStretch(1);
    topSubscriptionRow->addWidget(subscriptionDotsBtn, 0, Qt::AlignRight);

    subscriptionPanelLayout->addLayout(topSubscriptionRow);

    QHBoxLayout* badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(0, 0, 0, 0);
    badgeRow->setSpacing(0);

    freeSubscriptionLabel = createFreeSubscriptionBadge();
    badgeRow->addWidget(freeSubscriptionLabel);
    badgeRow->addStretch(1);

    subscriptionPanelLayout->addLayout(badgeRow);

    panelContainer->addWidget(subscriptionPanel);
    sidebarLayout->addLayout(panelContainer);

    sidebarLayout->addSpacing(6);

    // Logo and App name
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);
    topLayout->setContentsMargins(0, 0, 0, 0);

    logoLabel = new QLabel(sidebarFrame);
    logoLabel->setFixedSize(40, 40);
    logoLabel->setCursor(Qt::PointingHandCursor);
    logoLabel->setStyleSheet("margin-left: 2px; margin-top: 0px;"); // Allow full logo shape

    logoLabel->installEventFilter(this);

    appNameLabel = new QLabel("rhynecsecurity", sidebarFrame);
    QFont appNameFont;
    if (poppinsFontId != -1 && !poppinsFontFamily.isEmpty()) {
        appNameFont = QFont(poppinsFontFamily);
        appNameFont.setWeight(QFont::Bold); // Bold 700
    } else {
        appNameFont.setBold(true);
    }
    appNameFont.setPixelSize(22);
    appNameLabel->setFont(appNameFont);
    appNameLabel->setStyleSheet("margin-left: 4px;");

    topLayout->addWidget(logoLabel, 0, Qt::AlignVCenter);
    topLayout->addWidget(appNameLabel, 0, Qt::AlignVCenter);
    topLayout->addStretch(1);

    sidebarLayout->addLayout(topLayout);

    sidebarLayout->addSpacing(12);

    QStringList menuItems = {"Dashboard", "VPN", "Security", "Network", "Settings", "Profile"};
    QString developmentPath = "C:/Users/sem/Documents/untitled8/assets/";

    QStringList menuIcons = {
        developmentPath + "home.svg",
        developmentPath + "vpn.svg",
        developmentPath + "Security.svg",
        developmentPath + "Network.svg",
        developmentPath + "Settings.svg",
        developmentPath + "Profile.svg"
    };

    QFont poppinsMenuFont;
    if (poppinsFontId != -1 && !poppinsFontFamily.isEmpty()) {
        poppinsMenuFont = QFont(poppinsFontFamily);
        poppinsMenuFont.setWeight(QFont::Medium); // Medium 500
    } else {
        poppinsMenuFont = QFont("Poppins");
        poppinsMenuFont.setWeight(QFont::Medium);
    }
    poppinsMenuFont.setPixelSize(14);

    for (int i = 0; i < menuItems.size(); i++) {
        QWidget* menuItem = new QWidget();
        QHBoxLayout* menuItemLayout = new QHBoxLayout(menuItem);
        menuItemLayout->setContentsMargins(0, 0, 0, 0);
        menuItemLayout->setSpacing(0);

        QWidget* iconContainer = new QWidget();
        iconContainer->setFixedSize(36, 36);
        iconContainer->setStyleSheet("background-color: #f8f8f8; border-radius: 7px;");

        QHBoxLayout* iconLayout = new QHBoxLayout(iconContainer);
        iconLayout->setContentsMargins(0, 0, 0, 0);
        iconLayout->setSpacing(0);

        bool isHomeIcon = (i == 0);
        QPushButton* iconBtn = createMenuButton(menuIcons[i], menuItems[i], isHomeIcon);
        iconBtn->setFixedSize(32, 32);
        iconLayout->addWidget(iconBtn);

        menuButtons[menuItems[i]] = iconBtn;

        QLabel* textLabel = new QLabel(menuItems[i]);
        textLabel->setFont(poppinsMenuFont);
        textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        textLabel->setCursor(Qt::PointingHandCursor);
        textLabel->installEventFilter(this);
        menuTexts[menuItems[i]] = textLabel;

        QSpacerItem* spacer = new QSpacerItem(14, 10, QSizePolicy::Fixed, QSizePolicy::Minimum);

        menuItemLayout->addWidget(iconContainer, 0, Qt::AlignLeft);
        menuItemLayout->addSpacerItem(spacer);
        menuItemLayout->addWidget(textLabel, 0, Qt::AlignLeft);
        menuItemLayout->addStretch(1);

        sidebarLayout->addWidget(menuItem);

        if (i < menuItems.size() - 1) {
            sidebarLayout->addSpacing(10);
        }
    }

    activateTab("Dashboard");

    QSpacerItem* profileSpacer = new QSpacerItem(20, 22, QSizePolicy::Minimum, QSizePolicy::Expanding);
    sidebarLayout->addItem(profileSpacer);

    // Minimize button centered above profile pic
    minimizeBtn = new QWidget();
    minimizeBtn->setFixedSize(32, 32);
    minimizeBtn->setCursor(Qt::PointingHandCursor);
    minimizeBtn->setStyleSheet(
        "QWidget {"
        "   background-color: transparent;"
        "   border-radius: 5px;"
        "}"
        "QWidget:hover {"
        "   background-color: #f0f0f0;"
        "}"
        );
    QVBoxLayout* minimizeLayout = new QVBoxLayout(minimizeBtn);
    minimizeLayout->setContentsMargins(0, 0, 0, 0);
    minimizeLayout->setSpacing(0);

    updateMinimizeButtonIcon();

    minimizeBtn->installEventFilter(this);

    // Center minimizeBtn
    QHBoxLayout* minimizeRow = new QHBoxLayout();
    minimizeRow->setContentsMargins(0, 0, 0, 0);
    minimizeRow->setSpacing(0);
    minimizeRow->addStretch();
    minimizeRow->addWidget(minimizeBtn, 0, Qt::AlignCenter);
    minimizeRow->addStretch();
    sidebarLayout->addLayout(minimizeRow);

    sidebarLayout->addSpacing(5);

    // Profile Section
    QHBoxLayout* profileLayout = new QHBoxLayout();
    profileLayout->setContentsMargins(8, 0, 0, 0);
    profileLayout->setSpacing(0);

    // Profile Picture (circular)
    profilePicBtn = new QPushButton();
    profilePicBtn->setFixedSize(48, 48);
    profilePicBtn->setStyleSheet(
        "QPushButton { background-color: #e0e0e0; border-radius: 24px; }"
        );
    profilePicBtn->setCursor(Qt::PointingHandCursor);

    connect(profilePicBtn, &QPushButton::clicked, this, &MainWindow::onProfilePictureClicked);

    // Three dots button always centered on profile pic
    threeDots = createThreeDotsButton(false);
    threeDots->setParent(profilePicBtn);
    int dotsOffset = (profilePicBtn->width() - threeDots->width())/2;
    threeDots->move(dotsOffset, dotsOffset);
    threeDots->show();
    threeDots->raise();

    usernameLabel = new QLabel("Username");
    QFont usernameFont;
    if (poppinsFontId != -1 && !poppinsFontFamily.isEmpty()) {
        usernameFont = QFont(poppinsFontFamily);
        usernameFont.setWeight(QFont::Bold);
    } else {
        usernameFont.setBold(true);
    }
    usernameFont.setPixelSize(14);
    usernameLabel->setFont(usernameFont);

    profileLayout->addStretch();
    profileLayout->addWidget(profilePicBtn, 0, Qt::AlignCenter);
    profileLayout->addStretch();

    sidebarLayout->addLayout(profileLayout);
}

void MainWindow::onMinimizeClicked()
{
    if (isCollapsed) {
        expandSidebar();
    } else {
        collapseSidebar();
    }
    updateMinimizeButtonIcon();
}

void MainWindow::collapseSidebar()
{
    if (isCollapsed)
        return;

    QPropertyAnimation *widthAnimation = new QPropertyAnimation(sidebarFrame, "minimumWidth");
    widthAnimation->setDuration(450);
    widthAnimation->setStartValue(expandedSidebarWidth);
    widthAnimation->setEndValue(collapsedSidebarWidth);
    widthAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    // Animate the minimizeBtn to center above profilePicBtn
    QRect startRect = minimizeBtn->geometry();
    QPoint profilePicCenter = profilePicBtn->geometry().center();
    int minBtnX = profilePicBtn->x() + (profilePicBtn->width() - minimizeBtn->width())/2;
    int minBtnY = profilePicBtn->y() - minimizeBtn->height() - 10;
    QRect endRect(minBtnX, minBtnY, minimizeBtn->width(), minimizeBtn->height());

    QPropertyAnimation* minBtnAnim = new QPropertyAnimation(minimizeBtn, "geometry");
    minBtnAnim->setDuration(450);
    minBtnAnim->setStartValue(startRect);
    minBtnAnim->setEndValue(endRect);
    minBtnAnim->setEasingCurve(QEasingCurve::InOutCubic);

    QParallelAnimationGroup* group = new QParallelAnimationGroup;
    group->addAnimation(widthAnimation);
    group->addAnimation(minBtnAnim);

    connect(group, &QParallelAnimationGroup::finished, [this]() {
        sidebarFrame->setFixedWidth(collapsedSidebarWidth);
        isCollapsed = true;
        foreach (QLabel *label, menuTexts.values()) label->setVisible(false);
        appNameLabel->setVisible(false);
        subscriptionPanel->setFixedHeight(40);
        subscriptionDotsBtn->setVisible(false);
        freeSubscriptionLabel->setFixedSize(40, 16);
        usernameLabel->setVisible(false);
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::expandSidebar()
{
    if (!isCollapsed)
        return;

    QPropertyAnimation *widthAnimation = new QPropertyAnimation(sidebarFrame, "minimumWidth");
    widthAnimation->setDuration(450);
    widthAnimation->setStartValue(collapsedSidebarWidth);
    widthAnimation->setEndValue(expandedSidebarWidth);
    widthAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    QRect startRect = minimizeBtn->geometry();
    QHBoxLayout* minimizeRow = (QHBoxLayout*)sidebarLayout->itemAt(sidebarLayout->count()-2)->layout();
    QWidget* minBtnContainer = minimizeBtn->parentWidget();
    QPoint destPoint = minBtnContainer->mapToParent(QPoint(0,0));
    QRect endRect(destPoint.x(), destPoint.y(), minimizeBtn->width(), minimizeBtn->height());

    QPropertyAnimation* minBtnAnim = new QPropertyAnimation(minimizeBtn, "geometry");
    minBtnAnim->setDuration(450);
    minBtnAnim->setStartValue(startRect);
    minBtnAnim->setEndValue(endRect);
    minBtnAnim->setEasingCurve(QEasingCurve::InOutCubic);

    QParallelAnimationGroup* group = new QParallelAnimationGroup;
    group->addAnimation(widthAnimation);
    group->addAnimation(minBtnAnim);

    connect(group, &QParallelAnimationGroup::finished, [this]() {
        sidebarFrame->setFixedWidth(expandedSidebarWidth);
        isCollapsed = false;
        foreach (QLabel *label, menuTexts.values()) label->setVisible(true);
        appNameLabel->setVisible(true);
        subscriptionPanel->setFixedHeight(55);
        subscriptionDotsBtn->setVisible(true);
        freeSubscriptionLabel->setFixedSize(60, 24);
        usernameLabel->setVisible(true);
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::downloadLogo()
{
    QDir assetsDir(QApplication::applicationDirPath() + "/assets");
    if (!assetsDir.exists()) assetsDir.mkpath(".");

    QString logoPath = assetsDir.absoluteFilePath("logo.png");
    QFileInfo fileInfo(logoPath);

    if (fileInfo.exists()) {
        QPixmap logo(logoPath);
        qreal dpr = qApp->devicePixelRatio();
        QPixmap hiDpiLogo = logo.scaled(logoLabel->width() * dpr, logoLabel->height() * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        hiDpiLogo.setDevicePixelRatio(dpr);
        logoLabel->setPixmap(hiDpiLogo);
    } else {
        QUrl logoUrl("https://rhynec.com/logo.png");
        QNetworkRequest request(logoUrl);

        QNetworkReply* reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, [this, reply, logoPath](){
            onLogoDownloaded(reply);
        });
    }
}

void MainWindow::onLogoDownloaded(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();

        QDir assetsDir(QApplication::applicationDirPath() + "/assets");
        if (!assetsDir.exists()) assetsDir.mkpath(".");

        QString logoPath = assetsDir.absoluteFilePath("logo.png");
        QFile file(logoPath);

        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageData);
            file.close();

            QPixmap logo;
            logo.loadFromData(imageData);
            qreal dpr = qApp->devicePixelRatio();
            QPixmap hiDpiLogo = logo.scaled(logoLabel->width() * dpr, logoLabel->height() * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            hiDpiLogo.setDevicePixelRatio(dpr);
            logoLabel->setPixmap(hiDpiLogo);
        }
    } else {
        logoLabel->setText("R");
        logoLabel->setStyleSheet("QLabel { background-color: #4C4C4C; color: white; border-radius: 20px; text-align: center; }");
        logoLabel->setAlignment(Qt::AlignCenter);
    }
    reply->deleteLater();
}

void MainWindow::onLogoClicked()
{
    QDesktopServices::openUrl(QUrl("https://rhynec.com"));
}

void MainWindow::loadProfilePicture()
{
    QSettings settings("Rhynec", "RhynecSecurity");
    QString savedPath = settings.value("ProfilePicturePath").toString();

    if (!savedPath.isEmpty() && QFile::exists(savedPath)) {
        applyProfilePicture(savedPath);
    }
}

void MainWindow::onProfilePictureClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Image"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                    tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    if (!fileName.isEmpty()) {
        QSettings settings("Rhynec", "RhynecSecurity");
        settings.setValue("ProfilePicturePath", fileName);

        applyProfilePicture(fileName);
    }
}

void MainWindow::applyProfilePicture(const QString &imagePath)
{
    QPixmap originalPixmap(imagePath);
    if (originalPixmap.isNull())
        return;

    qreal dpr = qApp->devicePixelRatio();

    QPixmap circularPixmap(profilePicBtn->size() * dpr);
    circularPixmap.fill(Qt::transparent);
    circularPixmap.setDevicePixelRatio(dpr);

    QPainter painter(&circularPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath clipPath;
    clipPath.addEllipse(QRectF(1, 1, circularPixmap.width()/dpr - 2, circularPixmap.height()/dpr - 2));
    painter.setClipPath(clipPath);

    QPixmap scaledPixmap = originalPixmap.scaled(
        circularPixmap.size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
        );
    scaledPixmap.setDevicePixelRatio(dpr);

    QRect targetRect = QRect(0, 0, circularPixmap.width()/dpr, circularPixmap.height()/dpr);
    if (scaledPixmap.width()/dpr > targetRect.width()) {
        targetRect.setX((scaledPixmap.width()/dpr - targetRect.width()) / -2 + 1);
    }
    if (scaledPixmap.height()/dpr > targetRect.height()) {
        targetRect.setY((scaledPixmap.height()/dpr - targetRect.height()) / -2 + 1);
    }

    painter.drawPixmap(targetRect, scaledPixmap);
    painter.end();

    profilePicBtn->setIcon(QIcon(circularPixmap));
    profilePicBtn->setIconSize(profilePicBtn->size());
    profilePicBtn->setText("");
    profilePicBtn->setStyleSheet(
        "QPushButton { background-color: transparent; border-radius: 24px; }"
        );
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == logoLabel && event->type() == QEvent::MouseButtonRelease) {
        onLogoClicked();
        return true;
    }

    QMap<QString, QLabel*>::iterator i;
    for (i = menuTexts.begin(); i != menuTexts.end(); ++i) {
        if (obj == i.value() && event->type() == QEvent::MouseButtonRelease) {
            activateTab(i.key());
            return true;
        }
    }

    if (obj == minimizeBtn && event->type() == QEvent::MouseButtonRelease) {
        onMinimizeClicked();
        return true;
    }

    if (obj == this && event->type() == QEvent::Resize) {
        QList<QFrame*> frames = findChildren<QFrame*>();
        for (QFrame* frame : frames) {
            if (frame->frameShape() == QFrame::Box) {
                frame->setGeometry(0, 0, width(), height());
                break;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}
