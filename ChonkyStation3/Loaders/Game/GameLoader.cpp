#include "GameLoader.hpp"
#include "PlayStation3.hpp"


GameLoader::GameLoader(PlayStation3* ps3) : ps3(ps3) {
    refresh();
}

void GameLoader::refresh() {
    const auto game_dir = ps3->fs.guestPathToHost("/dev_hdd0/game");
    SFOLoader sfo_loader = SFOLoader(ps3->fs);
    games.clear();

    // Register installed games
    for (auto& i : fs::directory_iterator(game_dir)) {
        // Sometimes the path will contain weird characters that break .generic_string();
        // Safe to say that if our path contains any of those characters it's not a game.
        try {
            i.path().generic_string();
        }
        catch (...) {
            continue;
        }

        if (i.is_directory()) {
            const fs::path sfo_path = "/dev_hdd0/game" / i.path().filename() / "PARAM.SFO";
            if (ps3->fs.exists(sfo_path)) {
                auto sfo = sfo_loader.parse(sfo_path);
                if (sfo.ints["BOOTABLE"] == 1) {
                    const auto title = std::string(reinterpret_cast<const char*>(sfo.strings["TITLE"].c_str()));    // Convert u8string to string
                    const auto id = std::string(reinterpret_cast<const char*>(sfo.strings["TITLE_ID"].c_str()));
                    const fs::path content_path = "/dev_hdd0/game" / i.path().filename();
                    games.push_back({ title, id, content_path, sfo });
                }
            }
        }
    }
}

bool GameLoader::isDiscGameOK() {
    // TODO: more checks
    if (!ps3->fs.exists("/dev_bdvd/PS3_GAME")) return false;
    if (!ps3->fs.exists("/dev_bdvd/PS3_GAME/PARAM.SFO")) return false;
    return true;
}

GameLoader::InstalledGame GameLoader::getDiscGame() {
    if (!isDiscGameOK()) Helpers::panic("GameLoader: invalid disc game\n");

    SFOLoader sfo_loader = SFOLoader(ps3->fs);
    const fs::path sfo_path = "/dev_bdvd/PS3_GAME/PARAM.SFO";
    auto sfo = sfo_loader.parse(sfo_path);
    if (sfo.ints["BOOTABLE"] == 1) {
        const auto title = std::string(reinterpret_cast<const char*>(sfo.strings["TITLE"].c_str()));    // Convert u8string to string
        const auto id = std::string(reinterpret_cast<const char*>(sfo.strings["TITLE_ID"].c_str()));
        const fs::path content_path = "/dev_bdvd/PS3_GAME";
        return { title, id, content_path, sfo };
    }
    else {
        Helpers::panic("GameLoader: disc game is not bootable\n");
    }
}
