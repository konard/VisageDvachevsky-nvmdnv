#include "NovelMind/editor/qt/nm_icon_manager.hpp"
#include <QBuffer>
#include <QPainter>
#include <QSvgRenderer>

namespace NovelMind::editor::qt {

NMIconManager &NMIconManager::instance() {
  static NMIconManager instance;
  return instance;
}

NMIconManager::NMIconManager()
    : m_defaultColor(220, 220, 220) // Light gray for dark theme
{
  initializeIcons();
}

void NMIconManager::initializeIcons() {
  // =========================================================================
  // FILE OPERATIONS
  // =========================================================================
  m_iconSvgData["file-new"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 3h-2V1c0-.55-.45-1-1-1H1C.45 0 0 .45 0 "
      "1v14c0 .55.45 1 1 1h10c.55 0 1-.45 1-1v-2h2c.55 0 1-.45 "
      "1-1V4c0-.55-.45-1-1-1zM11 15H1V1h10v14zm3-3h-2V4h2v8z'/>"
      "<path fill='%COLOR%' d='M3 6h6v1H3zm0 2h6v1H3zm0 2h6v1H3z'/>"
      "</svg>";

  m_iconSvgData["file-open"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 4h-4.38l-1.3-1.3c-.2-.2-.45-.7-.7-.7H1c-.55 "
      "0-1 .45-1 1v10c0 .55.45 1 1 1h13c.55 0 1-.45 1-1V5c0-.55-.45-1-1-1zm0 "
      "9H1V3h5l2 2h6v8z'/>"
      "</svg>";

  m_iconSvgData["file-save"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 0H2C.9 0 0 .9 0 2v12c0 1.1.9 2 2 2h12c1.1 0 "
      "2-.9 2-2V2c0-1.1-.9-2-2-2zM5 2h6v3H5V2zm9 12H2V2h1v4h10V2h1v12z'/>"
      "<rect fill='%COLOR%' x='4' y='9' width='8' height='1'/>"
      "<rect fill='%COLOR%' x='4' y='11' width='8' height='1'/>"
      "<rect fill='%COLOR%' x='4' y='13' width='8' height='1'/>"
      "</svg>";

  m_iconSvgData["file-close"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 3h-2V1c0-.55-.45-1-1-1H1C.45 0 0 .45 0 "
      "1v14c0 .55.45 1 1 1h10c.55 0 1-.45 1-1v-2h2c.55 0 1-.45 "
      "1-1V4c0-.55-.45-1-1-1zM11 15H1V1h10v14z'/>"
      "<path fill='%COLOR%' d='M10.3 4.7l-1.4-1.4L8 4.2 7.1 3.3 5.7 4.7 6.6 "
      "5.6 5.7 6.5l1.4 1.4.9-.9.9.9 1.4-1.4-.9-.9z'/>"
      "</svg>";

  // =========================================================================
  // EDIT OPERATIONS
  // =========================================================================
  m_iconSvgData["edit-undo"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1c-1.7 0-3.3.6-4.6 1.7L1 1v5h5L3.5 3.5C4.5 "
      "2.6 6.2 2 8 2c3.3 0 6 2.7 6 6s-2.7 6-6 6-6-2.7-6-6H1c0 3.9 3.1 7 7 "
      "7s7-3.1 7-7-3.1-7-7-7z'/>"
      "</svg>";

  m_iconSvgData["edit-redo"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1c1.7 0 3.3.6 4.6 1.7L15 1v5h-5l2.5-2.5C11.5 "
      "2.6 9.8 2 8 2 4.7 2 2 4.7 2 8s2.7 6 6 6 6-2.7 6-6h1c0 3.9-3.1 7-7 "
      "7s-7-3.1-7-7 3.1-7 7-7z'/>"
      "</svg>";

  m_iconSvgData["edit-cut"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M13.5 3l-6 6-2-2L3 9.5c-.8.8-.8 2.1 0 2.8.8.8 "
      "2.1.8 2.8 0L8.5 9.7l2 2 5-5-2-3.7zM4.5 11c-.3 "
      "0-.5-.2-.5-.5s.2-.5.5-.5.5.2.5.5-.2.5-.5.5z'/>"
      "<circle fill='%COLOR%' cx='4.5' cy='3.5' r='1.5'/>"
      "</svg>";

  m_iconSvgData["edit-copy"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M13 0H3C2.4 0 2 .4 2 1v2H0v11c0 .6.4 1 1 "
      "1h10c.6 0 1-.4 1-1v-2h2c.6 0 1-.4 1-1V1c0-.6-.4-1-1-1zm-2 "
      "14H1V4h10v10zm2-3h-1V4c0-.6-.4-1-1-1H4V1h9v10z'/>"
      "</svg>";

  m_iconSvgData["edit-paste"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M11 2h1c.6 0 1 .4 1 1v11c0 .6-.4 1-1 1H4c-.6 "
      "0-1-.4-1-1V3c0-.6.4-1 1-1h1V1h6v1zM5 4v10h6V4H5z'/>"
      "<rect fill='%COLOR%' x='6' y='0' width='4' height='2' rx='1'/>"
      "</svg>";

  m_iconSvgData["edit-delete"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M13 3h-2V1c0-.55-.45-1-1-1H6c-.55 0-1 .45-1 "
      "1v2H3c-.55 0-1 .45-1 1v1h12V4c0-.55-.45-1-1-1zM6 1h4v2H6V1z'/>"
      "<path fill='%COLOR%' d='M4 6v9c0 .55.45 1 1 1h6c.55 0 1-.45 1-1V6H4zm2 "
      "8H5V8h1v6zm2 0H7V8h1v6zm2 0H9V8h1v6z'/>"
      "</svg>";

  m_iconSvgData["copy"] = m_iconSvgData["edit-copy"];
  m_iconSvgData["delete"] = m_iconSvgData["edit-delete"];

  // =========================================================================
  // PLAYBACK CONTROLS
  // =========================================================================
  m_iconSvgData["play"] = "<svg viewBox='0 0 16 16'>"
                          "<path fill='%COLOR%' d='M3 2v12l10-6z'/>"
                          "</svg>";

  m_iconSvgData["pause"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='3' y='2' width='3' height='12'/>"
      "<rect fill='%COLOR%' x='10' y='2' width='3' height='12'/>"
      "</svg>";

  m_iconSvgData["stop"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='3' y='3' width='10' height='10'/>"
      "</svg>";

  m_iconSvgData["step-forward"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M2 2v12l9-6z'/>"
      "<rect fill='%COLOR%' x='12' y='2' width='2' height='12'/>"
      "</svg>";

  m_iconSvgData["step-backward"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 2v12L5 8z'/>"
      "<rect fill='%COLOR%' x='2' y='2' width='2' height='12'/>"
      "</svg>";

  // =========================================================================
  // PANEL ICONS
  // =========================================================================
  m_iconSvgData["panel-scene"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='2' width='12' height='12' rx='1' "
      "fill='none' stroke='%COLOR%' stroke-width='1.5'/>"
      "<circle fill='%COLOR%' cx='8' cy='6' r='2'/>"
      "<path fill='%COLOR%' d='M4 14l3-4 2 2 3-4 2 2v4z'/>"
      "</svg>";

  m_iconSvgData["panel-graph"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='3' cy='8' r='2'/>"
      "<circle fill='%COLOR%' cx='13' cy='8' r='2'/>"
      "<circle fill='%COLOR%' cx='8' cy='4' r='2'/>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1.5' d='M5 8h6M5 "
      "7l3-2M11 7l-3-2'/>"
      "</svg>";

  m_iconSvgData["panel-inspector"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='2' width='12' height='2'/>"
      "<rect fill='%COLOR%' x='2' y='6' width='12' height='2'/>"
      "<rect fill='%COLOR%' x='2' y='10' width='12' height='2'/>"
      "<circle fill='%COLOR%' cx='12' cy='3' r='1.5'/>"
      "<circle fill='%COLOR%' cx='12' cy='7' r='1.5'/>"
      "<circle fill='%COLOR%' cx='12' cy='11' r='1.5'/>"
      "</svg>";

  m_iconSvgData["panel-console"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='1' width='14' height='14' rx='1' "
      "fill='none' stroke='%COLOR%' stroke-width='1.5'/>"
      "<path fill='%COLOR%' d='M3 5l3 2-3 2v-4z'/>"
      "<rect fill='%COLOR%' x='8' y='8' width='5' height='1'/>"
      "</svg>";

  m_iconSvgData["panel-hierarchy"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='2' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='4' y='6' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='4' y='10' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='10' y='6' width='4' height='2'/>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1' d='M4 4h2M6 4v3M6 "
      "7h2M6 7v4M6 11h2M8 7h2'/>"
      "</svg>";

  m_iconSvgData["panel-assets"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='1' width='6' height='6' rx='0.5'/>"
      "<rect fill='%COLOR%' x='9' y='1' width='6' height='6' rx='0.5'/>"
      "<rect fill='%COLOR%' x='1' y='9' width='6' height='6' rx='0.5'/>"
      "<rect fill='%COLOR%' x='9' y='9' width='6' height='6' rx='0.5'/>"
      "</svg>";

  m_iconSvgData["panel-timeline"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='3' width='14' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='7' width='14' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='11' width='14' height='2'/>"
      "<rect fill='%COLOR%' x='3' y='3' width='1' height='10'/>"
      "<circle fill='%COLOR%' cx='6' cy='4' r='1'/>"
      "<circle fill='%COLOR%' cx='9' cy='8' r='1'/>"
      "<circle fill='%COLOR%' cx='12' cy='12' r='1'/>"
      "</svg>";

  m_iconSvgData["panel-curve"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1.5' d='M2 14Q5 2 8 "
      "8T14 2'/>"
      "<circle fill='%COLOR%' cx='2' cy='14' r='1.5'/>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='1.5'/>"
      "<circle fill='%COLOR%' cx='14' cy='2' r='1.5'/>"
      "</svg>";

  m_iconSvgData["panel-voice"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='7' y='2' width='2' height='7' rx='1'/>"
      "<path fill='%COLOR%' d='M4 7v1c0 2.2 1.8 4 4 4s4-1.8 4-4V7h1v1c0 "
      "2.5-1.8 4.6-4.2 4.9V15h2v1H5v-1h2v-2.1C4.8 12.6 3 10.5 3 8V7h1z'/>"
      "</svg>";

  m_iconSvgData["panel-localization"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='8' cy='8' "
      "r='6'/>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1.5' d='M2 8h12M8 "
      "2c-1.5 0-3 2.7-3 6s1.5 6 3 6 3-2.7 3-6-1.5-6-3-6z'/>"
      "</svg>";

  m_iconSvgData["panel-diagnostics"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='8' cy='8' "
      "r='6'/>"
      "<path fill='%COLOR%' d='M7.5 4h1v5h-1z'/>"
      "<circle fill='%COLOR%' cx='8' cy='11' r='0.8'/>"
      "</svg>";

  m_iconSvgData["panel-build"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M13.8 6.2l-4-4c-.3-.3-.7-.2-1 0l-1 1c-.3.3-.3.7 "
      "0 1l1.3 1.3-5.4 5.4-1.3-1.3c-.3-.3-.7-.3-1 0l-1 1c-.3.3-.3.7 0 1l4 "
      "4c.3.3.7.3 1 0l1-1c.3-.3.3-.7 0-1l-1.3-1.3 5.4-5.4 1.3 1.3c.3.3.7.3 1 "
      "0l1-1c.3-.3.3-.7 0-1z'/>"
      "</svg>";

  // =========================================================================
  // ZOOM AND VIEW CONTROLS
  // =========================================================================
  m_iconSvgData["zoom-in"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='6.5' "
      "cy='6.5' r='4.5'/>"
      "<path fill='%COLOR%' d='M9.5 11l4.5 4.5-1 1L8.5 12z'/>"
      "<path fill='%COLOR%' d='M6 4v2h2v1H6v2H5V7H3V6h2V4z'/>"
      "</svg>";

  m_iconSvgData["zoom-out"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='6.5' "
      "cy='6.5' r='4.5'/>"
      "<path fill='%COLOR%' d='M9.5 11l4.5 4.5-1 1L8.5 12z'/>"
      "<rect fill='%COLOR%' x='3' y='6' width='7' height='1'/>"
      "</svg>";

  m_iconSvgData["zoom-fit"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='2' y='2' "
      "width='12' height='12'/>"
      "<path fill='%COLOR%' d='M5 5h2V4H4v3h1zM11 5h-2V4h3v3h-1zM11 "
      "11h-2v1h3v-3h-1zM5 11h2v1H4v-3h1z'/>"
      "</svg>";

  // =========================================================================
  // UTILITY ICONS
  // =========================================================================
  m_iconSvgData["settings"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14.5 "
      "6.6l-1.4-.4c-.1-.3-.2-.6-.4-.9l.7-1.3c.2-.4.1-.8-.2-1.1l-.8-.8c-.3-.3-."
      "7-.4-1.1-.2l-1.3.7c-.3-.2-.6-.3-.9-.4L8.7 1c-.1-.5-.5-.8-1-.8h-1.1c-.5 "
      "0-.9.3-1 .8l-.4 1.4c-.3.1-.6.2-.9.4L3 "
      "2.1c-.4-.2-.8-.1-1.1.2l-.8.8c-.3.3-.4.7-.2 1.1l.7 "
      "1.3c-.2.3-.3.6-.4.9L.2 6.8c-.5.1-.8.5-.8 1v1.1c0 .5.3.9.8 "
      "1l1.4.4c.1.3.2.6.4.9l-.7 1.3c-.2.4-.1.8.2 1.1l.8.8c.3.3.7.4 "
      "1.1.2l1.3-.7c.3.2.6.3.9.4l.4 1.4c.1.5.5.8 1 .8h1.1c.5 0 .9-.3 "
      "1-.8l.4-1.4c.3-.1.6-.2.9-.4l1.3.7c.4.2.8.1 "
      "1.1-.2l.8-.8c.3-.3.4-.7.2-1.1l-.7-1.3c.2-.3.3-.6.4-.9l1.4-.4c.5-.1.8-.5."
      "8-1V7.6c0-.5-.3-.9-.8-1zM8 11c-1.7 0-3-1.3-3-3s1.3-3 3-3 3 1.3 3 3-1.3 "
      "3-3 3z'/>"
      "</svg>";

  m_iconSvgData["help"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='8' cy='8' "
      "r='6'/>"
      "<path fill='%COLOR%' d='M8 6c-.8 0-1.5.7-1.5 1.5h-1C5.5 6.1 6.6 5 8 "
      "5s2.5 1.1 2.5 2.5c0 1-.6 1.8-1.5 2.2v.8h-1v-1.2c0-.3.2-.5.5-.5.8 0 "
      "1.5-.7 1.5-1.5S8.8 5.8 8 5.8z'/>"
      "<circle fill='%COLOR%' cx='8' cy='12' r='0.8'/>"
      "</svg>";

  m_iconSvgData["search"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='6.5' "
      "cy='6.5' r='4.5'/>"
      "<path fill='%COLOR%' d='M9.5 11l4.5 4.5-1 1L8.5 12z'/>"
      "</svg>";

  m_iconSvgData["filter"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M1 2h14l-5.5 6.5v4.3l-3 1.5V8.5z'/>"
      "</svg>";

  m_iconSvgData["refresh"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 2v5h-5l2-2C9.7 3.7 8 3 8 3 4.7 3 2 5.7 2 "
      "9s2.7 6 6 6c2.8 0 5.2-2 5.8-4.6h1.1C14.3 13.6 11.4 16 8 16c-3.9 "
      "0-7-3.1-7-7s3.1-7 7-7c1.9 0 3.6.8 4.9 2l2-2z'/>"
      "</svg>";

  m_iconSvgData["add"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 7H9V2H7v5H2v2h5v5h2V9h5z'/>"
      "</svg>";

  m_iconSvgData["remove"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='7' width='12' height='2'/>"
      "</svg>";

  m_iconSvgData["warning"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1L1 14h14L8 1zm0 4h1v5H8V5zm.5 7c-.3 "
      "0-.5-.2-.5-.5s.2-.5.5-.5.5.2.5.5-.2.5-.5.5z'/>"
      "</svg>";

  m_iconSvgData["error"] = "<svg viewBox='0 0 16 16'>"
                           "<circle fill='%COLOR%' cx='8' cy='8' r='7'/>"
                           "<path fill='#2d2d2d' d='M7.5 4h1v5h-1z'/>"
                           "<circle fill='#2d2d2d' cx='8' cy='11' r='0.8'/>"
                           "</svg>";

  m_iconSvgData["info"] = "<svg viewBox='0 0 16 16'>"
                          "<circle fill='none' stroke='%COLOR%' "
                          "stroke-width='1.5' cx='8' cy='8' r='6'/>"
                          "<circle fill='%COLOR%' cx='8' cy='5' r='0.8'/>"
                          "<path fill='%COLOR%' d='M7.5 7h1v5h-1z'/>"
                          "</svg>";

  // =========================================================================
  // ARROW AND NAVIGATION
  // =========================================================================
  m_iconSvgData["arrow-up"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 3L3 8l1.4 1.4L7 6.8V13h2V6.8l2.6 2.6L13 8z'/>"
      "</svg>";

  m_iconSvgData["arrow-down"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 13l5-5-1.4-1.4L9 9.2V3H7v6.2L4.4 6.6 3 8z'/>"
      "</svg>";

  m_iconSvgData["arrow-left"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M3 8l5-5 1.4 1.4L6.8 7H13v2H6.8l2.6 2.6L8 13z'/>"
      "</svg>";

  m_iconSvgData["arrow-right"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M13 8l-5 5-1.4-1.4L9.2 9H3V7h6.2L6.6 4.4 8 3z'/>"
      "</svg>";

  m_iconSvgData["chevron-up"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 5l6 6-1 1-5-5-5 5-1-1z'/>"
      "</svg>";

  m_iconSvgData["chevron-down"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 11L2 5l1-1 5 5 5-5 1 1z'/>"
      "</svg>";

  m_iconSvgData["chevron-left"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M5 8l6-6 1 1-5 5 5 5-1 1z'/>"
      "</svg>";

  m_iconSvgData["chevron-right"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M11 8L5 2 4 3l5 5-5 5 1 1z'/>"
      "</svg>";

  // =========================================================================
  // NODE TYPE ICONS (for Story Graph)
  // =========================================================================
  m_iconSvgData["node-dialogue"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='4' width='12' height='8' rx='2'/>"
      "<path fill='%COLOR%' d='M6 12l2 2 2-2z'/>"
      "<rect fill='#2d2d2d' x='4' y='6' width='8' height='1'/>"
      "<rect fill='#2d2d2d' x='4' y='8' width='6' height='1'/>"
      "</svg>";

  m_iconSvgData["node-choice"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 2l6 5-2 0v7h-2V7H6v7H4V7H2z'/>"
      "</svg>";

  m_iconSvgData["node-event"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 2l2 6h5l-4 3 2 5-5-3-5 3 2-5-4-3h5z'/>"
      "</svg>";

  m_iconSvgData["node-condition"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1l7 7-7 7-7-7z'/>"
      "<path fill='#2d2d2d' d='M6 6h4v1H6zm0 3h4v1H6z'/>"
      "</svg>";

  m_iconSvgData["node-random"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='2' width='5' height='5' rx='1'/>"
      "<rect fill='%COLOR%' x='9' y='2' width='5' height='5' rx='1'/>"
      "<rect fill='%COLOR%' x='2' y='9' width='5' height='5' rx='1'/>"
      "<rect fill='%COLOR%' x='9' y='9' width='5' height='5' rx='1'/>"
      "<circle fill='#2d2d2d' cx='4.5' cy='4.5' r='1'/>"
      "<circle fill='#2d2d2d' cx='11.5' cy='4.5' r='1'/>"
      "<circle fill='#2d2d2d' cx='4.5' cy='11.5' r='1'/>"
      "<circle fill='#2d2d2d' cx='11.5' cy='11.5' r='1'/>"
      "</svg>";

  m_iconSvgData["node-start"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='6'/>"
      "<path fill='#2d2d2d' d='M6 5v6l5-3z'/>"
      "</svg>";

  m_iconSvgData["node-end"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='6'/>"
      "<rect fill='#2d2d2d' x='5' y='5' width='6' height='6'/>"
      "</svg>";

  m_iconSvgData["node-jump"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 2l5 5-3 0v4l3 0-5 5-5-5 3 0V7l-3 0z'/>"
      "</svg>";

  m_iconSvgData["node-variable"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='2' width='12' height='12' rx='2'/>"
      "<text fill='#2d2d2d' x='8' y='11' text-anchor='middle' "
      "font-size='10' font-family='monospace' font-weight='bold'>x</text>"
      "</svg>";

  // =========================================================================
  // SCENE OBJECT TYPE ICONS
  // =========================================================================
  m_iconSvgData["object-character"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='4' r='3'/>"
      "<path fill='%COLOR%' d='M8 8c-3 0-5 2-5 4v2h10v-2c0-2-2-4-5-4z'/>"
      "</svg>";

  m_iconSvgData["object-background"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='1' width='14' height='14' rx='1'/>"
      "<circle fill='#2d2d2d' cx='5' cy='5' r='2'/>"
      "<path fill='#2d2d2d' d='M2 14l4-5 3 3 5-6v8z'/>"
      "</svg>";

  m_iconSvgData["object-prop"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='3' y='3' width='10' height='10' rx='1'/>"
      "<rect fill='#2d2d2d' x='5' y='5' width='6' height='6' rx='0.5'/>"
      "</svg>";

  m_iconSvgData["object-effect"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 2l1.5 4.5H14l-3.5 2.5 1.5 4.5L8 11l-4 "
      "2.5 1.5-4.5L2 6.5h4.5z'/>"
      "</svg>";

  m_iconSvgData["object-ui"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='2' y='2' "
      "width='12' height='12' rx='1'/>"
      "<rect fill='%COLOR%' x='4' y='4' width='8' height='2'/>"
      "<rect fill='%COLOR%' x='4' y='7' width='6' height='1'/>"
      "<rect fill='%COLOR%' x='4' y='9' width='6' height='1'/>"
      "</svg>";

  // =========================================================================
  // TRANSFORM AND MANIPULATION ICONS
  // =========================================================================
  m_iconSvgData["transform-move"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1L6 3h4zM15 8l-2-2v4zM8 15l2-2H6zM1 8l2 "
      "2V6z'/>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='2'/>"
      "</svg>";

  m_iconSvgData["transform-rotate"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1.5' "
      "d='M14,8 A6,6 0 1,1 8,2'/>"
      "<path fill='%COLOR%' d='M14 4V2h-2l2 2z'/>"
      "<path fill='%COLOR%' d='M12 2l2 2 2-2z'/>"
      "</svg>";

  m_iconSvgData["transform-scale"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='3' y='3' "
      "width='10' height='10'/>"
      "<path fill='%COLOR%' d='M2 2h2v2H2zM12 2h2v2h-2zM2 12h2v2H2zM12 "
      "12h2v2h-2z'/>"
      "</svg>";

  // =========================================================================
  // ASSET TYPE ICONS
  // =========================================================================
  m_iconSvgData["asset-image"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='1' y='1' "
      "width='14' height='14' rx='1'/>"
      "<circle fill='%COLOR%' cx='5' cy='5' r='2'/>"
      "<path fill='%COLOR%' d='M2 14l4-5 2 2 4-5 2 2v6z'/>"
      "</svg>";

  m_iconSvgData["asset-audio"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M6 4v8H4c-1 0-2-1-2-2V6c0-1 1-2 2-2z'/>"
      "<path fill='%COLOR%' d='M7 3l7-2v10l-7 2z'/>"
      "</svg>";

  m_iconSvgData["asset-video"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='3' width='10' height='10' rx='1'/>"
      "<path fill='%COLOR%' d='M12 5l3-2v10l-3-2z'/>"
      "<path fill='#2d2d2d' d='M5 6v4l3-2z'/>"
      "</svg>";

  m_iconSvgData["asset-font"] =
      "<svg viewBox='0 0 16 16'>"
      "<text fill='%COLOR%' x='8' y='13' text-anchor='middle' "
      "font-size='12' font-weight='bold'>A</text>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='2' y='2' "
      "width='12' height='12' rx='1'/>"
      "</svg>";

  m_iconSvgData["asset-script"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='2' y='1' width='12' height='14' rx='1'/>"
      "<rect fill='#2d2d2d' x='4' y='3' width='8' height='1'/>"
      "<rect fill='#2d2d2d' x='4' y='5' width='6' height='1'/>"
      "<rect fill='#2d2d2d' x='4' y='7' width='7' height='1'/>"
      "<rect fill='#2d2d2d' x='4' y='9' width='5' height='1'/>"
      "</svg>";

  m_iconSvgData["asset-folder"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M14 4h-6L6 2H2C1 2 0 3 0 4v8c0 1 1 2 2 "
      "2h12c1 0 2-1 2-2V6c0-1-1-2-2-2z'/>"
      "</svg>";

  // =========================================================================
  // ACTION AND TOOL ICONS
  // =========================================================================
  m_iconSvgData["tool-select"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M2 1v12l4-4 2 4 2-1-2-4 4-1z'/>"
      "</svg>";

  m_iconSvgData["tool-hand"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M9 1v5h1V2c0-.6.4-1 1-1s1 .4 1 1v4h1V3c0-.6.4-1 "
      "1-1s1 .4 1 1v6c0 2.2-1.8 4-4 4H8c-2.2 0-4-1.8-4-4V6c0-.6.4-1 "
      "1-1s1 .4 1 1v3h1V3c0-.6.4-1 1-1s1 .4 1 1v3z'/>"
      "</svg>";

  m_iconSvgData["tool-zoom"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='none' stroke='%COLOR%' stroke-width='1.5' cx='6' "
      "cy='6' r='4'/>"
      "<path fill='%COLOR%' d='M9 9l5 5-1 1-5-5z'/>"
      "</svg>";

  m_iconSvgData["tool-frame"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='none' stroke='%COLOR%' stroke-width='1.5' x='2' y='2' "
      "width='12' height='12'/>"
      "<path fill='%COLOR%' d='M4 4h2v2H4zM10 4h2v2h-2zM4 10h2v2H4zM10 "
      "10h2v2h-2z'/>"
      "</svg>";

  // =========================================================================
  // STATUS AND INDICATOR ICONS
  // =========================================================================
  m_iconSvgData["status-success"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='7'/>"
      "<path fill='#2d2d2d' d='M6 8l2 2 4-4-1-1-3 3-1-1z'/>"
      "</svg>";

  m_iconSvgData["status-warning"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 1L1 14h14L8 1z'/>"
      "<path fill='#2d2d2d' d='M7.5 5h1v4h-1z'/>"
      "<circle fill='#2d2d2d' cx='8' cy='11' r='0.7'/>"
      "</svg>";

  m_iconSvgData["status-error"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='7'/>"
      "<path fill='#2d2d2d' d='M5 5l6 6M11 5l-6 6'/>"
      "</svg>";

  m_iconSvgData["status-info"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='7'/>"
      "<circle fill='#2d2d2d' cx='8' cy='5' r='0.8'/>"
      "<rect fill='#2d2d2d' x='7.5' y='7' width='1' height='5'/>"
      "</svg>";

  m_iconSvgData["breakpoint"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='6'/>"
      "<circle fill='#ff6b6b' cx='8' cy='8' r='4'/>"
      "</svg>";

  m_iconSvgData["execute"] =
      "<svg viewBox='0 0 16 16'>"
      "<circle fill='%COLOR%' cx='8' cy='8' r='6'/>"
      "<path fill='#2d2d2d' d='M6 5v6l5-3z'/>"
      "</svg>";

  // =========================================================================
  // GRID AND LAYOUT ICONS
  // =========================================================================
  m_iconSvgData["layout-grid"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='1' width='6' height='6'/>"
      "<rect fill='%COLOR%' x='9' y='1' width='6' height='6'/>"
      "<rect fill='%COLOR%' x='1' y='9' width='6' height='6'/>"
      "<rect fill='%COLOR%' x='9' y='9' width='6' height='6'/>"
      "</svg>";

  m_iconSvgData["layout-list"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='1' y='2' width='14' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='7' width='14' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='12' width='14' height='2'/>"
      "</svg>";

  m_iconSvgData["layout-tree"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='6' y='1' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='7' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='11' y='7' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='1' y='13' width='4' height='2'/>"
      "<rect fill='%COLOR%' x='11' y='13' width='4' height='2'/>"
      "<path fill='none' stroke='%COLOR%' stroke-width='1' d='M8 3v4M8 "
      "7H3v6M8 7h5v6'/>"
      "</svg>";

  // =========================================================================
  // VISIBILITY AND LOCK ICONS
  // =========================================================================
  m_iconSvgData["visible"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 3C3 3 1 8 1 8s2 5 7 5 7-5 7-5-2-5-7-5z'/>"
      "<circle fill='#2d2d2d' cx='8' cy='8' r='2'/>"
      "</svg>";

  m_iconSvgData["hidden"] =
      "<svg viewBox='0 0 16 16'>"
      "<path fill='%COLOR%' d='M8 3C3 3 1 8 1 8s2 5 7 5 7-5 7-5-2-5-7-5z'/>"
      "<path fill='#dc3545' d='M2 2l12 12-1 1L1 3z'/>"
      "</svg>";

  m_iconSvgData["locked"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='4' y='7' width='8' height='7' rx='1'/>"
      "<path fill='%COLOR%' d='M5 7V5c0-1.7 1.3-3 3-3s3 1.3 3 3v2h1V5c0-2.2-1.8-4-4-4S4 "
      "2.8 4 5v2z'/>"
      "<circle fill='#2d2d2d' cx='8' cy='10.5' r='1'/>"
      "</svg>";

  m_iconSvgData["unlocked"] =
      "<svg viewBox='0 0 16 16'>"
      "<rect fill='%COLOR%' x='4' y='7' width='8' height='7' rx='1'/>"
      "<path fill='%COLOR%' d='M5 7V5c0-1.7 1.3-3 3-3s3 1.3 3 3h1c0-2.2-1.8-4-4-4S4 "
      "2.8 4 5v2z'/>"
      "<circle fill='#2d2d2d' cx='8' cy='10.5' r='1'/>"
      "</svg>";
}

QIcon NMIconManager::getIcon(const QString &iconName, int size,
                             const QColor &color) {
  QColor iconColor = color.isValid() ? color : m_defaultColor;

  QString cacheKey =
      QString("%1_%2_%3").arg(iconName).arg(size).arg(iconColor.name());

  if (m_iconCache.contains(cacheKey)) {
    return m_iconCache[cacheKey];
  }

  QPixmap pixmap = getPixmap(iconName, size, iconColor);
  QIcon icon(pixmap);
  m_iconCache[cacheKey] = icon;

  return icon;
}

QPixmap NMIconManager::getPixmap(const QString &iconName, int size,
                                 const QColor &color) {
  QString svgData = getSvgData(iconName);
  if (svgData.isEmpty()) {
    // Return empty pixmap if icon not found
    return QPixmap(size, size);
  }

  QColor iconColor = color.isValid() ? color : m_defaultColor;
  return renderSvg(svgData, size, iconColor);
}

void NMIconManager::clearCache() { m_iconCache.clear(); }

void NMIconManager::setDefaultColor(const QColor &color) {
  if (m_defaultColor != color) {
    m_defaultColor = color;
    clearCache(); // Clear cache when color changes
  }
}

QString NMIconManager::getSvgData(const QString &iconName) {
  return m_iconSvgData.value(iconName, QString());
}

QPixmap NMIconManager::renderSvg(const QString &svgData, int size,
                                 const QColor &color) {
  // Replace color token with the requested color.
  QString coloredSvg = svgData;
  coloredSvg.replace("%COLOR%", color.name());

  // Create SVG renderer
  QByteArray svgBytes = coloredSvg.toUtf8();
  QSvgRenderer renderer(svgBytes);

  // Check if SVG renderer is valid to prevent rendering crashes
  if (!renderer.isValid()) {
    // Return a valid but empty pixmap if SVG rendering fails
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    return pixmap;
  }

  // Create pixmap
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  // Render SVG with error handling
  QPainter painter(&pixmap);
  if (!painter.isActive()) {
    // If painter fails to initialize, return empty pixmap
    return pixmap;
  }

  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  renderer.render(&painter);
  painter.end();

  return pixmap;
}

} // namespace NovelMind::editor::qt
