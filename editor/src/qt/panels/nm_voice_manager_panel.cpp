#include "NovelMind/editor/qt/panels/nm_voice_manager_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"

#include <QAudioOutput>
#include <QCheckBox>
#include <QComboBox>
#include <QTimer>
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
#include <QMediaPlayer>
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

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace fs = std::filesystem;

namespace NovelMind::editor::qt {

NMVoiceManagerPanel::NMVoiceManagerPanel(QWidget *parent)
    : NMDockPanel("Voice Manager", parent) {}

NMVoiceManagerPanel::~NMVoiceManagerPanel() {
  // Stop any ongoing playback before destruction
  stopPlayback();

  // Stop probing
  if (m_probePlayer) {
    m_probePlayer->stop();
  }
}

void NMVoiceManagerPanel::onInitialize() {
  setupUI();
  setupMediaPlayer();
}

void NMVoiceManagerPanel::onShutdown() {
  stopPlayback();

  // Clean up probing
  m_probeQueue.clear();
  m_isProbing = false;
  if (m_probePlayer) {
    m_probePlayer->stop();
  }
}

void NMVoiceManagerPanel::onUpdate([[maybe_unused]] double deltaTime) {
  // Qt Multimedia handles position updates via signals, no manual update needed
}

void NMVoiceManagerPanel::setupUI() {
  auto *mainWidget = new QWidget(this);
  setContentWidget(mainWidget);
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
  m_durationLabel->setMinimumWidth(100);
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

void NMVoiceManagerPanel::setupMediaPlayer() {
  // Create audio output with parent ownership
  m_audioOutput = new QAudioOutput(this);
  m_audioOutput->setVolume(static_cast<float>(m_volume));

  // Create media player with parent ownership
  m_mediaPlayer = new QMediaPlayer(this);
  m_mediaPlayer->setAudioOutput(m_audioOutput);

  // Connect signals once during initialization (not on each play)
  connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this,
          &NMVoiceManagerPanel::onPlaybackStateChanged);
  connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this,
          &NMVoiceManagerPanel::onMediaStatusChanged);
  connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this,
          &NMVoiceManagerPanel::onDurationChanged);
  connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this,
          &NMVoiceManagerPanel::onPositionChanged);
  connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this,
          &NMVoiceManagerPanel::onMediaErrorOccurred);

  // Create separate player for duration probing
  m_probePlayer = new QMediaPlayer(this);

  // Connect probe player signals
  connect(m_probePlayer, &QMediaPlayer::durationChanged, this,
          &NMVoiceManagerPanel::onProbeDurationFinished);
  connect(m_probePlayer, &QMediaPlayer::errorOccurred, this,
          [this]([[maybe_unused]] QMediaPlayer::Error error,
                 [[maybe_unused]] const QString &errorString) {
            // On probe error, just move to next file
            if (VERBOSE_LOGGING) {
#ifdef QT_DEBUG
              qDebug() << "Probe error for" << m_currentProbeFile << ":"
                       << errorString;
#endif
            }
            processNextDurationProbe();
          });
}

void NMVoiceManagerPanel::scanProject() {
  m_voiceLines.clear();
  m_voiceFiles.clear();
  m_characters.clear();
  m_durationCache.clear();  // Clear cache on full rescan

  scanScriptsForDialogue();
  scanVoiceFolder();
  autoMatchVoiceFiles();
  updateVoiceList();
  updateStatistics();

  // Start async duration probing for voice files
  startDurationProbing();
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
      qint64 durationMs = static_cast<qint64>(entry.duration * 1000);
      item->setText(5, formatDuration(durationMs));
    }
  }
}

void NMVoiceManagerPanel::updateStatistics() {
  if (!m_statsLabel) {
    return;
  }

  int total = static_cast<int>(m_voiceLines.size());
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
    out << "\"" << QString(entry.dialogueText).replace("\"", "\"\"") << "\",";
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
  if (filePath.isEmpty()) {
    setPlaybackError(tr("No file specified"));
    return false;
  }

  if (!QFile::exists(filePath)) {
    setPlaybackError(tr("File not found: %1").arg(filePath));
    return false;
  }

  // Stop any current playback first
  stopPlayback();

  m_currentlyPlayingFile = filePath;

  // Set the source and play
  m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
  m_mediaPlayer->play();

  m_isPlaying = true;

  if (m_playBtn)
    m_playBtn->setEnabled(false);
  if (m_stopBtn)
    m_stopBtn->setEnabled(true);

  return true;
}

void NMVoiceManagerPanel::stopPlayback() {
  if (m_mediaPlayer) {
    m_mediaPlayer->stop();
    m_mediaPlayer->setSource(QUrl());  // Clear source
  }

  m_isPlaying = false;
  m_currentlyPlayingFile.clear();
  m_currentDuration = 0;

  resetPlaybackUI();
}

void NMVoiceManagerPanel::resetPlaybackUI() {
  if (m_playBtn)
    m_playBtn->setEnabled(true);
  if (m_stopBtn)
    m_stopBtn->setEnabled(false);
  if (m_playbackProgress) {
    m_playbackProgress->setValue(0);
    m_playbackProgress->setRange(0, 1000);  // Reset to default range
  }
  if (m_durationLabel)
    m_durationLabel->setText("0:00 / 0:00");
}

void NMVoiceManagerPanel::setPlaybackError(const QString &message) {
  if (VERBOSE_LOGGING) {
#ifdef QT_DEBUG
    qDebug() << "Playback error:" << message;
#endif
  }

  // Update status label with error
  if (m_statsLabel) {
    m_statsLabel->setStyleSheet("padding: 4px; color: #c44;");
    m_statsLabel->setText(tr("Error: %1").arg(message));

    // Reset style after 3 seconds
    QTimer::singleShot(3000, this, [this]() {
      if (m_statsLabel) {
        m_statsLabel->setStyleSheet("padding: 4px; color: #888;");
        updateStatistics();
      }
    });
  }

  emit playbackError(message);
}

QString NMVoiceManagerPanel::formatDuration(qint64 ms) const {
  int totalSeconds = static_cast<int>(ms / 1000);
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  int tenths = static_cast<int>((ms % 1000) / 100);

  return QString("%1:%2.%3")
      .arg(minutes)
      .arg(seconds, 2, 10, QChar('0'))
      .arg(tenths);
}

// Qt Multimedia signal handlers
void NMVoiceManagerPanel::onPlaybackStateChanged() {
  if (!m_mediaPlayer) return;

  QMediaPlayer::PlaybackState state = m_mediaPlayer->playbackState();

  switch (state) {
    case QMediaPlayer::StoppedState:
      m_isPlaying = false;
      // Only reset UI if we're not switching tracks
      if (m_currentlyPlayingFile.isEmpty()) {
        resetPlaybackUI();
      }
      break;

    case QMediaPlayer::PlayingState:
      m_isPlaying = true;
      if (m_playBtn)
        m_playBtn->setEnabled(false);
      if (m_stopBtn)
        m_stopBtn->setEnabled(true);
      break;

    case QMediaPlayer::PausedState:
      // Currently not exposing pause, but handle it gracefully
      break;
  }
}

void NMVoiceManagerPanel::onMediaStatusChanged() {
  if (!m_mediaPlayer) return;

  QMediaPlayer::MediaStatus status = m_mediaPlayer->mediaStatus();

  switch (status) {
    case QMediaPlayer::EndOfMedia:
      // Playback finished naturally
      m_currentlyPlayingFile.clear();
      resetPlaybackUI();
      break;

    case QMediaPlayer::InvalidMedia:
      setPlaybackError(tr("Invalid or unsupported media format"));
      m_currentlyPlayingFile.clear();
      resetPlaybackUI();
      break;

    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadingMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::StalledMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
      // Normal states, no action needed
      break;
  }
}

void NMVoiceManagerPanel::onDurationChanged(qint64 duration) {
  m_currentDuration = duration;

  if (m_playbackProgress && duration > 0) {
    m_playbackProgress->setRange(0, static_cast<int>(duration));
  }

  // Update duration label
  if (m_durationLabel) {
    qint64 position = m_mediaPlayer ? m_mediaPlayer->position() : 0;
    m_durationLabel->setText(QString("%1 / %2")
                                 .arg(formatDuration(position))
                                 .arg(formatDuration(duration)));
  }
}

void NMVoiceManagerPanel::onPositionChanged(qint64 position) {
  if (m_playbackProgress && m_currentDuration > 0) {
    m_playbackProgress->setValue(static_cast<int>(position));
  }

  if (m_durationLabel) {
    m_durationLabel->setText(QString("%1 / %2")
                                 .arg(formatDuration(position))
                                 .arg(formatDuration(m_currentDuration)));
  }
}

void NMVoiceManagerPanel::onMediaErrorOccurred() {
  if (!m_mediaPlayer) return;

  QString errorMsg = m_mediaPlayer->errorString();
  if (errorMsg.isEmpty()) {
    errorMsg = tr("Unknown playback error");
  }

  setPlaybackError(errorMsg);
  m_currentlyPlayingFile.clear();
  resetPlaybackUI();
}

// Async duration probing
void NMVoiceManagerPanel::startDurationProbing() {
  // Clear the probe queue and add all matched voice files
  m_probeQueue.clear();

  for (const auto &entry : m_voiceLines) {
    if (entry.isMatched && !entry.voiceFilePath.isEmpty()) {
      // Check if we have a valid cached duration
      double cached = getCachedDuration(entry.voiceFilePath);
      if (cached <= 0) {
        m_probeQueue.enqueue(entry.voiceFilePath);
      }
    }
  }

  // Start processing
  if (!m_probeQueue.isEmpty() && !m_isProbing) {
    processNextDurationProbe();
  }
}

void NMVoiceManagerPanel::processNextDurationProbe() {
  if (m_probeQueue.isEmpty()) {
    m_isProbing = false;
    m_currentProbeFile.clear();
    // Update the list with newly probed durations
    updateDurationsInList();
    return;
  }

  m_isProbing = true;
  m_currentProbeFile = m_probeQueue.dequeue();

  if (!m_probePlayer) {
    m_isProbing = false;
    return;
  }

  m_probePlayer->setSource(QUrl::fromLocalFile(m_currentProbeFile));
}

void NMVoiceManagerPanel::onProbeDurationFinished() {
  if (!m_probePlayer || m_currentProbeFile.isEmpty()) {
    processNextDurationProbe();
    return;
  }

  qint64 durationMs = m_probePlayer->duration();
  if (durationMs > 0) {
    double durationSec = static_cast<double>(durationMs) / 1000.0;
    cacheDuration(m_currentProbeFile, durationSec);

    // Update the voice line entry with the duration
    for (auto it = m_voiceLines.begin(); it != m_voiceLines.end(); ++it) {
      if (it->voiceFilePath == m_currentProbeFile) {
        it->duration = durationSec;
        break;
      }
    }
  }

  // Continue with next file
  processNextDurationProbe();
}

double NMVoiceManagerPanel::getCachedDuration(const QString &filePath) const {
  auto it = m_durationCache.find(filePath.toStdString());
  if (it == m_durationCache.end()) {
    return 0.0;
  }

  // Check if file has been modified
  QFileInfo fileInfo(filePath);
  if (!fileInfo.exists()) {
    return 0.0;
  }

  qint64 currentMtime = fileInfo.lastModified().toMSecsSinceEpoch();
  if (currentMtime != it->second.mtime) {
    return 0.0;  // Cache invalidated
  }

  return it->second.duration;
}

void NMVoiceManagerPanel::cacheDuration(const QString &filePath, double duration) {
  QFileInfo fileInfo(filePath);
  DurationCacheEntry entry;
  entry.duration = duration;
  entry.mtime = fileInfo.lastModified().toMSecsSinceEpoch();

  m_durationCache[filePath.toStdString()] = entry;
}

void NMVoiceManagerPanel::updateDurationsInList() {
  if (!m_voiceTree) return;

  // Update durations in the tree widget items
  for (int i = 0; i < m_voiceTree->topLevelItemCount(); ++i) {
    auto *item = m_voiceTree->topLevelItem(i);
    if (!item) continue;

    QString dialogueId = item->data(0, Qt::UserRole).toString();
    if (m_voiceLines.contains(dialogueId)) {
      const auto &entry = m_voiceLines[dialogueId];
      if (entry.duration > 0) {
        qint64 durationMs = static_cast<qint64>(entry.duration * 1000);
        item->setText(5, formatDuration(durationMs));
      }
    }
  }
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

  // Stop current playback when selecting a new line
  if (m_isPlaying) {
    stopPlayback();
  }

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
  if (m_audioOutput) {
    m_audioOutput->setVolume(static_cast<float>(m_volume));
  }
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

    // Queue duration probe for this file
    if (!m_probeQueue.contains(filePath)) {
      m_probeQueue.enqueue(filePath);
      if (!m_isProbing) {
        processNextDurationProbe();
      }
    }

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
  m_voiceLines[dialogueId].duration = 0.0;
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
