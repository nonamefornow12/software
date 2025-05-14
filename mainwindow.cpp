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
#include <QResizeEvent>
#include <QImage>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)), currentTab("Status")
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

    // Initialize SVG content for minimize and expand icons - using the new SVGs
    minimizeSvgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg class=\"w-6 h-6 text-gray-800 dark:text-white\" aria-hidden=\"true\" xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" fill=\"none\" viewBox=\"0 0 24 24\">"
        "  <path stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M20 12H8m12 0-4 4m4-4-4-4M9 4H7a3 3 0 0 0-3 3v10a3 3 0 0 0 3 3h2\" transform=\"scale(-1, 1) translate(-24, 0)\"/>"
        "</svg>";

    expandSvgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg class=\"w-6 h-6 text-gray-800 dark:text-white\" aria-hidden=\"true\" xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" fill=\"none\" viewBox=\"0 0 24 24\">"
        "  <path stroke=\"currentColor\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M16 12H4m12 0-4 4m4-4-4-4m3-4h2a3 3 0 0 1 3 3v10a3 3 0 0 1-3 3h-2\"/>"
        "</svg>";

    // New FREE subscription badge SVG
    freeSubscriptionSvgContent =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
        "<svg width=\"60\" height=\"32\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"
        "  <rect x=\"0\" y=\"0\" width=\"60\" height=\"32\" rx=\"8\" ry=\"8\" fill=\"black\"/>"
        "  <text x=\"50%\" y=\"50%\" dominant-baseline=\"middle\" text-anchor=\"middle\""
        "        fill=\"white\" font-size=\"14\" font-family=\"Arial, sans-serif\" font-weight=\"900\">"
        "    FREE"
        "  </text>"
        "</svg>";

    // Initialize collapsedContainer to nullptr
    collapsedContainer = nullptr;

    // Setup custom fonts
    setupFonts();

    // Pre-render the minimize/expand buttons for crisp display
    prepareMinimizeButtonImages();

    setupUi();
    createSidebar();
    downloadLogo();
    loadProfilePicture(); // Load saved profile picture on startup

    // Install event filter to resize frame when window resizes
    installEventFilter(this);

    // Store exact profile position immediately and again after full initialization
    QTimer::singleShot(0, this, &MainWindow::storeProfilePosition);
    QTimer::singleShot(500, this, &MainWindow::storeProfilePosition);
}

MainWindow::~MainWindow()
{
    if (collapsedContainer) {
        delete collapsedContainer;
    }
}

void MainWindow::setupFonts()
{
    // Load Poppins SemiBold font from the specified path
    QString fontPath = "C:\\Users\\sem\\Documents\\untitled8\\assets\\fonts\\poppins.semibold.ttf";
    QFile fontFile(fontPath);

    if (fontFile.exists() && fontFile.open(QIODevice::ReadOnly)) {
        // Load the font from file
        QByteArray fontData = fontFile.readAll();
        fontFile.close();

        poppinsBoldId = QFontDatabase::addApplicationFontFromData(fontData);
        if (poppinsBoldId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(poppinsBoldId);
            if (!families.isEmpty()) {
                poppinsBoldFamily = families.at(0);
                qDebug() << "Poppins font loaded from:" << fontPath;
                qDebug() << "Font family:" << poppinsBoldFamily;
            }
        } else {
            qDebug() << "Failed to load Poppins font from:" << fontPath;
        }
    } else {
        qDebug() << "Poppins font file not found at:" << fontPath;
    }
}

void MainWindow::prepareMinimizeButtonImages()
{
    // Calculate exact target sizes based on device pixel ratio
    int baseSize = 24;
    int renderSize = baseSize * 2;  // Render at 2x the base size for high quality

    // For the expand SVG (used in collapsed mode)
    QSvgRenderer expandRenderer(expandSvgContent.toUtf8());
    QImage expandImage(renderSize, renderSize, QImage::Format_ARGB32_Premultiplied);
    expandImage.fill(Qt::transparent);

    QPainter expandPainter(&expandImage);
    expandPainter.setRenderHint(QPainter::Antialiasing, true);
    expandPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // Removed HighQualityAntialiasing as it's not available in your Qt version
    expandRenderer.render(&expandPainter, QRectF(0, 0, renderSize, renderSize));
    expandPainter.end();

    // Convert to pixmap with smooth scaling
    QPixmap expandRaw = QPixmap::fromImage(expandImage);
    expandButtonImage = expandRaw.scaled(
        baseSize, baseSize,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
        );
    expandButtonImage.setDevicePixelRatio(qApp->devicePixelRatio());

    // For the minimize SVG (used in expanded mode)
    QSvgRenderer minimizeRenderer(minimizeSvgContent.toUtf8());
    QImage minimizeImage(renderSize, renderSize, QImage::Format_ARGB32_Premultiplied);
    minimizeImage.fill(Qt::transparent);

    QPainter minimizePainter(&minimizeImage);
    minimizePainter.setRenderHint(QPainter::Antialiasing, true);
    minimizePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // Removed HighQualityAntialiasing as it's not available in your Qt version
    minimizeRenderer.render(&minimizePainter, QRectF(0, 0, renderSize, renderSize));
    minimizePainter.end();

    // Convert to pixmap with smooth scaling
    QPixmap minimizeRaw = QPixmap::fromImage(minimizeImage);
    minimizeButtonImage = minimizeRaw.scaled(
        baseSize, baseSize,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation
        );
    minimizeButtonImage.setDevicePixelRatio(qApp->devicePixelRatio());
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
    sidebarLayout->setContentsMargins(10, 15, 8, 15);  // Adjusted top margin to 15px instead of 25px
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
        "   margin-left: 2px;" // Move icon a tiny bit to the right
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e8e8e8; }"
        "QPushButton:focus { outline: none; border: none; }" // Prevent blue focus outline
        );

    button->setCursor(Qt::PointingHandCursor);

    // Connect the button's click signal to our handler
    connect(button, &QPushButton::clicked, this, &MainWindow::onMenuButtonClicked);

    // Check if the file exists
    QFileInfo check_file(normalIcon);
    if (!check_file.exists()) {
        qDebug() << "Initial icon file not found:" << normalIcon;
        return button; // Return early to avoid crashes
    }

    // Different rendering approaches for different icons
    QSize displaySize = isHomeIcon ? QSize(32, 32) : QSize(28, 28);

    // Set a bigger render size for extra quality
    QSize renderSize = isHomeIcon ? QSize(48, 48) : QSize(42, 42);

    // Use extra high-quality rendering approach for home icon
    if (isHomeIcon) {
        // Create a high-res render of the SVG
        QSvgRenderer renderer(normalIcon);
        QImage image(renderSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);
        painter.end();

        // Set the icon with downscaled image for better quality
        QPixmap pixmap = QPixmap::fromImage(image);
        button->setIcon(QIcon(pixmap));
    } else {
        // Standard rendering for other icons
        QSvgRenderer renderer(normalIcon);
        QPixmap pixmap(renderSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter, QRectF(0, 0, pixmap.width(), pixmap.height()));
        painter.end();

        button->setIcon(QIcon(pixmap));
    }

    button->setIconSize(displaySize);
    button->setProperty("iconSize", displaySize);
    button->setProperty("renderSize", renderSize);

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

    // Get the icon size from the button's property
    QSize displaySize = button->property("iconSize").toSize();
    QSize renderSize = button->property("renderSize").toSize();
    bool isHomeIcon = (tabName == "Status");

    // Get the appropriate icon based on the active state
    QString iconPath = button->property("activeIcon").toString();

    // Check if file exists before trying to render it
    QFileInfo check_file(iconPath);
    if (!check_file.exists()) {
        qDebug() << "Icon file not found:" << iconPath;
        return;
    }

    // Use extra high-quality rendering approach for home icon
    if (isHomeIcon) {
        // Create a high-res render of the SVG
        QSvgRenderer renderer(iconPath);
        QImage image(renderSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);
        painter.end();

        // Set the icon with downscaled image for better quality
        QPixmap pixmap = QPixmap::fromImage(image);
        button->setIcon(QIcon(pixmap));
    } else {
        // Standard rendering for other icons
        QSvgRenderer renderer(iconPath);
        QPixmap pixmap(renderSize);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);
        painter.end();

        button->setIcon(QIcon(pixmap));
    }

    button->setIconSize(displaySize);
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
        bool isHomeIcon = (key == "Status");
        bool isCurrentTab = (key == tabName);

        QSize displaySize = btn->property("iconSize").toSize();
        QSize renderSize = btn->property("renderSize").toSize();

        // Determine which icon file to use
        QString iconPath;
        if (isCurrentTab) {
            iconPath = btn->property("activeIcon").toString();
        } else {
            iconPath = btn->property("normalIcon").toString();
        }

        // Set the active state property
        btn->setProperty("isActive", isCurrentTab);

        // Check if file exists before continuing
        QFileInfo check_file(iconPath);
        if (!check_file.exists()) {
            continue; // Skip if file not found
        }

        // Use extra high-quality rendering approach for home icon
        if (isHomeIcon) {
            // Create a high-res render of the SVG
            QSvgRenderer renderer(iconPath);
            QImage image(renderSize, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);

            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            renderer.render(&painter);
            painter.end();

            // Set the icon with downscaled image for better quality
            QPixmap pixmap = QPixmap::fromImage(image);
            btn->setIcon(QIcon(pixmap));
        } else {
            // Standard rendering for other icons
            QSvgRenderer renderer(iconPath);
            QPixmap pixmap(renderSize);
            pixmap.fill(Qt::transparent);

            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            renderer.render(&painter);
            painter.end();

            btn->setIcon(QIcon(pixmap));
        }

        btn->setIconSize(displaySize);
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
QWidget* MainWindow::createFreeSubscriptionBadge(bool small)
{
    // Create a custom widget to render the FREE badge using SVG
    QWidget* badgeWidget = new QWidget();
    badgeWidget->setFixedSize(small ? 40 : 60, small ? 20 : 32); // Size adjusted to match the new SVG

    // Create a layout for the container
    QVBoxLayout* layout = new QVBoxLayout(badgeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Render the SVG directly using the new SVG content
    QSvgRenderer* renderer = new QSvgRenderer(freeSubscriptionSvgContent.toUtf8(), badgeWidget);
    QWidget* svgWidget = new QWidget();
    svgWidget->setFixedSize(small ? 40 : 60, small ? 20 : 32); // Size adjusted to match the new SVG
    svgWidget->installEventFilter(new SvgPainter(renderer, svgWidget));

    layout->addWidget(svgWidget);

    badgeWidget->setCursor(Qt::PointingHandCursor);
    return badgeWidget;
}

// Create a modern divider that matches the sidebar border and subscription panel border
QWidget* MainWindow::createModernDivider(bool minimized)
{
    // Container for the divider to apply margins
    QWidget* container = new QWidget();
    container->setObjectName("dividerContainer"); // Add an object name for styling
    container->setFixedHeight(16); // Height including space
    container->setProperty("originalHeight", 16); // Store the original height

    // Create layout for container
    QVBoxLayout* layout = new QVBoxLayout(container);

    // Adjust margins based on whether it's minimized
    if (minimized) {
        layout->setContentsMargins(8, 6, 8, 6);
        container->setFixedWidth(50); // Narrower width for minimized mode
    } else {
        layout->setContentsMargins(8, 6, 8, 6);
    }

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

    // SVG content for three dots with thicker circles
    QString svgContent =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<svg width=\"" + QString::number(size) + "\" height=\"" + QString::number(size) + "\" viewBox=\"0 0 24 24\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">"
                                                                           "  <circle cx=\"12\" cy=\"12\" r=\"2\" fill=\"black\" stroke=\"black\" stroke-width=\"0.5\"/>"
                                                                           "  <circle cx=\"12\" cy=\"6\" r=\"2\" fill=\"black\" stroke=\"black\" stroke-width=\"0.5\"/>"
                                                                           "  <circle cx=\"12\" cy=\"18\" r=\"2\" fill=\"black\" stroke=\"black\" stroke-width=\"0.5\"/>"
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

QLabel* MainWindow::createCrispMinimizeButton(bool forExpandedMode)
{
    // Increase size based on mode - SLIGHTLY BIGGER for better visibility
    int baseSize = forExpandedMode ? 22 : 28;  // Increased from 20/26 to 22/28

    // Create a QLabel to hold our pre-rendered image
    QLabel* label = new QLabel();
    label->setFixedSize(baseSize, baseSize);
    label->setCursor(Qt::PointingHandCursor);

    // Get the pre-rendered pixmap
    QPixmap pixmap = forExpandedMode ? minimizeButtonImage : expandButtonImage;

    // Scale if necessary with smooth transformation
    if (pixmap.width() != baseSize) {
        pixmap = pixmap.scaled(
            baseSize, baseSize,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
            );
    }

    // Set the proper device pixel ratio
    pixmap.setDevicePixelRatio(qApp->devicePixelRatio());

    // Set the pixmap to the label
    label->setPixmap(pixmap);
    label->setAlignment(Qt::AlignCenter);

    // Add hover effect with light gray background
    label->setStyleSheet(
        "QLabel {"
        "   border-radius: " + QString::number(baseSize/2) + "px;"
                                          "   margin-right: 2px;" // Move a tiny bit to the right
                                          "}"
                                          "QLabel:hover {"
                                          "   background-color: #f0f0f0;"
                                          "}"
        );

    // Set up event handling for clicks
    label->setObjectName(forExpandedMode ? "expandMinimizeBtn" : "collapseMinimizeBtn");
    label->installEventFilter(this);

    return label;
}

void MainWindow::createSidebar()
{
    // Create a container for the logo
    QWidget* logoContainer = new QWidget();
    logoContainer->setFixedHeight(42); // Reduced height to move logo up

    // Create a layout for logo container - this is always left-aligned
    QHBoxLayout* logoLayout = new QHBoxLayout(logoContainer);
    // Set left margin to 5px to move the logo to the left
    logoLayout->setContentsMargins(5, 0, 0, 0);
    logoLayout->setSpacing(0);

    // Create the logo
    logoLabel = new QLabel();
    logoLabel->setFixedSize(32, 32); // Smaller logo as requested (was 36x36)
    logoLabel->setCursor(Qt::PointingHandCursor);
    logoLabel->installEventFilter(this); // Make logo clickable

    // Add logo to layout
    logoLayout->addWidget(logoLabel);

    // App name - will be hidden in collapsed mode
    appNameLabel = new QLabel("rhynecsecurity");
    QFont appNameFont = appNameLabel->font();
    appNameFont.setBold(true);
    appNameFont.setPixelSize(18);
    appNameLabel->setFont(appNameFont);
    appNameLabel->setStyleSheet("margin-left: 8px;");

    // Add app name to layout (will be hidden when collapsed)
    logoLayout->addWidget(appNameLabel);
    logoLayout->addStretch(1);

    // Add logo container to sidebar
    sidebarLayout->addWidget(logoContainer);

    // Add spacing after logo section
    sidebarLayout->addSpacing(15);

    // Menu items with icons left and text with perfect spacing to match the image
    QStringList menuItems = {"Status", "VPN", "Security", "Network", "Settings", "Profile"};

    // Use the full file paths instead of resource paths
    QString assetsPath = QApplication::applicationDirPath() + "/assets/";
    // If you're developing, you might want to use the direct path where your assets are
    QString developmentPath = "C:/Users/sem/Documents/untitled8/assets/";

    // Using home.svg for the Status icon
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
    if (!poppinsBoldFamily.isEmpty()) {
        poppinsFont = QFont(poppinsBoldFamily);
        poppinsFont.setWeight(QFont::DemiBold);
    } else {
        // Fallback to system Poppins if available
        poppinsFont = QFont("Poppins");
        poppinsFont.setWeight(QFont::DemiBold);
    }
    poppinsFont.setPixelSize(14);

    // Create menu items exactly like in the image but with SMALLER SIZE
    for (int i = 0; i < menuItems.size(); i++) {
        // Container for each menu item
        QWidget* menuItem = new QWidget();
        QHBoxLayout* menuItemLayout = new QHBoxLayout(menuItem);
        menuItemLayout->setContentsMargins(0, 0, 0, 0);
        menuItemLayout->setSpacing(0);

        // Create container for icon with grey background - SMALLER SIZE and centered
        QWidget* iconContainer = new QWidget();
        iconContainer->setFixedSize(36, 36);
        iconContainer->setStyleSheet("background-color: #f8f8f8; border-radius: 7px;");

        QHBoxLayout* iconLayout = new QHBoxLayout(iconContainer);
        // Center the icon within the container
        iconLayout->setContentsMargins(4, 4, 4, 4); // Equal margins on all sides
        iconLayout->setSpacing(0);
        iconLayout->setAlignment(Qt::AlignCenter);

        // Create icon button (left)
        // Make home icon slightly bigger and special rendering for better quality
        bool isHomeIcon = (i == 0); // Check if this is the Status (home) icon
        QPushButton* iconBtn = createMenuButton(menuIcons[i], menuItems[i], isHomeIcon);
        iconBtn->setFixedSize(isHomeIcon ? 30 : 28, isHomeIcon ? 30 : 28);

        // Prevent focus outline
        iconBtn->setFocusPolicy(Qt::NoFocus);
        iconBtn->setStyleSheet(iconBtn->styleSheet() + " QPushButton:focus { outline: none; border: none; }");

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

    // Activate the Status tab by default
    activateTab("Status");

    // Spacer - this will be the primary flexible spacer
    QSpacerItem* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    sidebarLayout->addItem(spacer);

    // Add extra spacing to move subscription panel a bit further down
    sidebarLayout->addSpacing(10);

    // Create subscription panel - perfectly sized with margins and smaller height
    subscriptionPanel = new QFrame(sidebarFrame);
    subscriptionPanel->setObjectName("subscriptionPanel");
    subscriptionPanel->setFixedHeight(75); // Reduced from 80 to 75 for perfect height
    subscriptionPanel->setProperty("originalHeight", 75); // Store original height

    // Set margins for perfect position (left and right spacing)
    panelContainer = new QHBoxLayout();
    panelContainer->setContentsMargins(8, 0, 8, 0);
    panelContainer->setProperty("originalMargins", QVariant::fromValue(QMargins(8, 0, 8, 0)));

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
    freeSubscriptionLabel = createFreeSubscriptionBadge(false);

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

    // Small spacing before divider
    sidebarLayout->addSpacing(15); // Increased from 8 to 15 to move the divider down

    // Add thin divider - exact match to sidebar border
    modernDivider = createModernDivider(false); // Not minimized initially
    sidebarLayout->addWidget(modernDivider);

    // Create minimized divider for use in collapsed state
    minimizedDivider = createModernDivider(true);

    // Add spacing for profile section
    sidebarLayout->addSpacing(10);

    // Create a container for the minimize button for collapsed mode with a layout
    minimizeButtonContainer = new QWidget();
    minimizeButtonContainer->setObjectName("minimizeButtonContainer");
    QVBoxLayout* minBtnLayout = new QVBoxLayout(minimizeButtonContainer);
    minBtnLayout->setContentsMargins(0, 0, 0, 0);
    minBtnLayout->setSpacing(0);
    minBtnLayout->setAlignment(Qt::AlignCenter);

    // Create ultra-sharp minimize/expand buttons for collapsed mode
    collapseMinimizeBtn = createCrispMinimizeButton(false);
    minBtnLayout->addWidget(collapseMinimizeBtn, 0, Qt::AlignCenter);

    // Profile Section - Create a container for the profile
    QWidget* profileContainer = new QWidget();
    profileContainer->setFixedHeight(50); // Fixed height for stability

    // Create layout for profile section
    profileLayout = new QHBoxLayout(profileContainer);
    profileLayout->setContentsMargins(8, 0, 0, 0);
    profileLayout->setSpacing(10);

    // Container for profile picture to lock its position
    profilePicContainer = new QWidget();
    profilePicContainer->setFixedSize(36, 36);
    profilePicContainer->setObjectName("profilePicContainer");
    QVBoxLayout* picLayout = new QVBoxLayout(profilePicContainer);
    picLayout->setContentsMargins(0, 0, 0, 0);
    picLayout->setSpacing(0);

    // Profile Picture (circular)
    profilePicBtn = new QPushButton();
    profilePicBtn->setFixedSize(36, 36);
    profilePicBtn->setFocusPolicy(Qt::NoFocus);
    profilePicBtn->setStyleSheet(
        "QPushButton { background-color: #e0e0e0; border-radius: 18px; }"
        "QPushButton:focus { outline: none; border: none; }" // Prevent blue focus outline
        );
    profilePicBtn->setCursor(Qt::PointingHandCursor);

    picLayout->addWidget(profilePicBtn);

    // Connect profile picture to file dialog
    connect(profilePicBtn, &QPushButton::clicked, this, &MainWindow::onProfilePictureClicked);

    // Username
    usernameLabel = new QLabel("Username");
    QFont usernameFont = usernameLabel->font();
    usernameFont.setBold(true);
    usernameFont.setPixelSize(14);
    usernameLabel->setFont(usernameFont);

    // Create a horizontal layout for buttons (side by side)
    buttonsContainer = new QWidget();
    buttonLayout = new QHBoxLayout(buttonsContainer);
    buttonLayout->setContentsMargins(0, 5, 0, 0);
    buttonLayout->setSpacing(6);
    buttonLayout->setAlignment(Qt::AlignVCenter);

    // Three dots button
    threeDots = createThreeDotsButton(false);
    buttonLayout->addWidget(threeDots);

    // Create ultra-sharp minimize button for expanded mode
    expandMinimizeBtn = createCrispMinimizeButton(true);
    buttonLayout->addWidget(expandMinimizeBtn);

    // Add widgets to the profile layout
    profileLayout->addWidget(profilePicContainer);
    profileLayout->addWidget(usernameLabel, 1);
    profileLayout->addWidget(buttonsContainer);

    // Add profile section to sidebar layout
    sidebarLayout->addWidget(profileContainer);

    // Store original parent of profile picture
    originalProfileParent = profilePicContainer->parentWidget();
}

void MainWindow::storeProfilePosition()
{
    // Only capture if not already in collapsed mode
    if (!isCollapsed && profilePicContainer && !profilePicOrig.contains("y")) {
        QPoint posRelativeToFrame = profilePicContainer->mapTo(sidebarFrame, QPoint(0, 0));

        // Store in a hash for exact precision
        profilePicOrig.insert("parent", QVariant::fromValue<QWidget*>(profilePicContainer->parentWidget()));
        profilePicOrig.insert("x", posRelativeToFrame.x());
        profilePicOrig.insert("y", posRelativeToFrame.y());
        profilePicOrig.insert("width", profilePicContainer->width());
        profilePicOrig.insert("height", profilePicContainer->height());

        qDebug() << "Stored exact profile position:"
                 << "x=" << profilePicOrig["x"].toInt()
                 << "y=" << profilePicOrig["y"].toInt();
    }
}

void MainWindow::onMinimizeClicked()
{
    // Ensure we have position data
    storeProfilePosition();

    if (isCollapsed) {
        expandSidebar();
    } else {
        collapseSidebar();
    }
}

void MainWindow::collapseSidebar()
{
    if (isCollapsed)
        return;

    // Make sure we have position data
    if (!profilePicOrig.contains("y")) {
        storeProfilePosition();
    }

    // Apply the width to the frame immediately
    sidebarFrame->setFixedWidth(collapsedSidebarWidth);

    isCollapsed = true;

    // Hide app name but keep the logo visible and positioned
    appNameLabel->setVisible(false);

    // Hide text labels for menu items
    foreach (QLabel *label, menuTexts.values()) {
        label->setVisible(false);
    }

    // Hide username and buttons container but keep profile picture
    usernameLabel->setVisible(false);
    buttonsContainer->setVisible(false);

    // Hide the regular divider
    if (modernDivider) {
        modernDivider->setVisible(false);
    }

    // Clean up any previous collapsed container to prevent memory leaks
    if (collapsedContainer) {
        delete collapsedContainer;
        collapsedContainer = nullptr;
    }

    // Create our stacked layout for the collapsed sidebar elements in the bottom corner
    QVBoxLayout* collapsedLayout = new QVBoxLayout();
    collapsedLayout->setContentsMargins(0, 0, 0, 0);
    collapsedLayout->setSpacing(12); // Uniform spacing between elements
    collapsedLayout->setAlignment(Qt::AlignHCenter); // Center everything horizontally

    // Get profile position for reference
    int profileY = profilePicOrig["y"].toInt();

    // Create a container to hold our stacked widgets
    collapsedContainer = new QWidget(sidebarFrame);
    collapsedContainer->setLayout(collapsedLayout);

    // Prepare each widget for the collapsed layout

    // 1. Subscription panel
    subscriptionPanel->setParent(collapsedContainer);
    subscriptionPanel->setFixedHeight(40);
    subscriptionPanel->setFixedWidth(60);
    collapsedLayout->addWidget(subscriptionPanel, 0, Qt::AlignHCenter);

    // Hide email label and dots
    emailLabel->setVisible(false);
    subscriptionDotsBtn->setVisible(false);

    // Resize FREE subscription badge to be small
    freeSubscriptionLabel->setFixedSize(40, 20);

    // 2. Divider
    minimizedDivider->setParent(collapsedContainer);
    minimizedDivider->setFixedWidth(50);
    collapsedLayout->addWidget(minimizedDivider, 0, Qt::AlignHCenter);

    // 3. Minimize button
    minimizeButtonContainer->setParent(collapsedContainer);
    minimizeButtonContainer->setFixedSize(28, 28); // Match the new button size
    collapsedLayout->addWidget(minimizeButtonContainer, 0, Qt::AlignHCenter);

    // Calculate space needed for the stacked widgets
    int totalHeight = 40 + 16 + 28 + collapsedLayout->spacing() * 2; // Panel + divider + button + spacings

    // Position the container above the profile picture with proper spacing
    // Moving down a tiny bit (3px)
    int containerBottom = profileY - 12; // Original was 15, reduced to 12 to move down
    int containerTop = containerBottom - totalHeight;

    collapsedContainer->setGeometry(0, containerTop, collapsedSidebarWidth, totalHeight);
    collapsedContainer->show();

    // Position the profile picture
    int xPos = (collapsedSidebarWidth - profilePicContainer->width()) / 2; // Center horizontally
    profilePicContainer->setParent(sidebarFrame);
    profilePicContainer->setGeometry(xPos, profileY, profilePicContainer->width(), profilePicContainer->height());
    profilePicContainer->show();
}

void MainWindow::expandSidebar()
{
    if (!isCollapsed)
        return;

    // Apply the width to the frame immediately
    sidebarFrame->setFixedWidth(expandedSidebarWidth);

    isCollapsed = false;

    // Clean up the collapsed container
    if (collapsedContainer) {
        // Remove widgets from the collapsed container before deleting it
        subscriptionPanel->setParent(nullptr);
        minimizedDivider->setParent(nullptr);
        minimizeButtonContainer->setParent(nullptr);

        delete collapsedContainer;
        collapsedContainer = nullptr;
    }

    // Return profile picture to the exact parent and position from before
    if (profilePicOrig.contains("parent")) {
        QWidget* parent = profilePicOrig["parent"].value<QWidget*>();
        if (parent) {
            // Temporarily hide to prevent flickering
            profilePicContainer->hide();

            // Put back in its original parent
            profilePicContainer->setParent(parent);

            // Reposition it in the layout
            profileLayout->insertWidget(0, profilePicContainer);

            // Show it again
            profilePicContainer->show();
        }
    }

    // Show the expanded elements
    modernDivider->setVisible(true);
    modernDivider->setParent(sidebarFrame); // Ensure it's in the sidebar

    // Show app name
    appNameLabel->setVisible(true);

    // Show text labels
    foreach (QLabel *label, menuTexts.values()) {
        label->setVisible(true);
    }

    // Put the subscription panel back into its original container
    panelContainer->addWidget(subscriptionPanel);

    // Restore subscription panel position and size
    subscriptionPanel->setFixedHeight(75); // Restore to original height
    subscriptionPanel->setFixedWidth(QWIDGETSIZE_MAX); // Reset the fixed width (used in collapsed mode)
    panelContainer->setContentsMargins(8, 0, 8, 0);

    // Show email label
    emailLabel->setVisible(true);

    // Show 3 dots in subscription panel
    subscriptionDotsBtn->setVisible(true);

    // Restore FREE subscription badge size
    freeSubscriptionLabel->setFixedSize(60, 32);

    // Show username and buttons container
    usernameLabel->setVisible(true);
    buttonsContainer->setVisible(true);

    // Add minimize button back to buttons container
    buttonLayout->addWidget(expandMinimizeBtn);
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
        QPixmap hiDpiLogo = logo.scaled(32 * dpr, 32 * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation); // Reduced to 32x32
        hiDpiLogo.setDevicePixelRatio(dpr);
        logoLabel->setPixmap(hiDpiLogo);

        // Set content margins to ensure logo is centered
        logoLabel->setAlignment(Qt::AlignCenter);
        logoLabel->setContentsMargins(0, 0, 0, 0);
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

            // Display the logo
            QPixmap logo;
            logo.loadFromData(imageData);

            // Create a slightly larger pixmap to ensure margins
            QSize displaySize = logoLabel->size();
            QPixmap resizedLogo(displaySize);
            resizedLogo.fill(Qt::transparent);

            QPainter painter(&resizedLogo);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

            // Center the logo with some padding to prevent cutting off edges
            QRect targetRect(2, 2, displaySize.width() - 4, displaySize.height() - 4); // Smaller padding for smaller logo
            painter.drawPixmap(targetRect, logo, logo.rect());
            painter.end();

            logoLabel->setPixmap(resizedLogo);
            logoLabel->setScaledContents(false); // Do not scale contents to prevent distortion
            logoLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        // Use a fallback image or placeholder
        logoLabel->setText("R");
        logoLabel->setStyleSheet("QLabel { background-color: #4C4C4C; color: white; border-radius: 16px; text-align: center; }"); // Smaller radius for smaller logo
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
        "QPushButton:focus { outline: none; border: none; }" // Prevent blue focus outline
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

// Implement the resizeEvent handler to fix the compilation error
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // Update border frame size when window is resized
    QList<QFrame*> frames = findChildren<QFrame*>();
    for (QFrame* frame : frames) {
        if (frame->frameShape() == QFrame::Box) {
            frame->setGeometry(0, 0, width(), height());
            break;
        }
    }

    // If collapsed, ensure profile picture stays in exact position
    if (isCollapsed && profilePicContainer && profilePicOrig.contains("y")) {
        // Use exact stored position for profile picture
        int profileY = profilePicOrig["y"].toInt();
        int profileWidth = profilePicOrig["width"].toInt();
        int profileHeight = profilePicOrig["height"].toInt();

        // Center horizontally but keep exact Y position
        int xPos = (collapsedSidebarWidth - profileWidth) / 2;
        profilePicContainer->setGeometry(xPos, profileY, profileWidth, profileHeight);
    }
    // Otherwise, capture position if not already done
    else if (!isCollapsed && profilePicContainer && !profilePicOrig.contains("y")) {
        QTimer::singleShot(0, this, &MainWindow::storeProfilePosition);
    }

    QMainWindow::resizeEvent(event);
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

    // Handle clicks on the minimize buttons
    if ((obj == expandMinimizeBtn || obj == collapseMinimizeBtn) &&
        event->type() == QEvent::MouseButtonRelease) {
        onMinimizeClicked();
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}
