#include "NovelMind/editor/qt/panels/nm_diagnostics_panel.hpp"
#include "NovelMind/editor/qt/qt_event_bus.hpp"

#include <QPushButton>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMDiagnosticsPanel::NMDiagnosticsPanel(QWidget *parent)
    : NMDockPanel("Diagnostics", parent) {}

NMDiagnosticsPanel::~NMDiagnosticsPanel() = default;

void NMDiagnosticsPanel::onInitialize() { setupUI(); }

void NMDiagnosticsPanel::onShutdown() {}

void NMDiagnosticsPanel::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(contentWidget());
  layout->setContentsMargins(0, 0, 0, 0);

  m_toolbar = new QToolBar(contentWidget());
  auto *clearButton = new QPushButton("Clear All", m_toolbar);
  m_toolbar->addWidget(clearButton);
  m_toolbar->addSeparator();
  auto *allButton = new QPushButton("All", m_toolbar);
  auto *errorButton = new QPushButton("Errors", m_toolbar);
  auto *warningButton = new QPushButton("Warnings", m_toolbar);
  auto *infoButton = new QPushButton("Info", m_toolbar);
  m_toolbar->addWidget(allButton);
  m_toolbar->addWidget(errorButton);
  m_toolbar->addWidget(warningButton);
  m_toolbar->addWidget(infoButton);
  layout->addWidget(m_toolbar);

  m_diagnosticsTree = new QTreeWidget(contentWidget());
  m_diagnosticsTree->setHeaderLabels({"Type", "Message", "File", "Line"});
  layout->addWidget(m_diagnosticsTree, 1);

  // Enable double-click to navigate
  connect(m_diagnosticsTree, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem *item, int column) {
            Q_UNUSED(column);
            if (!item) {
              return;
            }
            // Get location from item data
            QString location = item->data(0, Qt::UserRole).toString();
            if (!location.isEmpty()) {
              emit diagnosticActivated(location);
            }
          });

  connect(clearButton, &QPushButton::clicked, this,
          &NMDiagnosticsPanel::clearDiagnostics);
  connect(allButton, &QPushButton::clicked, this, [this]() {
    m_activeFilter.clear();
    applyTypeFilter();
  });
  connect(errorButton, &QPushButton::clicked, this, [this]() {
    m_activeFilter = "Error";
    applyTypeFilter();
  });
  connect(warningButton, &QPushButton::clicked, this, [this]() {
    m_activeFilter = "Warning";
    applyTypeFilter();
  });
  connect(infoButton, &QPushButton::clicked, this, [this]() {
    m_activeFilter = "Info";
    applyTypeFilter();
  });

  auto &bus = QtEventBus::instance();
  connect(&bus, &QtEventBus::errorOccurred, this,
          [this](const QString &message, const QString &details) {
            QString msg = message;
            if (!details.isEmpty()) {
              msg = message + " (" + details + ")";
            }
            addDiagnostic("Error", msg);
          });
  connect(&bus, &QtEventBus::logMessage, this,
          [this](const QString &message, const QString &source, int level) {
            Q_UNUSED(level);
            addDiagnostic("Info",
                          source.isEmpty() ? message
                                           : source + ": " + message);
          });
}

void NMDiagnosticsPanel::onUpdate([[maybe_unused]] double deltaTime) {}

void NMDiagnosticsPanel::addDiagnostic(const QString &type,
                                       const QString &message,
                                       const QString &file, int line) {
  if (!m_diagnosticsTree) {
    return;
  }
  auto *item = new QTreeWidgetItem(m_diagnosticsTree);
  item->setText(0, type);
  item->setText(1, message);
  item->setText(2, file);
  item->setText(3, line >= 0 ? QString::number(line) : QString("-"));

  // Store location string for navigation
  if (!file.isEmpty()) {
    QString location;
    if (line > 0) {
      location = QString("Script:%1:%2").arg(file).arg(line);
    } else {
      location = QString("Script:%1").arg(file);
    }
    item->setData(0, Qt::UserRole, location);
  }

  QColor color;
  if (type.compare("Error", Qt::CaseInsensitive) == 0) {
    color = QColor("#f44336");
  } else if (type.compare("Warning", Qt::CaseInsensitive) == 0) {
    color = QColor("#ff9800");
  } else {
    color = QColor("#64b5f6");
  }
  item->setForeground(0, QBrush(color));
  applyTypeFilter();
}

void NMDiagnosticsPanel::addDiagnosticWithLocation(const QString &type,
                                                   const QString &message,
                                                   const QString &location) {
  if (!m_diagnosticsTree) {
    return;
  }
  auto *item = new QTreeWidgetItem(m_diagnosticsTree);
  item->setText(0, type);
  item->setText(1, message);

  // Parse location to populate file/line columns if it's a Script location
  if (location.startsWith("Script:", Qt::CaseInsensitive)) {
    QStringList parts = location.split(':');
    if (parts.size() >= 2) {
      item->setText(2, parts[1]);
    }
    if (parts.size() >= 3) {
      item->setText(3, parts[2]);
    }
  } else if (location.startsWith("StoryGraph:", Qt::CaseInsensitive)) {
    QStringList parts = location.split(':');
    if (parts.size() >= 2) {
      item->setText(2, QString("Node: %1").arg(parts[1]));
    }
    item->setText(3, QString("-"));
  }

  // Store the full location string
  item->setData(0, Qt::UserRole, location);

  QColor color;
  if (type.compare("Error", Qt::CaseInsensitive) == 0) {
    color = QColor("#f44336");
  } else if (type.compare("Warning", Qt::CaseInsensitive) == 0) {
    color = QColor("#ff9800");
  } else {
    color = QColor("#64b5f6");
  }
  item->setForeground(0, QBrush(color));
  applyTypeFilter();
}

void NMDiagnosticsPanel::clearDiagnostics() {
  if (m_diagnosticsTree) {
    m_diagnosticsTree->clear();
  }
}

void NMDiagnosticsPanel::applyTypeFilter() {
  if (!m_diagnosticsTree) {
    return;
  }
  for (int i = 0; i < m_diagnosticsTree->topLevelItemCount(); ++i) {
    auto *item = m_diagnosticsTree->topLevelItem(i);
    const bool visible = m_activeFilter.isEmpty() ||
                         item->text(0).compare(m_activeFilter,
                                               Qt::CaseInsensitive) == 0;
    item->setHidden(!visible);
  }
}

} // namespace NovelMind::editor::qt
