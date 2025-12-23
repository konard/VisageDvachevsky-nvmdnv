#include "NovelMind/editor/editor_runtime_host.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace NovelMind::editor;
namespace fs = std::filesystem;

static const char *SCRIPT = R"(
scene node_2 {
    say "New scene"
    goto main
}

character Hero(name="Alex", color="#ffcc00")
character Narrator(name="Narrator", color="#cccccc")

scene main {
    show background "title.png"
    say "Welcome to your visual novel!"
    say "Replace this script with your story."
    Hero "Let's begin."
}
)";

static fs::path createTempDir() {
  auto tempDir = fs::temp_directory_path() / "nm_goto_repro";
  fs::create_directories(tempDir / "scripts");
  fs::create_directories(tempDir / "assets");
  fs::create_directories(tempDir / "Scenes");
  return tempDir;
}

static void writeScript(const fs::path &dir) {
  std::ofstream file(dir / "scripts" / "main.nms");
  file << SCRIPT;
}

int main() {
  auto tempDir = createTempDir();
  writeScript(tempDir);

  ProjectDescriptor project;
  project.name = "GotoRepro";
  project.path = tempDir.string();
  project.scriptsPath = (tempDir / "scripts").string();
  project.assetsPath = (tempDir / "assets").string();
  project.startScene = "node_2";

  EditorRuntimeHost host;
  auto loadResult = host.loadProject(project);
  if (!loadResult.isOk()) {
    std::cerr << "Load failed: " << loadResult.error() << "\n";
    return 1;
  }

  host.setOnSceneChanged([](const std::string &sceneId) {
    std::cout << "[SceneChanged] " << sceneId << "\n";
  });
  host.setOnDialogueChanged([](const std::string &speaker,
                               const std::string &text) {
    std::cout << "[Dialogue] speaker='" << speaker << "' text='" << text
              << "'\n";
  });

  auto playResult = host.play();
  if (!playResult.isOk()) {
    std::cerr << "Play failed: " << playResult.error() << "\n";
    return 1;
  }

  for (int i = 0; i < 6; ++i) {
    host.update(0.016);
    std::cout << "Tick " << i << " scene=" << host.getCurrentScene()
              << " state=" << static_cast<int>(host.getState()) << "\n";
    if (i == 1) {
      host.simulateClick();
    }
  }

  return 0;
}
