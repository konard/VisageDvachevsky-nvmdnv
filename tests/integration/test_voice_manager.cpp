/**
 * @file test_voice_manager.cpp
 * @brief Integration tests for NMVoiceManagerPanel
 *
 * Tests the Qt Multimedia integration for voice file playback,
 * duration probing, and caching functionality.
 */

#include <catch2/catch_test_macros.hpp>

#include "NovelMind/editor/qt/panels/nm_voice_manager_panel.hpp"

#include <QApplication>
#include <QAudioOutput>
#include <QDir>
#include <QFile>
#include <QMediaPlayer>
#include <QTextStream>

using namespace NovelMind::editor::qt;

// Helper to ensure QApplication exists for Qt tests
class QtTestFixture {
public:
  QtTestFixture() {
    if (!QApplication::instance()) {
      static int argc = 1;
      static char *argv[] = {const_cast<char *>("test"), nullptr};
      m_app = std::make_unique<QApplication>(argc, argv);
    }
  }

private:
  std::unique_ptr<QApplication> m_app;
};

TEST_CASE("VoiceManagerPanel: Qt Multimedia component availability",
          "[integration][editor][voice]") {
  QtTestFixture fixture;

  SECTION("QMediaPlayer can be instantiated") {
    QMediaPlayer player;
    REQUIRE(player.playbackState() == QMediaPlayer::StoppedState);
  }

  SECTION("QAudioOutput can be instantiated") {
    QAudioOutput output;
    REQUIRE(output.volume() >= 0.0f);
    REQUIRE(output.volume() <= 1.0f);
  }

  SECTION("QMediaPlayer can connect to QAudioOutput") {
    QMediaPlayer player;
    QAudioOutput output;
    player.setAudioOutput(&output);
    REQUIRE(player.audioOutput() == &output);
  }
}

TEST_CASE("VoiceManagerPanel: Panel creation and initialization",
          "[integration][editor][voice]") {
  QtTestFixture fixture;

  SECTION("Panel can be constructed") {
    NMVoiceManagerPanel panel;
    REQUIRE(panel.panelId().isEmpty());  // Default empty ID
  }

  SECTION("Panel initializes without crash") {
    NMVoiceManagerPanel panel;
    panel.onInitialize();
    // Should complete without throwing
    SUCCEED();
  }

  SECTION("Panel shuts down cleanly") {
    NMVoiceManagerPanel panel;
    panel.onInitialize();
    panel.onShutdown();
    SUCCEED();
  }
}

TEST_CASE("VoiceManagerPanel: VoiceLineEntry structure",
          "[integration][editor][voice]") {
  SECTION("Default values are correct") {
    VoiceLineEntry entry;
    REQUIRE(entry.dialogueId.isEmpty());
    REQUIRE(entry.scriptPath.isEmpty());
    REQUIRE(entry.lineNumber == 0);
    REQUIRE(entry.speaker.isEmpty());
    REQUIRE(entry.dialogueText.isEmpty());
    REQUIRE(entry.voiceFilePath.isEmpty());
    REQUIRE(entry.actor.isEmpty());
    REQUIRE(entry.isMatched == false);
    REQUIRE(entry.isVerified == false);
    REQUIRE(entry.duration == 0.0);
  }
}

TEST_CASE("VoiceManagerPanel: DurationCacheEntry structure",
          "[integration][editor][voice]") {
  SECTION("Default values are correct") {
    DurationCacheEntry entry;
    REQUIRE(entry.duration == 0.0);
    REQUIRE(entry.mtime == 0);
  }
}

TEST_CASE("VoiceManagerPanel: CSV export format",
          "[integration][editor][voice]") {
  QtTestFixture fixture;

  SECTION("Empty panel exports empty CSV") {
    NMVoiceManagerPanel panel;
    panel.onInitialize();

    QString tempPath = QDir::tempPath() + "/test_voice_export.csv";
    bool result = panel.exportToCsv(tempPath);
    REQUIRE(result == true);

    QFile file(tempPath);
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    QString header = in.readLine();
    REQUIRE(header.contains("DialogueID"));
    REQUIRE(header.contains("VoiceFile"));
    file.close();

    QFile::remove(tempPath);
  }
}

TEST_CASE("VoiceManagerPanel: Unmatched lines retrieval",
          "[integration][editor][voice]") {
  QtTestFixture fixture;

  SECTION("Empty panel returns empty list") {
    NMVoiceManagerPanel panel;
    panel.onInitialize();
    auto unmatched = panel.getUnmatchedLines();
    REQUIRE(unmatched.isEmpty());
  }
}
