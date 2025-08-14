#pragma once

#include <common.hpp>

#include <atomic>
#include <thread>

#include <Frontend/GameWindow.hpp>
#include "Frontend/UI/ui_mainwindow.h"
#include <QtWidgets>
#include <PlayStation3.hpp>
#include <Loaders/Game/GameLoader.hpp>

#include "AboutWindow.hpp"
#include "SettingsWidget.hpp"
#include "ThreadDebuggerWidget.hpp"
#include "PPUDebuggerWidget.hpp"


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    Ui::Main ui;
    SettingsWidget* settings;
    ThreadDebuggerWidget* thread_debugger;
    PPUDebuggerWidget* ppu_debugger;
    AboutWindow* about_window;
    
    void setListItem(int row, int column, std::string str);
    void setListIcon(int row, fs::path icon);
    void enableWidgets();
    void disableWidgets();

    int curr_selection = -1;
    std::atomic<bool> is_game_running = false;
    bool is_paused = false;
    bool ensureGameNotRunning();

    // Actions
    void launchDiscGame();
    void launchELF();
    void replayRSXCapture();
    void loadAndLaunchGame(int idx);
    void updateBackgroundImage();

    // Exit callback
    void onExit();

    PlayStation3* ps3;
    GameLoader* game_loader;
    GameWindow* game_window;
    std::thread game_thread;
    void launchGame();
    void gameThread();
    void pause();
    
public slots:
    // MacOS-only functions to manage the SDL window on the main thread
    void createGameWindow();
    void updateGameWindow();
    void destroyGameWindow();
    void pollGameWindowInput();

private:
    QTimer timer;
    
    // Overrides
    void resizeEvent(QResizeEvent* event) override;
};
