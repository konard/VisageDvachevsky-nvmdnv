#include "NovelMind/editor/qt/nm_hotkeys_dialog.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMHotkeysDialog::NMHotkeysDialog(const QList<NMHotkeyEntry> &entries,
                                 QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(tr("Hotkeys & Tips"));
  setModal(true);
  resize(680, 520);
  buildUi(entries);
}

void NMHotkeysDialog::buildUi(const QList<NMHotkeyEntry> &entries) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);

  auto *title = new QLabel(
      tr("<b>Hotkeys & Tips</b><br><span style='color: gray;'>Type to filter.</span>"),
      this);
  title->setTextFormat(Qt::RichText);
  layout->addWidget(title);

  m_filterEdit = new QLineEdit(this);
  m_filterEdit->setPlaceholderText(tr("Filter actions, shortcuts, or tips..."));
  layout->addWidget(m_filterEdit);

  m_tree = new QTreeWidget(this);
  m_tree->setHeaderLabels({tr("Action"), tr("Shortcut"), tr("Notes")});
  m_tree->setRootIsDecorated(true);
  m_tree->setAllColumnsShowFocus(true);
  m_tree->setAlternatingRowColors(true);
  m_tree->setIndentation(18);
  m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);

  const auto &palette = NMStyleManager::instance().palette();
  m_tree->setStyleSheet(
      QString("QTreeWidget {"
              "  background-color: %1;"
              "  color: %2;"
              "  border: 1px solid %3;"
              "}"
              "QTreeWidget::item:selected {"
              "  background-color: %4;"
              "}"
              "QHeaderView::section {"
              "  background-color: %5;"
              "  color: %2;"
              "  padding: 4px 6px;"
              "  border: 1px solid %3;"
              "}")
          .arg(palette.bgMedium.name())
          .arg(palette.textPrimary.name())
          .arg(palette.borderDark.name())
          .arg(palette.bgLight.name())
          .arg(palette.bgDark.name()));

  QMap<QString, QTreeWidgetItem *> sectionItems;
  for (const auto &entry : entries) {
    QTreeWidgetItem *sectionItem = sectionItems.value(entry.section, nullptr);
    if (!sectionItem) {
      sectionItem = new QTreeWidgetItem(m_tree);
      sectionItem->setText(0, entry.section);
      sectionItem->setFirstColumnSpanned(true);
      QFont boldFont = sectionItem->font(0);
      boldFont.setBold(true);
      sectionItem->setFont(0, boldFont);
      sectionItem->setExpanded(true);
      sectionItems.insert(entry.section, sectionItem);
    }

    auto *item = new QTreeWidgetItem(sectionItem);
    item->setText(0, entry.action);
    item->setText(1, entry.shortcut);
    item->setText(2, entry.notes);
  }

  layout->addWidget(m_tree, 1);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  connect(m_filterEdit, &QLineEdit::textChanged, this,
          &NMHotkeysDialog::applyFilter);
}

void NMHotkeysDialog::applyFilter(const QString &text) {
  const QString filter = text.trimmed().toLower();

  for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
    auto *sectionItem = m_tree->topLevelItem(i);
    if (!sectionItem) {
      continue;
    }

    bool anyVisible = false;
    for (int j = 0; j < sectionItem->childCount(); ++j) {
      auto *child = sectionItem->child(j);
      if (!child) {
        continue;
      }
      const QString haystack =
          (child->text(0) + " " + child->text(1) + " " + child->text(2))
              .toLower();
      const bool match = filter.isEmpty() || haystack.contains(filter);
      child->setHidden(!match);
      anyVisible = anyVisible || match;
    }

    sectionItem->setHidden(!filter.isEmpty() && !anyVisible);
    sectionItem->setExpanded(true);
  }
}

} // namespace NovelMind::editor::qt
