#pragma once

/**
 * @file nm_script_editor_panel.hpp
 * @brief Script editor panel for NMScript editing with IDE-like features
 */

#include "NovelMind/editor/qt/nm_dock_panel.hpp"
#include "NovelMind/editor/qt/panels/nm_issues_panel.hpp"
#include <QHash>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QToolBar>
#include <QTimer>
#include <QTreeWidget>

class QCompleter;
class QFileSystemWatcher;
class NMIssuesPanel;

namespace NovelMind::editor::qt {

class NMScriptHighlighter final : public QSyntaxHighlighter {
  Q_OBJECT

public:
  explicit NMScriptHighlighter(QTextDocument *parent = nullptr);

protected:
  void highlightBlock(const QString &text) override;

private:
  struct Rule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };

  QVector<Rule> m_rules;
  QTextCharFormat m_commentFormat;
  QRegularExpression m_commentStart;
  QRegularExpression m_commentEnd;
};


class NMScriptEditor final : public QPlainTextEdit {
  Q_OBJECT

public:
  struct CompletionEntry {
    QString text;
    QString detail;
  };

  explicit NMScriptEditor(QWidget *parent = nullptr);

  void setCompletionWords(const QStringList &words);
  void setCompletionEntries(const QList<CompletionEntry> &entries);
  void setHoverDocs(const QHash<QString, QString> &docs);
  void setDocHtml(const QHash<QString, QString> &docs);
  void setProjectDocs(const QHash<QString, QString> &docs);
  [[nodiscard]] int indentSize() const { return m_indentSize; }

  int lineNumberAreaWidth() const;
  void lineNumberAreaPaintEvent(QPaintEvent *event);

signals:
  void requestSave();
  void hoverDocChanged(const QString &token, const QString &html);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  QString textUnderCursor() const;
  void insertCompletion(const QString &completion);
  void refreshDynamicCompletions();
  void rebuildCompleterModel(const QList<CompletionEntry> &entries);
  void updateLineNumberAreaWidth(int newBlockCount = 0);
  void updateLineNumberArea(const QRect &rect, int dy);
  void highlightCurrentLine();
  void handleTabKey(QKeyEvent *event);
  void handleBacktabKey(QKeyEvent *event);
  void handleReturnKey(QKeyEvent *event);
  void indentSelection(int delta);
  QString indentForCurrentLine(int *outLogicalIndent = nullptr) const;

  QCompleter *m_completer = nullptr;
  QHash<QString, QString> m_hoverDocs;
  QHash<QString, QString> m_docHtml;
  QHash<QString, QString> m_projectDocs;
  QStringList m_baseCompletionWords;
  QString m_lastHoverToken;
  QList<CompletionEntry> m_staticCompletionEntries;
  QList<CompletionEntry> m_cachedCompletionEntries;
  QWidget *m_lineNumberArea = nullptr;
  int m_indentSize = 2;
};

class NMScriptEditorPanel final : public NMDockPanel {
  Q_OBJECT

public:
  explicit NMScriptEditorPanel(QWidget *parent = nullptr);
  void setIssuesPanel(class NMIssuesPanel *panel) { m_issuesPanel = panel; }
  ~NMScriptEditorPanel() override;

  void onInitialize() override;
  void onUpdate(double deltaTime) override;

  void openScript(const QString &path);
  void refreshFileList();
  void goToLocation(const QString &path, int line);

signals:
  void docHtmlChanged(const QString &html);

private slots:
  void onFileActivated(QTreeWidgetItem *item, int column);
  void onSaveRequested();
  void onSaveAllRequested();
  void onFormatRequested();
  void onCurrentTabChanged(int index);
  void runDiagnostics();

private:
  void setupContent();
  void addEditorTab(const QString &path);
  bool saveEditor(QPlainTextEdit *editor);
  bool ensureScriptFile(const QString &path);
  QList<NMScriptIssue> validateSource(const QString &path,
                                      const QString &source) const;
  void refreshSymbolIndex();
  void pushCompletionsToEditors();
  QList<NMScriptEditor::CompletionEntry> buildProjectCompletionEntries() const;
  QHash<QString, QString> buildProjectHoverDocs() const;
  QHash<QString, QString> buildProjectDocHtml() const;
  void rebuildWatchList();
  QString scriptsRootPath() const;
  QList<NMScriptEditor *> editors() const;

  QWidget *m_contentWidget = nullptr;
  QSplitter *m_splitter = nullptr;
  QTreeWidget *m_fileTree = nullptr;
  QTabWidget *m_tabs = nullptr;
  QToolBar *m_toolBar = nullptr;

  QHash<QWidget *, QString> m_tabPaths;
  QPointer<QFileSystemWatcher> m_scriptWatcher;

  struct ScriptSymbolIndex {
    QHash<QString, QString> scenes;     // name -> file path
    QHash<QString, QString> characters; // name -> file path
    QHash<QString, QString> flags;      // name -> file path
    QHash<QString, QString> variables;  // name -> file path
    QStringList backgrounds;            // asset ids seen in scripts
    QStringList voices;                 // voice ids seen in scripts
    QStringList music;                  // music ids seen in scripts
  } m_symbolIndex;

  QTimer m_diagnosticsTimer;
  class NMIssuesPanel *m_issuesPanel = nullptr;
};

} // namespace NovelMind::editor::qt
