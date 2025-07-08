#pragma once

#include <common.hpp>

#include <string>
#include <format>

#if defined(CHONKYSTATION3_QT_BUILD) && defined(__APPLE__)
#include <QtWidgets>
#include <semaphore>    // semaphore and atomic are for pausing
#include <atomic>
#endif

#include <SDL.h>
#include <PlayStation3.hpp>


// Circular dependency
class MainWindow;

class GameWindow {
public:
    GameWindow(MainWindow* main_window = nullptr);
    void init();
    void run(PlayStation3* ps3, bool is_rsx_replay = false);
    void flipHandler();
    
    void createWindow();
    void updateWindow();
    void destroyWindow();
    void pollInput();

    PlayStation3* ps3;
    MainWindow* main_window;
    std::string title_game;
    SDL_Window* window;
    SDL_GLContext context;
    SDL_GameController* controller;
    
#ifdef CHONKYSTATION3_QT_BUILD
    std::binary_semaphore pause_sema;
    std::atomic<bool> paused = false;
    std::atomic<bool> in_pause = false;
#endif

    bool quit = false;
    bool fullscreen = false;
    bool vsync_enabled = false;
    int frame_count = 0;
    double last_time = 0;
    double curr_time = 0;
    float ppu_usage = 0;
    u64 last_timestamp = 0;
};
