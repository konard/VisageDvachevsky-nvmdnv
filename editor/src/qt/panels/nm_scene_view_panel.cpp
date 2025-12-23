#include "NovelMind/editor/qt/panels/nm_scene_view_panel.hpp"
#include "NovelMind/editor/qt/nm_play_mode_controller.hpp"
#include "nm_scene_view_overlays.hpp"

#include <QDebug>

namespace NovelMind::editor::qt {

NMSceneViewPanel::NMSceneViewPanel(QWidget *parent)
    : NMDockPanel(tr("Scene View"), parent) {
  setPanelId("SceneView");
  setupContent();
  setupToolBar();
}

NMSceneViewPanel::~NMSceneViewPanel() = default;

void NMSceneViewPanel::onInitialize() {
  // Center the view on origin
  if (m_view) {
    m_view->centerOnScene();
  }

  // Connect to Play Mode Controller for runtime preview
  auto &playController = NMPlayModeController::instance();
  connect(&playController, &NMPlayModeController::currentNodeChanged, this,
          &NMSceneViewPanel::onPlayModeCurrentNodeChanged);
  connect(&playController, &NMPlayModeController::dialogueLineChanged, this,
          &NMSceneViewPanel::onPlayModeDialogueChanged);
  connect(&playController, &NMPlayModeController::choicesChanged, this,
          &NMSceneViewPanel::onPlayModeChoicesChanged);
  connect(&playController, &NMPlayModeController::playModeChanged, this,
          &NMSceneViewPanel::onPlayModeChanged);
  connect(&playController, &NMPlayModeController::sceneSnapshotUpdated, this,
          [this, &playController]() {
            applyRuntimeSnapshot(playController.lastSnapshot());
          });

  qDebug() << "[SceneView] Connected to PlayModeController for runtime preview";

  connect(this, &NMSceneViewPanel::sceneObjectsChanged, this, [this]() {
    if (!m_isLoadingScene && !m_runtimePreviewActive && !m_playModeActive &&
        !m_suppressSceneSave) {
      saveSceneDocument();
    }
  });
  connect(this, &NMSceneViewPanel::objectTransformFinished, this,
          [this](const QString &, const QPointF &, const QPointF &, qreal,
                 qreal, qreal, qreal, qreal, qreal) {
            if (!m_isLoadingScene && !m_runtimePreviewActive &&
                !m_playModeActive && !m_suppressSceneSave) {
              saveSceneDocument();
            }
          });
}

void NMSceneViewPanel::onUpdate(double /*deltaTime*/) {
  syncCameraToPreview();
  if (m_glViewport) {
    m_glViewport->update();
  }
}

void NMSceneViewPanel::onResize(int /*width*/, int /*height*/) {
  updateInfoOverlay();
  syncCameraToPreview();
  if (m_glViewport) {
    m_glViewport->update();
  }
}

void NMSceneViewPanel::setGridVisible(bool visible) {
  if (m_scene) {
    m_scene->setGridVisible(visible);
  }
}

void NMSceneViewPanel::setZoomLevel(qreal zoom) {
  if (m_view) {
    m_view->setZoomLevel(zoom);
  }
}

void NMSceneViewPanel::setGizmoMode(NMTransformGizmo::GizmoMode mode) {
  if (m_scene) {
    m_scene->setGizmoMode(mode);
  }
}


} // namespace NovelMind::editor::qt
