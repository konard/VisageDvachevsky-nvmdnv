#include "NovelMind/editor/qt/panels/nm_localization_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/qt/nm_dialogs.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMLocalizationPanel::NMLocalizationPanel(QWidget *parent)
    : NMDockPanel("Localization Manager", parent) {}

NMLocalizationPanel::~NMLocalizationPanel() = default;

void NMLocalizationPanel::onInitialize() { setupUI(); }

void NMLocalizationPanel::onShutdown() {}

void NMLocalizationPanel::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(contentWidget());
  layout->setContentsMargins(4, 4, 4, 4);

  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->addWidget(new QLabel("Language:", contentWidget()));
  m_languageSelector = new QComboBox(contentWidget());
  connect(m_languageSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int) {
            if (!m_languageSelector) {
              return;
            }
            const QString localeCode =
                m_languageSelector->currentData().toString();
            if (!localeCode.isEmpty()) {
              loadLocale(localeCode);
            }
          });
  topLayout->addWidget(m_languageSelector);
  topLayout->addStretch();
  m_importButton = new QPushButton("Import", contentWidget());
  m_exportButton = new QPushButton("Export", contentWidget());
  connect(m_importButton, &QPushButton::clicked, this,
          &NMLocalizationPanel::importLocale);
  connect(m_exportButton, &QPushButton::clicked, this,
          &NMLocalizationPanel::exportLocale);
  topLayout->addWidget(m_importButton);
  topLayout->addWidget(m_exportButton);
  layout->addLayout(topLayout);

  m_stringsTable = new QTableWidget(contentWidget());
  m_stringsTable->setColumnCount(3);
  m_stringsTable->setHorizontalHeaderLabels({"ID", "Source", "Translation"});
  m_stringsTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                  QAbstractItemView::EditKeyPressed);
  m_stringsTable->horizontalHeader()->setStretchLastSection(true);
  m_stringsTable->verticalHeader()->setVisible(false);
  m_stringsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(m_stringsTable, &QTableWidget::cellChanged, this,
          [this](int row, int column) {
            if (!m_stringsTable || column != 2) {
              return;
            }
            auto *idItem = m_stringsTable->item(row, 0);
            auto *valueItem = m_stringsTable->item(row, 2);
            if (!idItem || !valueItem || m_currentLocale.isEmpty()) {
              return;
            }
            const std::string id = idItem->text().toStdString();
            const std::string value = valueItem->text().toStdString();
            NovelMind::localization::LocaleId locale;
            locale.language = m_currentLocale.toStdString();
            m_localization.setString(locale, id, value);
          });
  layout->addWidget(m_stringsTable, 1);

  refreshLocales();
}

void NMLocalizationPanel::refreshLocales() {
  if (!m_languageSelector) {
    return;
  }

  m_languageSelector->blockSignals(true);
  m_languageSelector->clear();

  auto &pm = ProjectManager::instance();
  const QString localizationRoot =
      QString::fromStdString(pm.getFolderPath(ProjectFolder::Localization));

  QStringList localeCodes;
  QDir dir(localizationRoot);
  if (dir.exists()) {
    const QStringList filters = {"*.csv", "*.json", "*.po", "*.xliff",
                                 "*.xlf"};
    const QFileInfoList files =
        dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &info : files) {
      const QString locale = info.baseName();
      if (!locale.isEmpty()) {
        localeCodes.append(locale);
      }
    }
  }

  if (!localeCodes.contains(m_defaultLocale)) {
    localeCodes.prepend(m_defaultLocale);
  }

  localeCodes.removeDuplicates();
  localeCodes.sort();

  for (const auto &code : localeCodes) {
    m_languageSelector->addItem(code.toUpper(), code);
  }

  m_languageSelector->blockSignals(false);

  if (!localeCodes.isEmpty()) {
    loadLocale(localeCodes.first());
  }
}

static NovelMind::localization::LocalizationFormat formatForExtension(
    const QString &ext) {
  const QString lower = ext.toLower();
  if (lower == "csv")
    return NovelMind::localization::LocalizationFormat::CSV;
  if (lower == "json")
    return NovelMind::localization::LocalizationFormat::JSON;
  if (lower == "po")
    return NovelMind::localization::LocalizationFormat::PO;
  if (lower == "xliff" || lower == "xlf")
    return NovelMind::localization::LocalizationFormat::XLIFF;
  return NovelMind::localization::LocalizationFormat::CSV;
}

void NMLocalizationPanel::loadLocale(const QString &localeCode) {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    return;
  }

  m_currentLocale = localeCode;

  NovelMind::localization::LocaleId locale;
  locale.language = localeCode.toStdString();

  const QString localizationRoot =
      QString::fromStdString(pm.getFolderPath(ProjectFolder::Localization));
  QDir dir(localizationRoot);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QStringList candidates = {dir.filePath(localeCode + ".csv"),
                            dir.filePath(localeCode + ".json"),
                            dir.filePath(localeCode + ".po"),
                            dir.filePath(localeCode + ".xliff"),
                            dir.filePath(localeCode + ".xlf")};
  for (const auto &path : candidates) {
    QFileInfo info(path);
    if (!info.exists()) {
      continue;
    }
    const auto format = formatForExtension(info.suffix());
    m_localization.loadStrings(locale, info.absoluteFilePath().toStdString(),
                               format);
    break;
  }

  rebuildTable();
}

void NMLocalizationPanel::rebuildTable() {
  if (!m_stringsTable) {
    return;
  }

  m_stringsTable->blockSignals(true);
  m_stringsTable->setRowCount(0);

  NovelMind::localization::LocaleId defaultLocale;
  defaultLocale.language = m_defaultLocale.toStdString();
  NovelMind::localization::LocaleId currentLocale;
  currentLocale.language = m_currentLocale.toStdString();

  const auto *defaultTable =
      m_localization.getStringTable(defaultLocale);
  const auto *currentTable =
      m_localization.getStringTable(currentLocale);

  std::vector<std::string> ids;
  if (defaultTable) {
    ids = defaultTable->getStringIds();
  } else if (currentTable) {
    ids = currentTable->getStringIds();
  }

  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  int row = 0;
  for (const auto &id : ids) {
    m_stringsTable->insertRow(row);
    auto *idItem = new QTableWidgetItem(QString::fromStdString(id));
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    m_stringsTable->setItem(row, 0, idItem);

    QString sourceText;
    if (defaultTable) {
      auto val = defaultTable->getString(id);
      if (val) {
        sourceText = QString::fromStdString(*val);
      }
    }
    auto *sourceItem = new QTableWidgetItem(sourceText);
    sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable);
    m_stringsTable->setItem(row, 1, sourceItem);

    QString translationText;
    if (currentTable) {
      auto val = currentTable->getString(id);
      if (val) {
        translationText = QString::fromStdString(*val);
      }
    }
    auto *translationItem = new QTableWidgetItem(translationText);
    m_stringsTable->setItem(row, 2, translationItem);
    row++;
  }

  m_stringsTable->blockSignals(false);
}

void NMLocalizationPanel::exportLocale() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject() || m_currentLocale.isEmpty()) {
    return;
  }

  const QString localizationRoot =
      QString::fromStdString(pm.getFolderPath(ProjectFolder::Localization));
  const QString filter = tr("Localization (*.csv *.json *.po *.xliff *.xlf)");
  const QString defaultName =
      QDir(localizationRoot).filePath(m_currentLocale + ".csv");
  const QString path = QFileDialog::getSaveFileName(
      this, tr("Export Localization"), defaultName, filter);
  if (path.isEmpty()) {
    return;
  }

  const QFileInfo info(path);
  const auto format = formatForExtension(info.suffix());

  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  auto result =
      m_localization.exportStrings(locale, path.toStdString(), format);
  if (result.isError()) {
    NMMessageDialog::showError(this, tr("Export Failed"),
                               QString::fromStdString(result.error()));
  }
}

void NMLocalizationPanel::importLocale() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    NMMessageDialog::showInfo(this, tr("Import Localization"),
                              tr("Open a project before importing strings."));
    return;
  }

  const QString filter = tr("Localization (*.csv *.json *.po *.xliff *.xlf)");
  const QString path =
      NMFileDialog::getOpenFileName(this, tr("Import Localization"),
                                    QDir::homePath(), filter);
  if (path.isEmpty()) {
    return;
  }

  QFileInfo info(path);
  const QString localeCode = info.baseName();
  const auto format = formatForExtension(info.suffix());

  NovelMind::localization::LocaleId locale;
  locale.language = localeCode.toStdString();
  auto result =
      m_localization.loadStrings(locale, path.toStdString(), format);
  if (result.isError()) {
    NMMessageDialog::showError(this, tr("Import Failed"),
                               QString::fromStdString(result.error()));
    return;
  }

  m_currentLocale = localeCode;
  refreshLocales();
}

void NMLocalizationPanel::onUpdate([[maybe_unused]] double deltaTime) {}

} // namespace NovelMind::editor::qt
