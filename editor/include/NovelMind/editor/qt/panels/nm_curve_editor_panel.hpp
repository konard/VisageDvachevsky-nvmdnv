#pragma once

/**
 * @file nm_curve_editor_panel.hpp
 * @brief Curve editor for animation curves and interpolation
 */

#include "NovelMind/editor/qt/nm_dock_panel.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QToolBar>
#include <QWidget>

class QToolBar;
class QPushButton;
class QComboBox;

namespace NovelMind::editor::qt {

/**
 * @brief Curve editor panel for editing animation curves
 */
class NMCurveEditorPanel : public NMDockPanel {
  Q_OBJECT

public:
  explicit NMCurveEditorPanel(QWidget *parent = nullptr);
  ~NMCurveEditorPanel() override;

  void onInitialize() override;
  void onShutdown() override;
  void onUpdate(double deltaTime) override;

private:
  void setupUI();

  QGraphicsView *m_curveView = nullptr;
  QGraphicsScene *m_curveScene = nullptr;
  QToolBar *m_toolbar = nullptr;
};

} // namespace NovelMind::editor::qt
