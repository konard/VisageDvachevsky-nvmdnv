#include "NovelMind/editor/qt/panels/nm_script_editor_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"
#include "NovelMind/editor/qt/nm_icon_manager.hpp"
#include "NovelMind/editor/qt/panels/nm_issues_panel.hpp"
#include "NovelMind/scripting/compiler.hpp"
#include "NovelMind/scripting/lexer.hpp"
#include "NovelMind/scripting/parser.hpp"
#include "NovelMind/scripting/validator.hpp"
#include "NovelMind/core/logger.hpp"

#include <QAbstractItemView>
#include <QCompleter>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QStringListModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QTextBlock>
#include <QTextFormat>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWindow>
#include <algorithm>
#include <filesystem>
#include <limits>

#include "nm_script_editor_panel_detail.hpp"

namespace NovelMind::editor::qt {

// NMScriptEditorPanel
// ============================================================================

NMScriptEditorPanel::NMScriptEditorPanel(QWidget *parent)
    : NMDockPanel(tr("Script Editor"), parent) {
  setPanelId("ScriptEditor");
  setWindowIcon(NMIconManager::instance().getIcon("panel-console", 16));
  m_diagnosticsTimer.setSingleShot(true);
  m_diagnosticsTimer.setInterval(600);
  connect(&m_diagnosticsTimer, &QTimer::timeout, this,
          &NMScriptEditorPanel::runDiagnostics);
  m_scriptWatcher = new QFileSystemWatcher(this);
  connect(m_scriptWatcher, &QFileSystemWatcher::directoryChanged, this,
          [this](const QString &) {
            refreshFileList();
            refreshSymbolIndex();
          });
  connect(m_scriptWatcher, &QFileSystemWatcher::fileChanged, this,
          [this](const QString &) { refreshSymbolIndex(); });
  setupContent();
}

NMScriptEditorPanel::~NMScriptEditorPanel() = default;

void NMScriptEditorPanel::onInitialize() { refreshFileList(); }

void NMScriptEditorPanel::onUpdate(double /*deltaTime*/) {}

void NMScriptEditorPanel::openScript(const QString &path) {
  if (path.isEmpty()) {
    return;
  }

  QString resolvedPath = path;
  if (QFileInfo(path).isRelative()) {
    resolvedPath = QString::fromStdString(
        ProjectManager::instance().toAbsolutePath(path.toStdString()));
  }

  if (!ensureScriptFile(resolvedPath)) {
    return;
  }

  refreshFileList();

  for (int i = 0; i < m_tabs->count(); ++i) {
    if (m_tabPaths.value(m_tabs->widget(i)) == resolvedPath) {
      m_tabs->setCurrentIndex(i);
      return;
    }
  }

  addEditorTab(resolvedPath);
}

void NMScriptEditorPanel::refreshFileList() {
  m_fileTree->clear();

  const QString rootPath = scriptsRootPath();
  if (rootPath.isEmpty()) {
    if (m_issuesPanel) {
      m_issuesPanel->setIssues({});
    }
    return;
  }

  QTreeWidgetItem *rootItem = new QTreeWidgetItem(m_fileTree);
  rootItem->setText(0, QFileInfo(rootPath).fileName());
  rootItem->setData(0, Qt::UserRole, rootPath);

  namespace fs = std::filesystem;
  fs::path base(rootPath.toStdString());
  if (!fs::exists(base)) {
    return;
  }

  try {
    for (const auto &entry : fs::recursive_directory_iterator(base)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() != ".nms") {
        continue;
      }

      const fs::path rel = fs::relative(entry.path(), base);
      QTreeWidgetItem *parentItem = rootItem;

      for (const auto &part : rel.parent_path()) {
        const QString partName = QString::fromStdString(part.string());
        QTreeWidgetItem *child = nullptr;
        for (int i = 0; i < parentItem->childCount(); ++i) {
          if (parentItem->child(i)->text(0) == partName) {
            child = parentItem->child(i);
            break;
          }
        }
        if (!child) {
          child = new QTreeWidgetItem(parentItem);
          child->setText(0, partName);
          child->setData(0, Qt::UserRole, QString());
        }
        parentItem = child;
      }

      QTreeWidgetItem *fileItem = new QTreeWidgetItem(parentItem);
      fileItem->setText(
          0, QString::fromStdString(entry.path().filename().string()));
      fileItem->setData(
          0, Qt::UserRole,
          QString::fromStdString(entry.path().string()));
    }
  } catch (const std::exception &e) {
    core::Logger::instance().warning(
        std::string("Failed to scan scripts folder: ") + e.what());
  }

  m_fileTree->expandAll();

  rebuildWatchList();
  refreshSymbolIndex();
}

void NMScriptEditorPanel::onFileActivated(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (!item) {
    return;
  }
  const QString path = item->data(0, Qt::UserRole).toString();
  if (path.isEmpty()) {
    return;
  }
  openScript(path);
}

void NMScriptEditorPanel::onSaveRequested() {
  if (!m_tabs) {
    return;
  }
  if (auto *editor = qobject_cast<QPlainTextEdit *>(m_tabs->currentWidget())) {
    saveEditor(editor);
  }
  refreshSymbolIndex();
  m_diagnosticsTimer.start();
}

void NMScriptEditorPanel::onSaveAllRequested() {
  for (int i = 0; i < m_tabs->count(); ++i) {
    if (auto *editor = qobject_cast<QPlainTextEdit *>(m_tabs->widget(i))) {
      saveEditor(editor);
    }
  }
  refreshSymbolIndex();
  m_diagnosticsTimer.start();
}

void NMScriptEditorPanel::onFormatRequested() {
  if (!m_tabs) {
    return;
  }
  auto *editor = qobject_cast<NMScriptEditor *>(m_tabs->currentWidget());
  if (!editor) {
    return;
  }

  const QStringList lines = editor->toPlainText().split('\n');
  QStringList formatted;
  formatted.reserve(lines.size());

  int indentLevel = 0;
  const int indentSize = editor->indentSize();

  for (const auto &line : lines) {
    const QString trimmed = line.trimmed();

    int leadingClosers = 0;
    while (leadingClosers < trimmed.size() &&
           trimmed.at(leadingClosers) == '}') {
      ++leadingClosers;
    }
    indentLevel = std::max(0, indentLevel - leadingClosers);

    if (trimmed.isEmpty()) {
      formatted.append(QString());
    } else {
      const QString indent(indentLevel * indentSize, ' ');
      formatted.append(indent + trimmed);
    }

    const int netBraces =
        static_cast<int>(trimmed.count('{') - trimmed.count('}'));
    indentLevel = std::max(0, indentLevel + netBraces);
  }

  const int originalPos = editor->textCursor().position();
  editor->setPlainText(formatted.join("\n"));
  QTextCursor cursor = editor->textCursor();
  cursor.setPosition(std::min(originalPos,
                              editor->document()->characterCount() - 1));
  editor->setTextCursor(cursor);
}

void NMScriptEditorPanel::onCurrentTabChanged(int index) {
  Q_UNUSED(index);
  emit docHtmlChanged(QString());
  m_diagnosticsTimer.start();
}

void NMScriptEditorPanel::setupContent() {
  m_contentWidget = new QWidget(this);
  auto *layout = new QVBoxLayout(m_contentWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  m_toolBar = new QToolBar(m_contentWidget);
  m_toolBar->setIconSize(QSize(16, 16));
  QAction *actionSave = m_toolBar->addAction(tr("Save"));
  actionSave->setToolTip(tr("Save (Ctrl+S)"));
  connect(actionSave, &QAction::triggered, this,
          &NMScriptEditorPanel::onSaveRequested);

  QAction *actionSaveAll = m_toolBar->addAction(tr("Save All"));
  actionSaveAll->setToolTip(tr("Save all open scripts"));
  connect(actionSaveAll, &QAction::triggered, this,
          &NMScriptEditorPanel::onSaveAllRequested);

  QAction *actionFormat = m_toolBar->addAction(tr("Format"));
  actionFormat->setToolTip(tr("Auto-format script (Ctrl+Shift+F)"));
  actionFormat->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
  actionFormat->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(actionFormat);
  connect(actionFormat, &QAction::triggered, this,
          &NMScriptEditorPanel::onFormatRequested);

  layout->addWidget(m_toolBar);

  m_splitter = new QSplitter(Qt::Horizontal, m_contentWidget);
  m_fileTree = new QTreeWidget(m_splitter);
  m_fileTree->setHeaderHidden(true);
  m_fileTree->setMinimumWidth(180);
  m_fileTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

  connect(m_fileTree, &QTreeWidget::itemDoubleClicked, this,
          &NMScriptEditorPanel::onFileActivated);
  connect(m_fileTree, &QTreeWidget::itemActivated, this,
          &NMScriptEditorPanel::onFileActivated);

  m_tabs = new QTabWidget(m_splitter);
  m_tabs->setTabsClosable(true);
  connect(m_tabs, &QTabWidget::currentChanged, this,
          &NMScriptEditorPanel::onCurrentTabChanged);
  connect(m_tabs, &QTabWidget::tabCloseRequested, this,
          [this](int index) {
            QWidget *widget = m_tabs->widget(index);
            m_tabPaths.remove(widget);
            m_tabs->removeTab(index);
            delete widget;
          });

  m_splitter->setStretchFactor(1, 1);
  layout->addWidget(m_splitter);

  setContentWidget(m_contentWidget);
}

void NMScriptEditorPanel::addEditorTab(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  const QString content = QString::fromUtf8(file.readAll());

  auto *editor = new NMScriptEditor(m_tabs);
  editor->setPlainText(content);
  editor->setHoverDocs(detail::buildHoverDocs());
  editor->setDocHtml(detail::buildDocHtml());
  connect(editor, &NMScriptEditor::requestSave, this,
          &NMScriptEditorPanel::onSaveRequested);
  connect(editor, &NMScriptEditor::hoverDocChanged, this,
          [this](const QString &, const QString &html) {
            emit docHtmlChanged(html);
          });
  connect(editor, &QPlainTextEdit::textChanged, this, [this]() {
    m_diagnosticsTimer.start();
  });

  connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
    const int index = m_tabs->indexOf(editor);
    if (index < 0) {
      return;
    }
    const QString filePath = m_tabPaths.value(editor);
    const QString name = QFileInfo(filePath).fileName();
    if (!m_tabs->tabText(index).endsWith("*")) {
      m_tabs->setTabText(index, name + "*");
    }
    m_diagnosticsTimer.start();
  });

  const QString name = QFileInfo(path).fileName();
  m_tabs->addTab(editor, name);
  m_tabs->setCurrentWidget(editor);
  editor->setFocus();
  m_tabPaths.insert(editor, path);
  pushCompletionsToEditors();
}

bool NMScriptEditorPanel::saveEditor(QPlainTextEdit *editor) {
  if (!editor) {
    return false;
  }

  const QString path = m_tabPaths.value(editor);
  if (path.isEmpty()) {
    return false;
  }

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  out << editor->toPlainText();
  file.close();

  const QString name = QFileInfo(path).fileName();
  const int index = m_tabs->indexOf(editor);
  if (index >= 0) {
    m_tabs->setTabText(index, name);
  }

  m_diagnosticsTimer.start();
  return true;
}

bool NMScriptEditorPanel::ensureScriptFile(const QString &path) {
  if (path.isEmpty()) {
    return false;
  }

  QFileInfo info(path);
  QDir dir = info.dir();
  if (!dir.exists()) {
    if (!dir.mkpath(".")) {
      return false;
    }
  }

  if (info.exists()) {
    return true;
  }

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  const QString sceneName = info.completeBaseName().isEmpty()
                                ? "scene"
                                : info.completeBaseName();
  QTextStream out(&file);
  out << "// " << sceneName << "\n";
  out << "scene " << sceneName << " {\n";
  out << "  say Narrator \"New script\"\n";
  out << "}\n";
  file.close();
  return true;
}

QString NMScriptEditorPanel::scriptsRootPath() const {
  const auto path =
      ProjectManager::instance().getFolderPath(ProjectFolder::Scripts);
  return QString::fromStdString(path);
}

void NMScriptEditorPanel::rebuildWatchList() {
  if (!m_scriptWatcher) {
    return;
  }

  const QString root = scriptsRootPath();
  const QStringList watchedDirs = m_scriptWatcher->directories();
  if (!watchedDirs.isEmpty()) {
    m_scriptWatcher->removePaths(watchedDirs);
  }
  const QStringList watchedFiles = m_scriptWatcher->files();
  if (!watchedFiles.isEmpty()) {
    m_scriptWatcher->removePaths(watchedFiles);
  }

  if (root.isEmpty() || !QFileInfo::exists(root)) {
    return;
  }

  QStringList directories;
  QStringList files;
  directories.append(root);

  namespace fs = std::filesystem;
  fs::path base(root.toStdString());
  try {
    for (const auto &entry : fs::recursive_directory_iterator(base)) {
      if (entry.is_directory()) {
        directories.append(QString::fromStdString(entry.path().string()));
      } else if (entry.is_regular_file() &&
                 entry.path().extension() == ".nms") {
        files.append(QString::fromStdString(entry.path().string()));
      }
    }
  } catch (const std::exception &e) {
    core::Logger::instance().warning(
        std::string("Failed to rebuild script watcher: ") + e.what());
  }

  if (!directories.isEmpty()) {
    m_scriptWatcher->addPaths(directories);
  }
  if (!files.isEmpty()) {
    m_scriptWatcher->addPaths(files);
  }
}

void NMScriptEditorPanel::refreshSymbolIndex() {
  m_symbolIndex = {};
  const QString root = scriptsRootPath();
  if (root.isEmpty()) {
    pushCompletionsToEditors();
    if (m_issuesPanel) {
      m_issuesPanel->setIssues({});
    }
    return;
  }

  namespace fs = std::filesystem;
  fs::path base(root.toStdString());
  if (!fs::exists(base)) {
    pushCompletionsToEditors();
    return;
  }

  QSet<QString> seenScenes;
  QSet<QString> seenCharacters;
  QSet<QString> seenFlags;
  QSet<QString> seenVariables;
  QSet<QString> seenBackgrounds;
  QSet<QString> seenVoices;
  QSet<QString> seenMusic;

  auto insertMap = [](QHash<QString, QString> &map, QSet<QString> &seen,
                      const QString &value, const QString &filePath) {
    const QString key = value.toLower();
    if (value.isEmpty() || seen.contains(key)) {
      return;
    }
    seen.insert(key);
    map.insert(value, filePath);
  };

  auto insertList = [](QStringList &list, QSet<QString> &seen,
                       const QString &value) {
    const QString key = value.toLower();
    if (value.isEmpty() || seen.contains(key)) {
      return;
    }
    seen.insert(key);
    list.append(value);
  };

  const QRegularExpression reScene("\\bscene\\s+([A-Za-z_][A-Za-z0-9_]*)");
  const QRegularExpression reCharacter(
      "\\bcharacter\\s+([A-Za-z_][A-Za-z0-9_]*)");
  const QRegularExpression reSetFlag(
      "\\bset\\s+flag\\s+([A-Za-z_][A-Za-z0-9_]*)");
  const QRegularExpression reFlag("\\bflag\\s+([A-Za-z_][A-Za-z0-9_]*)");
  const QRegularExpression reSetVar(
      "\\bset\\s+(?!flag\\s)([A-Za-z_][A-Za-z0-9_]*)");
  const QRegularExpression reBackground(
      "show\\s+background\\s+\"([^\"]+)\"");
  const QRegularExpression reVoice("play\\s+voice\\s+\"([^\"]+)\"");
  const QRegularExpression reMusic("play\\s+music\\s+\"([^\"]+)\"");

  QList<NMScriptIssue> issues;

  try {
    for (const auto &entry : fs::recursive_directory_iterator(base)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".nms") {
        continue;
      }

      const QString path = QString::fromStdString(entry.path().string());
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        continue;
      }
      const QString content = QString::fromUtf8(file.readAll());
      file.close();

      auto collect = [&](const QRegularExpression &regex,
                         auto &&callback) {
        QRegularExpressionMatchIterator it = regex.globalMatch(content);
        while (it.hasNext()) {
          const QRegularExpressionMatch match = it.next();
          callback(match.captured(1));
        }
      };

      collect(reScene, [&](const QString &value) {
        insertMap(m_symbolIndex.scenes, seenScenes, value, path);
        if (value.isEmpty()) {
          issues.push_back({QFileInfo(path).fileName(), 0,
                            tr("Scene block missing identifier"), "warning"});
        }
      });
      collect(reCharacter, [&](const QString &value) {
        insertMap(m_symbolIndex.characters, seenCharacters, value, path);
      });
      collect(reSetFlag, [&](const QString &value) {
        insertMap(m_symbolIndex.flags, seenFlags, value, path);
      });
      collect(reFlag, [&](const QString &value) {
        insertMap(m_symbolIndex.flags, seenFlags, value, path);
      });
      collect(reSetVar, [&](const QString &value) {
        insertMap(m_symbolIndex.variables, seenVariables, value, path);
      });
      collect(reBackground, [&](const QString &value) {
        insertList(m_symbolIndex.backgrounds, seenBackgrounds, value);
      });
      collect(reVoice, [&](const QString &value) {
        insertList(m_symbolIndex.voices, seenVoices, value);
      });
      collect(reMusic, [&](const QString &value) {
        insertList(m_symbolIndex.music, seenMusic, value);
      });
    }
  } catch (const std::exception &e) {
    core::Logger::instance().warning(
        std::string("Failed to build script symbols: ") + e.what());
  }

  pushCompletionsToEditors();
  if (m_issuesPanel) {
    m_issuesPanel->setIssues(issues);
  }
}

QList<NMScriptIssue>
NMScriptEditorPanel::validateSource(const QString &path,
                                    const QString &source) const {
  QList<NMScriptIssue> out;
  using namespace scripting;

  std::string src = source.toStdString();

  Lexer lexer;
  auto lexResult = lexer.tokenize(src);
  for (const auto &err : lexer.getErrors()) {
    out.push_back(
        {path, static_cast<int>(err.location.line),
         QString::fromStdString(err.message), "error"});
  }
  if (!lexResult.isOk()) {
    return out;
  }

  Parser parser;
  auto parseResult = parser.parse(lexResult.value());
  for (const auto &err : parser.getErrors()) {
    out.push_back(
        {path, static_cast<int>(err.location.line),
         QString::fromStdString(err.message), "error"});
  }
  if (!parseResult.isOk()) {
    return out;
  }

  Validator validator;
  auto validation = validator.validate(parseResult.value());
  for (const auto &err : validation.errors.all()) {
    QString severity = "info";
    if (err.severity == Severity::Warning) {
      severity = "warning";
    } else if (err.severity == Severity::Error) {
      severity = "error";
    }
    out.push_back({path,
                   static_cast<int>(err.span.start.line),
                   QString::fromStdString(err.message), severity});
  }

  return out;
}

void NMScriptEditorPanel::runDiagnostics() {
  auto *editor = qobject_cast<NMScriptEditor *>(m_tabs->currentWidget());
  if (!editor) {
    return;
  }
  QString path = m_tabPaths.value(editor);
  if (path.isEmpty()) {
    return;
  }
  const QString text = editor->toPlainText();
  QList<NMScriptIssue> issues = validateSource(path, text);
  if (m_issuesPanel) {
    m_issuesPanel->setIssues(issues);
  }
}

void NMScriptEditorPanel::goToLocation(const QString &path, int line) {
  openScript(path);
  auto *editor = qobject_cast<NMScriptEditor *>(m_tabs->currentWidget());
  if (!editor) {
    return;
  }
  QTextCursor cursor(editor->document()->findBlockByLineNumber(
      std::max(0, line - 1)));
  editor->setTextCursor(cursor);
  editor->setFocus();
}

QList<NMScriptEditor::CompletionEntry>
NMScriptEditorPanel::buildProjectCompletionEntries() const {
  QList<NMScriptEditor::CompletionEntry> entries;

  auto addEntries = [&entries](const QStringList &list,
                               const QString &detail) {
    for (const auto &item : list) {
      entries.push_back({item, detail});
    }
  };

  addEntries(m_symbolIndex.scenes.keys(), "scene");
  addEntries(m_symbolIndex.characters.keys(), "character");
  addEntries(m_symbolIndex.flags.keys(), "flag");
  addEntries(m_symbolIndex.variables.keys(), "variable");
  addEntries(m_symbolIndex.backgrounds, "background");
  addEntries(m_symbolIndex.music, "music");
  addEntries(m_symbolIndex.voices, "voice");

  return entries;
}

QHash<QString, QString> NMScriptEditorPanel::buildProjectHoverDocs() const {
  QHash<QString, QString> docs;
  auto relPath = [](const QString &path) {
    if (path.isEmpty()) {
      return QString();
    }
    return QString::fromStdString(
        ProjectManager::instance().toRelativePath(path.toStdString()));
  };

  auto addDocs = [&](const QHash<QString, QString> &map,
                     const QString &label) {
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
      const QString key = it.key();
      const QString path = relPath(it.value());
      docs.insert(key.toLower(),
                  QString("%1 \"%2\"%3")
                      .arg(label, key,
                           path.isEmpty() ? QString()
                                          : QString(" (%1)").arg(path)));
    }
  };

  addDocs(m_symbolIndex.scenes, tr("Scene"));
  addDocs(m_symbolIndex.characters, tr("Character"));
  addDocs(m_symbolIndex.flags, tr("Flag"));
  addDocs(m_symbolIndex.variables, tr("Variable"));

  for (const auto &bg : m_symbolIndex.backgrounds) {
    docs.insert(bg.toLower(),
                tr("Background asset \"%1\"").arg(bg));
  }
  for (const auto &m : m_symbolIndex.music) {
    docs.insert(m.toLower(), tr("Music track \"%1\"").arg(m));
  }
  for (const auto &v : m_symbolIndex.voices) {
    docs.insert(v.toLower(), tr("Voice asset \"%1\"").arg(v));
  }

  return docs;
}

QHash<QString, QString> NMScriptEditorPanel::buildProjectDocHtml() const {
  QHash<QString, QString> docs;

  auto relPath = [](const QString &path) {
    if (path.isEmpty()) {
      return QString();
    }
    return QString::fromStdString(
        ProjectManager::instance().toRelativePath(path.toStdString()));
  };

  auto addDocs = [&](const QHash<QString, QString> &map,
                     const QString &label) {
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
      const QString name = it.key();
      const QString file = relPath(it.value());
      QString html = QString("<h3>%1</h3>").arg(name.toHtmlEscaped());
      html += QString("<p>%1 definition%2</p>")
                  .arg(label.toHtmlEscaped(),
                       file.isEmpty()
                           ? QString()
                           : QString(" in <code>%1</code>")
                                 .arg(file.toHtmlEscaped()));
      docs.insert(name.toLower(), html);
    }
  };

  addDocs(m_symbolIndex.scenes, tr("Scene"));
  addDocs(m_symbolIndex.characters, tr("Character"));
  addDocs(m_symbolIndex.flags, tr("Flag"));
  addDocs(m_symbolIndex.variables, tr("Variable"));

  auto addSimple = [&](const QStringList &list, const QString &label) {
    for (const auto &item : list) {
      QString html = QString("<h3>%1</h3><p>%2</p>")
                         .arg(item.toHtmlEscaped(), label.toHtmlEscaped());
      docs.insert(item.toLower(), html);
    }
  };

  addSimple(m_symbolIndex.backgrounds, tr("Background asset"));
  addSimple(m_symbolIndex.music, tr("Music track"));
  addSimple(m_symbolIndex.voices, tr("Voice asset"));

  return docs;
}

void NMScriptEditorPanel::pushCompletionsToEditors() {
  QList<NMScriptEditor::CompletionEntry> entries = detail::buildKeywordEntries();
  entries.append(buildProjectCompletionEntries());

  QHash<QString, NMScriptEditor::CompletionEntry> merged;
  for (const auto &entry : entries) {
    const QString key = entry.text.toLower();
    if (!merged.contains(key)) {
      merged.insert(key, entry);
    }
  }

  QList<NMScriptEditor::CompletionEntry> combined = merged.values();
  std::sort(combined.begin(), combined.end(),
            [](const NMScriptEditor::CompletionEntry &a,
               const NMScriptEditor::CompletionEntry &b) {
              return a.text.compare(b.text, Qt::CaseInsensitive) < 0;
            });

  QHash<QString, QString> hoverDocs = detail::buildHoverDocs();
  const QHash<QString, QString> projectHoverDocs = buildProjectHoverDocs();
  for (auto it = projectHoverDocs.constBegin(); it != projectHoverDocs.constEnd();
       ++it) {
    hoverDocs.insert(it.key(), it.value());
  }

  QHash<QString, QString> docHtml = detail::buildDocHtml();
  const QHash<QString, QString> projectDocHtml = buildProjectDocHtml();
  for (auto it = projectDocHtml.constBegin(); it != projectDocHtml.constEnd();
       ++it) {
    docHtml.insert(it.key(), it.value());
  }

  for (auto *editor : editors()) {
    editor->setCompletionEntries(combined);
    editor->setHoverDocs(hoverDocs);
    editor->setProjectDocs(projectHoverDocs);
    editor->setDocHtml(docHtml);
  }

  m_diagnosticsTimer.start();
}

QList<NMScriptEditor *> NMScriptEditorPanel::editors() const {
  QList<NMScriptEditor *> list;
  if (!m_tabs) {
    return list;
  }
  for (int i = 0; i < m_tabs->count(); ++i) {
    if (auto *editor = qobject_cast<NMScriptEditor *>(m_tabs->widget(i))) {
      list.push_back(editor);
    }
  }
  return list;
}


} // namespace NovelMind::editor::qt
