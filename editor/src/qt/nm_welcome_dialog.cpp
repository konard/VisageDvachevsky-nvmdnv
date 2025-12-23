#include "NovelMind/editor/qt/nm_welcome_dialog.hpp"
#include "NovelMind/editor/qt/nm_icon_manager.hpp"
#include "NovelMind/editor/qt/nm_dialogs.hpp"

#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QEasingCurve>
#include <QEvent>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QSequentialAnimationGroup>
#include <QSettings>
#include <QShowEvent>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMWelcomeDialog::NMWelcomeDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("Welcome to NovelMind Editor");
  setMinimumSize(1200, 700);
  resize(1400, 800);

  setupUI();
  loadRecentProjects();
  loadTemplates();
  styleDialog();
}

NMWelcomeDialog::~NMWelcomeDialog() = default;

void NMWelcomeDialog::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Header
  QWidget *header = new QWidget(this);
  header->setObjectName("WelcomeHeader");
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(24, 16, 24, 16);

  QLabel *titleLabel = new QLabel("NovelMind Editor", header);
  titleLabel->setObjectName("WelcomeTitle");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);

  QLabel *versionLabel = new QLabel("v0.3.0", header);
  versionLabel->setObjectName("WelcomeVersion");

  // Search box
  m_searchBox = new QLineEdit(header);
  m_searchBox->setPlaceholderText("Search projects and templates...");
  m_searchBox->setMinimumWidth(300);
  connect(m_searchBox, &QLineEdit::textChanged, this,
          &NMWelcomeDialog::onSearchTextChanged);

  headerLayout->addWidget(titleLabel);
  headerLayout->addWidget(versionLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(m_searchBox);

  mainLayout->addWidget(header);

  // Content area - three columns
  QWidget *content = new QWidget(this);
  QHBoxLayout *contentLayout = new QHBoxLayout(content);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);

  setupLeftPanel();
  setupCenterPanel();
  setupRightPanel();

  contentLayout->addWidget(m_leftPanel, 1);
  contentLayout->addWidget(m_centerPanel, 2);
  contentLayout->addWidget(m_rightPanel, 1);

  mainLayout->addWidget(content, 1);

  // Footer
  m_footer = new QWidget(this);
  m_footer->setObjectName("WelcomeFooter");
  QHBoxLayout *footerLayout = new QHBoxLayout(m_footer);
  footerLayout->setContentsMargins(24, 12, 24, 12);

  QCheckBox *skipCheckbox = new QCheckBox("Don't show this again", m_footer);
  connect(skipCheckbox, &QCheckBox::toggled,
          [this](bool checked) { m_skipInFuture = checked; });

  m_btnClose = new QPushButton("Close", m_footer);
  m_btnClose->setMinimumWidth(100);
  connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);

  footerLayout->addWidget(skipCheckbox);
  footerLayout->addStretch();
  footerLayout->addWidget(m_btnClose);

  mainLayout->addWidget(m_footer);
}

void NMWelcomeDialog::setupLeftPanel() {
  m_leftPanel = new QWidget(this);
  m_leftPanel->setObjectName("WelcomeLeftPanel");
  m_leftLayout = new QVBoxLayout(m_leftPanel);
  m_leftLayout->setContentsMargins(24, 24, 12, 24);
  m_leftLayout->setSpacing(12);

  // Section: Quick Actions
  QLabel *quickActionsLabel = new QLabel("Quick Actions", m_leftPanel);
  quickActionsLabel->setObjectName("SectionTitle");
  QFont sectionFont = quickActionsLabel->font();
  sectionFont.setPointSize(12);
  sectionFont.setBold(true);
  quickActionsLabel->setFont(sectionFont);
  m_leftLayout->addWidget(quickActionsLabel);

  // New Project button
  m_btnNewProject = new QPushButton("New Project", m_leftPanel);
  m_btnNewProject->setObjectName("PrimaryActionButton");
  m_btnNewProject->setMinimumHeight(48);
  connect(m_btnNewProject, &QPushButton::clicked, this,
          &NMWelcomeDialog::onNewProjectClicked);
  m_leftLayout->addWidget(m_btnNewProject);

  // Open Project button
  m_btnOpenProject = new QPushButton("Open Project", m_leftPanel);
  m_btnOpenProject->setObjectName("SecondaryActionButton");
  m_btnOpenProject->setMinimumHeight(48);
  connect(m_btnOpenProject, &QPushButton::clicked, this,
          &NMWelcomeDialog::onOpenProjectClicked);
  m_leftLayout->addWidget(m_btnOpenProject);

  // Browse Examples button
  m_btnBrowseExamples = new QPushButton("Browse Examples", m_leftPanel);
  m_btnBrowseExamples->setObjectName("SecondaryActionButton");
  m_btnBrowseExamples->setMinimumHeight(48);
  connect(m_btnBrowseExamples, &QPushButton::clicked, this,
          &NMWelcomeDialog::onBrowseExamplesClicked);
  m_leftLayout->addWidget(m_btnBrowseExamples);

  m_leftLayout->addSpacing(24);

  // Section: Recent Projects
  QLabel *recentLabel = new QLabel("Recent Projects", m_leftPanel);
  recentLabel->setObjectName("SectionTitle");
  recentLabel->setFont(sectionFont);
  m_leftLayout->addWidget(recentLabel);

  m_recentProjectsList = new QListWidget(m_leftPanel);
  m_recentProjectsList->setObjectName("RecentProjectsList");
  connect(m_recentProjectsList, &QListWidget::itemClicked, this,
          &NMWelcomeDialog::onRecentProjectClicked);
  m_leftLayout->addWidget(m_recentProjectsList, 1);
}

void NMWelcomeDialog::setupCenterPanel() {
  m_centerPanel = new QWidget(this);
  m_centerPanel->setObjectName("WelcomeCenterPanel");
  QVBoxLayout *centerLayout = new QVBoxLayout(m_centerPanel);
  centerLayout->setContentsMargins(12, 24, 12, 24);
  centerLayout->setSpacing(16);

  // Section title
  QLabel *templatesLabel = new QLabel("Project Templates", m_centerPanel);
  templatesLabel->setObjectName("SectionTitle");
  QFont sectionFont = templatesLabel->font();
  sectionFont.setPointSize(12);
  sectionFont.setBold(true);
  templatesLabel->setFont(sectionFont);
  centerLayout->addWidget(templatesLabel);

  // Templates scroll area
  m_templatesScrollArea = new QScrollArea(m_centerPanel);
  m_templatesScrollArea->setObjectName("TemplatesScrollArea");
  m_templatesScrollArea->setWidgetResizable(true);
  m_templatesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_templatesContainer = new QWidget();
  m_templatesLayout = new QGridLayout(m_templatesContainer);
  m_templatesLayout->setSpacing(16);
  m_templatesLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

  m_templatesScrollArea->setWidget(m_templatesContainer);
  centerLayout->addWidget(m_templatesScrollArea, 1);
}

void NMWelcomeDialog::setupRightPanel() {
  m_rightPanel = new QWidget(this);
  m_rightPanel->setObjectName("WelcomeRightPanel");
  QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPanel);
  rightLayout->setContentsMargins(12, 24, 24, 24);
  rightLayout->setSpacing(16);

  // Section title
  QLabel *resourcesLabel = new QLabel("Learning Resources", m_rightPanel);
  resourcesLabel->setObjectName("SectionTitle");
  QFont sectionFont = resourcesLabel->font();
  sectionFont.setPointSize(12);
  sectionFont.setBold(true);
  resourcesLabel->setFont(sectionFont);
  rightLayout->addWidget(resourcesLabel);

  // Resources scroll area
  m_resourcesScrollArea = new QScrollArea(m_rightPanel);
  m_resourcesScrollArea->setObjectName("ResourcesScrollArea");
  m_resourcesScrollArea->setWidgetResizable(true);
  m_resourcesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_resourcesContainer = new QWidget();
  QVBoxLayout *resourcesLayout = new QVBoxLayout(m_resourcesContainer);
  resourcesLayout->setSpacing(12);
  resourcesLayout->setAlignment(Qt::AlignTop);

  // Add some learning resources
  struct Resource {
    QString title;
    QString description;
    QString url;
  };

  QVector<Resource> resources = {
      {"Getting Started Guide", "Learn the basics of NovelMind Editor",
       "https://github.com/VisageDvachevsky/NovelMind"},
      {"Tutorial Videos", "Video tutorials for common tasks",
       "https://github.com/VisageDvachevsky/NovelMind/tree/main/examples/sample_vn"},
      {"API Documentation", "Complete API reference",
       "https://github.com/VisageDvachevsky/NovelMind/tree/main/docs"},
      {"Community Forum", "Ask questions and share projects",
       "https://github.com/VisageDvachevsky/NovelMind/discussions"},
      {"Report Issues", "Found a bug? Let us know!",
       "https://github.com/VisageDvachevsky/NovelMind/issues"}};

  for (const auto &resource : resources) {
    QFrame *resourceCard = new QFrame(m_resourcesContainer);
    resourceCard->setObjectName("ResourceCard");
    resourceCard->setFrameShape(QFrame::StyledPanel);
    resourceCard->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *cardLayout = new QVBoxLayout(resourceCard);
    cardLayout->setContentsMargins(12, 12, 12, 12);

    QLabel *titleLabel = new QLabel(resource.title, resourceCard);
    titleLabel->setObjectName("ResourceTitle");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *descLabel = new QLabel(resource.description, resourceCard);
    descLabel->setObjectName("ResourceDescription");
    descLabel->setWordWrap(true);

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(descLabel);

    resourcesLayout->addWidget(resourceCard);

    // Make the card clickable
    resourceCard->installEventFilter(this);
    resourceCard->setProperty("url", resource.url);
  }

  resourcesLayout->addStretch();

  m_resourcesScrollArea->setWidget(m_resourcesContainer);
  rightLayout->addWidget(m_resourcesScrollArea, 1);
}

void NMWelcomeDialog::loadRecentProjects() {
  QSettings settings("NovelMind", "Editor");
  int count = settings.beginReadArray("RecentProjects");

  for (int i = 0; i < count && i < MAX_RECENT_PROJECTS; ++i) {
    settings.setArrayIndex(i);
    RecentProject project;
    project.name = settings.value("name").toString();
    project.path = settings.value("path").toString();
    project.lastOpened = settings.value("lastOpened").toString();
    project.thumbnail = settings.value("thumbnail").toString();

    // Verify the project file still exists
    if (QFileInfo::exists(project.path)) {
      m_recentProjects.append(project);

      // Add to list widget
      QString displayText =
          QString("%1\n%2").arg(project.name, project.lastOpened);
      QListWidgetItem *item =
          new QListWidgetItem(displayText, m_recentProjectsList);
      item->setData(Qt::UserRole, project.path);
    }
  }

  settings.endArray();
}

void NMWelcomeDialog::loadTemplates() {
  // Define available templates
  m_templates = {
      {"Blank Project", "Start with an empty project", "", "Blank"},
      {"Visual Novel", "Traditional visual novel with dialogue and choices", "",
       "Visual Novel"},
      {"Dating Sim", "Dating simulation with relationship mechanics", "",
       "Dating Sim"},
      {"Mystery/Detective", "Investigation-focused story with clues", "",
       "Mystery"},
      {"RPG Story", "Story with stat tracking and combat", "", "RPG"},
      {"Horror", "Atmospheric horror visual novel", "", "Horror"}};

  // Create template cards in a grid
  int col = 0;
  int row = 0;
  const int COLS = 2;

  for (int i = 0; i < m_templates.size(); ++i) {
    QWidget *card = createTemplateCard(m_templates[i], i);
    m_templatesLayout->addWidget(card, row, col);

    col++;
    if (col >= COLS) {
      col = 0;
      row++;
    }
  }
}

QWidget *NMWelcomeDialog::createTemplateCard(const ProjectTemplate &tmpl,
                                             int index) {
  QFrame *card = new QFrame(m_templatesContainer);
  card->setObjectName("TemplateCard");
  card->setFrameShape(QFrame::StyledPanel);
  card->setMinimumSize(CARD_WIDTH, CARD_HEIGHT);
  card->setMaximumSize(CARD_WIDTH, CARD_HEIGHT);
  card->setCursor(Qt::PointingHandCursor);

  QVBoxLayout *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(16, 16, 16, 16);
  cardLayout->setSpacing(8);

  // Icon
  QLabel *iconLabel = new QLabel(card);
  iconLabel->setObjectName("TemplateIcon");
  iconLabel->setMinimumSize(48, 48);
  iconLabel->setMaximumSize(48, 48);
  iconLabel->setAlignment(Qt::AlignCenter);
  auto &iconMgr = NMIconManager::instance();
  iconLabel->setPixmap(iconMgr.getPixmap("file-new", 32));
  QFont iconFont = iconLabel->font();
  iconFont.setPointSize(24);
  iconLabel->setFont(iconFont);

  // Title
  QLabel *titleLabel = new QLabel(tmpl.name, card);
  titleLabel->setObjectName("TemplateTitle");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(11);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);

  // Description
  QLabel *descLabel = new QLabel(tmpl.description, card);
  descLabel->setObjectName("TemplateDescription");
  descLabel->setWordWrap(true);

  cardLayout->addWidget(iconLabel);
  cardLayout->addWidget(titleLabel);
  cardLayout->addWidget(descLabel);
  cardLayout->addStretch();

  // Store template index for click handling
  card->setProperty("templateIndex", index);
  card->installEventFilter(this);

  return card;
}

void NMWelcomeDialog::refreshRecentProjects() {
  m_recentProjects.clear();
  m_recentProjectsList->clear();
  loadRecentProjects();
}

void NMWelcomeDialog::onNewProjectClicked() {
  // Default to blank template
  m_selectedTemplate = "Blank Project";
  m_createNewProject = true;
  accept();
}

void NMWelcomeDialog::onOpenProjectClicked() {
  QString projectPath = NMFileDialog::getExistingDirectory(
      this, tr("Open NovelMind Project"), QDir::homePath());

  if (!projectPath.isEmpty()) {
    m_selectedProjectPath = projectPath;
    m_createNewProject = false;
    accept();
  }
}

void NMWelcomeDialog::onRecentProjectClicked(QListWidgetItem *item) {
  if (item) {
    m_selectedProjectPath = item->data(Qt::UserRole).toString();
    m_createNewProject = false;
    accept();
  }
}

void NMWelcomeDialog::onTemplateClicked(int templateIndex) {
  if (templateIndex >= 0 && templateIndex < m_templates.size()) {
    m_selectedTemplate = m_templates[templateIndex].name;
    m_createNewProject = true;
    accept();
  }
}

void NMWelcomeDialog::onBrowseExamplesClicked() {
  QDesktopServices::openUrl(
      QUrl("https://github.com/VisageDvachevsky/NovelMind/tree/main/examples"));
}

void NMWelcomeDialog::onSearchTextChanged(const QString &text) {
  // Filter templates and recent projects based on search text
  const QString query = text.trimmed().toLower();

  // Filter templates
  if (m_templatesContainer) {
    const auto cards = m_templatesContainer->findChildren<QWidget *>();
    for (QWidget *card : cards) {
      bool match = query.isEmpty();
      const QVariant indexValue = card->property("templateIndex");
      if (indexValue.isValid()) {
        const int index = indexValue.toInt();
        if (index >= 0 && index < m_templates.size()) {
          const QString name = m_templates[index].name.toLower();
          const QString description = m_templates[index].description.toLower();
          match = query.isEmpty() || name.contains(query) ||
                  description.contains(query);
        }
        card->setVisible(match);
      }
    }
  }

  // Filter recent projects list
  if (m_recentProjectsList) {
    for (int i = 0; i < m_recentProjectsList->count(); ++i) {
      auto *item = m_recentProjectsList->item(i);
      if (!item) {
        continue;
      }
      const QString textValue = item->text().toLower();
      const QString pathValue = item->data(Qt::UserRole).toString().toLower();
      const bool match = query.isEmpty() || textValue.contains(query) ||
                         pathValue.contains(query);
      item->setHidden(!match);
    }
  }
}

void NMWelcomeDialog::styleDialog() {
  // Apply stylesheet for welcome dialog
  setStyleSheet(R"(
        QDialog {
            background-color: #1a1a1a;
        }

        #WelcomeHeader {
            background-color: #232323;
            border-bottom: 1px solid #404040;
        }

        #WelcomeTitle {
            color: #e0e0e0;
        }

        #WelcomeVersion {
            color: #a0a0a0;
            font-size: 10pt;
        }

        #WelcomeLeftPanel, #WelcomeCenterPanel, #WelcomeRightPanel {
            background-color: #1a1a1a;
        }

        #WelcomeFooter {
            background-color: #232323;
            border-top: 1px solid #404040;
        }

        #SectionTitle {
            color: #e0e0e0;
            margin-bottom: 8px;
        }

        #PrimaryActionButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 12px;
            font-weight: bold;
            font-size: 11pt;
        }

        #PrimaryActionButton:hover {
            background-color: #1a88e0;
        }

        #PrimaryActionButton:pressed {
            background-color: #006cbd;
        }

        #SecondaryActionButton {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #404040;
            border-radius: 4px;
            padding: 12px;
            font-size: 10pt;
        }

        #SecondaryActionButton:hover {
            background-color: #383838;
            border-color: #0078d4;
        }

        #SecondaryActionButton:pressed {
            background-color: #232323;
        }

        #RecentProjectsList {
            background-color: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 4px;
            color: #e0e0e0;
        }

        #RecentProjectsList::item {
            padding: 8px;
            border-bottom: 1px solid #404040;
        }

        #RecentProjectsList::item:hover {
            background-color: #383838;
        }

        #RecentProjectsList::item:selected {
            background-color: #0078d4;
        }

        #TemplateCard, #ResourceCard {
            background-color: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 6px;
        }

        #TemplateCard:hover, #ResourceCard:hover {
            background-color: #383838;
            border-color: #0078d4;
        }

        #TemplateTitle, #ResourceTitle {
            color: #e0e0e0;
        }

        #TemplateDescription, #ResourceDescription {
            color: #a0a0a0;
            font-size: 9pt;
        }

        QLineEdit {
            background-color: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 4px;
            padding: 8px;
            color: #e0e0e0;
        }

        QLineEdit:focus {
            border-color: #0078d4;
        }

        QScrollArea {
            border: none;
            background-color: transparent;
        }

        QCheckBox {
            color: #e0e0e0;
        }

        QPushButton {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #404040;
            border-radius: 4px;
            padding: 8px 16px;
        }

        QPushButton:hover {
            background-color: #383838;
        }

        QPushButton:pressed {
            background-color: #232323;
        }
    )");
}

void NMWelcomeDialog::setupAnimations() {
  // IMPORTANT: Entrance animations are disabled to ensure panels are always
  // visible. Previously, setting opacity to 0 before animations caused panels
  // to become invisible if animations failed to start or were interrupted. The
  // opacity effects are no longer used to prevent any visibility issues.

  // Animation system is disabled - panels remain fully visible at all times
  return;
}

void NMWelcomeDialog::startEntranceAnimations() {
  if (m_animationsPlayed) {
    return;
  }
  m_animationsPlayed = true;

  // Entrance animations are disabled to prevent visibility issues.
  // Panels remain fully visible without any opacity animations.
  return;
}

void NMWelcomeDialog::animateButtonHover(QWidget *button, bool entering) {
  if (!button)
    return;

  // Create smooth scale animation for button hover
  QPropertyAnimation *scaleAnim =
      new QPropertyAnimation(button, "geometry", this);
  scaleAnim->setDuration(150);
  scaleAnim->setEasingCurve(entering ? QEasingCurve::OutBack
                                     : QEasingCurve::InOutQuad);

  QRect currentGeom = button->geometry();
  QRect targetGeom = currentGeom;

  if (entering) {
    // Slight scale up on hover
    targetGeom.adjust(-2, -2, 2, 2);
  }

  scaleAnim->setStartValue(button->geometry());
  scaleAnim->setEndValue(targetGeom);
  scaleAnim->start(QPropertyAnimation::DeleteWhenStopped);
}

void NMWelcomeDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);

  if (!m_animationsPlayed) {
    setupAnimations();
    // Start animations after a short delay to ensure widgets are fully laid out
    QTimer::singleShot(50, this, &NMWelcomeDialog::startEntranceAnimations);
  }
}

bool NMWelcomeDialog::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    // Handle template card clicks
    QWidget *widget = qobject_cast<QWidget *>(watched);
    if (widget && widget->objectName() == "TemplateCard") {
      int templateIndex = widget->property("templateIndex").toInt();
      onTemplateClicked(templateIndex);
      return true;
    }

    // Handle resource card clicks
    if (widget && widget->objectName() == "ResourceCard") {
      QString url = widget->property("url").toString();
      if (!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
      }
      return true;
    }
  } else if (event->type() == QEvent::Enter) {
    // Handle hover enter on buttons only (avoid geometry shifts on cards)
    QWidget *widget = qobject_cast<QWidget *>(watched);
    if (widget && widget->objectName().contains("Button")) {
      animateButtonHover(widget, true);
    }
  } else if (event->type() == QEvent::Leave) {
    // Handle hover leave on buttons only (avoid geometry shifts on cards)
    QWidget *widget = qobject_cast<QWidget *>(watched);
    if (widget && widget->objectName().contains("Button")) {
      animateButtonHover(widget, false);
    }
  }

  return QDialog::eventFilter(watched, event);
}

} // namespace NovelMind::editor::qt
