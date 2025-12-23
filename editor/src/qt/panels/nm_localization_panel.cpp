#include "NovelMind/editor/qt/panels/nm_localization_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/qt/nm_dialogs.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

// Key validation: allows alphanumeric, underscore, dot, dash
const QRegularExpression NMLocalizationPanel::s_keyValidationRegex(
    QStringLiteral("^[A-Za-z0-9_.-]+$"));

NMLocalizationPanel::NMLocalizationPanel(QWidget *parent)
    : NMDockPanel("Localization Manager", parent) {}

NMLocalizationPanel::~NMLocalizationPanel() = default;

void NMLocalizationPanel::onInitialize() { setupUI(); }

void NMLocalizationPanel::onShutdown() {}

void NMLocalizationPanel::setupUI() {
  QVBoxLayout *layout = new QVBoxLayout(contentWidget());
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  // Top bar: Language selector, buttons
  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->addWidget(new QLabel(tr("Language:"), contentWidget()));
  m_languageSelector = new QComboBox(contentWidget());
  connect(m_languageSelector,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &NMLocalizationPanel::onLocaleChanged);
  topLayout->addWidget(m_languageSelector);
  topLayout->addStretch();

  m_saveBtn = new QPushButton(tr("Save"), contentWidget());
  m_saveBtn->setEnabled(false);
  connect(m_saveBtn, &QPushButton::clicked, this,
          &NMLocalizationPanel::onSaveClicked);
  topLayout->addWidget(m_saveBtn);

  m_importButton = new QPushButton(tr("Import"), contentWidget());
  m_exportButton = new QPushButton(tr("Export"), contentWidget());
  connect(m_importButton, &QPushButton::clicked, this,
          &NMLocalizationPanel::importLocale);
  connect(m_exportButton, &QPushButton::clicked, this,
          &NMLocalizationPanel::exportLocale);
  topLayout->addWidget(m_importButton);
  topLayout->addWidget(m_exportButton);
  layout->addLayout(topLayout);

  // Filter bar: Search, Show Missing Only, Add/Delete buttons
  QHBoxLayout *filterLayout = new QHBoxLayout();

  m_searchEdit = new QLineEdit(contentWidget());
  m_searchEdit->setPlaceholderText(tr("Search keys, source, or translations..."));
  m_searchEdit->setClearButtonEnabled(true);
  connect(m_searchEdit, &QLineEdit::textChanged, this,
          &NMLocalizationPanel::onSearchTextChanged);
  filterLayout->addWidget(m_searchEdit, 1);

  m_showMissingOnly = new QCheckBox(tr("Show Missing Only"), contentWidget());
  connect(m_showMissingOnly, &QCheckBox::toggled, this,
          &NMLocalizationPanel::onShowOnlyMissingToggled);
  filterLayout->addWidget(m_showMissingOnly);

  m_addKeyBtn = new QPushButton(tr("Add Key"), contentWidget());
  connect(m_addKeyBtn, &QPushButton::clicked, this,
          &NMLocalizationPanel::onAddKeyClicked);
  filterLayout->addWidget(m_addKeyBtn);

  m_deleteKeyBtn = new QPushButton(tr("Delete Key"), contentWidget());
  connect(m_deleteKeyBtn, &QPushButton::clicked, this,
          &NMLocalizationPanel::onDeleteKeyClicked);
  filterLayout->addWidget(m_deleteKeyBtn);

  layout->addLayout(filterLayout);

  // Table
  m_stringsTable = new QTableWidget(contentWidget());
  m_stringsTable->setColumnCount(3);
  m_stringsTable->setHorizontalHeaderLabels(
      {tr("ID"), tr("Source"), tr("Translation")});
  m_stringsTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                  QAbstractItemView::EditKeyPressed);
  m_stringsTable->horizontalHeader()->setStretchLastSection(true);
  m_stringsTable->verticalHeader()->setVisible(false);
  m_stringsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_stringsTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_stringsTable, &QTableWidget::cellChanged, this,
          &NMLocalizationPanel::onCellChanged);
  connect(m_stringsTable, &QTableWidget::customContextMenuRequested, this,
          &NMLocalizationPanel::onContextMenu);
  layout->addWidget(m_stringsTable, 1);

  // Status bar
  m_statusLabel = new QLabel(contentWidget());
  layout->addWidget(m_statusLabel);

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
    const QStringList filters = {"*.csv", "*.json", "*.po", "*.xliff", "*.xlf"};
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
  m_availableLocales = localeCodes;

  for (const auto &code : localeCodes) {
    m_languageSelector->addItem(code.toUpper(), code);
  }

  m_languageSelector->blockSignals(false);

  if (!localeCodes.isEmpty()) {
    loadLocale(localeCodes.first());
  }
}

static NovelMind::localization::LocalizationFormat
formatForExtension(const QString &ext) {
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

  syncEntriesFromManager();
  rebuildTable();
  highlightMissingTranslations();
  updateStatusBar();
}

void NMLocalizationPanel::syncEntriesFromManager() {
  m_entries.clear();
  m_deletedKeys.clear();

  NovelMind::localization::LocaleId defaultLocale;
  defaultLocale.language = m_defaultLocale.toStdString();
  NovelMind::localization::LocaleId currentLocale;
  currentLocale.language = m_currentLocale.toStdString();

  const auto *defaultTable = m_localization.getStringTable(defaultLocale);
  const auto *currentTable = m_localization.getStringTable(currentLocale);

  std::vector<std::string> ids;
  if (defaultTable) {
    ids = defaultTable->getStringIds();
  }
  if (currentTable) {
    auto currentIds = currentTable->getStringIds();
    ids.insert(ids.end(), currentIds.begin(), currentIds.end());
  }

  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  for (const auto &id : ids) {
    LocalizationEntry entry;
    entry.key = QString::fromStdString(id);

    if (defaultTable) {
      auto val = defaultTable->getString(id);
      if (val) {
        entry.translations[m_defaultLocale] = QString::fromStdString(*val);
      }
    }
    if (currentTable && m_currentLocale != m_defaultLocale) {
      auto val = currentTable->getString(id);
      if (val) {
        entry.translations[m_currentLocale] = QString::fromStdString(*val);
      }
    }

    // Check if missing translation for current locale
    entry.isMissing =
        !entry.translations.contains(m_currentLocale) ||
        entry.translations.value(m_currentLocale).isEmpty();

    m_entries[entry.key] = entry;
  }
}

void NMLocalizationPanel::syncEntriesToManager() {
  NovelMind::localization::LocaleId defaultLocale;
  defaultLocale.language = m_defaultLocale.toStdString();
  NovelMind::localization::LocaleId currentLocale;
  currentLocale.language = m_currentLocale.toStdString();

  // Remove deleted keys from manager
  for (const QString &key : m_deletedKeys) {
    m_localization.removeString(defaultLocale, key.toStdString());
    m_localization.removeString(currentLocale, key.toStdString());
  }

  // Update/add entries
  for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
    const LocalizationEntry &entry = it.value();
    if (entry.isDeleted) {
      continue;
    }

    const std::string id = entry.key.toStdString();

    if (entry.translations.contains(m_defaultLocale)) {
      m_localization.setString(
          defaultLocale, id,
          entry.translations.value(m_defaultLocale).toStdString());
    }
    if (entry.translations.contains(m_currentLocale)) {
      m_localization.setString(
          currentLocale, id,
          entry.translations.value(m_currentLocale).toStdString());
    }
  }
}

void NMLocalizationPanel::rebuildTable() {
  if (!m_stringsTable) {
    return;
  }

  m_stringsTable->blockSignals(true);
  m_stringsTable->setRowCount(0);

  QList<QString> sortedKeys = m_entries.keys();
  std::sort(sortedKeys.begin(), sortedKeys.end());

  int row = 0;
  for (const QString &key : sortedKeys) {
    const LocalizationEntry &entry = m_entries.value(key);

    if (entry.isDeleted) {
      continue;
    }

    m_stringsTable->insertRow(row);

    auto *idItem = new QTableWidgetItem(entry.key);
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    m_stringsTable->setItem(row, 0, idItem);

    QString sourceText = entry.translations.value(m_defaultLocale);
    auto *sourceItem = new QTableWidgetItem(sourceText);
    sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable);
    m_stringsTable->setItem(row, 1, sourceItem);

    QString translationText = entry.translations.value(m_currentLocale);
    auto *translationItem = new QTableWidgetItem(translationText);
    m_stringsTable->setItem(row, 2, translationItem);

    row++;
  }

  m_stringsTable->blockSignals(false);
  applyFilters();
  highlightMissingTranslations();
}

void NMLocalizationPanel::applyFilters() {
  if (!m_stringsTable) {
    return;
  }

  const QString searchText = m_searchEdit ? m_searchEdit->text().toLower() : QString();
  const bool showMissingOnly = m_showMissingOnly && m_showMissingOnly->isChecked();

  int visibleCount = 0;
  int missingCount = 0;

  for (int row = 0; row < m_stringsTable->rowCount(); ++row) {
    auto *idItem = m_stringsTable->item(row, 0);
    auto *sourceItem = m_stringsTable->item(row, 1);
    auto *translationItem = m_stringsTable->item(row, 2);

    if (!idItem) {
      m_stringsTable->setRowHidden(row, true);
      continue;
    }

    const QString key = idItem->text();
    const QString source = sourceItem ? sourceItem->text() : QString();
    const QString translation = translationItem ? translationItem->text() : QString();

    bool isMissing = translation.isEmpty();
    if (isMissing) {
      missingCount++;
    }

    // Apply search filter (case-insensitive)
    bool matchesSearch = searchText.isEmpty() ||
                         key.toLower().contains(searchText) ||
                         source.toLower().contains(searchText) ||
                         translation.toLower().contains(searchText);

    // Apply missing filter
    bool matchesMissingFilter = !showMissingOnly || isMissing;

    bool visible = matchesSearch && matchesMissingFilter;
    m_stringsTable->setRowHidden(row, !visible);

    if (visible) {
      visibleCount++;
    }
  }

  // Update status
  updateStatusBar();
}

void NMLocalizationPanel::highlightMissingTranslations() {
  if (!m_stringsTable) {
    return;
  }

  const QColor missingBg(255, 230, 230);  // Light red
  const QColor normalBg = m_stringsTable->palette().base().color();

  for (int row = 0; row < m_stringsTable->rowCount(); ++row) {
    auto *translationItem = m_stringsTable->item(row, 2);
    if (!translationItem) {
      continue;
    }

    const bool isMissing = translationItem->text().isEmpty();

    // Set background for all columns in the row
    for (int col = 0; col < m_stringsTable->columnCount(); ++col) {
      auto *item = m_stringsTable->item(row, col);
      if (item) {
        item->setBackground(isMissing ? missingBg : normalBg);
      }
    }
  }
}

void NMLocalizationPanel::updateStatusBar() {
  if (!m_statusLabel) {
    return;
  }

  int totalCount = 0;
  int visibleCount = 0;
  int missingCount = 0;

  for (int row = 0; row < m_stringsTable->rowCount(); ++row) {
    totalCount++;
    if (!m_stringsTable->isRowHidden(row)) {
      visibleCount++;
    }
    auto *translationItem = m_stringsTable->item(row, 2);
    if (translationItem && translationItem->text().isEmpty()) {
      missingCount++;
    }
  }

  QString status = tr("Showing %1 of %2 keys | Missing translations: %3")
                       .arg(visibleCount)
                       .arg(totalCount)
                       .arg(missingCount);

  if (m_dirty) {
    status += tr(" | [Modified]");
  }

  m_statusLabel->setText(status);
}

void NMLocalizationPanel::onSearchTextChanged(const QString &text) {
  Q_UNUSED(text);
  applyFilters();
}

void NMLocalizationPanel::onShowOnlyMissingToggled(bool checked) {
  Q_UNUSED(checked);
  applyFilters();
}

void NMLocalizationPanel::onLocaleChanged(int index) {
  Q_UNUSED(index);
  if (!m_languageSelector) {
    return;
  }

  const QString localeCode = m_languageSelector->currentData().toString();
  if (!localeCode.isEmpty()) {
    loadLocale(localeCode);
  }
}

void NMLocalizationPanel::onCellChanged(int row, int column) {
  if (!m_stringsTable || column != 2) {
    return;
  }

  auto *idItem = m_stringsTable->item(row, 0);
  auto *valueItem = m_stringsTable->item(row, 2);
  if (!idItem || !valueItem || m_currentLocale.isEmpty()) {
    return;
  }

  const QString key = idItem->text();
  const QString value = valueItem->text();

  // Update in-memory entry
  if (m_entries.contains(key)) {
    m_entries[key].translations[m_currentLocale] = value;
    m_entries[key].isMissing = value.isEmpty();
    m_entries[key].isModified = true;
  }

  // Update manager
  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  m_localization.setString(locale, key.toStdString(), value.toStdString());

  setDirty(true);
  highlightMissingTranslations();
  updateStatusBar();

  emit translationChanged(key, m_currentLocale, value);
}

void NMLocalizationPanel::onAddKeyClicked() {
  QString key;
  QString defaultValue;

  if (!showAddKeyDialog(key, defaultValue)) {
    return;
  }

  if (addKey(key, defaultValue)) {
    rebuildTable();

    // Select the new row
    for (int row = 0; row < m_stringsTable->rowCount(); ++row) {
      auto *item = m_stringsTable->item(row, 0);
      if (item && item->text() == key) {
        m_stringsTable->selectRow(row);
        m_stringsTable->scrollToItem(item);
        break;
      }
    }
  }
}

bool NMLocalizationPanel::showAddKeyDialog(QString &outKey,
                                           QString &outDefaultValue) {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Add Localization Key"));
  dialog.setMinimumWidth(400);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  QFormLayout *formLayout = new QFormLayout();

  QLineEdit *keyEdit = new QLineEdit(&dialog);
  keyEdit->setPlaceholderText(tr("e.g., menu.button.start"));
  formLayout->addRow(tr("Key:"), keyEdit);

  QLineEdit *valueEdit = new QLineEdit(&dialog);
  valueEdit->setPlaceholderText(tr("Default value (optional)"));
  formLayout->addRow(tr("Default Value:"), valueEdit);

  QLabel *errorLabel = new QLabel(&dialog);
  errorLabel->setStyleSheet("color: red;");
  errorLabel->hide();

  layout->addLayout(formLayout);
  layout->addWidget(errorLabel);

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                           &dialog);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(false);

  layout->addWidget(buttonBox);

  // Validation on key change
  connect(keyEdit, &QLineEdit::textChanged, [this, okButton, errorLabel,
                                             keyEdit](const QString &text) {
    QString error;
    bool valid = true;

    if (text.isEmpty()) {
      valid = false;
      error = tr("Key cannot be empty");
    } else if (!isValidKeyName(text)) {
      valid = false;
      error = tr("Key must contain only letters, numbers, underscore, dot, or dash");
    } else if (!isKeyUnique(text)) {
      valid = false;
      error = tr("Key already exists");
    }

    okButton->setEnabled(valid);
    if (!valid && !text.isEmpty()) {
      errorLabel->setText(error);
      errorLabel->show();
    } else {
      errorLabel->hide();
    }
  });

  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return false;
  }

  outKey = keyEdit->text().trimmed();
  outDefaultValue = valueEdit->text();
  return true;
}

bool NMLocalizationPanel::isValidKeyName(const QString &key) const {
  return s_keyValidationRegex.match(key).hasMatch();
}

bool NMLocalizationPanel::isKeyUnique(const QString &key) const {
  return !m_entries.contains(key) || m_entries.value(key).isDeleted;
}

bool NMLocalizationPanel::addKey(const QString &key,
                                  const QString &defaultValue) {
  if (key.isEmpty() || !isValidKeyName(key) || !isKeyUnique(key)) {
    return false;
  }

  LocalizationEntry entry;
  entry.key = key;
  entry.translations[m_defaultLocale] = defaultValue;
  entry.isNew = true;
  entry.isMissing = m_currentLocale != m_defaultLocale;

  m_entries[key] = entry;

  // Add to manager
  NovelMind::localization::LocaleId locale;
  locale.language = m_defaultLocale.toStdString();
  m_localization.setString(locale, key.toStdString(),
                           defaultValue.toStdString());

  setDirty(true);
  return true;
}

void NMLocalizationPanel::onDeleteKeyClicked() {
  if (!m_stringsTable) {
    return;
  }

  QList<QTableWidgetItem *> selectedItems = m_stringsTable->selectedItems();
  if (selectedItems.isEmpty()) {
    NMMessageDialog::showInfo(this, tr("Delete Key"),
                              tr("Please select a key to delete."));
    return;
  }

  // Get unique keys from selection
  QSet<QString> keysToDelete;
  for (auto *item : selectedItems) {
    int row = item->row();
    auto *idItem = m_stringsTable->item(row, 0);
    if (idItem) {
      keysToDelete.insert(idItem->text());
    }
  }

  QString message;
  if (keysToDelete.size() == 1) {
    message = tr("Are you sure you want to delete the key '%1'?")
                  .arg(*keysToDelete.begin());
  } else {
    message = tr("Are you sure you want to delete %1 keys?")
                  .arg(keysToDelete.size());
  }

  NMDialogButton result = NMMessageDialog::showQuestion(
      this, tr("Confirm Delete"), message,
      {NMDialogButton::Yes, NMDialogButton::No}, NMDialogButton::No);

  if (result != NMDialogButton::Yes) {
    return;
  }

  for (const QString &key : keysToDelete) {
    deleteKey(key);
  }

  rebuildTable();
}

bool NMLocalizationPanel::deleteKey(const QString &key) {
  if (!m_entries.contains(key)) {
    return false;
  }

  m_entries[key].isDeleted = true;
  m_deletedKeys.insert(key);

  // Remove from manager
  NovelMind::localization::LocaleId defaultLocale;
  defaultLocale.language = m_defaultLocale.toStdString();
  NovelMind::localization::LocaleId currentLocale;
  currentLocale.language = m_currentLocale.toStdString();

  m_localization.removeString(defaultLocale, key.toStdString());
  m_localization.removeString(currentLocale, key.toStdString());

  setDirty(true);
  return true;
}

void NMLocalizationPanel::onContextMenu(const QPoint &pos) {
  QMenu menu(this);

  QAction *addAction = menu.addAction(tr("Add Key..."));
  connect(addAction, &QAction::triggered, this,
          &NMLocalizationPanel::onAddKeyClicked);

  auto *idItem = m_stringsTable->itemAt(pos);
  if (idItem) {
    menu.addSeparator();

    QAction *deleteAction = menu.addAction(tr("Delete Key"));
    connect(deleteAction, &QAction::triggered, this,
            &NMLocalizationPanel::onDeleteKeyClicked);

    QAction *copyKeyAction = menu.addAction(tr("Copy Key"));
    connect(copyKeyAction, &QAction::triggered, [this, idItem]() {
      int row = idItem->row();
      auto *keyItem = m_stringsTable->item(row, 0);
      if (keyItem) {
        QApplication::clipboard()->setText(keyItem->text());
      }
    });
  }

  menu.exec(m_stringsTable->viewport()->mapToGlobal(pos));
}

QStringList
NMLocalizationPanel::findMissingTranslations(const QString &locale) const {
  QStringList missing;

  for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
    const LocalizationEntry &entry = it.value();
    if (entry.isDeleted) {
      continue;
    }

    if (!entry.translations.contains(locale) ||
        entry.translations.value(locale).isEmpty()) {
      missing.append(entry.key);
    }
  }

  return missing;
}

QStringList NMLocalizationPanel::findUnusedKeys() const {
  QStringList unused;

  for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
    const LocalizationEntry &entry = it.value();
    if (entry.isDeleted) {
      continue;
    }

    if (entry.isUnused || entry.usageLocations.isEmpty()) {
      unused.append(entry.key);
    }
  }

  return unused;
}

void NMLocalizationPanel::scanProjectForUsages() {
  // TODO: Implement project scanning for key usages
  // This would scan script files for references to localization keys
}

void NMLocalizationPanel::navigateToUsage(const QString &key, int usageIndex) {
  if (!m_entries.contains(key)) {
    return;
  }

  const LocalizationEntry &entry = m_entries.value(key);
  if (usageIndex >= 0 && usageIndex < entry.usageLocations.size()) {
    emit navigateToFile(entry.usageLocations.at(usageIndex), 0);
  }
}

void NMLocalizationPanel::setDirty(bool dirty) {
  if (m_dirty != dirty) {
    m_dirty = dirty;
    if (m_saveBtn) {
      m_saveBtn->setEnabled(dirty);
    }
    updateStatusBar();
    emit dirtyStateChanged(dirty);
  }
}

void NMLocalizationPanel::onSaveClicked() { saveChanges(); }

bool NMLocalizationPanel::saveChanges() {
  auto &pm = ProjectManager::instance();
  if (!pm.hasOpenProject()) {
    NMMessageDialog::showError(this, tr("Save Failed"),
                               tr("No project is open."));
    return false;
  }

  syncEntriesToManager();

  const QString localizationRoot =
      QString::fromStdString(pm.getFolderPath(ProjectFolder::Localization));
  QDir dir(localizationRoot);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  // Save each locale to its file
  for (const QString &localeCode : m_availableLocales) {
    NovelMind::localization::LocaleId locale;
    locale.language = localeCode.toStdString();

    // Default to CSV format
    const QString filePath = dir.filePath(localeCode + ".csv");
    auto result = m_localization.exportStrings(
        locale, filePath.toStdString(),
        NovelMind::localization::LocalizationFormat::CSV);

    if (result.isError()) {
      NMMessageDialog::showError(
          this, tr("Save Failed"),
          tr("Failed to save %1: %2")
              .arg(localeCode)
              .arg(QString::fromStdString(result.error())));
      return false;
    }
  }

  m_deletedKeys.clear();
  setDirty(false);

  // Mark entries as not modified
  for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
    it->isModified = false;
    it->isNew = false;
  }

  return true;
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

  syncEntriesToManager();

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
  const QString path = NMFileDialog::getOpenFileName(
      this, tr("Import Localization"), QDir::homePath(), filter);
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
  setDirty(true);
}

void NMLocalizationPanel::onUpdate([[maybe_unused]] double deltaTime) {}

void NMLocalizationPanel::onItemDoubleClicked(QTableWidgetItem *item) {
  if (!item) {
    return;
  }

  int row = item->row();
  auto *idItem = m_stringsTable->item(row, 0);
  if (idItem) {
    emit keySelected(idItem->text());
  }
}

void NMLocalizationPanel::onFilterChanged(int index) {
  Q_UNUSED(index);
  applyFilters();
}

void NMLocalizationPanel::onExportClicked() { exportLocale(); }

void NMLocalizationPanel::onImportClicked() { importLocale(); }

void NMLocalizationPanel::onRefreshClicked() {
  if (m_dirty) {
    NMDialogButton result = NMMessageDialog::showQuestion(
        this, tr("Unsaved Changes"),
        tr("You have unsaved changes. Do you want to discard them and refresh?"),
        {NMDialogButton::Yes, NMDialogButton::No}, NMDialogButton::No);

    if (result != NMDialogButton::Yes) {
      return;
    }
  }

  setDirty(false);
  refreshLocales();
}

void NMLocalizationPanel::setupToolBar() {
  // Toolbar setup is handled in setupUI
}

void NMLocalizationPanel::setupFilterBar() {
  // Filter bar setup is handled in setupUI
}

void NMLocalizationPanel::setupTable() {
  // Table setup is handled in setupUI
}

void NMLocalizationPanel::exportToCsv(const QString &filePath) {
  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  m_localization.exportStrings(locale, filePath.toStdString(),
                               NovelMind::localization::LocalizationFormat::CSV);
}

void NMLocalizationPanel::exportToJson(const QString &filePath) {
  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  m_localization.exportStrings(locale, filePath.toStdString(),
                               NovelMind::localization::LocalizationFormat::JSON);
}

void NMLocalizationPanel::importFromCsv(const QString &filePath) {
  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  m_localization.loadStrings(locale, filePath.toStdString(),
                             NovelMind::localization::LocalizationFormat::CSV);
  syncEntriesFromManager();
  rebuildTable();
}

void NMLocalizationPanel::importFromJson(const QString &filePath) {
  NovelMind::localization::LocaleId locale;
  locale.language = m_currentLocale.toStdString();
  m_localization.loadStrings(locale, filePath.toStdString(),
                             NovelMind::localization::LocalizationFormat::JSON);
  syncEntriesFromManager();
  rebuildTable();
}

} // namespace NovelMind::editor::qt
