#pragma once

/**
 * @file nm_diagnostics_panel.hpp
 * @brief Diagnostics panel for errors, warnings, and issues
 */

#include "NovelMind/editor/qt/nm_dock_panel.hpp"

#include <QToolBar>
#include <QWidget>

class QTreeWidget;
class QToolBar;
class QPushButton;

namespace NovelMind::editor::qt {

class NMDiagnosticsPanel : public NMDockPanel {
  Q_OBJECT

public:
  explicit NMDiagnosticsPanel(QWidget *parent = nullptr);
  ~NMDiagnosticsPanel() override;

  void onInitialize() override;
  void onShutdown() override;
  void onUpdate(double deltaTime) override;

  void addDiagnostic(const QString &type, const QString &message,
                     const QString &file = QString(), int line = -1);
  void clearDiagnostics();

private:
  void setupUI();
  void applyTypeFilter();

  QTreeWidget *m_diagnosticsTree = nullptr;
  QToolBar *m_toolbar = nullptr;
  QString m_activeFilter;
};

} // namespace NovelMind::editor::qt
