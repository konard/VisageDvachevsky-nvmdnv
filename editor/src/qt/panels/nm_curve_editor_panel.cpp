#include "NovelMind/editor/qt/panels/nm_curve_editor_panel.hpp"

#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainterPath>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMCurveEditorPanel::NMCurveEditorPanel(QWidget *parent)
    : NMDockPanel("Curve Editor", parent) {}

NMCurveEditorPanel::~NMCurveEditorPanel() = default;

void NMCurveEditorPanel::onInitialize() { setupUI(); }

void NMCurveEditorPanel::onShutdown() {}

void NMCurveEditorPanel::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget());
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_toolbar = new QToolBar(contentWidget());
  m_toolbar->addWidget(new QPushButton("Add Point", m_toolbar));
  m_toolbar->addWidget(new QPushButton("Delete Point", m_toolbar));

  QComboBox *interpCombo = new QComboBox(m_toolbar);
  interpCombo->addItems(
      {"Linear", "Ease In", "Ease Out", "Ease In/Out", "Bezier"});
  m_toolbar->addWidget(interpCombo);

  mainLayout->addWidget(m_toolbar);

  m_curveScene = new QGraphicsScene(this);
  m_curveView = new QGraphicsView(m_curveScene, contentWidget());
  m_curveView->setObjectName("CurveView");
  mainLayout->addWidget(m_curveView, 1);

  // Draw grid and example curve
  for (int x = 0; x <= 400; x += 40) {
    m_curveScene->addLine(x, 0, x, 300, QPen(QColor("#404040")));
  }
  for (int y = 0; y <= 300; y += 30) {
    m_curveScene->addLine(0, y, 400, y, QPen(QColor("#404040")));
  }

  // Example curve path
  QPainterPath curvePath;
  curvePath.moveTo(0, 300);
  curvePath.cubicTo(100, 300, 100, 0, 200, 0);
  curvePath.cubicTo(300, 0, 300, 300, 400, 300);
  m_curveScene->addPath(curvePath, QPen(QColor("#0078d4"), 2));
}

void NMCurveEditorPanel::onUpdate([[maybe_unused]] double deltaTime) {
  // Update curve visualization if needed
}

} // namespace NovelMind::editor::qt
