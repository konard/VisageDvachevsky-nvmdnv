#include "NovelMind/editor/qt/panels/nm_build_settings_panel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMBuildSettingsPanel::NMBuildSettingsPanel(QWidget *parent)
    : NMDockPanel("Build Settings", parent) {}

NMBuildSettingsPanel::~NMBuildSettingsPanel() = default;

void NMBuildSettingsPanel::onInitialize() { setupUI(); }

void NMBuildSettingsPanel::onShutdown() {}

void NMBuildSettingsPanel::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(contentWidget());
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(12);

  // Platform settings
  QGroupBox *platformGroup = new QGroupBox("Platform", contentWidget());
  QFormLayout *platformLayout = new QFormLayout(platformGroup);
  m_platformSelector = new QComboBox(platformGroup);
  m_platformSelector->addItems(
      {"Windows", "Linux", "macOS", "Web (WASM)", "Android", "iOS"});
  platformLayout->addRow("Target Platform:", m_platformSelector);
  layout->addWidget(platformGroup);

  // Output settings
  QGroupBox *outputGroup = new QGroupBox("Output", contentWidget());
  QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);
  QFormLayout *outputFormLayout = new QFormLayout();
  m_outputPathEdit = new QLineEdit("./build/", outputGroup);
  outputFormLayout->addRow("Output Directory:", m_outputPathEdit);
  outputFormLayout->addRow("Build Name:",
                           new QLineEdit("MyVisualNovel", outputGroup));
  outputLayout->addLayout(outputFormLayout);
  layout->addWidget(outputGroup);

  // Build options
  QGroupBox *optionsGroup = new QGroupBox("Options", contentWidget());
  QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
  optionsLayout->addWidget(new QCheckBox("Compress Assets", optionsGroup));
  optionsLayout->addWidget(new QCheckBox("Include Debug Info", optionsGroup));
  optionsLayout->addWidget(new QCheckBox("Strip Unused Assets", optionsGroup));
  optionsLayout->addWidget(new QCheckBox("Encrypt Scripts", optionsGroup));
  layout->addWidget(optionsGroup);

  layout->addStretch();

  // Build button
  m_buildButton = new QPushButton("Build Project", contentWidget());
  m_buildButton->setMinimumHeight(40);
  m_buildButton->setStyleSheet("background-color: #0078d4; font-weight: bold;");
  layout->addWidget(m_buildButton);
}

void NMBuildSettingsPanel::onUpdate([[maybe_unused]] double deltaTime) {}

} // namespace NovelMind::editor::qt
