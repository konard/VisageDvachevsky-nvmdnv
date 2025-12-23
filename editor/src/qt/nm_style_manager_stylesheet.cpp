#include "NovelMind/editor/qt/nm_style_manager.hpp"

namespace NovelMind::editor::qt {

QString NMStyleManager::colorToStyleString(const QColor &color) {
  return QString("rgb(%1, %2, %3)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue());
}

QString NMStyleManager::colorToRgbaString(const QColor &color, int alpha) {
  return QString("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(alpha);
}

QString NMStyleManager::getStyleSheet() const {
  const auto &p = m_palette;

  // Generate comprehensive stylesheet for Unreal-like dark theme
  return QString(R"(
/* ========================================================================== */
/* Global Styles                                                              */
/* ========================================================================== */

* {
    color: %1;
    background-color: %2;
    selection-background-color: %3;
    selection-color: %4;
}

/* ========================================================================== */
/* QMainWindow                                                                */
/* ========================================================================== */

QMainWindow {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
      stop:0 %5, stop:1 %2);
}

QMainWindow::separator {
    background-color: %6;
    width: 3px;
    height: 3px;
}

QMainWindow::separator:hover {
    background-color: %3;
}

/* ========================================================================== */
/* QDockWidget & Tabs                                                         */
/* ========================================================================== */

QDockWidget {
    background: %2;
    titlebar-close-icon: none;
    titlebar-normal-icon: none;
    border: 1px solid %6;
    border-radius: 3px;
}

QDockWidget::title {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
      stop:0 %2, stop:1 %5);
    color: %1;
    padding: 5px 6px;
    border-bottom: 1px solid %6;
    font-weight: 600;
}

QDockWidget::float-button, QDockWidget::close-button {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 2px;
    width: 16px;
    height: 16px;
}

QDockWidget::float-button:hover, QDockWidget::close-button:hover {
    background: %7;
    border-color: %6;
}

QTabBar::tab {
    background: %2;
    color: %1;
    padding: 4px 8px;
    margin-right: 1px;
    border: 1px solid %6;
    border-bottom: 2px solid %6;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
}

QTabBar::tab:selected {
    background: %5;
    border-color: %3;
    border-bottom: 2px solid %3;
    font-weight: 600;
}

QTabBar::tab:hover {
    background: %7;
}

QTabWidget::pane {
    border: 1px solid %6;
    top: -1px;
}

QSplitter::handle {
    background: %6;
}

QSplitter::handle:hover {
    background: %3;
}

/* ========================================================================== */
/* QMenuBar                                                                   */
/* ========================================================================== */

QMenuBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
      stop:0 %2, stop:1 %5);
    border-bottom: 1px solid %6;
    padding: 1px;
}

QMenuBar::item {
    background-color: transparent;
    padding: 3px 6px;
    border-radius: 2px;
}

QMenuBar::item:selected {
    background-color: %7;
}

QMenuBar::item:pressed {
    background-color: %3;
}

/* ========================================================================== */
/* QMenu                                                                      */
/* ========================================================================== */

QMenu {
    background-color: %2;
    border: 1px solid %6;
    padding: 3px;
}

QMenu::item {
    padding: 4px 18px 4px 6px;
    border-radius: 2px;
}

QMenu::item:selected {
    background-color: %7;
}

QMenu::item:disabled {
    color: %8;
}

QMenu::separator {
    height: 1px;
    background-color: %6;
    margin: 3px 6px;
}

QMenu::indicator {
    width: 16px;
    height: 16px;
    margin-left: 4px;
}

/* ========================================================================== */
/* QToolBar                                                                   */
/* ========================================================================== */

QToolBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
      stop:0 %2, stop:1 %5);
    border: none;
    border-bottom: 1px solid %6;
    padding: 2px;
    spacing: 2px;
}

QToolBar::separator {
    background-color: %6;
    width: 1px;
    margin: 4px 2px;
}

QToolButton {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 3px;
    padding: 3px 5px;
}

QToolButton:hover {
    background-color: %7;
    border-color: %6;
}

QToolButton:pressed {
    background-color: %9;
}

QToolButton:checked {
    background-color: %3;
    border-color: %3;
}

/* ========================================================================== */
/* QDockWidget                                                                */
/* ========================================================================== */

QDockWidget {
    titlebar-close-icon: url(:/icons/close.svg);
    titlebar-normal-icon: url(:/icons/float.svg);
}

QDockWidget::title {
    background-color: %5;
    padding: 5px 6px;
    border-bottom: 1px solid %6;
    border-left: 3px solid transparent;
    text-align: left;
    font-weight: 600;
}

QDockWidget::close-button,
QDockWidget::float-button {
    background-color: transparent;
    border: none;
    padding: 2px;
}

QDockWidget::close-button:hover,
QDockWidget::float-button:hover {
    background-color: %7;
}

/* ========================================================================== */
/* QTabWidget / QTabBar                                                       */
/* ========================================================================== */

QTabWidget::pane {
    border: 1px solid %6;
    background-color: %5;
}

QTabBar::tab {
    background-color: %2;
    border: 1px solid %6;
    border-bottom: 2px solid transparent;
    padding: 4px 8px;
    margin-right: 1px;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
}

QTabBar::tab:selected {
    background-color: %5;
    border-color: %3;
    border-bottom: 2px solid %3;
    color: %1;
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    background-color: %7;
}

/* ========================================================================== */
/* QPushButton                                                                */
/* ========================================================================== */

QPushButton {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 4px;
    padding: 4px 10px;
    min-width: 64px;
    font-weight: 500;
}

QPushButton:hover {
    background-color: %7;
    border-color: %3;
}

QPushButton:pressed {
    background-color: %9;
}

QPushButton:disabled {
    background-color: %5;
    color: %8;
}

QPushButton:default {
    border-color: %3;
}

/* ========================================================================== */
/* QLineEdit                                                                  */
/* ========================================================================== */

QLineEdit {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 3px;
    padding: 3px 6px;
    selection-background-color: %3;
}

QLineEdit:focus {
    border-color: %3;
}

QLineEdit:disabled {
    background-color: %2;
    color: %8;
}

/* ========================================================================== */
/* QTextEdit / QPlainTextEdit                                                 */
/* ========================================================================== */

QTextEdit, QPlainTextEdit {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 3px;
    selection-background-color: %3;
}

QTextEdit:focus, QPlainTextEdit:focus {
    border-color: %3;
}

/* ========================================================================== */
/* QTreeView / QListView / QTableView                                         */
/* ========================================================================== */

QTreeView, QListView, QTableView {
    background-color: %5;
    border: 1px solid %6;
    alternate-background-color: %10;
    selection-background-color: %3;
}

QTreeView::item, QListView::item, QTableView::item {
    padding: 3px;
}

QTreeView::item:hover, QListView::item:hover, QTableView::item:hover {
    background-color: %7;
}

QTreeView::item:selected, QListView::item:selected, QTableView::item:selected {
    background-color: %3;
}

QTreeView::item:selected {
    border-left: 3px solid %12;
}

QListView::item {
    background-color: %2;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 4px;
}

QListView::item:selected {
    background-color: %7;
    border-color: %3;
}

QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    border-image: none;
    image: url(:/icons/branch-closed.svg);
}

QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    border-image: none;
    image: url(:/icons/branch-open.svg);
}

QHeaderView::section {
    background-color: %2;
    border: none;
    border-right: 1px solid %6;
    border-bottom: 1px solid %6;
    padding: 4px 8px;
    font-weight: 600;
}

QHeaderView::section:hover {
    background-color: %7;
}

/* ========================================================================== */
/* QScrollBar                                                                 */
/* ========================================================================== */

QScrollBar:vertical {
    background-color: %5;
    width: 10px;
    margin: 0;
}

QScrollBar:horizontal {
    background-color: %5;
    height: 10px;
    margin: 0;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background-color: %6;
    border-radius: 3px;
    min-height: 24px;
    min-width: 24px;
    margin: 1px;
}

QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
    background-color: %11;
}

QScrollBar::add-line, QScrollBar::sub-line {
    height: 0;
    width: 0;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}

/* ========================================================================== */
/* QSplitter                                                                  */
/* ========================================================================== */

QSplitter::handle {
    background-color: %6;
}

QSplitter::handle:hover {
    background-color: %3;
}

QSplitter::handle:horizontal {
    width: 2px;
}

QSplitter::handle:vertical {
    height: 2px;
}

/* ========================================================================== */
/* QComboBox                                                                  */
/* ========================================================================== */

QComboBox {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 3px;
    padding: 3px 6px;
    min-width: 90px;
}

QComboBox:hover {
    border-color: %3;
}

QComboBox::drop-down {
    border: none;
    width: 18px;
}

QComboBox::down-arrow {
    image: url(:/icons/dropdown.svg);
    width: 12px;
    height: 12px;
}

QComboBox QAbstractItemView {
    background-color: %2;
    border: 1px solid %6;
    selection-background-color: %3;
}

/* ========================================================================== */
/* QSpinBox / QDoubleSpinBox                                                  */
/* ========================================================================== */

QSpinBox, QDoubleSpinBox {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 3px;
    padding: 3px;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: %3;
}

QSpinBox::up-button, QDoubleSpinBox::up-button {
    border: none;
    background-color: transparent;
    width: 16px;
}

QSpinBox::down-button, QDoubleSpinBox::down-button {
    border: none;
    background-color: transparent;
    width: 16px;
}

/* ========================================================================== */
/* QSlider                                                                    */
/* ========================================================================== */

QSlider::groove:horizontal {
    height: 4px;
    background-color: %6;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background-color: %3;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}

QSlider::handle:horizontal:hover {
    background-color: %12;
}

/* ========================================================================== */
/* QProgressBar                                                               */
/* ========================================================================== */

QProgressBar {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 3px;
    text-align: center;
}

QProgressBar::chunk {
    background-color: %3;
    border-radius: 2px;
}

/* ========================================================================== */
/* QGroupBox                                                                  */
/* ========================================================================== */

QGroupBox {
    border: 1px solid %6;
    border-radius: 3px;
    margin-top: 6px;
    padding-top: 6px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 4px;
    color: %13;
}

/* ========================================================================== */
/* QToolTip                                                                   */
/* ========================================================================== */

QToolTip {
    background-color: %2;
    border: 1px solid %6;
    color: %1;
    padding: 3px 6px;
}

/* ========================================================================== */
/* QStatusBar                                                                 */
/* ========================================================================== */

QStatusBar {
    background-color: %2;
    border-top: 1px solid %6;
}

QStatusBar QLabel#StatusPlay[mode="playing"] {
    color: %15;
    font-weight: 600;
}

QStatusBar QLabel#StatusPlay[mode="paused"] {
    color: %14;
    font-weight: 600;
}

QStatusBar QLabel#StatusPlay[mode="stopped"] {
    color: %11;
}

QStatusBar QLabel#StatusUnsaved[status="dirty"] {
    color: %14;
    font-weight: 600;
}

QStatusBar QLabel#StatusUnsaved[status="clean"] {
    color: %15;
}

QStatusBar::item {
    border: none;
}

/* ========================================================================== */
/* Focus & Accent                                                             */
/* ========================================================================== */

QDockWidget[focusedDock="true"] {
    border: 1px solid %3;
    border-left: 4px solid %3;
}

QDockWidget[focusedDock="true"]::title {
    background-color: %10;
    border-left: 4px solid %3;
}

QDockWidget#SceneViewPanel::title {
    border-left: 4px solid #2ec4b6;
}

QDockWidget#StoryGraphPanel::title {
    border-left: 4px solid #6aa6ff;
}

QDockWidget#InspectorPanel::title {
    border-left: 4px solid #f0b24a;
}

QDockWidget#AssetBrowserPanel::title {
    border-left: 4px solid #5fd18a;
}

QDockWidget#ScriptEditorPanel::title {
    border-left: 4px solid #ff9b66;
}

QDockWidget#ConsolePanel::title {
    border-left: 4px solid #8ea1b5;
}

QDockWidget#PlayToolbarPanel::title {
    border-left: 4px solid #48c76e;
}

QToolBar#MainToolBar {
    background-color: %5;
    border-bottom: 1px solid %6;
    padding: 3px 4px;
}

QToolBar#MainToolBar QToolButton {
    background-color: %2;
    border: 1px solid %6;
    border-radius: 4px;
    padding: 3px 6px;
}

QToolBar#MainToolBar QToolButton:hover {
    background-color: %7;
    border-color: %3;
}

QToolBar#SceneViewToolBar,
QToolBar#StoryGraphToolBar,
QToolBar#AssetBrowserToolBar,
QToolBar#HierarchyToolBar,
QToolBar#ConsoleToolBar,
QToolBar#DebugOverlayToolBar {
    background-color: %5;
    border-bottom: 1px solid %6;
}

QToolBar QLineEdit {
    background-color: %10;
    border-radius: 8px;
    padding: 3px 8px;
}

QToolBar#PlayControlBar {
    background-color: %5;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 3px;
}

QToolBar#PlayControlBar QPushButton {
    background-color: %2;
    border: 1px solid %6;
    border-radius: 4px;
    padding: 3px 8px;
}

QPushButton#PlayButton,
QPushButton#PauseButton,
QPushButton#StopButton,
QPushButton#StepButton {
    font-weight: 600;
    color: %1;
}

QPushButton#PlayButton {
    background-color: #48c76e;
    border-color: #3aa65c;
}

QPushButton#PlayButton:hover {
    background-color: #5bdc7f;
}

QPushButton#PauseButton {
    background-color: #f0b24a;
    border-color: #d79a39;
}

QPushButton#PauseButton:hover {
    background-color: #f5c26a;
}

QPushButton#StopButton {
    background-color: #e3564a;
    border-color: #c9473e;
}

QPushButton#StopButton:hover {
    background-color: #ef6c62;
}

QPushButton#StepButton {
    background-color: #4f9df7;
    border-color: #3c7cc6;
}

QPushButton#StepButton:hover {
    background-color: #66b0ff;
}

QPushButton#PrimaryActionButton,
QPushButton#NMPrimaryButton {
    background-color: %3;
    border-color: %3;
    color: %1;
    font-weight: 600;
}

QPushButton#PrimaryActionButton:hover,
QPushButton#NMPrimaryButton:hover {
    background-color: %12;
    border-color: %12;
}

QPushButton#SecondaryActionButton,
QPushButton#NMSecondaryButton {
    background-color: %10;
    border-color: %6;
}

QLabel#SectionTitle {
    color: %11;
    font-weight: 600;
    padding: 3px 0;
}

QLabel#SceneViewFontWarning {
    background-color: %5;
    border: 1px solid %6;
    color: %14;
    padding: 6px 10px;
    border-radius: 6px;
}

NMPropertyGroup {
    background-color: %5;
    border: 1px solid %6;
    border-radius: 6px;
}

NMPropertyGroup #InspectorGroupHeader {
    background-color: %2;
    border-bottom: 1px solid %6;
    padding: 4px 6px;
}

NMPropertyGroup #InspectorGroupHeader:hover {
    background-color: %7;
}

NMPropertyGroup #InspectorGroupContent {
    background-color: %5;
}

QLabel#InspectorHeader {
    background-color: %2;
    border-bottom: 1px solid %6;
    padding: 6px;
}

QLabel#InspectorEmptyState {
    color: %11;
}

QFrame#AssetPreviewFrame {
    background-color: %5;
    border: 1px solid %6;
    border-radius: 6px;
}

QFrame#ScenePaletteDropArea {
    background-color: %5;
    border: 1px dashed %6;
    border-radius: 6px;
}

QDockWidget#ScenePalettePanel QToolButton {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 6px;
}

QDockWidget#ScenePalettePanel QToolButton:hover {
    background-color: %7;
    border-color: %3;
}

/* ========================================================================== */
/* Command Palette                                                           */
/* ========================================================================== */

QDialog#CommandPalette {
    background-color: %2;
    border: 1px solid %6;
    border-radius: 8px;
}

QLineEdit#CommandPaletteInput {
    background-color: %10;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 4px 8px;
}

QListWidget#CommandPaletteList {
    background-color: %5;
    border: 1px solid %6;
    border-radius: 6px;
}

QListWidget#CommandPaletteList::item {
    padding: 4px 6px;
    border-radius: 4px;
}

QListWidget#CommandPaletteList::item:selected {
    background-color: %3;
}

)")
      .arg(colorToStyleString(p.textPrimary))    // %1
      .arg(colorToStyleString(p.bgDark))         // %2
      .arg(colorToStyleString(p.accentPrimary))  // %3
      .arg(colorToStyleString(p.textPrimary))    // %4
      .arg(colorToStyleString(p.bgDarkest))      // %5
      .arg(colorToStyleString(p.borderLight))    // %6
      .arg(colorToStyleString(p.bgLight))        // %7
      .arg(colorToStyleString(p.textDisabled))   // %8
      .arg(colorToStyleString(p.accentActive))   // %9
      .arg(colorToStyleString(p.bgMedium))       // %10
      .arg(colorToStyleString(p.textSecondary))  // %11
      .arg(colorToStyleString(p.accentHover))    // %12
      .arg(colorToStyleString(p.textSecondary))  // %13
      .arg(colorToStyleString(p.warning))        // %14
      .arg(colorToStyleString(p.success));       // %15
}

} // namespace NovelMind::editor::qt
