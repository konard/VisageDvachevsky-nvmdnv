#include "NovelMind/editor/qt/panels/nm_asset_browser_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"
#include "NovelMind/editor/qt/nm_dialogs.hpp"
#include "NovelMind/editor/qt/qt_event_bus.hpp"

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHeaderView>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPixmapCache>
#include <QMimeData>
#include <QDropEvent>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

namespace {

constexpr int kAssetThumbSize = 80;
constexpr int kPreviewHeight = 140;

bool isImageExtension(const QString &extension) {
  const QString ext = extension.toLower();
  return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
         ext == "gif";
}

class NMAssetFilterProxy final : public QSortFilterProxyModel {
public:
  explicit NMAssetFilterProxy(QObject *parent = nullptr)
      : QSortFilterProxyModel(parent) {}

  void setNameFilterText(const QString &text) {
    m_nameFilter = text.trimmed();
    invalidateFilter();
  }

  void setTypeFilterIndex(int index) {
    m_typeFilterIndex = index;
    invalidateFilter();
  }

  void setPinnedDirectory(const QString &path) {
    m_pinnedDirectory = QDir::cleanPath(path);
    invalidateFilter();
  }

protected:
  bool filterAcceptsRow(int sourceRow,
                        const QModelIndex &sourceParent) const override {
    auto *fsModel = qobject_cast<QFileSystemModel *>(sourceModel());
    if (!fsModel) {
      return true;
    }

    QModelIndex idx = fsModel->index(sourceRow, 0, sourceParent);
    if (!idx.isValid()) {
      return false;
    }

    const QFileInfo info = fsModel->fileInfo(idx);
    if (info.isDir()) {
      return true;
    }

    if (!info.isFile()) {
      return false;
    }

    if (!m_nameFilter.isEmpty()) {
      if (!info.fileName().contains(m_nameFilter, Qt::CaseInsensitive)) {
        return false;
      }
    }

    if (m_typeFilterIndex == 0) {
      return true;
    }

    const QString ext = info.suffix().toLower();
    switch (m_typeFilterIndex) {
    case 1: // Images
      return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
             ext == "gif";
    case 2: // Audio
      return ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac";
    case 3: // Fonts
      return ext == "ttf" || ext == "otf";
    case 4: // Scripts
      return ext == "nms" || ext == "nmscene";
    case 5: // Data
      return ext == "json" || ext == "xml" || ext == "yaml" || ext == "yml";
    default:
      return true;
    }
  }

private:
  QString m_nameFilter;
  QString m_pinnedDirectory;
  int m_typeFilterIndex = 0;
};

class NMAssetIconProvider : public QFileIconProvider {
public:
  explicit NMAssetIconProvider(QSize iconSize, QObject *parent = nullptr)
      : QFileIconProvider(), m_iconSize(iconSize) {
    Q_UNUSED(parent);
  }

  void setIconSize(const QSize &size) { m_iconSize = size; }

  QIcon icon(const QFileInfo &info) const override {
    if (info.isFile() && isImageExtension(info.suffix())) {
      const QString cacheKey =
          QString("nm_asset_thumb_%1_%2x%3")
              .arg(info.absoluteFilePath())
              .arg(m_iconSize.width())
              .arg(m_iconSize.height());
      QPixmap cached;
      if (QPixmapCache::find(cacheKey, &cached)) {
        return QIcon(cached);
      }

      QImageReader reader(info.absoluteFilePath());
      reader.setAutoTransform(true);
      QImage image = reader.read();
      if (!image.isNull()) {
        QPixmap pix =
            QPixmap::fromImage(image.scaled(m_iconSize, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));
        QPixmapCache::insert(cacheKey, pix);
        return QIcon(pix);
      }
    }
    return QFileIconProvider::icon(info);
  }

private:
  QSize m_iconSize;
};

} // namespace

NMAssetBrowserPanel::NMAssetBrowserPanel(QWidget *parent)
    : NMDockPanel(tr("Asset Browser"), parent) {
  setPanelId("AssetBrowser");
  setupContent();
  setupToolBar();
}

NMAssetBrowserPanel::~NMAssetBrowserPanel() = default;

void NMAssetBrowserPanel::onInitialize() {
  auto &projectManager = ProjectManager::instance();
  if (projectManager.hasOpenProject()) {
    setRootPath(QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Assets)));
  } else {
    setRootPath(QDir::homePath());
  }
}

void NMAssetBrowserPanel::onUpdate(double /*deltaTime*/) {
  // No continuous update needed
}

void NMAssetBrowserPanel::setRootPath(const QString &path) {
  m_rootPath = path;
  m_currentPath = path;

  if (m_treeModel) {
    m_treeModel->setRootPath(path);
    m_treeView->setRootIndex(m_treeModel->index(path));
  }

  if (m_listModel) {
    if (m_filterProxy) {
      if (auto *proxy = static_cast<NMAssetFilterProxy *>(m_filterProxy)) {
        proxy->setPinnedDirectory(path);
      }
    }
    m_listModel->setRootPath(path);
    QModelIndex idx = m_listModel->index(path);
    m_listView->setRootIndex(m_filterProxy ? m_filterProxy->mapFromSource(idx)
                                           : idx);
  }
  updatePreview(selectedAssetPath());
}

QString NMAssetBrowserPanel::selectedAssetPath() const {
  if (m_listView && m_listModel) {
    QModelIndex index = m_listView->currentIndex();
    if (index.isValid()) {
      QModelIndex source =
          m_filterProxy ? m_filterProxy->mapToSource(index) : index;
      return m_listModel->filePath(source);
    }
  }
  return QString();
}

void NMAssetBrowserPanel::refresh() {
  if (m_listModel && !m_currentPath.isEmpty()) {
    if (m_filterProxy) {
      if (auto *proxy = static_cast<NMAssetFilterProxy *>(m_filterProxy)) {
        proxy->setPinnedDirectory(m_currentPath);
      }
    }
    QModelIndex idx = m_listModel->index(m_currentPath);
    m_listView->setRootIndex(m_filterProxy ? m_filterProxy->mapFromSource(idx)
                                           : idx);
  }
  updatePreview(selectedAssetPath());
}

void NMAssetBrowserPanel::setupToolBar() {
  m_toolBar = new QToolBar(this);
  m_toolBar->setObjectName("AssetBrowserToolBar");
  m_toolBar->setIconSize(QSize(14, 14));

  m_filterEdit = new QLineEdit(this);
  m_filterEdit->setPlaceholderText(tr("Filter by name..."));
  m_filterEdit->setClearButtonEnabled(true);
  m_filterEdit->setMaximumWidth(200);
  m_filterEdit->setToolTip(tr("Filter assets by name"));
  connect(m_filterEdit, &QLineEdit::textChanged, this,
          &NMAssetBrowserPanel::onFilterTextChanged);
  m_toolBar->addWidget(m_filterEdit);

  m_typeFilter = new QComboBox(this);
  m_typeFilter->addItems(
      {tr("All"), tr("Images"), tr("Audio"), tr("Fonts"), tr("Scripts"),
       tr("Data")});
  m_typeFilter->setMaximumWidth(120);
  m_typeFilter->setToolTip(tr("Filter assets by type"));
  connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &NMAssetBrowserPanel::onTypeFilterChanged);
  m_toolBar->addWidget(m_typeFilter);

  m_toolBar->addSeparator();

  QAction *actionRefresh = m_toolBar->addAction(tr("Refresh"));
  actionRefresh->setToolTip(tr("Refresh Asset List"));
  connect(actionRefresh, &QAction::triggered, this,
          &NMAssetBrowserPanel::refresh);

  m_togglePreviewAction = m_toolBar->addAction(tr("Preview"));
  m_togglePreviewAction->setCheckable(true);
  m_togglePreviewAction->setChecked(m_previewVisible);
  m_togglePreviewAction->setToolTip(tr("Toggle asset preview pane"));
  connect(m_togglePreviewAction, &QAction::toggled, this,
          &NMAssetBrowserPanel::setPreviewVisible);

  m_thumbSizeCombo = new QComboBox(this);
  m_thumbSizeCombo->addItem(tr("Small"), 64);
  m_thumbSizeCombo->addItem(tr("Medium"), 80);
  m_thumbSizeCombo->addItem(tr("Large"), 96);
  const int thumbIndex = m_thumbSizeCombo->findData(m_thumbSize);
  if (thumbIndex >= 0) {
    m_thumbSizeCombo->setCurrentIndex(thumbIndex);
  } else {
    m_thumbSizeCombo->setCurrentIndex(1);
  }
  m_thumbSizeCombo->setToolTip(tr("Thumbnail size"));
  connect(m_thumbSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int) {
            if (!m_thumbSizeCombo) {
              return;
            }
            const int size = m_thumbSizeCombo->currentData().toInt();
            if (size > 0) {
              updateThumbnailSize(size);
            }
          });
  m_toolBar->addWidget(m_thumbSizeCombo);

  m_toolBar->addSeparator();

  QAction *actionImport = m_toolBar->addAction(tr("Import"));
  actionImport->setToolTip(tr("Import Assets"));
  connect(actionImport, &QAction::triggered, this,
          &NMAssetBrowserPanel::onImportAssets);

  if (auto *layout = qobject_cast<QVBoxLayout *>(m_contentWidget->layout())) {
    layout->insertWidget(0, m_toolBar);
  }
}

void NMAssetBrowserPanel::setupContent() {
  m_contentWidget = new QWidget(this);
  auto *layout = new QVBoxLayout(m_contentWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Create splitter for tree/list view
  m_splitter = new QSplitter(Qt::Horizontal, m_contentWidget);

  // Tree view for directory navigation
  m_treeModel = new QFileSystemModel(this);
  m_treeModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

  m_treeView = new QTreeView(m_splitter);
  m_treeView->setModel(m_treeModel);
  m_treeView->setHeaderHidden(true);
  // Hide all columns except name
  for (int i = 1; i < m_treeModel->columnCount(); ++i) {
    m_treeView->hideColumn(i);
  }
  m_treeView->setAnimated(true);
  m_treeView->setIndentation(16);

  connect(m_treeView, &QTreeView::clicked, this,
          &NMAssetBrowserPanel::onTreeClicked);

  m_splitter->addWidget(m_treeView);

  // List view for asset display
  m_listModel = new QFileSystemModel(this);
  m_listModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

  m_filterProxy = new NMAssetFilterProxy(this);
  m_filterProxy->setSourceModel(m_listModel);

  m_listView = new QListView(m_splitter);
  m_listView->setObjectName("AssetBrowserListView");
  m_listView->setModel(m_filterProxy);
  m_listView->setViewMode(QListView::IconMode);
  m_listView->setIconSize(QSize(kAssetThumbSize, kAssetThumbSize));
  m_listView->setGridSize(QSize(kAssetThumbSize + 28, kAssetThumbSize + 32));
  m_listView->setSpacing(6);
  m_listView->setUniformItemSizes(true);
  m_listView->setWordWrap(true);
  m_listView->setResizeMode(QListView::Adjust);
  m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
  m_listView->setDragEnabled(true);
  m_listView->setDragDropMode(QAbstractItemView::DragOnly);
  m_listView->setAcceptDrops(true);
  m_listView->viewport()->setAcceptDrops(true);
  m_listView->installEventFilter(this);

  connect(m_listView, &QListView::doubleClicked, this,
          &NMAssetBrowserPanel::onListDoubleClicked);
  connect(m_listView, &QListView::customContextMenuRequested, this,
          &NMAssetBrowserPanel::onListContextMenu);
  if (auto *selectionModel = m_listView->selectionModel()) {
    connect(selectionModel, &QItemSelectionModel::currentChanged, this,
            &NMAssetBrowserPanel::onListSelectionChanged);
  }

  m_listPane = new QWidget(m_splitter);
  auto *listLayout = new QVBoxLayout(m_listPane);
  listLayout->setContentsMargins(0, 0, 0, 0);
  listLayout->setSpacing(4);
  listLayout->addWidget(m_listView, 1);

  m_previewFrame = new QFrame(m_listPane);
  m_previewFrame->setObjectName("AssetPreviewFrame");
  m_previewFrame->setFrameShape(QFrame::StyledPanel);
  auto *previewLayout = new QVBoxLayout(m_previewFrame);
  previewLayout->setContentsMargins(6, 6, 6, 6);
  previewLayout->setSpacing(3);

  m_previewImage = new QLabel(m_previewFrame);
  m_previewImage->setAlignment(Qt::AlignCenter);
  m_previewImage->setMinimumHeight(kPreviewHeight);
  m_previewImage->setText(tr("No preview"));

  m_previewName = new QLabel(m_previewFrame);
  m_previewName->setWordWrap(true);
  m_previewMeta = new QLabel(m_previewFrame);
  m_previewMeta->setWordWrap(true);

  previewLayout->addWidget(m_previewImage);
  previewLayout->addWidget(m_previewName);
  previewLayout->addWidget(m_previewMeta);

  listLayout->addWidget(m_previewFrame);

  m_splitter->addWidget(m_listPane);

  // Set splitter sizes (30% tree, 70% list)
  m_splitter->setSizes({180, 520});

  layout->addWidget(m_splitter);

  setContentWidget(m_contentWidget);
  applyFilters();

  m_iconProvider = new NMAssetIconProvider(m_listView->iconSize(), this);
  m_listModel->setIconProvider(m_iconProvider);
  m_listModel->setObjectName("AssetBrowserListModel");
  m_treeModel->setObjectName("AssetBrowserTreeModel");

  m_treeView->setAcceptDrops(true);
  m_treeView->viewport()->setAcceptDrops(true);
  m_treeView->installEventFilter(this);

  updateThumbnailSize(kAssetThumbSize);
  setPreviewVisible(m_previewVisible);
}

void NMAssetBrowserPanel::onTreeClicked(const QModelIndex &index) {
  if (!index.isValid())
    return;

  QString path = m_treeModel->filePath(index);
  m_currentPath = path;

  if (m_listModel) {
    if (m_filterProxy) {
      if (auto *proxy = static_cast<NMAssetFilterProxy *>(m_filterProxy)) {
        proxy->setPinnedDirectory(path);
      }
    }
    m_listModel->setRootPath(path);
    QModelIndex idx = m_listModel->index(path);
    m_listView->setRootIndex(m_filterProxy ? m_filterProxy->mapFromSource(idx)
                                           : idx);
  }
}

void NMAssetBrowserPanel::onListDoubleClicked(const QModelIndex &index) {
  if (!index.isValid())
    return;

  QModelIndex source = m_filterProxy ? m_filterProxy->mapToSource(index) : index;
  QString path = m_listModel->filePath(source);
  emit assetDoubleClicked(path);
}

void NMAssetBrowserPanel::onListContextMenu(const QPoint &pos) {
  QModelIndex index = m_listView->indexAt(pos);
  if (!index.isValid())
    return;

  QModelIndex source = m_filterProxy ? m_filterProxy->mapToSource(index) : index;
  QString path = m_listModel->filePath(source);
  emit assetContextMenu(path, m_listView->mapToGlobal(pos));
}

void NMAssetBrowserPanel::onListSelectionChanged(
    const QModelIndex &current, const QModelIndex & /*previous*/) {
  QString path;
  if (current.isValid()) {
    QModelIndex source =
        m_filterProxy ? m_filterProxy->mapToSource(current) : current;
    path = m_listModel->filePath(source);
  }
  updatePreview(path);
  if (!path.isEmpty()) {
    emit assetSelected(path);
  }
}

void NMAssetBrowserPanel::onImportAssets() {
  auto &projectManager = ProjectManager::instance();
  if (!projectManager.hasOpenProject()) {
    NMMessageDialog::showInfo(this, tr("Import Assets"),
                              tr("Open a project before importing assets."));
    return;
  }

  const QString filter =
      tr("Assets (*.png *.jpg *.jpeg *.bmp *.gif *.wav *.mp3 *.ogg *.flac "
         "*.ttf *.otf *.nms *.nmscene *.json *.xml *.yaml *.yml)");
  QStringList files = NMFileDialog::getOpenFileNames(
      this, tr("Import Assets"), QDir::homePath(), filter);
  if (files.isEmpty()) {
    return;
  }

  importFiles(files, true);
}

void NMAssetBrowserPanel::importFiles(const QStringList &files,
                                      bool interactive) {
  auto &projectManager = ProjectManager::instance();
  if (!projectManager.hasOpenProject() || files.isEmpty()) {
    return;
  }

  enum class ImportTargetMode { AutoByType, CurrentFolder, CustomFolder };

  const QString assetsRoot = QString::fromStdString(
      projectManager.getFolderPath(ProjectFolder::Assets));
  QString currentTarget;
  if (!m_currentPath.isEmpty()) {
    QDir currentDir(m_currentPath);
    if (currentDir.exists() && m_currentPath.startsWith(assetsRoot) &&
        projectManager.isPathInProject(m_currentPath.toStdString())) {
      currentTarget = m_currentPath;
    }
  }

  ImportTargetMode targetMode = ImportTargetMode::AutoByType;
  QString customTarget;
  auto chooseImportOptions = [&](ImportTargetMode &mode,
                                 QString &customDir) -> bool {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Import Assets"));
    dialog.setModal(true);
    dialog.setMinimumSize(600, 380);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *header = new QLabel(
        tr("Selected %1 file(s) for import").arg(files.size()), &dialog);
    layout->addWidget(header);

    auto *list = new QListWidget(&dialog);
    list->setViewMode(QListView::IconMode);
    list->setIconSize(QSize(48, 48));
    list->setResizeMode(QListWidget::Adjust);
    list->setSpacing(6);
    list->setMinimumHeight(150);

    QFileIconProvider iconProvider;
    for (const QString &path : files) {
      QFileInfo info(path);
      QListWidgetItem *item = new QListWidgetItem(info.fileName(), list);
      item->setToolTip(info.absoluteFilePath());
      if (info.suffix().compare("png", Qt::CaseInsensitive) == 0 ||
          info.suffix().compare("jpg", Qt::CaseInsensitive) == 0 ||
          info.suffix().compare("jpeg", Qt::CaseInsensitive) == 0 ||
          info.suffix().compare("bmp", Qt::CaseInsensitive) == 0 ||
          info.suffix().compare("gif", Qt::CaseInsensitive) == 0) {
        QPixmap pix(path);
        if (!pix.isNull()) {
          item->setIcon(QIcon(pix.scaled(56, 56, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation)));
          continue;
        }
      }
      item->setIcon(iconProvider.icon(info));
    }
    layout->addWidget(list, 1);

    auto *destLabel = new QLabel(tr("Import destination"), &dialog);
    layout->addWidget(destLabel);

    auto *destRow = new QHBoxLayout();
    auto *destCombo = new QComboBox(&dialog);
    destCombo->addItem(tr("Auto by type (Images/Audio/Fonts/etc)"),
                       static_cast<int>(ImportTargetMode::AutoByType));
    const QString currentLabel =
        currentTarget.isEmpty()
            ? tr("Current folder")
            : tr("Current folder: %1")
                  .arg(QDir(assetsRoot).relativeFilePath(currentTarget));
    destCombo->addItem(currentLabel,
                       static_cast<int>(ImportTargetMode::CurrentFolder));
    destCombo->addItem(tr("Choose folder..."),
                       static_cast<int>(ImportTargetMode::CustomFolder));

    auto *customEdit = new QLineEdit(&dialog);
    customEdit->setPlaceholderText(tr("Select a folder inside Assets"));
    customEdit->setEnabled(false);
    auto *browseButton = new QPushButton(tr("Browse"), &dialog);
    browseButton->setEnabled(false);

    destRow->addWidget(destCombo, 1);
    destRow->addWidget(customEdit, 2);
    destRow->addWidget(browseButton);
    layout->addLayout(destRow);

    auto updateCustomState = [&]() {
      const auto modeValue =
          static_cast<ImportTargetMode>(destCombo->currentData().toInt());
      const bool custom = (modeValue == ImportTargetMode::CustomFolder);
      customEdit->setEnabled(custom);
      browseButton->setEnabled(custom);
    };
    updateCustomState();

    connect(destCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &dialog, [&](int) { updateCustomState(); });
    connect(browseButton, &QPushButton::clicked, &dialog, [&]() {
      const QString dir = NMFileDialog::getExistingDirectory(
          &dialog, tr("Select Import Folder"), assetsRoot);
      if (!dir.isEmpty()) {
        customEdit->setText(dir);
      }
    });

    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                             &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
      return false;
    }

    mode = static_cast<ImportTargetMode>(destCombo->currentData().toInt());
    if (mode == ImportTargetMode::CustomFolder) {
      customDir = customEdit->text().trimmed();
      if (customDir.isEmpty()) {
        NMMessageDialog::showWarning(
            this, tr("Import Assets"),
            tr("Please choose a destination folder inside Assets."));
        return false;
      }
      if (!customDir.startsWith(assetsRoot)) {
        NMMessageDialog::showWarning(
            this, tr("Import Assets"),
            tr("Destination must be inside the Assets folder."));
        return false;
      }
    }

    return true;
  };

  if (interactive) {
    if (!chooseImportOptions(targetMode, customTarget)) {
      return;
    }
  } else {
    targetMode = currentTarget.isEmpty() ? ImportTargetMode::AutoByType
                                         : ImportTargetMode::CurrentFolder;
  }

  int importedCount = 0;
  QSet<QString> targetDirs;
  QStringList failedFiles;
  for (const QString &filePath : files) {
    QFileInfo info(filePath);
    if (!info.exists()) {
      failedFiles.append(info.fileName());
      QtEventBus::instance().publish(
          {QtEditorEventType::ErrorOccurred, "AssetImport",
           {{"message", tr("Missing source file: %1").arg(info.fileName())}}});
      continue;
    }

    const QString extension = info.suffix().toLower();
    QString targetDir;
    switch (targetMode) {
    case ImportTargetMode::AutoByType:
      targetDir =
          currentTarget.isEmpty() ? importDestinationForExtension(extension)
                                  : currentTarget;
      break;
    case ImportTargetMode::CurrentFolder:
      targetDir = currentTarget.isEmpty() ? assetsRoot : currentTarget;
      break;
    case ImportTargetMode::CustomFolder:
      targetDir = customTarget;
      break;
    }

    if (targetDir.isEmpty()) {
      targetDir = assetsRoot;
    }

    QDir target(targetDir);
    if (!target.exists()) {
      target.mkpath(".");
    }

    const QString destPath =
        generateUniquePath(target.absolutePath(), info.fileName());

    if (auto *database = projectManager.assetDatabase()) {
      auto result =
          database->importAssetToPath(filePath.toStdString(),
                                      destPath.toStdString());
      if (result.isError()) {
        failedFiles.append(info.fileName());
        QtEventBus::instance().publish(
            {QtEditorEventType::ErrorOccurred, "AssetImport",
             {{"message",
               tr("Import failed: %1").arg(info.fileName())},
              {"details",
               QString::fromStdString(result.error())}}});
        continue;
      }
    } else {
      if (!QFile::copy(filePath, destPath)) {
        failedFiles.append(info.fileName());
        QtEventBus::instance().publish(
            {QtEditorEventType::ErrorOccurred, "AssetImport",
             {{"message",
               tr("Import failed: %1").arg(info.fileName())},
              {"details", tr("File copy failed")}}});
        continue;
      }
    }

    importedCount++;
    targetDirs.insert(target.absolutePath());
  }

  if (importedCount > 0) {
    if (targetDirs.size() == 1) {
      const QString targetDir = *targetDirs.begin();
      m_currentPath = targetDir;
      if (m_listModel && m_listView) {
        m_listModel->setRootPath(targetDir);
        const QModelIndex idx = m_listModel->index(targetDir);
        m_listView->setRootIndex(m_filterProxy ? m_filterProxy->mapFromSource(idx)
                                               : idx);
      }
      if (m_treeModel && m_treeView) {
        const QModelIndex index = m_treeModel->index(targetDir);
        if (index.isValid()) {
          m_treeView->setCurrentIndex(index);
          m_treeView->scrollTo(index);
        }
      }
    }
  }
  if (importedCount == 0) {
    NMMessageDialog::showWarning(
        this, tr("Import Assets"),
        tr("No assets were imported. Check file permissions or formats."));
  } else if (!failedFiles.isEmpty()) {
    NMMessageDialog::showWarning(
        this, tr("Import Assets"),
        tr("Imported %1 asset(s). Failed to import:\n%2")
            .arg(importedCount)
            .arg(failedFiles.join("\n")));
  }
  refresh();
}

bool NMAssetBrowserPanel::eventFilter(QObject *watched, QEvent *event) {
  if ((watched == m_listView || watched == m_treeView ||
       watched == m_listView->viewport() ||
       watched == m_treeView->viewport()) &&
      event) {
    if (event->type() == QEvent::DragEnter) {
      auto *dragEvent = static_cast<QDragEnterEvent *>(event);
      if (dragEvent->mimeData() && dragEvent->mimeData()->hasUrls()) {
        dragEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::DragMove) {
      auto *dragEvent = static_cast<QDragMoveEvent *>(event);
      if (dragEvent->mimeData() && dragEvent->mimeData()->hasUrls()) {
        dragEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::Drop) {
      auto *dropEvent = static_cast<QDropEvent *>(event);
      if (dropEvent->mimeData() && dropEvent->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl &url : dropEvent->mimeData()->urls()) {
          if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
          }
        }
        importFiles(paths, false);
        dropEvent->acceptProposedAction();
        return true;
      }
    }
  }
  return NMDockPanel::eventFilter(watched, event);
}

void NMAssetBrowserPanel::onFilterTextChanged(const QString &text) {
  if (m_filterProxy) {
    auto *proxy = static_cast<NMAssetFilterProxy *>(m_filterProxy);
    proxy->setNameFilterText(text);
  }
}

void NMAssetBrowserPanel::onTypeFilterChanged(int index) {
  Q_UNUSED(index);
  applyFilters();
}

void NMAssetBrowserPanel::applyFilters() {
  if (!m_listModel || !m_filterProxy) {
    return;
  }

  auto *proxy = static_cast<NMAssetFilterProxy *>(m_filterProxy);
  proxy->setTypeFilterIndex(m_typeFilter ? m_typeFilter->currentIndex() : 0);
  if (m_filterEdit) {
    proxy->setNameFilterText(m_filterEdit->text());
  }
  updatePreview(selectedAssetPath());
}

void NMAssetBrowserPanel::setPreviewVisible(bool visible) {
  m_previewVisible = visible;
  if (m_previewFrame) {
    m_previewFrame->setVisible(visible);
  }
  if (visible) {
    updatePreview(selectedAssetPath());
  }
}

void NMAssetBrowserPanel::updateThumbnailSize(int size) {
  if (size < 48) {
    size = 48;
  }
  m_thumbSize = size;
  if (m_listView) {
    m_listView->setIconSize(QSize(size, size));
    m_listView->setGridSize(QSize(size + 28, size + 32));
    m_listView->update();
  }
  if (m_iconProvider) {
    auto *provider = static_cast<NMAssetIconProvider *>(m_iconProvider);
    provider->setIconSize(QSize(size, size));
  }
}

void NMAssetBrowserPanel::updatePreview(const QString &path) {
  if (!m_previewImage || !m_previewName || !m_previewMeta) {
    return;
  }
  if (!m_previewVisible) {
    return;
  }

  m_previewPath = path;
  if (path.isEmpty()) {
    clearPreview();
    return;
  }

  QFileInfo info(path);
  if (!info.exists() || !info.isFile()) {
    clearPreview();
    return;
  }

  m_previewName->setText(info.fileName());
  const QString sizeText =
      tr("%1 KB").arg(QString::number((info.size() + 1023) / 1024));

  if (isImageExtension(info.suffix())) {
    QImageReader reader(info.absoluteFilePath());
    reader.setAutoTransform(true);
    QSize imgSize = reader.size();
    QImage image = reader.read();
    if (!image.isNull()) {
      QPixmap pix = QPixmap::fromImage(image);
      QSize target = m_previewImage->size();
      QPixmap scaled = pix.scaled(target, Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);
      m_previewImage->setPixmap(scaled);
      m_previewImage->setText(QString());
      m_previewMeta->setText(
          tr("%1 x %2 | %3").arg(imgSize.width()).arg(imgSize.height()).arg(
              sizeText));
      return;
    }
  }

  m_previewImage->setPixmap(QPixmap());
  m_previewImage->setText(tr("No preview"));
  m_previewMeta->setText(sizeText);
}

void NMAssetBrowserPanel::clearPreview() {
  m_previewPath.clear();
  if (m_previewImage) {
    m_previewImage->setPixmap(QPixmap());
    m_previewImage->setText(tr("No preview"));
  }
  if (m_previewName) {
    m_previewName->setText(QString());
  }
  if (m_previewMeta) {
    m_previewMeta->setText(QString());
  }
}

QString NMAssetBrowserPanel::importDestinationForExtension(
    const QString &extension) const {
  auto &projectManager = ProjectManager::instance();
  if (!projectManager.hasOpenProject()) {
    return QString();
  }

  const QString ext = extension.toLower();
  if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
      ext == "gif") {
    return QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Images));
  }
  if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac") {
    return QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Audio));
  }
  if (ext == "ttf" || ext == "otf") {
    return QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Fonts));
  }
  if (ext == "nms") {
    return QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Scripts));
  }
  if (ext == "nmscene") {
    return QString::fromStdString(
        projectManager.getFolderPath(ProjectFolder::Scenes));
  }

  return QString::fromStdString(
      projectManager.getFolderPath(ProjectFolder::Assets));
}

QString NMAssetBrowserPanel::generateUniquePath(const QString &directory,
                                                const QString &fileName) const {
  QDir dir(directory);
  QString baseName = QFileInfo(fileName).completeBaseName();
  QString suffix = QFileInfo(fileName).suffix();
  QString candidate = dir.filePath(fileName);
  int counter = 1;

  while (QFileInfo::exists(candidate)) {
    QString numbered =
        suffix.isEmpty()
            ? QString("%1_%2").arg(baseName).arg(counter)
            : QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
    candidate = dir.filePath(numbered);
    counter++;
  }

  return candidate;
}

} // namespace NovelMind::editor::qt


