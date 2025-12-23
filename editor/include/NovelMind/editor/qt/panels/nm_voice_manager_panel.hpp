#pragma once

/**
 * @file nm_voice_manager_panel.hpp
 * @brief Voice file management panel
 *
 * Provides comprehensive voice-over file management:
 * - Auto-detection and matching of voice files to dialogue lines
 * - Voice file preview/playback
 * - Import/export of voice mapping tables
 * - Actor assignment and metadata management
 * - Missing voice detection
 */

#include "NovelMind/editor/qt/nm_dock_panel.hpp"

#include <QHash>
#include <QToolBar>
#include <QWidget>

class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QToolBar;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSlider;
class QSplitter;
class QProgressBar;

namespace NovelMind::editor::qt {

/**
 * @brief Voice line entry representing a dialogue line and its voice file
 */
struct VoiceLineEntry {
  QString dialogueId;       // Unique ID of the dialogue line
  QString scriptPath;       // Script file containing the line
  int lineNumber = 0;       // Line number in script
  QString speaker;          // Character speaking
  QString dialogueText;     // The dialogue text
  QString voiceFilePath;    // Path to voice file (if assigned)
  QString actor;            // Voice actor name
  bool isMatched = false;   // Whether a voice file is assigned
  bool isVerified = false;  // Whether the match has been verified
  double duration = 0.0;    // Voice file duration in seconds
};

class NMVoiceManagerPanel : public NMDockPanel {
  Q_OBJECT

public:
  explicit NMVoiceManagerPanel(QWidget *parent = nullptr);
  ~NMVoiceManagerPanel() override;

  void onInitialize() override;
  void onShutdown() override;
  void onUpdate(double deltaTime) override;

  /**
   * @brief Scan project for dialogue lines and voice files
   */
  void scanProject();

  /**
   * @brief Auto-match voice files to dialogue lines
   */
  void autoMatchVoiceFiles();

  /**
   * @brief Get list of unmatched dialogue lines
   */
  [[nodiscard]] QList<VoiceLineEntry> getUnmatchedLines() const;

  /**
   * @brief Export voice mapping to CSV
   */
  bool exportToCsv(const QString &filePath);

  /**
   * @brief Import voice mapping from CSV
   */
  bool importFromCsv(const QString &filePath);

signals:
  void voiceLineSelected(const QString &dialogueId);
  void voiceFileChanged(const QString &dialogueId, const QString &voiceFilePath);

private slots:
  void onScanClicked();
  void onAutoMatchClicked();
  void onImportClicked();
  void onExportClicked();
  void onPlayClicked();
  void onStopClicked();
  void onLineSelected(QTreeWidgetItem *item, int column);
  void onFilterChanged(const QString &text);
  void onCharacterFilterChanged(int index);
  void onShowOnlyUnmatched(bool checked);
  void onVolumeChanged(int value);
  void onAssignVoiceFile();
  void onClearVoiceFile();
  void onOpenVoiceFolder();

private:
  void setupUI();
  void setupToolBar();
  void setupFilterBar();
  void setupVoiceList();
  void setupPreviewBar();
  void updateVoiceList();
  void updateStatistics();
  void scanScriptsForDialogue();
  void scanVoiceFolder();
  void matchVoiceToDialogue(const QString &voiceFile);
  QString generateDialogueId(const QString &scriptPath, int lineNumber) const;
  bool playVoiceFile(const QString &filePath);
  void stopPlayback();

  // UI Elements
  QSplitter *m_splitter = nullptr;
  QTreeWidget *m_voiceTree = nullptr;
  QToolBar *m_toolbar = nullptr;
  QLineEdit *m_filterEdit = nullptr;
  QComboBox *m_characterFilter = nullptr;
  QPushButton *m_showUnmatchedBtn = nullptr;
  QPushButton *m_playBtn = nullptr;
  QPushButton *m_stopBtn = nullptr;
  QSlider *m_volumeSlider = nullptr;
  QLabel *m_durationLabel = nullptr;
  QProgressBar *m_playbackProgress = nullptr;
  QLabel *m_statsLabel = nullptr;

  // Data
  QHash<QString, VoiceLineEntry> m_voiceLines;
  QStringList m_voiceFiles;
  QStringList m_characters;
  QString m_currentlyPlayingFile;
  bool m_isPlaying = false;
  double m_playbackPosition = 0.0;
  double m_volume = 1.0;
};

} // namespace NovelMind::editor::qt
