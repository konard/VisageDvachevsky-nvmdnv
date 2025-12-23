// Debug test to understand timeout issue
#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "NovelMind/editor/editor_runtime_host.hpp"

using namespace NovelMind::editor;
namespace fs = std::filesystem;

const char* SCRIPT_WITH_VARIABLES = R"(
character Hero(name="Hero", color="#FF0000")

scene intro {
    set points = 0
    set flag visited = false
    say Hero "Starting adventure..."
    set points = points + 10
    set flag visited = true
    goto ending
}

scene ending {
    if points > 5 {
        say Hero "You scored high!"
    }
}
)";

std::filesystem::path createTempDir()
{
    auto tempDir = std::filesystem::temp_directory_path() / "nm_debug_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::create_directories(tempDir / "scripts");
    std::filesystem::create_directories(tempDir / "assets");
    return tempDir;
}

void cleanupTempDir(const std::filesystem::path& path)
{
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove_all(path);
    }
}

void writeTestScript(const std::filesystem::path& dir, const std::string& content)
{
    std::ofstream file(dir / "scripts" / "main.nms");
    file << content;
    file.close();
}

int main() {
    std::cout << "Starting debug test..." << std::endl;

    auto tempDir = createTempDir();
    writeTestScript(tempDir, SCRIPT_WITH_VARIABLES);

    std::cout << "Created temp dir: " << tempDir << std::endl;

    EditorRuntimeHost host;
    host.setAutoHotReload(false);
    std::cout << "Created host, disabled hot reload" << std::endl;

    ProjectDescriptor project;
    project.name = "TestProject";
    project.path = tempDir.string();
    project.scriptsPath = (tempDir / "scripts").string();
    project.assetsPath = (tempDir / "assets").string();
    project.startScene = "intro";

    std::cout << "Loading project..." << std::endl;
    auto loadResult = host.loadProject(project);
    if (!loadResult.isOk()) {
        std::cerr << "Failed to load project: " << loadResult.error() << std::endl;
        cleanupTempDir(tempDir);
        return 1;
    }
    std::cout << "Project loaded successfully" << std::endl;

    std::cout << "Starting playback..." << std::endl;
    auto playResult = host.play();
    if (!playResult.isOk()) {
        std::cerr << "Failed to play: " << playResult.error() << std::endl;
        cleanupTempDir(tempDir);
        return 1;
    }
    std::cout << "Playback started" << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::cout << "Loop iteration " << i << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "  Calling update..." << std::flush;
        host.update(0.1);
        auto updateEnd = std::chrono::high_resolution_clock::now();
        auto updateMs = std::chrono::duration_cast<std::chrono::milliseconds>(updateEnd - start).count();
        std::cout << " done (" << updateMs << "ms)" << std::endl;

        std::cout << "  Calling simulateClick..." << std::flush;
        host.simulateClick();
        auto clickEnd = std::chrono::high_resolution_clock::now();
        auto clickMs = std::chrono::duration_cast<std::chrono::milliseconds>(clickEnd - updateEnd).count();
        std::cout << " done (" << clickMs << "ms)" << std::endl;
    }

    std::cout << "Loop completed successfully!" << std::endl;
    cleanupTempDir(tempDir);
    return 0;
}
