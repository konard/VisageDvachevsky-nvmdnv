#pragma once

/**
 * @file nm_style_manager.hpp
 * @brief Style management for NovelMind Editor
 *
 * Provides Unreal Engine-like dark theme styling using Qt Style Sheets (QSS).
 * Manages:
 * - Application-wide dark theme
 * - High-DPI scaling
 * - Custom color palette
 * - Consistent widget styling
 */

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>

namespace NovelMind::editor::qt {

/**
 * @brief Color palette for the editor theme
 */
struct EditorPalette {
  // Background colors
  QColor bgDarkest{0x0f, 0x12, 0x16}; // Main background
  QColor bgDark{0x16, 0x1b, 0x22};    // Panel backgrounds
  QColor bgMedium{0x1f, 0x26, 0x30};  // Widget backgrounds
  QColor bgLight{0x2a, 0x32, 0x3f};   // Hover states

  // Text colors
  QColor textPrimary{0xe7, 0xec, 0xf2};   // Primary text
  QColor textSecondary{0xa3, 0xae, 0xbd}; // Secondary text
  QColor textDisabled{0x6b, 0x74, 0x82};  // Disabled text

  // Accent colors
  QColor accentPrimary{0x2f, 0x9b, 0xff}; // Selection, focus
  QColor accentHover{0x59, 0xb6, 0xff};   // Hover state
  QColor accentActive{0x22, 0x7d, 0xd6};  // Active state

  // Status colors
  QColor error{0xe1, 0x4e, 0x43};
  QColor warning{0xf2, 0xa2, 0x3a};
  QColor success{0x35, 0xc0, 0x7f};
  QColor info{0x4a, 0x92, 0xff};

  // Border colors
  QColor borderDark{0x0c, 0x10, 0x14};
  QColor borderLight{0x35, 0x3d, 0x49};

  // Graph/Node specific colors
  QColor nodeDefault{0x2a, 0x2f, 0x37};
  QColor nodeSelected{0x2d, 0x7c, 0xcf};
  QColor nodeHover{0x34, 0x3a, 0x44};
  QColor connectionLine{0x66, 0x71, 0x7f};
  QColor gridLine{0x23, 0x28, 0x31};
  QColor gridMajor{0x2f, 0x35, 0x3f};
};

/**
 * @brief Manages the editor's visual style and theme
 */
class NMStyleManager : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Get the singleton instance
   */
  static NMStyleManager &instance();

  /**
   * @brief Initialize the style manager and apply the default theme
   * @param app The QApplication instance
   */
  void initialize(QApplication *app);

  /**
   * @brief Apply the dark theme to the application
   */
  void applyDarkTheme();

  /**
   * @brief Get the current color palette
   */
  [[nodiscard]] const EditorPalette &palette() const { return m_palette; }

  /**
   * @brief Get the default font for the editor
   */
  [[nodiscard]] QFont defaultFont() const { return m_defaultFont; }

  /**
   * @brief Get the monospace font (for code/console)
   */
  [[nodiscard]] QFont monospaceFont() const { return m_monospaceFont; }

  /**
   * @brief Get the icon size for toolbars
   */
  [[nodiscard]] int toolbarIconSize() const { return m_toolbarIconSize; }

  /**
   * @brief Get the icon size for menus
   */
  [[nodiscard]] int menuIconSize() const { return m_menuIconSize; }

  /**
   * @brief Set the UI scale factor (for high-DPI support)
   * @param scale Scale factor (1.0 = 100%, 1.5 = 150%, etc.)
   */
  void setUiScale(double scale);

  /**
   * @brief Get the current UI scale factor
   */
  [[nodiscard]] double uiScale() const { return m_uiScale; }

  /**
   * @brief Get the complete stylesheet for the application
   */
  [[nodiscard]] QString getStyleSheet() const;

  /**
   * @brief Convert a color to a CSS-compatible string
   */
  static QString colorToStyleString(const QColor &color);

  /**
   * @brief Convert a color with alpha to a CSS rgba string
   */
  static QString colorToRgbaString(const QColor &color, int alpha = 255);

signals:
  /**
   * @brief Emitted when the theme changes
   */
  void themeChanged();

  /**
   * @brief Emitted when the UI scale changes
   */
  void scaleChanged(double newScale);

private:
  NMStyleManager();
  ~NMStyleManager() override = default;

  // Prevent copying
  NMStyleManager(const NMStyleManager &) = delete;
  NMStyleManager &operator=(const NMStyleManager &) = delete;

  void setupFonts();
  void setupHighDpi();

  QApplication *m_app = nullptr;
  EditorPalette m_palette;
  QFont m_defaultFont;
  QFont m_monospaceFont;
  double m_uiScale = 1.0;
  int m_toolbarIconSize = 24;
  int m_menuIconSize = 16;
};

} // namespace NovelMind::editor::qt
