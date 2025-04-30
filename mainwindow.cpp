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
    // Load Poppins SemiBold font
    // First, check if the font is already installed in the system
    QFontDatabase fontDatabase;
    QStringList fontFamilies = fontDatabase.families();

    if (fontFamilies.contains("Poppins", Qt::CaseInsensitive)) {
        // Font is installed, just use it
        qDebug() << "Using system Poppins font";
    } else {
        // Try to load from application resources or file
        QString fontPath = ":/fonts/Poppins-SemiBold.ttf"; // Adjust this path as needed
        QFile fontFile(fontPath);

        if (fontFile.exists() && fontFile.open(QIODevice::ReadOnly)) {
            // Load the font from file
            QByteArray fontData = fontFile.readAll();
            fontFile.close();

            poppinsFontId = QFontDatabase::addApplicationFontFromData(fontData);
            if (poppinsFontId != -1) {
                qDebug() << "Poppins font loaded from resources";
            } else {
                qDebug() << "Failed to load Poppins font from resources";
            }
        } else {
            // Fall back to a similar font
            qDebug() << "Poppins font not found, using fallback font";
        }
    }
}

void MainWindow::setupUi()
{
    // Set window properties
    setWindowTitle("Rhynec Security");

    // Get screen size and set window size
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int width = screenGeometry.width() * 0.8;
    int height = screenGeometry.height() * 0.8;
    resize(width, height);

    // Create central widget with layout
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(1, 1, 1, 1); // 1px margin for the border
    mainLayout->setSpacing(0);

    // Create sidebar frame
    sidebarFrame = new QFrame(centralWidget);
    sidebarFrame->setObjectName("sidebarFrame");
    sidebarFrame->setStyleSheet("QFrame#sidebarFrame { background-color: white; border-right: 1px solid #e0e0e0; }");
    sidebarFrame->setFixedWidth(expandedSidebarWidth);

    // Create content area - WHITE as requested
    contentArea = new QFrame(centralWidget);
    contentArea->setObjectName("contentArea");
    contentArea->setStyleSheet("QFrame#contentArea { background-color: white; }");

    // Create a content layout for the main area
    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(10);

    // Add content title label for displaying current tab
    contentTitleLabel = new QLabel(currentTab, contentArea);
    QFont titleFont = contentTitleLabel->font();
    titleFont.setWeight(QFont::Bold);
    titleFont.setPixelSize(32);
    contentTitleLabel->setFont(titleFont);
    contentTitleLabel->setAlignment(Qt::AlignCenter);
    contentTitleLabel->setStyleSheet("color: #333333;");

    contentLayout->addWidget(contentTitleLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);

    // Create sidebar layout with perfect spacing
    sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(10, 25, 8, 15);  // Adjusted margins to move everything a bit to right and down
    sidebarLayout->setSpacing(9);  // Perfect vertical spacing

    // Add frames to main layout
    mainLayout->addWidget(sidebarFrame);
    mainLayout->addWidget(contentArea, 1);

    // Set central widget
    setCentralWidget(centralWidget);
}

QPushButton* MainWindow::createMenuButton(const QString &icon, const QString &text, bool isHomeIcon)
{
    QPushButton *button = new QPushButton("", sidebarFrame);

    // Create copies of the icon path strings that we can modify
    QString normalIcon = icon;
    QString activeIcon = icon;

    // Replace .svg with -2.svg to get the alternate icon path
    activeIcon.replace(".svg", "-2.svg");

    // Store both icons in the button's property for later use when clicked
    button->setProperty("normalIcon", normalIcon);
    button->setProperty("activeIcon", activeIcon);
    button->setProperty("isActive", false);
    button->setProperty("tabName", text);

    // Set button style with NO TEXT - we'll add text separately
    button->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   border-radius: 4px;"
        "   background-color: #f8f8f8;" // Always visible light grey background like in image
        "   padding: 8px;" // Reduced padding
        "   margin: 0px;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e8e8e8; }"
        );

    button->setCursor(Qt::PointingHandCursor);

    // Connect the button's click signal to our handler
    connect(button, &QPushButton::clicked, this, &MainWindow::onMenuButtonClicked);

    // Use the original icon path for initial rendering
    QFileInfo check_file(normalIcon);
    if (!check_file.exists()) {
        qDebug() << "Initial icon file not found:" << normalIcon;
    }

    QSvgRenderer renderer(normalIcon);

    // Create a smaller fixed size viewport for the icon - Home icon slightly bigger
    QSize iconSize;
    if (isHomeIcon) {
        iconSize = QSize(30, 30); // Home icon is slightly bigger
    } else {
        iconSize = QSize(28, 28); // Normal size for other icons
    }

    // High DPI support
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap(iconSize * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Get the original SVG viewBox
    QRectF viewBox = renderer.viewBoxF();

    // Scale and center the SVG within our icon size
    qreal xScale = iconSize.width() / viewBox.width();
    qreal yScale = iconSize.height() / viewBox.height();
    qreal scaleFactor = qMin(xScale, yScale) * 0.9; // 90% of max scale to ensure full visibility

    // Center the SVG in the target icon area
    qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
    qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

    // Apply transformations to show the entire SVG
    painter.translate(xOffset, yOffset);
    painter.scale(scaleFactor, scaleFactor);

    // Render the entire SVG
    renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
    painter.end();

    // Set icon
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

    // Toggle active state
    bool isActive = true; // Always set to active when clicked
    button->setProperty("isActive", isActive);

    // Get the appropriate icon based on the active state (always the active icon when clicked)
    QString iconPath = button->property("activeIcon").toString();

    // Check if file exists before trying to render it
    QFileInfo check_file(iconPath);
    if (!check_file.exists()) {
        qDebug() << "Icon file not found:" << iconPath;
        return;
    }

    // Create a new renderer with the appropriate icon
    QSvgRenderer renderer(iconPath);

    // Get the icon size from the button's property
    QSize iconSize = button->property("iconSize").toSize();

    // High DPI support
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap(iconSize * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Get the original SVG viewBox
    QRectF viewBox = renderer.viewBoxF();

    // Scale and center the SVG within our icon size
    qreal xScale = iconSize.width() / viewBox.width();
    qreal yScale = iconSize.height() / viewBox.height();
    qreal scaleFactor = qMin(xScale, yScale) * 0.9; // 90% of max scale to ensure full visibility

    // Center the SVG in the target icon area
    qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
    qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

    // Apply transformations to show the entire SVG
    painter.translate(xOffset, yOffset);
    painter.scale(scaleFactor, scaleFactor);

    // Render the entire SVG
    renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
    painter.end();

    // Update the icon
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
    // First, reset all buttons to their normal state
    foreach (const QString &key, menuButtons.keys()) {
        QPushButton* btn = menuButtons[key];
        QString normalIcon = btn->property("normalIcon").toString();

        // Only change buttons that aren't the current one
        if (key != tabName) {
            btn->setProperty("isActive", false);

            QFileInfo check_file(normalIcon);
            if (check_file.exists()) {
                QSvgRenderer renderer(normalIcon);
                QSize iconSize = btn->property("iconSize").toSize();

                // High DPI support
                qreal dpr = qApp->devicePixelRatio();
                QPixmap pixmap(iconSize * dpr);
                pixmap.fill(Qt::transparent);
                pixmap.setDevicePixelRatio(dpr);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                // Get the original SVG viewBox
                QRectF viewBox = renderer.viewBoxF();

                // Scale and center the SVG within our icon size
                qreal xScale = iconSize.width() / viewBox.width();
                qreal yScale = iconSize.height() / viewBox.height();
                qreal scaleFactor = qMin(xScale, yScale) * 0.9; // 90% of max scale to ensure full visibility

                // Center the SVG in the target icon area
                qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
                qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

                // Apply transformations to show the entire SVG
                painter.translate(xOffset, yOffset);
                painter.scale(scaleFactor, scaleFactor);

                // Render the entire SVG
                renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
                painter.end();

                // Update the icon
                btn->setIcon(QIcon(pixmap));
            }
        } else {
            // Set the active state for the current button
            btn->setProperty("isActive", true);

            // Get the appropriate icon based on the active state
            QString activeIcon = btn->property("activeIcon").toString();

            QFileInfo check_file(activeIcon);
            if (check_file.exists()) {
                QSvgRenderer renderer(activeIcon);
                QSize iconSize = btn->property("iconSize").toSize();

                // High DPI support
                qreal dpr = qApp->devicePixelRatio();
                QPixmap pixmap(iconSize * dpr);
                pixmap.fill(Qt::transparent);
                pixmap.setDevicePixelRatio(dpr);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                // Get the original SVG viewBox
                QRectF viewBox = renderer.viewBoxF();

                // Scale and center the SVG within our icon size
                qreal xScale = iconSize.width() / viewBox.width();
                qreal yScale = iconSize.height() / viewBox.height();
                qreal scaleFactor = qMin(xScale, yScale) * 0.9; // 90% of max scale to ensure full visibility

                // Center the SVG in the target icon area
                qreal xOffset = (iconSize.width() - (viewBox.width() * scaleFactor)) / 2;
                qreal yOffset = (iconSize.height() - (viewBox.height() * scaleFactor)) / 2;

                // Apply transformations to show the entire SVG
                painter.translate(xOffset, yOffset);
                painter.scale(scaleFactor, scaleFactor);

                // Render the entire SVG
                renderer.render(&painter, QRectF(0, 0, viewBox.width(), viewBox.height()));
                painter.end();

                // Update the icon
                btn->setIcon(QIcon(pixmap));
            }
        }
    }

    // Update the current tab
    currentTab = tabName;

    // Update the center content with the new tab name
    updateCenterContent(tabName);
}

void MainWindow::updateCenterContent(const QString &tabName)
{
    // Update the content area title with the current tab name
    contentTitleLabel->setText(tabName);

    // You can add more tab-specific content here in the future
}

// Create a FREE subscription badge using the provided SVG
QWidget* MainWindow::createFreeSubscriptionBadge()
{
    // Create a custom widget to render the FREE badge using SVG
    QWidget* badgeWidget = new QWidget();
    badgeWidget->setFixedSize(70, 30);

    // SVG content for FREE badge
    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"70\" height=\"30\" viewBox=\"0 0 70 30\">"
        "  <rect x=\"5\" y=\"5\" width=\"60\" height=\"20\" rx=\"8\" ry=\"8\" stroke=\"black\" stroke-width=\"2\" fill=\"white\"/>"
        "  <text x=\"50%\" y=\"50%\" font-family=\"Arial\" font-size=\"12\" text-anchor=\"middle\" dy=\".3em\" fill=\"black\" stroke=\"black\" stroke-width=\"1\">FREE</text>"
        "</svg>";

    // Create a layout for the container
    QVBoxLayout* layout = new QVBoxLayout(badgeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Render the SVG directly
    QSvgRenderer* renderer = new QSvgRenderer(svgContent.toUtf8(), badgeWidget);
    QWidget* svgWidget = new QWidget();
    svgWidget->setFixedSize(70, 30);
    svgWidget->installEventFilter(new SvgPainter(renderer, svgWidget));

    layout->addWidget(svgWidget);

    badgeWidget->setCursor(Qt::PointingHandCursor);
    return badgeWidget;
}

// Create a modern divider that matches the sidebar border and subscription panel border
QWidget* MainWindow::createModernDivider()
{
    // Container for the divider to apply margins
    QWidget* container = new QWidget();
    container->setFixedHeight(16); // Height including space

    // Create layout for container
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(8, 6, 8, 6); // Horizontal margins to match sidebar padding

    // Create custom divider - exact same color and thickness as sidebar border
    QFrame* divider = new QFrame();
    divider->setFixedHeight(1); // Exactly 1px (same as sidebar border)
    divider->setStyleSheet(
        "QFrame {"
        "   background-color: #e0e0e0;" // Exact same color as sidebar border
        "   border: none;"
        "}"
        );

    layout->addWidget(divider);
    return container;
}

// Create vertical three dots using provided SVG - BIGGER SIZE
QWidget* MainWindow::createThreeDotsButton(bool smaller)
{
    // Container for the SVG - size based on parameter
    QWidget* container = new QWidget();
    int size = smaller ? 16 : 18; // Using 16px for smaller variant
    container->setFixedSize(size, size);

    // SVG content from the provided SVG
    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg width=\"" + QString::number(size) + "px\" height=\"" + QString::number(size) + "px\" viewBox=\"0 0 24 24\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">" // Dynamic size
                                                                             "<path d=\"M12 12H12.01M12 6H12.01M12 18H12.01M13 12C13 12.5523 12.5523 13 12 13C11.4477 13 11 12.5523 11 12C11 11.4477 11.4477 11 12 11C12.5523 11 13 11.4477 13 12ZM13 18C13 18.5523 12.5523 19 12 19C11.4477 19 11 18.5523 11 18C11 17.4477 11.4477 17 12 17C12.5523 17 13 17.4477 13 18ZM13 6C13 6.55228 12.5523 7 12 7C11.4477 7 11 6.55228 11 6C11 5.44772 11.4477 5 12 5C12.5523 5 13 5.44772 13 6Z\" stroke=\"#000000\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
                                                                             "</svg>";

    // Create a layout for the container
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Render the SVG directly
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
    // Create an SVG renderer based on the current state
    QString svgContent = isCollapsed ? expandSvgContent : minimizeSvgContent;

    // Size for the button
    int size = 18; // Using a bigger size of 18px

    // Create a layout for the container if needed
    if (minimizeBtn->layout() == nullptr) {
        QVBoxLayout* layout = new QVBoxLayout(minimizeBtn);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    } else {
        // Clear the existing layout
        QLayoutItem *item;
        while ((item = minimizeBtn->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }

    // Render the SVG directly
    QSvgRenderer* minimizeRenderer = new QSvgRenderer(svgContent.toUtf8(), minimizeBtn);
    QWidget* minimizeSvgWidget = new QWidget();
    minimizeSvgWidget->setFixedSize(size, size);
    minimizeSvgWidget->installEventFilter(new SvgPainter(minimizeRenderer, minimizeSvgWidget));

    // Add to layout
    minimizeBtn->layout()->addWidget(minimizeSvgWidget);
}

void MainWindow::createSidebar()
{
    // 1. Top Section with Logo and App Name - positioned perfectly
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // Logo positioned more right and made larger
    logoLabel = new QLabel(sidebarFrame);
    logoLabel->setFixedSize(30, 30); // Increased size from 24x24 to 30x30
    logoLabel->setCursor(Qt::PointingHandCursor);
    logoLabel->setStyleSheet("margin-left: 10px; margin-top: -2px;"); // Moved logo 10px right

    // Make logo clickable to open website
    logoLabel->installEventFilter(this);

    // App name - changed to "Rhynec Security" - moved right
    appNameLabel = new QLabel("Rhynec Security", sidebarFrame);
    QFont appNameFont = appNameLabel->font();
    appNameFont.setBold(true);
    appNameFont.setPixelSize(15); // Perfect size
    appNameLabel->setFont(appNameFont);
    appNameLabel->setStyleSheet("margin-left: 4px;"); // Moved text 4px right

    topLayout->addWidget(logoLabel);
    topLayout->addWidget(appNameLabel);
    topLayout->addStretch(1);

    // Removed minimize button from top layout

    sidebarLayout->addLayout(topLayout);

    // No divider after "Rhynec Security" as requested
    sidebarLayout->addSpacing(12); // Perfect spacing

    // Menu items with icons left and text with perfect spacing to match the image
    QStringList menuItems = {"Dashboard", "VPN", "Security", "Network", "Settings", "Profile"};

    // Use the full file paths instead of resource paths
    QString assetsPath = QApplication::applicationDirPath() + "/assets/";
    // If you're developing, you might want to use the direct path where your assets are
    QString developmentPath = "C:/Users/sem/Documents/untitled8/assets/";

    QStringList menuIcons = {
        developmentPath + "home.svg",
        developmentPath + "vpn.svg",
        developmentPath + "Security.svg",
        developmentPath + "Network.svg",
        developmentPath + "Settings.svg",
        developmentPath + "Profile.svg"
    };

    // Get Poppins SemiBold font for menu items
    QFont poppinsFont;
    if (poppinsFontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(poppinsFontId);
        if (!families.isEmpty()) {
            poppinsFont = QFont(families.at(0));
        } else {
            // Fallback
            poppinsFont = QFont("Poppins");
        }
    } else {
        // Try to find Poppins in the system
        poppinsFont = QFont("Poppins");
    }

    // Use 600 weight for SemiBold (DemiBold)
    poppinsFont.setWeight(QFont::DemiBold); // Correct enum for SemiBold/DemiBold
    poppinsFont.setPixelSize(14);

    // Create menu items exactly like in the image but with SMALLER SIZE
    for (int i = 0; i < menuItems.size(); i++) {
        // Container for each menu item
        QWidget* menuItem = new QWidget();
        QHBoxLayout* menuItemLayout = new QHBoxLayout(menuItem);
        menuItemLayout->setContentsMargins(0, 0, 0, 0);
        menuItemLayout->setSpacing(0);

        // Create container for icon with grey background - SMALLER SIZE
        QWidget* iconContainer = new QWidget();
        iconContainer->setFixedSize(36, 36); // Reduced from 44x44
        iconContainer->setStyleSheet("background-color: #f8f8f8; border-radius: 7px;"); // Light grey background with rounder corners

        QHBoxLayout* iconLayout = new QHBoxLayout(iconContainer);
        iconLayout->setContentsMargins(0, 0, 0, 0);
        iconLayout->setSpacing(0);

        // Create icon button (left)
        // Make home icon slightly bigger
        bool isHomeIcon = (i == 0); // Check if this is the Dashboard (home) icon
        QPushButton* iconBtn = createMenuButton(menuIcons[i], menuItems[i], isHomeIcon);
        iconBtn->setFixedSize(32, 32); // Reduced from 40x40
        iconLayout->addWidget(iconBtn);

        // Store the button for later use
        menuButtons[menuItems[i]] = iconBtn;

        // Create text label with Poppins SemiBold font
        QLabel* textLabel = new QLabel(menuItems[i]);
        textLabel->setFont(poppinsFont);
        textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Make text label clickable
        textLabel->setCursor(Qt::PointingHandCursor);
        textLabel->installEventFilter(this);
        menuTexts[menuItems[i]] = textLabel;

        // Add a spacer with smaller spacing (14px)
        QSpacerItem* spacer = new QSpacerItem(14, 10, QSizePolicy::Fixed, QSizePolicy::Minimum); // 14px spacing (reduced from 16)

        // Add items to layout
        menuItemLayout->addWidget(iconContainer, 0, Qt::AlignLeft);
        menuItemLayout->addSpacerItem(spacer);
        menuItemLayout->addWidget(textLabel, 0, Qt::AlignLeft);
        menuItemLayout->addStretch(1);

        sidebarLayout->addWidget(menuItem);

        // Add extra spacing between menu items - exactly like in image
        if (i < menuItems.size() - 1) {
            sidebarLayout->addSpacing(10); // More spacing to match image
        }
    }

    // Activate the Dashboard tab by default
    activateTab("Dashboard");

    // Spacer
    QSpacerItem* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    sidebarLayout->addItem(spacer);

    // Create subscription panel - perfectly sized with margins and smaller height
    subscriptionPanel = new QFrame(sidebarFrame);
    subscriptionPanel->setObjectName("subscriptionPanel");
    subscriptionPanel->setFixedHeight(75); // Reduced from 80 to 75 for perfect height

    // Set margins for perfect position (left and right spacing)
    QHBoxLayout* panelContainer = new QHBoxLayout();
    panelContainer->setContentsMargins(8, 0, 8, 0);

    subscriptionPanel->setStyleSheet(
        "QFrame#subscriptionPanel {"
        "   background-color: white;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 15px;"
        "}"
        );

    // Create layout for subscription panel
    QVBoxLayout* subscriptionPanelLayout = new QVBoxLayout(subscriptionPanel);
    subscriptionPanelLayout->setContentsMargins(8, 5, 8, 6); // Reduced top margin
    subscriptionPanelLayout->setSpacing(1);

    // Top row layout with settings button only
    QHBoxLayout* topSubscriptionRow = new QHBoxLayout();
    topSubscriptionRow->setContentsMargins(0, 0, 0, 0);
    topSubscriptionRow->setSpacing(0);

    // Add SVG dots button - BIGGER SIZE
    subscriptionDotsBtn = createThreeDotsButton();

    // Add to layout with the dots button on the right
    topSubscriptionRow->addStretch(1);
    topSubscriptionRow->addWidget(subscriptionDotsBtn, 0, Qt::AlignRight);

    subscriptionPanelLayout->addLayout(topSubscriptionRow);

    // Middle row - FREE badge aligned with the J of email
    QHBoxLayout* badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(0, 0, 0, 0);
    badgeRow->setSpacing(0);

    // Add FREE subscription badge using the SVG
    freeSubscriptionLabel = createFreeSubscriptionBadge();

    // Add badge container to row - positioned more left and much higher up
    badgeRow->addWidget(freeSubscriptionLabel);
    badgeRow->addStretch(1);

    subscriptionPanelLayout->addLayout(badgeRow);

    // Bottom row - Email address with left alignment to align with badge
    QHBoxLayout* emailRow = new QHBoxLayout();
    emailRow->setContentsMargins(0, 0, 0, 0);
    emailRow->setSpacing(0);

    // Email address in gray (left-aligned below badge) - CHANGED EMAIL ADDRESS
    emailLabel = new QLabel("john.doe@email.com");
    emailLabel->setStyleSheet("color: #888888; margin-left: 0px; margin-top: 8px;"); // Add more top margin to move email down
    QFont emailFont = emailLabel->font();
    emailFont.setPixelSize(12);
    emailLabel->setFont(emailFont);
    emailLabel->setAlignment(Qt::AlignLeft);

    // Add email to row
    emailRow->addWidget(emailLabel);
    emailRow->addStretch(1);

    subscriptionPanelLayout->addLayout(emailRow);

    // Add the panel to the container with margins
    panelContainer->addWidget(subscriptionPanel);
    sidebarLayout->addLayout(panelContainer);

    // Add spacing before divider
    sidebarLayout->addSpacing(8);

    // Add thin divider - exact match to sidebar border
    QWidget* modernDivider = createModernDivider();
    sidebarLayout->addWidget(modernDivider);

    // Profile Section
    QHBoxLayout* profileLayout = new QHBoxLayout();
    profileLayout->setContentsMargins(8, 0, 0, 0); // Add left margin for perfect alignment
    profileLayout->setSpacing(10); // Perfect spacing

    // Profile Picture (circular)
    profilePicBtn = new QPushButton();
    profilePicBtn->setFixedSize(36, 36);
    profilePicBtn->setStyleSheet(
        "QPushButton { background-color: #e0e0e0; border-radius: 18px; }"
        );
    profilePicBtn->setCursor(Qt::PointingHandCursor);

    // Connect profile picture to file dialog
    connect(profilePicBtn, &QPushButton::clicked, this, &MainWindow::onProfilePictureClicked);

    // Username
    usernameLabel = new QLabel("Username");
    QFont usernameFont = usernameLabel->font();
    usernameFont.setBold(true);
    usernameFont.setPixelSize(14);
    usernameLabel->setFont(usernameFont);

    // Create a horizontal layout for buttons (side by side)
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(0, 5, 0, 0); // Added top margin to align with username center
    buttonsLayout->setSpacing(6); // Space between buttons
    buttonsLayout->setAlignment(Qt::AlignVCenter); // Align buttons vertically centered

    // Three dots button - now on left of minimize
    threeDots = createThreeDotsButton(false); // Using regular size (18px)
    buttonsLayout->addWidget(threeDots);

    // Minimize button - next to three dots (on right)
    minimizeBtn = new QWidget();
    minimizeBtn->setFixedSize(18, 18); // Slightly bigger size
    minimizeBtn->setCursor(Qt::PointingHandCursor);
    minimizeBtn->setStyleSheet(
        "QWidget {"
        "   background-color: transparent;"
        "   border-radius: 3px;"
        "}"
        "QWidget:hover {"
        "   background-color: #f0f0f0;"
        "}"
        );

    // Create a layout for the minimize button
    QVBoxLayout* minimizeLayout = new QVBoxLayout(minimizeBtn);
    minimizeLayout->setContentsMargins(0, 0, 0, 0);
    minimizeLayout->setSpacing(0);

    // Render the SVG directly using the current state
    updateMinimizeButtonIcon();

    // Connect minimize button to the collapse/expand function
    minimizeBtn->installEventFilter(this);

    buttonsLayout->addWidget(minimizeBtn);

    // Add widgets to the profile layout
    profileLayout->addWidget(profilePicBtn);
    profileLayout->addWidget(usernameLabel, 1);
    profileLayout->addLayout(buttonsLayout); // Using the horizontal button layout

    sidebarLayout->addLayout(profileLayout);
}

void MainWindow::onMinimizeClicked()
{
    if (isCollapsed) {
        expandSidebar();
    } else {
        collapseSidebar();
    }

    // Update the minimize button icon based on the new state
    updateMinimizeButtonIcon();
}

void MainWindow::collapseSidebar()
{
    if (isCollapsed)
        return;

    // Apply the width to the frame BEFORE starting the animation to avoid lag
    sidebarFrame->setFixedWidth(collapsedSidebarWidth);

    isCollapsed = true;

    // Animation for sidebar width
    QPropertyAnimation *widthAnimation = new QPropertyAnimation(sidebarFrame, "minimumWidth");
    widthAnimation->setDuration(300);
    widthAnimation->setStartValue(collapsedSidebarWidth); // Start at already collapsed width
    widthAnimation->setEndValue(collapsedSidebarWidth);   // End at collapsed width
    widthAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Hide text labels and other elements that should be hidden in collapsed mode
    foreach (QLabel *label, menuTexts.values()) {
        label->setVisible(false);
    }

    // Hide app name
    appNameLabel->setVisible(false);

    // Make subscription panel smaller
    subscriptionPanel->setFixedHeight(50);

    // Hide email label
    emailLabel->setVisible(false);

    // Hide 3 dots in subscription panel when minimized
    subscriptionDotsBtn->setVisible(false);

    // Resize FREE subscription badge to be small when minimized
    freeSubscriptionLabel->setFixedSize(40, 20);

    // Hide username and three dots
    usernameLabel->setVisible(false);
    threeDots->setVisible(false);

    // Move the minimize button to a visible position when collapsed
    // Detach from layout to position manually
    minimizeBtn->setParent(sidebarFrame);
    minimizeBtn->setGeometry(collapsedSidebarWidth/2 - 9, 60, 18, 18); // Position it in the upper part, not overlapping with profile pic
    minimizeBtn->raise(); // Bring to front
    minimizeBtn->show();

    widthAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::expandSidebar()
{
    if (!isCollapsed)
        return;

    // Apply the width to the frame BEFORE starting the animation to avoid lag
    sidebarFrame->setFixedWidth(expandedSidebarWidth);

    isCollapsed = false;

    // Animation for sidebar width
    QPropertyAnimation *widthAnimation = new QPropertyAnimation(sidebarFrame, "minimumWidth");
    widthAnimation->setDuration(300);
    widthAnimation->setStartValue(expandedSidebarWidth); // Start at already expanded width
    widthAnimation->setEndValue(expandedSidebarWidth);   // End at expanded width
    widthAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Put the minimize button back in its original position
    minimizeBtn->setParent(nullptr);

    // Show text labels and other elements that should be visible in expanded mode
    foreach (QLabel *label, menuTexts.values()) {
        label->setVisible(true);
    }

    // Show app name
    appNameLabel->setVisible(true);

    // Restore subscription panel height
    subscriptionPanel->setFixedHeight(75);

    // Show email label
    emailLabel->setVisible(true);

    // Show 3 dots in subscription panel
    subscriptionDotsBtn->setVisible(true);

    // Restore FREE subscription badge size
    freeSubscriptionLabel->setFixedSize(70, 30);

    // Show username and three dots
    usernameLabel->setVisible(true);
    threeDots->setVisible(true);

    // Recreate the profile layout with the minimize button
    QHBoxLayout* profileLayout = (QHBoxLayout*)sidebarLayout->itemAt(sidebarLayout->count()-1)->layout();
    if (profileLayout) {
        // Create a horizontal layout for buttons (side by side)
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setContentsMargins(0, 5, 0, 0); // Added top margin to align with username center
        buttonsLayout->setSpacing(6); // Space between buttons
        buttonsLayout->setAlignment(Qt::AlignVCenter); // Align buttons vertically centered

        // Add the buttons to the layout
        buttonsLayout->addWidget(threeDots);
        buttonsLayout->addWidget(minimizeBtn);

        // Add layout to the profile layout
        profileLayout->addLayout(buttonsLayout);
    }

    widthAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::downloadLogo()
{
    // Create assets directory if it doesn't exist
    QDir assetsDir(QApplication::applicationDirPath() + "/assets");
    if (!assetsDir.exists()) {
        assetsDir.mkpath(".");
    }

    // Check if logo already exists locally
    QString logoPath = assetsDir.absoluteFilePath("logo.png");
    QFileInfo fileInfo(logoPath);

    if (fileInfo.exists()) {
        // Use the local logo file with proper HDPI scaling for crisp rendering
        QPixmap logo(logoPath);
        qreal dpr = qApp->devicePixelRatio();
        QPixmap hiDpiLogo = logo.scaled(30 * dpr, 30 * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation); // Increased size to 30x30
        hiDpiLogo.setDevicePixelRatio(dpr);
        logoLabel->setPixmap(hiDpiLogo);
    } else {
        // Download the logo
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

        // Save to file
        QDir assetsDir(QApplication::applicationDirPath() + "/assets");
        if (!assetsDir.exists()) {
            assetsDir.mkpath(".");
        }

        QString logoPath = assetsDir.absoluteFilePath("logo.png");
        QFile file(logoPath);

        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageData);
            file.close();

            // Display the logo with proper HDPI scaling for crisp rendering
            QPixmap logo;
            logo.loadFromData(imageData);
            qreal dpr = qApp->devicePixelRatio();
            QPixmap hiDpiLogo = logo.scaled(30 * dpr, 30 * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation); // Increased size to 30x30
            hiDpiLogo.setDevicePixelRatio(dpr);
            logoLabel->setPixmap(hiDpiLogo);
        }
    } else {
        // Use a fallback image or placeholder
        logoLabel->setText("R");
        logoLabel->setStyleSheet("QLabel { background-color: #4C4C4C; color: white; border-radius: 15px; text-align: center; }"); // Increased border radius for larger logo
        logoLabel->setAlignment(Qt::AlignCenter);
    }

    reply->deleteLater();
}

void MainWindow::onLogoClicked()
{
    QDesktopServices::openUrl(QUrl("https://rhynec.com"));
}

// Load saved profile picture on startup
void MainWindow::loadProfilePicture()
{
    // Load saved profile picture path using QSettings
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
        // Save the path for persistent storage
        QSettings settings("Rhynec", "RhynecSecurity");
        settings.setValue("ProfilePicturePath", fileName);

        // Apply the profile picture
        applyProfilePicture(fileName);
    }
}

void MainWindow::applyProfilePicture(const QString &imagePath)
{
    // Load the image
    QPixmap originalPixmap(imagePath);
    if (originalPixmap.isNull())
        return;

    // Get the device pixel ratio for high-DPI support
    qreal dpr = qApp->devicePixelRatio();

    // Create a high-quality circular mask
    QPixmap circularPixmap(profilePicBtn->size() * dpr);
    circularPixmap.fill(Qt::transparent);
    circularPixmap.setDevicePixelRatio(dpr);

    QPainter painter(&circularPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Create slightly imperfect circular path
    QPainterPath clipPath;
    clipPath.addEllipse(QRectF(1, 1, circularPixmap.width()/dpr - 2, circularPixmap.height()/dpr - 2));
    painter.setClipPath(clipPath);

    // Scale the image to fit, maintaining aspect ratio with high quality
    QPixmap scaledPixmap = originalPixmap.scaled(
        circularPixmap.size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
        );
    scaledPixmap.setDevicePixelRatio(dpr);

    // Center the image if needed with a slight offset
    QRect targetRect = QRect(0, 0, circularPixmap.width()/dpr, circularPixmap.height()/dpr);
    if (scaledPixmap.width()/dpr > targetRect.width()) {
        targetRect.setX((scaledPixmap.width()/dpr - targetRect.width()) / -2 + 1);  // Slight offset
    }
    if (scaledPixmap.height()/dpr > targetRect.height()) {
        targetRect.setY((scaledPixmap.height()/dpr - targetRect.height()) / -2 + 1);  // Slight offset
    }

    // Draw the image with high quality
    painter.drawPixmap(targetRect, scaledPixmap);
    painter.end();

    // Set the button icon with the circular image
    profilePicBtn->setIcon(QIcon(circularPixmap));
    profilePicBtn->setIconSize(profilePicBtn->size());
    profilePicBtn->setText("");  // Clear any text
    profilePicBtn->setStyleSheet(
        "QPushButton { background-color: transparent; border-radius: 18px; }"
        );
}

// Make window draggable
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

// Make the logo clickable and handle window resize
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == logoLabel && event->type() == QEvent::MouseButtonRelease) {
        onLogoClicked();
        return true;
    }

    // Handle clicks on menu text labels
    QMap<QString, QLabel*>::iterator i;
    for (i = menuTexts.begin(); i != menuTexts.end(); ++i) {
        if (obj == i.value() && event->type() == QEvent::MouseButtonRelease) {
            activateTab(i.key());
            return true;
        }
    }

    // Handle minimize button clicks
    if (obj == minimizeBtn && event->type() == QEvent::MouseButtonRelease) {
        onMinimizeClicked();
        return true;
    }

    // If this is a window resize event, resize the border frame
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
