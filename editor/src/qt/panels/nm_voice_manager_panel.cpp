#include "NovelMind/editor/qt/panels/nm_voice_manager_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSplitter>
#include <QTextStream>
#include <QToolBar>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <filesystem>

namespace fs = std::filesystem;

namespace NovelMind::editor::qt {

NMVoiceManagerPanel::NMVoiceManagerPanel(QWidget *parent)
    : NMDockPanel("Voice Manager", parent) {}

NMVoiceManagerPanel::~NMVoiceManagerPanel() = default;

void NMVoiceManagerPanel::onInitialize() { setupUI(); }

void NMVoiceManagerPanel::onShutdown() { stopPlayback(); }

void NMVoiceManagerPanel::onUpdate([[maybe_unused]] double deltaTime) {
  // Update playback progress if playing
  if (m_isPlaying && m_playbackProgress) {
    m_playbackPosition += deltaTime;
  }
}

void NMVoiceManagerPanel::setupUI() {
  auto *mainWidget = new QWidget(this);
  auto *mainLayout = new QVBoxLayout(mainWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(4);

  setupToolBar();
  mainLayout->addWidget(m_toolbar);

  setupFilterBar();

  // Main content - tree view of voice lines
  setupVoiceList();
  mainLayout->addWidget(m_voiceTree, 1);

  // Preview/playback bar
  setupPreviewBar();

  // Statistics bar
  m_statsLabel = new QLabel(mainWidget);
  m_statsLabel->setStyleSheet("padding: 4px; color: #888;");
  m_statsLabel->setText(tr("0 dialogue lines, 0 matched, 0 unmatched"));
  mainLayout->addWidget(m_statsLabel);

  setContentWidget(mainWidget);
}

void NMVoiceManagerPanel::setupToolBar() {
  m_toolbar = new QToolBar(this);
  m_toolbar->setIconSize(QSize(16, 16));

  auto *scanBtn = new QPushButton(tr("Scan"), m_toolbar);
  scanBtn->setToolTip(tr("Scan project for dialogue lines and voice files"));
  connect(scanBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onScanClicked);
  m_toolbar->addWidget(scanBtn);

  auto *autoMatchBtn = new QPushButton(tr("Auto-Match"), m_toolbar);
  autoMatchBtn->setToolTip(
      tr("Automatically match voice files to dialogue lines"));
  connect(autoMatchBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onAutoMatchClicked);
  m_toolbar->addWidget(autoMatchBtn);

  m_toolbar->addSeparator();

  auto *importBtn = new QPushButton(tr("Import"), m_toolbar);
  importBtn->setToolTip(tr("Import voice mapping from CSV"));
  connect(importBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onImportClicked);
  m_toolbar->addWidget(importBtn);

  auto *exportBtn = new QPushButton(tr("Export"), m_toolbar);
  exportBtn->setToolTip(tr("Export voice mapping to CSV"));
  connect(exportBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onExportClicked);
  m_toolbar->addWidget(exportBtn);

  m_toolbar->addSeparator();

  auto *openFolderBtn = new QPushButton(tr("Open Folder"), m_toolbar);
  openFolderBtn->setToolTip(tr("Open the voice files folder"));
  connect(openFolderBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onOpenVoiceFolder);
  m_toolbar->addWidget(openFolderBtn);
}

void NMVoiceManagerPanel::setupFilterBar() {
  auto *filterWidget = new QWidget(m_toolbar);
  auto *filterLayout = new QHBoxLayout(filterWidget);
  filterLayout->setContentsMargins(4, 2, 4, 2);
  filterLayout->setSpacing(6);

  filterLayout->addWidget(new QLabel(tr("Filter:"), filterWidget));

  m_filterEdit = new QLineEdit(filterWidget);
  m_filterEdit->setPlaceholderText(tr("Search dialogue text..."));
  m_filterEdit->setMaximumWidth(200);
  connect(m_filterEdit, &QLineEdit::textChanged, this,
          &NMVoiceManagerPanel::onFilterChanged);
  filterLayout->addWidget(m_filterEdit);

  filterLayout->addWidget(new QLabel(tr("Character:"), filterWidget));

  m_characterFilter = new QComboBox(filterWidget);
  m_characterFilter->addItem(tr("All Characters"));
  m_characterFilter->setMinimumWidth(120);
  connect(m_characterFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &NMVoiceManagerPanel::onCharacterFilterChanged);
  filterLayout->addWidget(m_characterFilter);

  m_showUnmatchedBtn = new QPushButton(tr("Unmatched Only"), filterWidget);
  m_showUnmatchedBtn->setCheckable(true);
  m_showUnmatchedBtn->setToolTip(tr("Show only lines without voice files"));
  connect(m_showUnmatchedBtn, &QPushButton::toggled, this,
          &NMVoiceManagerPanel::onShowOnlyUnmatched);
  filterLayout->addWidget(m_showUnmatchedBtn);

  filterLayout->addStretch();

  m_toolbar->addWidget(filterWidget);
}

void NMVoiceManagerPanel::setupVoiceList() {
  m_voiceTree = new QTreeWidget(this);
  m_voiceTree->setHeaderLabels(
      {tr("Line ID"), tr("Speaker"), tr("Dialogue Text"), tr("Voice File"),
       tr("Actor"), tr("Duration"), tr("Status")});
  m_voiceTree->setAlternatingRowColors(true);
  m_voiceTree->setSelectionMode(QAbstractItemView::SingleSelection);
  m_voiceTree->setContextMenuPolicy(Qt::CustomContextMenu);

  // Adjust column widths
  m_voiceTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_voiceTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
  m_voiceTree->setColumnWidth(0, 120);
  m_voiceTree->setColumnWidth(1, 100);
  m_voiceTree->setColumnWidth(3, 150);
  m_voiceTree->setColumnWidth(4, 80);
  m_voiceTree->setColumnWidth(5, 60);
  m_voiceTree->setColumnWidth(6, 80);

  connect(m_voiceTree, &QTreeWidget::itemClicked, this,
          &NMVoiceManagerPanel::onLineSelected);
  connect(m_voiceTree, &QTreeWidget::customContextMenuRequested, this,
          [this](const QPoint &pos) {
            auto *item = m_voiceTree->itemAt(pos);
            if (!item)
              return;

            QMenu menu;
            menu.addAction(tr("Assign Voice File"), this,
                           &NMVoiceManagerPanel::onAssignVoiceFile);
            menu.addAction(tr("Clear Voice File"), this,
                           &NMVoiceManagerPanel::onClearVoiceFile);
            menu.addSeparator();
            menu.addAction(tr("Go to Script"), this, [this, item]() {
              QString dialogueId = item->data(0, Qt::UserRole).toString();
              emit voiceLineSelected(dialogueId);
            });
            menu.exec(m_voiceTree->mapToGlobal(pos));
          });
}

void NMVoiceManagerPanel::setupPreviewBar() {
  auto *previewWidget = new QWidget(contentWidget());
  auto *previewLayout = new QHBoxLayout(previewWidget);
  previewLayout->setContentsMargins(4, 4, 4, 4);
  previewLayout->setSpacing(6);

  m_playBtn = new QPushButton(tr("Play"), previewWidget);
  m_playBtn->setEnabled(false);
  connect(m_playBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onPlayClicked);
  previewLayout->addWidget(m_playBtn);

  m_stopBtn = new QPushButton(tr("Stop"), previewWidget);
  m_stopBtn->setEnabled(false);
  connect(m_stopBtn, &QPushButton::clicked, this,
          &NMVoiceManagerPanel::onStopClicked);
  previewLayout->addWidget(m_stopBtn);

  m_playbackProgress = new QProgressBar(previewWidget);
  m_playbackProgress->setRange(0, 1000);
  m_playbackProgress->setValue(0);
  m_playbackProgress->setTextVisible(false);
  previewLayout->addWidget(m_playbackProgress, 1);

  m_durationLabel = new QLabel("0:00 / 0:00", previewWidget);
  m_durationLabel->setMinimumWidth(80);
  previewLayout->addWidget(m_durationLabel);

  previewLayout->addWidget(new QLabel(tr("Vol:"), previewWidget));

  m_volumeSlider = new QSlider(Qt::Horizontal, previewWidget);
  m_volumeSlider->setRange(0, 100);
  m_volumeSlider->setValue(100);
  m_volumeSlider->setMaximumWidth(80);
  connect(m_volumeSlider, &QSlider::valueChanged, this,
          &NMVoiceManagerPanel::onVolumeChanged);
  previewLayout->addWidget(m_volumeSlider);

  if (auto *layout = qobject_cast<QVBoxLayout *>(contentWidget()->layout())) {
    layout->addWidget(previewWidget);
  }
}

void NMVoiceManagerPanel::scanProject() {
  m_voiceLines.clear();
  m_voiceFiles.clear();
  m_characters.clear();

  scanScriptsForDialogue();
  scanVoiceFolder();
  autoMatchVoiceFiles();
  updateVoiceList();
  updateStatistics();
}

void NMVoiceManagerPanel::scanScriptsForDialogue() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    return;
  }

  QString scriptsDir =
      QString::fromStdString(pm.getFolderPath(ProjectFolder::Scripts));
  if (scriptsDir.isEmpty() || !fs::exists(scriptsDir.toStdString())) {
    return;
  }

  // Pattern to match dialogue lines: say Character "Text" or Character: "Text"
  QRegularExpression dialoguePattern(
      R"((?:say\s+)?(\w+)\s*[:\s]?\s*\"([^\"]+)\")");

  for (const auto &entry :
       fs::recursive_directory_iterator(scriptsDir.toStdString())) {
    if (!entry.is_regular_file() || entry.path().extension() != ".nms") {
      continue;
    }

    QString scriptPath = QString::fromStdString(entry.path().string());
    QString relPath = QString::fromStdString(
        pm.toRelativePath(entry.path().string()));

    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }

    QTextStream in(&file);
    int lineNum = 0;
    while (!in.atEnd()) {
      QString line = in.readLine();
      lineNum++;

      QRegularExpressionMatch match = dialoguePattern.match(line);
      if (match.hasMatch()) {
        VoiceLineEntry voiceEntry;
        voiceEntry.dialogueId = generateDialogueId(relPath, lineNum);
        voiceEntry.scriptPath = relPath;
        voiceEntry.lineNumber = lineNum;
        voiceEntry.speaker = match.captured(1);
        voiceEntry.dialogueText = match.captured(2);

        m_voiceLines.insert(voiceEntry.dialogueId, voiceEntry);

        if (!m_characters.contains(voiceEntry.speaker)) {
          m_characters.append(voiceEntry.speaker);
        }
      }
    }
    file.close();
  }

  // Update character filter
  if (m_characterFilter) {
    m_characterFilter->clear();
    m_characterFilter->addItem(tr("All Characters"));
    m_characterFilter->addItems(m_characters);
  }
}

void NMVoiceManagerPanel::scanVoiceFolder() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    return;
  }

  QString voiceDir = QString::fromStdString(pm.getProjectPath()) + "/Assets/Voice";
  if (!fs::exists(voiceDir.toStdString())) {
    return;
  }

  for (const auto &entry :
       fs::recursive_directory_iterator(voiceDir.toStdString())) {
    if (!entry.is_regular_file()) {
      continue;
    }

    QString ext = QString::fromStdString(entry.path().extension().string()).toLower();
    if (ext == ".ogg" || ext == ".wav" || ext == ".mp3" || ext == ".flac") {
      m_voiceFiles.append(QString::fromStdString(entry.path().string()));
    }
  }
}

void NMVoiceManagerPanel::autoMatchVoiceFiles() {
  for (const QString &voiceFile : m_voiceFiles) {
    matchVoiceToDialogue(voiceFile);
  }
}

void NMVoiceManagerPanel::matchVoiceToDialogue(const QString &voiceFile) {
  QString filename = QFileInfo(voiceFile).baseName();

  // Try exact match with dialogue ID
  if (m_voiceLines.contains(filename)) {
    m_voiceLines[filename].voiceFilePath = voiceFile;
    m_voiceLines[filename].isMatched = true;
    return;
  }

  // Try pattern matching: character_linenum
  QRegularExpression pattern(R"((\w+)_(\d+))");
  QRegularExpressionMatch match = pattern.match(filename);
  if (match.hasMatch()) {
    QString character = match.captured(1);
    int lineNum = match.captured(2).toInt();

    for (auto it = m_voiceLines.begin(); it != m_voiceLines.end(); ++it) {
      if (it->speaker.compare(character, Qt::CaseInsensitive) == 0 &&
          it->lineNumber == lineNum && !it->isMatched) {
        it->voiceFilePath = voiceFile;
        it->isMatched = true;
        return;
      }
    }
  }
}

QString NMVoiceManagerPanel::generateDialogueId(const QString &scriptPath,
                                                 int lineNumber) const {
  QString baseName = QFileInfo(scriptPath).baseName();
  return QString("%1_%2").arg(baseName).arg(lineNumber);
}

void NMVoiceManagerPanel::updateVoiceList() {
  if (!m_voiceTree) {
    return;
  }

  m_voiceTree->clear();

  QString filter = m_filterEdit ? m_filterEdit->text().trimmed() : QString();
  QString charFilter = m_characterFilter && m_characterFilter->currentIndex() > 0
                           ? m_characterFilter->currentText()
                           : QString();
  bool showUnmatched = m_showUnmatchedBtn && m_showUnmatchedBtn->isChecked();

  for (auto it = m_voiceLines.constBegin(); it != m_voiceLines.constEnd(); ++it) {
    const VoiceLineEntry &entry = it.value();

    if (!filter.isEmpty() &&
        !entry.dialogueText.contains(filter, Qt::CaseInsensitive) &&
        !entry.speaker.contains(filter, Qt::CaseInsensitive)) {
      continue;
    }

    if (!charFilter.isEmpty() &&
        entry.speaker.compare(charFilter, Qt::CaseInsensitive) != 0) {
      continue;
    }

    if (showUnmatched && entry.isMatched) {
      continue;
    }

    auto *item = new QTreeWidgetItem(m_voiceTree);
    item->setData(0, Qt::UserRole, entry.dialogueId);
    item->setText(0, entry.dialogueId);
    item->setText(1, entry.speaker);
    item->setText(2, entry.dialogueText.left(80) +
                         (entry.dialogueText.length() > 80 ? "..." : ""));
    item->setToolTip(2, entry.dialogueText);

    if (entry.isMatched) {
      item->setText(3, QFileInfo(entry.voiceFilePath).fileName());
      item->setToolTip(3, entry.voiceFilePath);
      item->setText(6, entry.isVerified ? tr("Verified") : tr("Matched"));
      item->setForeground(6, entry.isVerified ? QColor(60, 180, 60)
                                              : QColor(200, 180, 60));
    } else {
      item->setText(3, tr("(none)"));
      item->setText(6, tr("Missing"));
      item->setForeground(6, QColor(200, 60, 60));
    }

    item->setText(4, entry.actor);
    if (entry.duration > 0) {
      int secs = static_cast<int>(entry.duration);
      item->setText(5, QString("%1:%2")
                           .arg(secs / 60)
                           .arg(secs % 60, 2, 10, QChar('0')));
    }
  }
}

void NMVoiceManagerPanel::updateStatistics() {
  if (!m_statsLabel) {
    return;
  }

  int total = m_voiceLines.size();
  int matched = 0;
  int verified = 0;

  for (const auto &entry : m_voiceLines) {
    if (entry.isMatched) {
      matched++;
      if (entry.isVerified) {
        verified++;
      }
    }
  }

  m_statsLabel->setText(tr("%1 dialogue lines, %2 matched (%3 verified), %4 unmatched")
                            .arg(total)
                            .arg(matched)
                            .arg(verified)
                            .arg(total - matched));
}

QList<VoiceLineEntry> NMVoiceManagerPanel::getUnmatchedLines() const {
  QList<VoiceLineEntry> result;
  for (const auto &entry : m_voiceLines) {
    if (!entry.isMatched) {
      result.append(entry);
    }
  }
  return result;
}

bool NMVoiceManagerPanel::exportToCsv(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  out << "DialogueID,ScriptPath,LineNumber,Speaker,DialogueText,VoiceFile,Actor,Status\n";

  for (const auto &entry : m_voiceLines) {
    out << "\"" << entry.dialogueId << "\",";
    out << "\"" << entry.scriptPath << "\",";
    out << entry.lineNumber << ",";
    out << "\"" << entry.speaker << "\",";
    out << "\"" << entry.dialogueText.replace("\"", "\"\"") << "\",";
    out << "\"" << QFileInfo(entry.voiceFilePath).fileName() << "\",";
    out << "\"" << entry.actor << "\",";
    out << (entry.isMatched ? (entry.isVerified ? "Verified" : "Matched") : "Missing") << "\n";
  }

  file.close();
  return true;
}

bool NMVoiceManagerPanel::importFromCsv(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream in(&file);
  QString header = in.readLine();
  Q_UNUSED(header);

  while (!in.atEnd()) {
    QString line = in.readLine();
    QStringList fields = line.split(',');
    if (fields.size() < 7) {
      continue;
    }

    QString dialogueId = fields[0].remove('"').trimmed();
    if (!m_voiceLines.contains(dialogueId)) {
      continue;
    }

    QString voiceFileName = fields[5].remove('"').trimmed();
    QString actor = fields[6].remove('"').trimmed();

    if (!voiceFileName.isEmpty()) {
      for (const QString &voicePath : m_voiceFiles) {
        if (QFileInfo(voicePath).fileName() == voiceFileName) {
          m_voiceLines[dialogueId].voiceFilePath = voicePath;
          m_voiceLines[dialogueId].isMatched = true;
          break;
        }
      }
    }

    m_voiceLines[dialogueId].actor = actor;
  }

  file.close();
  updateVoiceList();
  updateStatistics();
  return true;
}

bool NMVoiceManagerPanel::playVoiceFile(const QString &filePath) {
  if (filePath.isEmpty() || !QFile::exists(filePath)) {
    return false;
  }

  stopPlayback();
  m_currentlyPlayingFile = filePath;
  m_isPlaying = true;
  m_playbackPosition = 0.0;

  if (m_playBtn)
    m_playBtn->setEnabled(false);
  if (m_stopBtn)
    m_stopBtn->setEnabled(true);

  return true;
}

void NMVoiceManagerPanel::stopPlayback() {
  m_isPlaying = false;
  m_currentlyPlayingFile.clear();
  m_playbackPosition = 0.0;

  if (m_playBtn)
    m_playBtn->setEnabled(true);
  if (m_stopBtn)
    m_stopBtn->setEnabled(false);
  if (m_playbackProgress)
    m_playbackProgress->setValue(0);
}

void NMVoiceManagerPanel::onScanClicked() { scanProject(); }

void NMVoiceManagerPanel::onAutoMatchClicked() {
  autoMatchVoiceFiles();
  updateVoiceList();
  updateStatistics();
}

void NMVoiceManagerPanel::onImportClicked() {
  QString filePath = QFileDialog::getOpenFileName(
      this, tr("Import Voice Mapping"), QString(), tr("CSV Files (*.csv)"));
  if (!filePath.isEmpty()) {
    importFromCsv(filePath);
  }
}

void NMVoiceManagerPanel::onExportClicked() {
  QString filePath = QFileDialog::getSaveFileName(
      this, tr("Export Voice Mapping"), "voice_mapping.csv",
      tr("CSV Files (*.csv)"));
  if (!filePath.isEmpty()) {
    exportToCsv(filePath);
  }
}

void NMVoiceManagerPanel::onPlayClicked() {
  auto *item = m_voiceTree->currentItem();
  if (!item) {
    return;
  }

  QString dialogueId = item->data(0, Qt::UserRole).toString();
  if (m_voiceLines.contains(dialogueId) && m_voiceLines[dialogueId].isMatched) {
    playVoiceFile(m_voiceLines[dialogueId].voiceFilePath);
  }
}

void NMVoiceManagerPanel::onStopClicked() { stopPlayback(); }

void NMVoiceManagerPanel::onLineSelected(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (!item) {
    return;
  }

  QString dialogueId = item->data(0, Qt::UserRole).toString();
  if (m_voiceLines.contains(dialogueId) && m_voiceLines[dialogueId].isMatched) {
    m_playBtn->setEnabled(true);
  } else {
    m_playBtn->setEnabled(false);
  }

  emit voiceLineSelected(dialogueId);
}

void NMVoiceManagerPanel::onFilterChanged(const QString &text) {
  Q_UNUSED(text);
  updateVoiceList();
}

void NMVoiceManagerPanel::onCharacterFilterChanged(int index) {
  Q_UNUSED(index);
  updateVoiceList();
}

void NMVoiceManagerPanel::onShowOnlyUnmatched(bool checked) {
  Q_UNUSED(checked);
  updateVoiceList();
}

void NMVoiceManagerPanel::onVolumeChanged(int value) {
  m_volume = value / 100.0;
}

void NMVoiceManagerPanel::onAssignVoiceFile() {
  auto *item = m_voiceTree->currentItem();
  if (!item) {
    return;
  }

  QString dialogueId = item->data(0, Qt::UserRole).toString();
  if (!m_voiceLines.contains(dialogueId)) {
    return;
  }

  auto &pm = ProjectManager::instance();
  QString voiceDir = QString::fromStdString(pm.getProjectPath()) + "/Assets/Voice";

  QString filePath = QFileDialog::getOpenFileName(
      this, tr("Select Voice File"), voiceDir,
      tr("Audio Files (*.ogg *.wav *.mp3 *.flac)"));

  if (!filePath.isEmpty()) {
    m_voiceLines[dialogueId].voiceFilePath = filePath;
    m_voiceLines[dialogueId].isMatched = true;
    updateVoiceList();
    updateStatistics();
    emit voiceFileChanged(dialogueId, filePath);
  }
}

void NMVoiceManagerPanel::onClearVoiceFile() {
  auto *item = m_voiceTree->currentItem();
  if (!item) {
    return;
  }

  QString dialogueId = item->data(0, Qt::UserRole).toString();
  if (!m_voiceLines.contains(dialogueId)) {
    return;
  }

  m_voiceLines[dialogueId].voiceFilePath.clear();
  m_voiceLines[dialogueId].isMatched = false;
  m_voiceLines[dialogueId].isVerified = false;
  updateVoiceList();
  updateStatistics();
  emit voiceFileChanged(dialogueId, QString());
}

void NMVoiceManagerPanel::onOpenVoiceFolder() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    return;
  }

  QString voiceDir = QString::fromStdString(pm.getProjectPath()) + "/Assets/Voice";

  if (!fs::exists(voiceDir.toStdString())) {
    std::error_code ec;
    fs::create_directories(voiceDir.toStdString(), ec);
  }

  QUrl url = QUrl::fromLocalFile(voiceDir);
  QDesktopServices::openUrl(url);
}

} // namespace NovelMind::editor::qt
