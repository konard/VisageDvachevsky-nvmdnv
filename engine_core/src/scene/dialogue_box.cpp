#include "NovelMind/scene/dialogue_box.hpp"
#include <algorithm>

namespace NovelMind::Scene {

DialogueBox::DialogueBox(const std::string &id)
    : scene::SceneObject(id), m_style(), m_bounds{0.0f, 0.0f, 800.0f, 200.0f},
      m_speakerName(), m_speakerColor(renderer::Color::White), m_text(),
      m_visibleCharacters(0), m_typewriterTimer(0.0f),
      m_typewriterComplete(true), m_showWaitIndicator(false),
      m_waitIndicatorTimer(0.0f), m_waitIndicatorVisible(false),
      m_autoAdvance(false), m_autoAdvanceDelay(2.0f), m_autoAdvanceTimer(0.0f),
      m_onComplete(nullptr) {}

DialogueBox::~DialogueBox() = default;

void DialogueBox::setStyle(const DialogueBoxStyle &style) { m_style = style; }

const DialogueBoxStyle &DialogueBox::getStyle() const { return m_style; }

void DialogueBox::setBounds(f32 x, f32 y, f32 width, f32 height) {
  m_bounds = renderer::Rect{x, y, width, height};
}

renderer::Rect DialogueBox::getBounds() const { return m_bounds; }

void DialogueBox::setSpeakerName(const std::string &name) {
  m_speakerName = name;
}

void DialogueBox::setSpeakerColor(const renderer::Color &color) {
  m_speakerColor = color;
}

void DialogueBox::setText(const std::string &text, bool immediate) {
  m_text = text;
  m_waitIndicatorVisible = false;
  m_autoAdvanceTimer = 0.0f;

  if (immediate || m_style.typewriterSpeed <= 0.0f) {
    m_visibleCharacters = m_text.size();
    m_typewriterComplete = true;
    m_showWaitIndicator = true;

    if (m_onComplete) {
      m_onComplete();
    }
  } else {
    m_visibleCharacters = 0;
    m_typewriterTimer = 0.0f;
    m_typewriterComplete = false;
    m_showWaitIndicator = false;
  }
}

const std::string &DialogueBox::getText() const { return m_text; }

std::string DialogueBox::getVisibleText() const {
  if (m_visibleCharacters >= m_text.size()) {
    return m_text;
  }
  return m_text.substr(0, m_visibleCharacters);
}

bool DialogueBox::isComplete() const { return m_typewriterComplete; }

void DialogueBox::skipAnimation() {
  if (!m_typewriterComplete) {
    m_visibleCharacters = m_text.size();
    m_typewriterComplete = true;
    m_showWaitIndicator = true;

    if (m_onComplete) {
      m_onComplete();
    }
  }
}

void DialogueBox::clear() {
  m_text.clear();
  m_speakerName.clear();
  m_visibleCharacters = 0;
  m_typewriterComplete = true;
  m_showWaitIndicator = false;
  m_waitIndicatorVisible = false;
}

void DialogueBox::setShowWaitIndicator(bool show) {
  m_showWaitIndicator = show;
}

bool DialogueBox::isWaitIndicatorVisible() const {
  return m_waitIndicatorVisible;
}

void DialogueBox::setOnComplete(CompletionCallback callback) {
  m_onComplete = std::move(callback);
}

void DialogueBox::setAutoAdvance(bool enabled, f32 delay) {
  m_autoAdvance = enabled;
  m_autoAdvanceDelay = delay;
}

bool DialogueBox::isAutoAdvanceEnabled() const { return m_autoAdvance; }

void DialogueBox::setTypewriterSpeed(f32 charsPerSecond) {
  m_style.typewriterSpeed = charsPerSecond;
}

void DialogueBox::startTypewriter() {
  m_visibleCharacters = 0;
  m_typewriterTimer = 0.0f;
  m_typewriterComplete = false;
  m_showWaitIndicator = false;
}

bool DialogueBox::isTypewriterComplete() const { return m_typewriterComplete; }

bool DialogueBox::isWaitingForInput() const {
  return m_typewriterComplete && m_showWaitIndicator;
}

void DialogueBox::show() { m_visible = true; }

void DialogueBox::hide() { m_visible = false; }

void DialogueBox::update(f64 deltaTime) {
  SceneObject::update(deltaTime);

  updateTypewriter(deltaTime);
  updateWaitIndicator(deltaTime);

  // Handle auto-advance
  if (m_autoAdvance && m_typewriterComplete) {
    m_autoAdvanceTimer += static_cast<f32>(deltaTime);
  }
}

void DialogueBox::updateTypewriter(f64 deltaTime) {
  if (m_typewriterComplete || m_text.empty()) {
    return;
  }

  m_typewriterTimer += static_cast<f32>(deltaTime);

  f32 charsPerSecond = m_style.typewriterSpeed;
  f32 charInterval = 1.0f / charsPerSecond;

  while (m_typewriterTimer >= charInterval &&
         m_visibleCharacters < m_text.size()) {
    m_typewriterTimer -= charInterval;
    ++m_visibleCharacters;

    // Handle punctuation pauses
    if (m_visibleCharacters > 0 && m_visibleCharacters < m_text.size()) {
      char lastChar = m_text[m_visibleCharacters - 1];
      if (lastChar == '.' || lastChar == '!' || lastChar == '?') {
        m_typewriterTimer -= charInterval * 3.0f; // Pause for punctuation
      } else if (lastChar == ',') {
        m_typewriterTimer -= charInterval * 1.5f;
      }
    }
  }

  if (m_visibleCharacters >= m_text.size()) {
    m_typewriterComplete = true;
    m_showWaitIndicator = true;

    if (m_onComplete) {
      m_onComplete();
    }
  }
}

void DialogueBox::updateWaitIndicator(f64 deltaTime) {
  if (!m_showWaitIndicator) {
    m_waitIndicatorVisible = false;
    return;
  }

  m_waitIndicatorTimer += static_cast<f32>(deltaTime);

  // Blink at 2 Hz
  const f32 blinkInterval = 0.5f;
  m_waitIndicatorVisible =
      (static_cast<int>(m_waitIndicatorTimer / blinkInterval) % 2) == 0;
}

void DialogueBox::render(renderer::IRenderer &renderer) {
  if (!m_visible) {
    return;
  }

  // Draw background
  renderer::Color bgColor = m_style.backgroundColor;
  bgColor.a = static_cast<u8>(bgColor.a * m_alpha);
  renderer.fillRect(m_bounds, bgColor);

  // Draw border
  if (m_style.borderWidth > 0.0f) {
    renderer::Color borderColor = m_style.borderColor;
    borderColor.a = static_cast<u8>(borderColor.a * m_alpha);
    renderer.drawRect(m_bounds, borderColor);
  }

  // Text rendering would require font support
  // For now, the dialogue box structure is in place

  // Draw wait indicator (simple rectangle for now)
  if (m_waitIndicatorVisible) {
    f32 indicatorSize = 12.0f;
    renderer::Rect indicatorRect{
        m_bounds.x + m_bounds.width - m_style.paddingRight - indicatorSize,
        m_bounds.y + m_bounds.height - m_style.paddingBottom - indicatorSize,
        indicatorSize, indicatorSize};
    renderer::Color indicatorColor = m_style.textColor;
    indicatorColor.a = static_cast<u8>(indicatorColor.a * m_alpha);
    renderer.fillRect(indicatorRect, indicatorColor);
  }
}

} // namespace NovelMind::Scene
