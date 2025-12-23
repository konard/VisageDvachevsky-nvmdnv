/**
 * @file ui_controls.cpp
 * @brief Basic UI control implementations
 */

#include "NovelMind/ui/ui_framework.hpp"

#include <algorithm>
#include <cmath>

namespace NovelMind::ui {

// ============================================================================
// Label Implementation
// ============================================================================

Label::Label(const std::string &text, const std::string &id)
    : Widget(id), m_text(text) {}

void Label::setText(const std::string &text) { m_text = text; }

void Label::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  Widget::render(renderer);

  renderer::Color textColor = m_style.textColor;
  textColor.a = static_cast<u8>(textColor.a * m_style.opacity);

  // Text rendering is handled when a TextRenderer is available.
  // The textRenderer.drawText() call uses the computed textColor.
  // Font/text renderer integration is managed by the application layer.
  (void)textColor; // Reserved for text renderer integration
}

Rect Label::measure(f32 /*availableWidth*/, f32 /*availableHeight*/) {
  // Simplified text measurement
  f32 charWidth = m_style.fontSize * 0.6f;
  f32 width = static_cast<f32>(m_text.length()) * charWidth +
              m_style.padding.left + m_style.padding.right;
  f32 height = m_style.fontSize + m_style.padding.top + m_style.padding.bottom;

  return {0, 0, width, height};
}

// ============================================================================
// Button Implementation
// ============================================================================

Button::Button(const std::string &text, const std::string &id)
    : Widget(id), m_text(text) {
  m_focusable = true;
}

void Button::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  Widget::render(renderer);

  renderer::Color textColor = m_style.textColor;
  if (!m_enabled) {
    textColor.a /= 2;
  }
  textColor.a = static_cast<u8>(textColor.a * m_style.opacity);

  // Center text in button
  f32 charWidth = m_style.fontSize * 0.6f;
  f32 textWidth = static_cast<f32>(m_text.length()) * charWidth;
  f32 textHeight = m_style.fontSize;

  f32 textX = m_bounds.x + (m_bounds.width - textWidth) / 2.0f;
  f32 textY = m_bounds.y + (m_bounds.height - textHeight) / 2.0f;

  // Text rendering is handled when a TextRenderer is available.
  // Center coordinates are pre-calculated for text placement.
  (void)textX;     // Reserved for text renderer
  (void)textY;     // Reserved for text renderer
  (void)textColor; // Reserved for text renderer
}

bool Button::handleEvent(UIEvent &event) {
  Widget::handleEvent(event);

  if (event.type == UIEventType::Click && m_enabled && m_onClick) {
    m_onClick();
    event.consume();
    return true;
  }

  return false;
}

Rect Button::measure(f32 /*availableWidth*/, f32 /*availableHeight*/) {
  f32 charWidth = m_style.fontSize * 0.6f;
  f32 width = static_cast<f32>(m_text.length()) * charWidth +
              m_style.padding.left + m_style.padding.right;
  f32 height = m_style.fontSize + m_style.padding.top + m_style.padding.bottom;

  width = std::max(width, m_constraints.minWidth);
  height = std::max(height, m_constraints.minHeight);

  return {0, 0, width, height};
}

// ============================================================================
// TextInput Implementation
// ============================================================================

TextInput::TextInput(const std::string &id) : Widget(id) { m_focusable = true; }

void TextInput::setText(const std::string &text) {
  m_text = text.substr(0, m_maxLength);
  m_cursorPos = m_text.length();
}

void TextInput::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  Widget::render(renderer);

  std::string displayText = m_text;
  if (m_password) {
    displayText = std::string(m_text.length(), '*');
  }

  renderer::Color textColor = m_style.textColor;
  if (m_text.empty() && !m_placeholder.empty()) {
    displayText = m_placeholder;
    textColor.a /= 2;
  }

  textColor.a = static_cast<u8>(textColor.a * m_style.opacity);

  // Text rendering is handled when a TextRenderer is available.
  // Display text accounts for password masking and placeholder state.
  (void)displayText; // Reserved for text renderer
  (void)textColor;   // Reserved for text renderer

  // Draw cursor
  if (m_focused) {
    f32 cursorX = m_bounds.x + m_style.padding.left +
                  static_cast<f32>(m_cursorPos) * m_style.fontSize * 0.6f -
                  m_scrollOffset;
    if (static_cast<i32>(m_cursorBlink * 2) % 2 == 0) {
      renderer::Color cursorColor{255, 255, 255, 255};
      renderer::Rect cursorRect{cursorX, m_bounds.y + m_style.padding.top, 2.0f,
                                m_style.fontSize};
      renderer.fillRect(cursorRect, cursorColor);
    }
  }
}

bool TextInput::handleEvent(UIEvent &event) {
  Widget::handleEvent(event);

  if (!m_enabled) {
    return false;
  }

  if (event.type == UIEventType::Click) {
    requestFocus();
    event.consume();
    return true;
  }

  if (m_focused) {
    if (event.type == UIEventType::KeyPress) {
      if (event.character >= 32 && m_text.length() < m_maxLength) {
        m_text.insert(m_cursorPos, 1, event.character);
        ++m_cursorPos;
        if (m_onChange) {
          m_onChange(m_text);
        }
        event.consume();
        return true;
      }
    } else if (event.type == UIEventType::KeyDown) {
      // Handle special keys
      if (event.keyCode == 8) // Backspace
      {
        if (m_cursorPos > 0) {
          m_text.erase(m_cursorPos - 1, 1);
          --m_cursorPos;
          if (m_onChange) {
            m_onChange(m_text);
          }
        }
        event.consume();
        return true;
      } else if (event.keyCode == 127) // Delete
      {
        if (m_cursorPos < m_text.length()) {
          m_text.erase(m_cursorPos, 1);
          if (m_onChange) {
            m_onChange(m_text);
          }
        }
        event.consume();
        return true;
      } else if (event.keyCode == 13) // Enter
      {
        if (m_onSubmit) {
          m_onSubmit(m_text);
        }
        event.consume();
        return true;
      } else if (event.keyCode == 37) // Left arrow
      {
        if (m_cursorPos > 0) {
          --m_cursorPos;
        }
        event.consume();
        return true;
      } else if (event.keyCode == 39) // Right arrow
      {
        if (m_cursorPos < m_text.length()) {
          ++m_cursorPos;
        }
        event.consume();
        return true;
      }
    }
  }

  return false;
}

Rect TextInput::measure(f32 /*availableWidth*/, f32 /*availableHeight*/) {
  f32 width = 200.0f + m_style.padding.left + m_style.padding.right;
  f32 height = m_style.fontSize + m_style.padding.top + m_style.padding.bottom;

  width = std::max(width, m_constraints.minWidth);
  height = std::max(height, m_constraints.minHeight);

  return {0, 0, width, height};
}

// ============================================================================
// Checkbox Implementation
// ============================================================================

Checkbox::Checkbox(const std::string &label, const std::string &id)
    : Widget(id), m_label(label) {
  m_focusable = true;
}

void Checkbox::setChecked(bool checked) {
  if (m_checked != checked) {
    m_checked = checked;
    if (m_onChange) {
      m_onChange(m_checked);
    }
  }
}

void Checkbox::toggle() { setChecked(!m_checked); }

void Checkbox::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  Widget::render(renderer);

  f32 boxSize = m_style.fontSize;
  f32 boxX = m_bounds.x + m_style.padding.left;
  f32 boxY = m_bounds.y + (m_bounds.height - boxSize) / 2.0f;

  // Draw checkbox box
  renderer::Color boxColor =
      m_checked ? m_style.accentColor : m_style.backgroundColor;
  renderer::Rect boxRect{boxX, boxY, boxSize, boxSize};
  renderer.fillRect(boxRect, boxColor);

  // Draw border
  renderer::Color borderColor =
      m_hovered ? m_style.accentColor : m_style.borderColor;
  // Border drawing would go here
  (void)borderColor;

  // Draw checkmark if checked
  if (m_checked) {
    renderer::Color checkColor{255, 255, 255, 255};
    // Checkmark drawing would go here
    (void)checkColor;
  }

  // Draw label
  if (!m_label.empty()) {
    renderer::Color textColor = m_style.textColor;
    textColor.a = static_cast<u8>(textColor.a * m_style.opacity);
    // Text rendering is handled when a TextRenderer is available.
    // Label is positioned to the right of the checkbox.
    (void)textColor; // Reserved for text renderer
  }
}

bool Checkbox::handleEvent(UIEvent &event) {
  Widget::handleEvent(event);

  if (event.type == UIEventType::Click && m_enabled) {
    toggle();
    event.consume();
    return true;
  }

  return false;
}

Rect Checkbox::measure(f32 /*availableWidth*/, f32 /*availableHeight*/) {
  f32 boxSize = m_style.fontSize;
  f32 charWidth = m_style.fontSize * 0.6f;
  f32 labelWidth = static_cast<f32>(m_label.length()) * charWidth;

  f32 width = boxSize + 8.0f + labelWidth + m_style.padding.left +
              m_style.padding.right;
  f32 height = std::max(boxSize, m_style.fontSize) + m_style.padding.top +
               m_style.padding.bottom;

  return {0, 0, width, height};
}

// ============================================================================
// Slider Implementation
// ============================================================================

Slider::Slider(const std::string &id) : Widget(id) { m_focusable = true; }

void Slider::setValue(f32 value) {
  f32 newValue = std::max(m_min, std::min(m_max, value));
  if (m_step > 0.0f) {
    newValue = std::round((newValue - m_min) / m_step) * m_step + m_min;
  }

  if (m_value != newValue) {
    m_value = newValue;
    if (m_onChange) {
      m_onChange(m_value);
    }
  }
}

void Slider::setRange(f32 min, f32 max) {
  m_min = min;
  m_max = max;
  setValue(m_value); // Clamp current value
}

void Slider::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  Widget::render(renderer);

  f32 trackHeight = 4.0f;
  f32 trackY = m_bounds.y + (m_bounds.height - trackHeight) / 2.0f;
  f32 trackWidth =
      m_bounds.width - m_style.padding.left - m_style.padding.right;
  f32 trackX = m_bounds.x + m_style.padding.left;

  // Draw track
  renderer::Color trackColor = m_style.backgroundColor;
  renderer::Rect trackRect{trackX, trackY, trackWidth, trackHeight};
  renderer.fillRect(trackRect, trackColor);

  // Draw filled portion
  f32 progress = (m_max > m_min) ? (m_value - m_min) / (m_max - m_min) : 0.0f;
  renderer::Color fillColor = m_style.accentColor;
  renderer::Rect fillRect{trackX, trackY, trackWidth * progress, trackHeight};
  renderer.fillRect(fillRect, fillColor);

  // Draw thumb
  f32 thumbSize = 16.0f;
  f32 thumbX = trackX + trackWidth * progress - thumbSize / 2.0f;
  f32 thumbY = m_bounds.y + (m_bounds.height - thumbSize) / 2.0f;

  renderer::Color thumbColor =
      m_dragging || m_hovered ? m_style.hoverColor : m_style.foregroundColor;
  renderer::Rect thumbRect{thumbX, thumbY, thumbSize, thumbSize};
  renderer.fillRect(thumbRect, thumbColor);
}

bool Slider::handleEvent(UIEvent &event) {
  Widget::handleEvent(event);

  if (!m_enabled) {
    return false;
  }

  if (event.type == UIEventType::MouseDown) {
    m_dragging = true;
    // Calculate value from mouse position
    f32 trackWidth =
        m_bounds.width - m_style.padding.left - m_style.padding.right;
    f32 trackX = m_bounds.x + m_style.padding.left;
    f32 progress = (event.mouseX - trackX) / trackWidth;
    setValue(m_min + progress * (m_max - m_min));
    event.consume();
    return true;
  } else if (event.type == UIEventType::MouseMove && m_dragging) {
    f32 trackWidth =
        m_bounds.width - m_style.padding.left - m_style.padding.right;
    f32 trackX = m_bounds.x + m_style.padding.left;
    f32 progress = (event.mouseX - trackX) / trackWidth;
    setValue(m_min + progress * (m_max - m_min));
    event.consume();
    return true;
  } else if (event.type == UIEventType::MouseUp) {
    m_dragging = false;
    event.consume();
    return true;
  }

  return false;
}

Rect Slider::measure(f32 /*availableWidth*/, f32 /*availableHeight*/) {
  f32 width = 200.0f + m_style.padding.left + m_style.padding.right;
  f32 height = 24.0f + m_style.padding.top + m_style.padding.bottom;

  width = std::max(width, m_constraints.minWidth);
  height = std::max(height, m_constraints.minHeight);

  return {0, 0, width, height};
}

} // namespace NovelMind::ui
