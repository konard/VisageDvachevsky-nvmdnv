#include "NovelMind/scripting/compiler.hpp"
#include "NovelMind/scripting/lexer.hpp"
#include "NovelMind/scripting/parser.hpp"
#include "NovelMind/scripting/script_runtime.hpp"
#include <iostream>

using namespace NovelMind::scripting;

static const char *SCRIPT = R"(
scene node_2 {
    say "New scene"
    goto main
}

scene main {
    show background "title.png"
    say "Welcome to your visual novel!"
}
)";

int main() {
  Lexer lexer;
  auto tokens = lexer.tokenize(SCRIPT);
  if (!tokens.isOk()) {
    std::cerr << "Lexer error: " << tokens.error() << "\n";
    return 1;
  }

  Parser parser;
  auto parse = parser.parse(tokens.value());
  if (!parse.isOk()) {
    std::cerr << "Parse error: " << parse.error() << "\n";
    return 1;
  }

  Compiler compiler;
  auto compiled = compiler.compile(parse.value());
  if (!compiled.isOk()) {
    std::cerr << "Compile error: " << compiled.error() << "\n";
    return 1;
  }

  ScriptRuntime runtime;
  runtime.setEventCallback([](const ScriptEvent &event) {
    if (event.type == ScriptEventType::SceneChange) {
      std::cout << "[SceneChange] " << event.name << "\n";
    }
    if (event.type == ScriptEventType::DialogueStart) {
      std::cout << "[Dialogue] " << event.name << ": "
                << asString(event.value) << "\n";
    }
  });

  auto load = runtime.load(compiled.value());
  if (!load.isOk()) {
    std::cerr << "Load error: " << load.error() << "\n";
    return 1;
  }

  auto gotoResult = runtime.gotoScene("node_2");
  if (!gotoResult.isOk()) {
    std::cerr << "Goto error: " << gotoResult.error() << "\n";
    return 1;
  }

  for (int i = 0; i < 6; ++i) {
    runtime.update(0.016);
    std::cout << "Tick " << i << " state=" << static_cast<int>(runtime.getState())
              << " scene=" << runtime.getCurrentScene() << "\n";
    if (i == 1) {
      runtime.continueExecution();
    }
  }

  return 0;
}
