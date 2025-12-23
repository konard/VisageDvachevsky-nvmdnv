#pragma once

#include "NovelMind/editor/qt/panels/nm_script_editor_panel.hpp"

namespace NovelMind::editor::qt::detail {

QStringList buildCompletionWords();
QHash<QString, QString> buildHoverDocs();
QHash<QString, QString> buildDocHtml();
QList<NMScriptEditor::CompletionEntry> buildKeywordEntries();

} // namespace NovelMind::editor::qt::detail
