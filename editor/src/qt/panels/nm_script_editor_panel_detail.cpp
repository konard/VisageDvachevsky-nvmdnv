#include "nm_script_editor_panel_detail.hpp"

namespace NovelMind::editor::qt::detail {

QStringList buildCompletionWords() {
  return {
      "and",        "or",         "not",        "true",      "false",
      "if",         "else",       "then",       "scene",     "character",
      "choice",     "goto",       "say",        "set",       "flag",
      "show",       "hide",       "with",       "transition","wait",
      "play",       "stop",       "music",      "sound",     "voice",
      "at",         "background", "left",       "center",    "right",
      "loop",       "fade",       "dissolve",   "slide_left","slide_right",
      "slide_up",   "slide_down", "shake",      "flash",     "fade_to",
      "fade_from",  "move",       "scale",      "rotate",    "textbox",
      "set_speed",  "allow_skip", "duration",   "intensity", "color",
      "to"};
}

QHash<QString, QString> buildHoverDocs() {
  QHash<QString, QString> docs;
  docs.insert("character", "Declare a character with properties.");
  docs.insert("scene", "Define a scene block with statements.");
  docs.insert("say", "Display dialogue: say <character> \"text\".");
  docs.insert("show", "Show a background or character in the scene.");
  docs.insert("hide", "Hide a background or character.");
  docs.insert("choice", "Present choices: choice { \"Option\" -> action }.");
  docs.insert("goto", "Jump to another scene.");
  docs.insert("set", "Assign a variable or flag: set [flag] name = expr.");
  docs.insert("flag", "Flag access or assignment in conditions/sets.");
  docs.insert("if", "Conditional branch: if expr { ... }.");
  docs.insert("else", "Fallback branch after if.");
  docs.insert("wait", "Pause execution for seconds.");
  docs.insert("play", "Play music/sound/voice.");
  docs.insert("stop", "Stop music/sound/voice.");
  docs.insert("music", "Audio channel for background music.");
  docs.insert("sound", "Audio channel for sound effects.");
  docs.insert("voice", "Audio channel for voice lines.");
  docs.insert("transition", "Transition effect: transition <id> <seconds>.");
  docs.insert("dissolve", "Blend between scenes.");
  docs.insert("fade", "Fade to/from current scene.");
  docs.insert("at", "Position helper for show/move commands.");
  docs.insert("with", "Apply expression/variant to a character.");
  docs.insert("left", "Left position.");
  docs.insert("center", "Center position.");
  docs.insert("right", "Right position.");
  docs.insert("slide_left", "Slide transition to the left.");
  docs.insert("slide_right", "Slide transition to the right.");
  docs.insert("slide_up", "Slide transition up.");
  docs.insert("slide_down", "Slide transition down.");
  docs.insert("shake", "Screen shake effect.");
  docs.insert("flash", "Screen flash effect.");
  docs.insert("fade_to", "Fade to color.");
  docs.insert("fade_from", "Fade from color.");
  docs.insert("move", "Move a character to a position over time.");
  docs.insert("scale", "Scale a character over time.");
  docs.insert("rotate", "Rotate a character over time.");
  docs.insert("background", "Background asset identifier.");
  docs.insert("textbox", "Show or hide the dialogue textbox.");
  docs.insert("set_speed", "Set typewriter speed (chars/sec).");
  docs.insert("allow_skip", "Enable or disable skip mode.");
  return docs;
}

QHash<QString, QString> buildDocHtml() {
  QHash<QString, QString> docs;
  docs.insert("scene",
              "<h3>scene</h3>"
              "<p>Define a scene block with statements.</p>"
              "<p><b>Usage:</b> <code>scene &lt;id&gt; { ... }</code></p>"
              "<pre>scene main {\n    \"Hello, world!\"\n}</pre>");
  docs.insert("character",
              "<h3>character</h3>"
              "<p>Declare a character with display properties.</p>"
              "<p><b>Usage:</b> <code>character &lt;id&gt;(name=\"Name\")</code></p>"
              "<pre>character Hero(name=\"Alex\", color=\"#00AAFF\")</pre>");
  docs.insert("say",
              "<h3>say</h3>"
              "<p>Display dialogue for a character.</p>"
              "<p><b>Usage:</b> <code>say &lt;character&gt; \"text\"</code></p>"
              "<pre>say hero \"We should go.\"</pre>");
  docs.insert("choice",
              "<h3>choice</h3>"
              "<p>Present interactive options.</p>"
              "<p><b>Usage:</b> <code>choice { \"Option\" -> scene_id }</code></p>"
              "<pre>choice {\n    \"Go left\" -> left_path\n    \"Go right\" -> right_path\n}</pre>");
  docs.insert("show",
              "<h3>show</h3>"
              "<p>Show a background or character.</p>"
              "<p><b>Usage:</b> <code>show background \"id\"</code></p>"
              "<p><b>Usage:</b> <code>show &lt;character&gt; at left</code></p>"
              "<p><b>Usage:</b> <code>show &lt;character&gt; at (x, y) with \"expr\"</code></p>");
  docs.insert("hide",
              "<h3>hide</h3>"
              "<p>Hide a background or character.</p>"
              "<p><b>Usage:</b> <code>hide &lt;id&gt;</code></p>");
  docs.insert("set",
              "<h3>set</h3>"
              "<p>Assign a variable or flag.</p>"
              "<p><b>Usage:</b> <code>set name = expr</code></p>"
              "<pre>set affection = affection + 5</pre>");
  docs.insert("flag",
              "<h3>flag</h3>"
              "<p>Access or set boolean flags.</p>"
              "<p><b>Usage:</b> <code>set flag has_key = true</code></p>"
              "<pre>if flag has_key { ... }</pre>");
  docs.insert("if",
              "<h3>if</h3>"
              "<p>Conditional branch.</p>"
              "<p><b>Usage:</b> <code>if expr { ... }</code></p>");
  docs.insert("else",
              "<h3>else</h3>"
              "<p>Fallback branch after if.</p>");
  docs.insert("goto",
              "<h3>goto</h3>"
              "<p>Jump to another scene.</p>"
              "<p><b>Usage:</b> <code>goto scene_id</code></p>");
  docs.insert("play",
              "<h3>play</h3>"
              "<p>Play music, sound, or voice.</p>"
              "<p><b>Usage:</b> <code>play music \"file.ogg\"</code></p>"
              "<p><b>Options:</b> <code>loop=false</code></p>");
  docs.insert("stop",
              "<h3>stop</h3>"
              "<p>Stop music, sound, or voice.</p>"
              "<p><b>Usage:</b> <code>stop music</code></p>"
              "<p><b>Options:</b> <code>fade=1.0</code></p>");
  docs.insert("transition",
              "<h3>transition</h3>"
              "<p>Run a visual transition.</p>"
              "<p><b>Usage:</b> <code>transition fade 0.5</code></p>"
              "<p><b>Types:</b> fade, dissolve, slide_left, slide_right, slide_up, slide_down</p>");
  docs.insert("slide_left",
              "<h3>slide_left</h3>"
              "<p>Slide transition to the left.</p>");
  docs.insert("slide_right",
              "<h3>slide_right</h3>"
              "<p>Slide transition to the right.</p>");
  docs.insert("slide_up",
              "<h3>slide_up</h3>"
              "<p>Slide transition upward.</p>");
  docs.insert("slide_down",
              "<h3>slide_down</h3>"
              "<p>Slide transition downward.</p>");
  docs.insert("dissolve",
              "<h3>dissolve</h3>"
              "<p>Blend between scenes.</p>"
              "<p><b>Usage:</b> <code>dissolve 0.4</code></p>");
  docs.insert("fade",
              "<h3>fade</h3>"
              "<p>Fade to/from the current scene.</p>"
              "<p><b>Usage:</b> <code>fade 0.6</code></p>");
  docs.insert("wait",
              "<h3>wait</h3>"
              "<p>Pause execution for seconds.</p>"
              "<p><b>Usage:</b> <code>wait 0.5</code></p>");
  docs.insert("shake",
              "<h3>shake</h3>"
              "<p>Screen shake effect.</p>"
              "<p><b>Usage:</b> <code>shake 0.4 0.2</code></p>");
  docs.insert("flash",
              "<h3>flash</h3>"
              "<p>Flash the screen.</p>"
              "<p><b>Usage:</b> <code>flash 0.4</code></p>");
  docs.insert("fade_to",
              "<h3>fade_to</h3>"
              "<p>Fade to color.</p>"
              "<p><b>Usage:</b> <code>fade_to #000000 0.3</code></p>");
  docs.insert("fade_from",
              "<h3>fade_from</h3>"
              "<p>Fade from color.</p>"
              "<p><b>Usage:</b> <code>fade_from #000000 0.3</code></p>");
  docs.insert("move",
              "<h3>move</h3>"
              "<p>Move a character to a position over time.</p>"
              "<p><b>Usage:</b> <code>move hero to (0.5, 0.3) 1.0</code></p>");
  docs.insert("scale",
              "<h3>scale</h3>"
              "<p>Scale a character over time.</p>"
              "<p><b>Usage:</b> <code>scale hero 1.2 0.5</code></p>");
  docs.insert("rotate",
              "<h3>rotate</h3>"
              "<p>Rotate a character over time.</p>"
              "<p><b>Usage:</b> <code>rotate hero 15 0.3</code></p>");
  docs.insert("background",
              "<h3>background</h3>"
              "<p>Background asset identifier.</p>"
              "<p><b>Usage:</b> <code>show background \"bg_id\"</code></p>");
  docs.insert("textbox",
              "<h3>textbox</h3>"
              "<p>Show or hide the dialogue textbox.</p>"
              "<p><b>Usage:</b> <code>textbox show</code></p>");
  docs.insert("set_speed",
              "<h3>set_speed</h3>"
              "<p>Set typewriter speed (chars/sec).</p>"
              "<p><b>Usage:</b> <code>set_speed 30</code></p>");
  docs.insert("allow_skip",
              "<h3>allow_skip</h3>"
              "<p>Enable or disable skip mode.</p>"
              "<p><b>Usage:</b> <code>allow_skip true</code></p>");
  return docs;
}

QList<NMScriptEditor::CompletionEntry> buildKeywordEntries() {
  QList<NMScriptEditor::CompletionEntry> entries;
  for (const auto &word : buildCompletionWords()) {
    entries.push_back({word, "keyword"});
  }
  return entries;
}

} // namespace NovelMind::editor::qt::detail
