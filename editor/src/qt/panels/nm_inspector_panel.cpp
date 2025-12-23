#include "NovelMind/editor/qt/panels/nm_inspector_panel.hpp"
#include "NovelMind/editor/qt/panels/nm_scene_view_panel.hpp"
#include "NovelMind/editor/qt/panels/nm_story_graph_panel.hpp"
#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"
#include "NovelMind/editor/qt/nm_dialogs.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDropEvent>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QDir>


namespace NovelMind::editor::qt {

// ============================================================================
// NMInspectorPanel
// ============================================================================

NMInspectorPanel::NMInspectorPanel(QWidget *parent)
    : NMDockPanel(tr("Inspector"), parent) {
  setPanelId("Inspector");
  setupContent();
}

NMInspectorPanel::~NMInspectorPanel() = default;

void NMInspectorPanel::onInitialize() { showNoSelection(); }

void NMInspectorPanel::onUpdate(double /*deltaTime*/) {
  // No continuous update needed
}

void NMInspectorPanel::clear() {
  // Remove all groups
  for (auto *group : m_groups) {
    m_mainLayout->removeWidget(group);
    delete group;
  }
  m_groups.clear();
  m_propertyWidgets.clear();

  m_headerLabel->clear();
}

void NMInspectorPanel::inspectObject(const QString &objectType,
                                     const QString &objectId, bool editable) {
  clear();
  m_noSelectionLabel->hide();
  m_currentObjectId = objectId;
  m_editMode = editable;

  // Set header
  m_headerLabel->setText(
      QString("<b>%1</b><br><span style='color: gray;'>%2</span>")
          .arg(objectType)
          .arg(objectId));
  m_headerLabel->show();

  // Add demo properties based on type
  auto *transformGroup = addGroup(tr("Transform"));

  if (m_editMode) {
    transformGroup->addEditableProperty(tr("Position X"), NMPropertyType::Float,
                                        "0.0");
    transformGroup->addEditableProperty(tr("Position Y"), NMPropertyType::Float,
                                        "0.0");
    transformGroup->addEditableProperty(tr("Rotation"), NMPropertyType::Float,
                                        "0.0");
    transformGroup->addEditableProperty(tr("Scale X"), NMPropertyType::Float,
                                        "1.0");
    transformGroup->addEditableProperty(tr("Scale Y"), NMPropertyType::Float,
                                        "1.0");
  } else {
    transformGroup->addProperty(tr("Position X"), "0.0");
    transformGroup->addProperty(tr("Position Y"), "0.0");
    transformGroup->addProperty(tr("Rotation"), "0.0");
    transformGroup->addProperty(tr("Scale X"), "1.0");
    transformGroup->addProperty(tr("Scale Y"), "1.0");
  }

  connect(transformGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  auto *renderGroup = addGroup(tr("Rendering"));

  if (m_editMode) {
    renderGroup->addEditableProperty(tr("Visible"), NMPropertyType::Boolean,
                                     "true");
    renderGroup->addEditableProperty(tr("Alpha"), NMPropertyType::Float, "1.0");
    renderGroup->addEditableProperty(tr("Z-Order"), NMPropertyType::Integer,
                                     "0");
    renderGroup->addEditableProperty(
        tr("Blend Mode"), NMPropertyType::Enum, "Normal",
        {"Normal", "Additive", "Multiply", "Screen", "Overlay"});
    renderGroup->addEditableProperty(tr("Tint Color"), NMPropertyType::Color,
                                     "#FFFFFF");
  } else {
    renderGroup->addProperty(tr("Visible"), "true");
    renderGroup->addProperty(tr("Alpha"), "1.0");
    renderGroup->addProperty(tr("Z-Order"), "0");
  }

  connect(renderGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  if (objectType == "Dialogue" || objectType == "Choice") {
    auto *dialogueGroup = addGroup(tr("Dialogue"));

    if (m_editMode) {
      dialogueGroup->addEditableProperty(tr("Speaker"), NMPropertyType::String,
                                         "Narrator");
      dialogueGroup->addEditableProperty(tr("Text"), NMPropertyType::String,
                                         objectId);
      dialogueGroup->addEditableProperty(tr("Voice Clip"),
                                         NMPropertyType::Asset, "");
    } else {
      dialogueGroup->addProperty(tr("Speaker"), "Narrator");
      dialogueGroup->addProperty(tr("Text"), objectId);
      dialogueGroup->addProperty(tr("Voice Clip"), "(none)");
    }

    connect(dialogueGroup, &NMPropertyGroup::propertyValueChanged, this,
            &NMInspectorPanel::onGroupPropertyChanged);
  }

  // Add spacer at the end
  m_mainLayout->addStretch();
}

void NMInspectorPanel::inspectSceneObject(NMSceneObject *object,
                                          bool editable) {
  if (!object) {
    showNoSelection();
    return;
  }

  clear();
  m_noSelectionLabel->hide();
  m_currentObjectId = object->id();
  m_editMode = editable;

  const QString typeName = [object]() {
    switch (object->objectType()) {
    case NMSceneObjectType::Background:
      return QString("Background");
    case NMSceneObjectType::Character:
      return QString("Character");
    case NMSceneObjectType::UI:
      return QString("UI");
    case NMSceneObjectType::Effect:
      return QString("Effect");
    }
    return QString("Scene Object");
  }();

  m_headerLabel->setText(
      QString("<b>%1</b><br><span style='color: gray;'>%2</span>")
          .arg(typeName)
          .arg(object->id()));
  m_headerLabel->show();

  auto *generalGroup = addGroup(tr("General"));
  generalGroup->addProperty(tr("ID"), object->id());
  if (m_editMode) {
    if (auto *nameEdit = generalGroup->addEditableProperty(
            "name", tr("Name"), NMPropertyType::String, object->name())) {
      trackPropertyWidget("name", nameEdit);
    }
    if (auto *assetEdit = generalGroup->addEditableProperty(
            "asset", tr("Asset"), NMPropertyType::Asset,
            object->assetPath())) {
      trackPropertyWidget("asset", assetEdit);
    }
  } else {
    generalGroup->addProperty(tr("Name"), object->name());
    generalGroup->addProperty(tr("Asset"), object->assetPath());
  }
  connect(generalGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  auto *transformGroup = addGroup(tr("Transform"));
  const QPointF pos = object->pos();
  if (m_editMode) {
    if (auto *xEdit =
            transformGroup->addEditableProperty("position_x", tr("Position X"),
                                                 NMPropertyType::Float,
                                                 QString::number(pos.x()))) {
      trackPropertyWidget("position_x", xEdit);
    }
    if (auto *yEdit =
            transformGroup->addEditableProperty("position_y", tr("Position Y"),
                                                 NMPropertyType::Float,
                                                 QString::number(pos.y()))) {
      trackPropertyWidget("position_y", yEdit);
    }
    if (auto *rotEdit =
            transformGroup->addEditableProperty("rotation", tr("Rotation"),
                                                 NMPropertyType::Float,
                                                 QString::number(object->rotation()))) {
      trackPropertyWidget("rotation", rotEdit);
    }
    if (auto *sxEdit =
            transformGroup->addEditableProperty("scale_x", tr("Scale X"),
                                                 NMPropertyType::Float,
                                                 QString::number(object->scaleX()))) {
      trackPropertyWidget("scale_x", sxEdit);
    }
    if (auto *syEdit =
            transformGroup->addEditableProperty("scale_y", tr("Scale Y"),
                                                 NMPropertyType::Float,
                                                 QString::number(object->scaleY()))) {
      trackPropertyWidget("scale_y", syEdit);
    }
  } else {
    transformGroup->addProperty(tr("Position X"), QString::number(pos.x()));
    transformGroup->addProperty(tr("Position Y"), QString::number(pos.y()));
    transformGroup->addProperty(tr("Rotation"),
                                QString::number(object->rotation()));
    transformGroup->addProperty(tr("Scale X"),
                                QString::number(object->scaleX()));
    transformGroup->addProperty(tr("Scale Y"),
                                QString::number(object->scaleY()));
  }
  connect(transformGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  auto *renderGroup = addGroup(tr("Rendering"));
  if (m_editMode) {
    if (auto *visibleEdit =
            renderGroup->addEditableProperty("visible", tr("Visible"),
                                             NMPropertyType::Boolean,
                                             object->isVisible() ? "true"
                                                                 : "false")) {
      trackPropertyWidget("visible", visibleEdit);
    }
    if (auto *alphaEdit =
            renderGroup->addEditableProperty("alpha", tr("Alpha"),
                                             NMPropertyType::Float,
                                             QString::number(object->opacity()))) {
      trackPropertyWidget("alpha", alphaEdit);
    }
    if (auto *zEdit =
            renderGroup->addEditableProperty("z", tr("Z-Order"),
                                             NMPropertyType::Integer,
                                             QString::number(object->zValue()))) {
      trackPropertyWidget("z", zEdit);
    }
    if (auto *lockEdit =
            renderGroup->addEditableProperty("locked", tr("Locked"),
                                             NMPropertyType::Boolean,
                                             object->isLocked() ? "true"
                                                                : "false")) {
      trackPropertyWidget("locked", lockEdit);
    }
  } else {
    renderGroup->addProperty(tr("Visible"),
                             object->isVisible() ? "true" : "false");
    renderGroup->addProperty(tr("Alpha"), QString::number(object->opacity()));
    renderGroup->addProperty(tr("Z-Order"),
                             QString::number(object->zValue()));
    renderGroup->addProperty(tr("Locked"),
                             object->isLocked() ? "true" : "false");
  }
  connect(renderGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  m_mainLayout->addStretch();
}

void NMInspectorPanel::inspectStoryGraphNode(NMGraphNodeItem *node,
                                             bool editable) {
  if (!node) {
    showNoSelection();
    return;
  }

  clear();
  m_noSelectionLabel->hide();
  m_currentObjectId = node->nodeIdString();
  m_editMode = editable;

  m_headerLabel->setText(
      QString("<b>%1</b><br><span style='color: gray;'>%2</span>")
          .arg(node->nodeType())
          .arg(node->nodeIdString()));
  m_headerLabel->show();

  auto *generalGroup = addGroup(tr("General"));
  generalGroup->addProperty(tr("ID"), node->nodeIdString());
  if (m_editMode) {
    if (auto *titleEdit =
            generalGroup->addEditableProperty("title", tr("Title"),
                                              NMPropertyType::String,
                                              node->title())) {
      trackPropertyWidget("title", titleEdit);
    }
    if (auto *typeEdit =
            generalGroup->addEditableProperty("type", tr("Type"),
                                              NMPropertyType::String,
                                              node->nodeType())) {
      trackPropertyWidget("type", typeEdit);
    }
  } else {
    generalGroup->addProperty(tr("Title"), node->title());
    generalGroup->addProperty(tr("Type"), node->nodeType());
  }
  connect(generalGroup, &NMPropertyGroup::propertyValueChanged, this,
          &NMInspectorPanel::onGroupPropertyChanged);

  if (node->nodeType().contains("Dialogue", Qt::CaseInsensitive) ||
      node->nodeType().contains("Choice", Qt::CaseInsensitive)) {
    auto *contentGroup = addGroup(tr("Content"));
    const QString speakerValue = node->dialogueSpeaker();
    const QString textValue = node->dialogueText();
    const QString choicesValue = node->choiceOptions().join("\n");

    if (m_editMode) {
      if (auto *speakerEdit =
              contentGroup->addEditableProperty("speaker", tr("Speaker"),
                                                NMPropertyType::String,
                                                speakerValue)) {
        trackPropertyWidget("speaker", speakerEdit);
      }
      if (auto *textEdit =
              contentGroup->addEditableProperty("text", tr("Text"),
                                                NMPropertyType::MultiLine,
                                                textValue)) {
        trackPropertyWidget("text", textEdit);
      }
      if (node->nodeType().contains("Choice", Qt::CaseInsensitive)) {
        if (auto *choicesEdit =
                contentGroup->addEditableProperty("choices", tr("Choices"),
                                                  NMPropertyType::MultiLine,
                                                  choicesValue)) {
          trackPropertyWidget("choices", choicesEdit);
        }
      }
    } else {
      contentGroup->addProperty(tr("Speaker"),
                                speakerValue.isEmpty() ? tr("Narrator")
                                                      : speakerValue);
      contentGroup->addProperty(tr("Text"), textValue);
      if (node->nodeType().contains("Choice", Qt::CaseInsensitive)) {
        contentGroup->addProperty(tr("Choices"), choicesValue);
      }
    }
    connect(contentGroup, &NMPropertyGroup::propertyValueChanged, this,
            &NMInspectorPanel::onGroupPropertyChanged);
  }

  if (node->nodeType().contains("Script", Qt::CaseInsensitive)) {
    auto *scriptGroup = addGroup(tr("Script"));
    scriptGroup->addProperty(tr("File"), node->scriptPath());
    if (m_editMode) {
      if (auto *scriptEdit =
              scriptGroup->addEditableProperty("scriptPath", tr("Path"),
                                               NMPropertyType::Asset,
                                               node->scriptPath())) {
        trackPropertyWidget("scriptPath", scriptEdit);
      }
    }
    connect(scriptGroup, &NMPropertyGroup::propertyValueChanged, this,
            &NMInspectorPanel::onGroupPropertyChanged);
  }

  m_mainLayout->addStretch();
}

void NMInspectorPanel::onGroupPropertyChanged(const QString &propertyName,
                                              const QString &newValue) {
  // Emit signal that property was changed
  emit propertyChanged(m_currentObjectId, propertyName, newValue);
}

void NMInspectorPanel::updatePropertyValue(const QString &propertyName,
                                           const QString &newValue) {
  auto it = m_propertyWidgets.find(propertyName);
  if (it == m_propertyWidgets.end()) {
    return;
  }

  QWidget *widget = it.value();
  QSignalBlocker blocker(widget);
  if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
    lineEdit->setText(newValue);
  } else if (auto *spinBox = qobject_cast<QSpinBox *>(widget)) {
    spinBox->setValue(newValue.toInt());
  } else if (auto *doubleSpinBox = qobject_cast<QDoubleSpinBox *>(widget)) {
    doubleSpinBox->setValue(newValue.toDouble());
  } else if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
    checkBox->setChecked(newValue.toLower() == "true" || newValue == "1");
  } else if (auto *comboBox = qobject_cast<QComboBox *>(widget)) {
    comboBox->setCurrentText(newValue);
  } else if (auto *textEdit = qobject_cast<QPlainTextEdit *>(widget)) {
    textEdit->setPlainText(newValue);
  } else if (auto *button = qobject_cast<QPushButton *>(widget)) {
    button->setText(newValue.isEmpty() ? "(Select Asset)" : newValue);
  }
}

void NMInspectorPanel::trackPropertyWidget(const QString &propertyName,
                                           QWidget *widget) {
  if (!propertyName.isEmpty() && widget) {
    m_propertyWidgets.insert(propertyName, widget);
  }
}

NMPropertyGroup *NMInspectorPanel::addGroup(const QString &title) {
  auto *group = new NMPropertyGroup(title, m_scrollContent);
  m_mainLayout->addWidget(group);
  m_groups.append(group);
  return group;
}

void NMInspectorPanel::showNoSelection() {
  clear();
  m_headerLabel->hide();
  m_noSelectionLabel->show();
}

void NMInspectorPanel::setupContent() {
  auto *container = new QWidget(this);
  auto *containerLayout = new QVBoxLayout(container);
  containerLayout->setContentsMargins(0, 0, 0, 0);
  containerLayout->setSpacing(0);

  // Header
  m_headerLabel = new QLabel(container);
  m_headerLabel->setObjectName("InspectorHeader");
  m_headerLabel->setWordWrap(true);
  m_headerLabel->setTextFormat(Qt::RichText);
  m_headerLabel->setMargin(8);
  m_headerLabel->hide();
  containerLayout->addWidget(m_headerLabel);

  // Scroll area
  m_scrollArea = new QScrollArea(container);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);

  m_scrollContent = new QWidget(m_scrollArea);
  m_mainLayout = new QVBoxLayout(m_scrollContent);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_mainLayout->setSpacing(8);
  m_mainLayout->setAlignment(Qt::AlignTop);

  m_scrollArea->setWidget(m_scrollContent);
  containerLayout->addWidget(m_scrollArea, 1);

  // No selection label
  m_noSelectionLabel =
      new QLabel(tr("Select an object to view its properties"), container);
  m_noSelectionLabel->setObjectName("InspectorEmptyState");
  m_noSelectionLabel->setAlignment(Qt::AlignCenter);
  m_noSelectionLabel->setWordWrap(true);

  const auto &palette = NMStyleManager::instance().palette();
  m_noSelectionLabel->setStyleSheet(
      QString("color: %1; padding: 20px;")
          .arg(NMStyleManager::colorToStyleString(palette.textSecondary)));

  m_mainLayout->addWidget(m_noSelectionLabel);

  setContentWidget(container);
}


} // namespace NovelMind::editor::qt
