#include "NovelMind/editor/qt/nm_main_window.hpp"
#include "NovelMind/editor/qt/nm_play_mode_controller.hpp"
#include "NovelMind/editor/qt/nm_undo_manager.hpp"

#include <QSettings>
#include <QTimer>

namespace NovelMind::editor::qt {

NMMainWindow::NMMainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("NovelMind Editor");
  setMinimumSize(1280, 720);
  resize(1920, 1080);

  // Enable docking with nesting and grouping
  setDockNestingEnabled(true);
  setDockOptions(AnimatedDocks | AllowNestedDocks | AllowTabbedDocks |
                 GroupedDragging);
}

NMMainWindow::~NMMainWindow() { shutdown(); }

bool NMMainWindow::initialize() {
  if (m_initialized)
    return true;

  // Initialize undo/redo system
  NMUndoManager::instance().initialize();

  setupMenuBar();
  setupToolBar();
  setupStatusBar();
  setupPanels();
  configureDocking();
  setupConnections();
  setupShortcuts();

  // Restore layout or use default
  QSettings settings("NovelMind", "Editor");
  if (settings.contains("mainwindow/geometry")) {
    restoreLayout();
  } else {
    createDefaultLayout();
  }

  // Start update timer
  m_updateTimer = new QTimer(this);
  connect(m_updateTimer, &QTimer::timeout, this, &NMMainWindow::onUpdateTick);
  m_updateTimer->start(UPDATE_INTERVAL_MS);

  updateStatusBarContext();

  m_initialized = true;
  return true;
}

void NMMainWindow::shutdown() {
  if (!m_initialized)
    return;

  if (m_updateTimer) {
    m_updateTimer->stop();
  }

  NMPlayModeController::instance().shutdown();

  saveLayout();

  m_initialized = false;
}

} // namespace NovelMind::editor::qt
