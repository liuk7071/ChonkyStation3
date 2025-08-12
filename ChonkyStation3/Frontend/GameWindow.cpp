#include "GameWindow.hpp"
#if defined(CHONKYSTATION3_QT_BUILD) && defined(__APPLE__)
#include "MainWindow.hpp"
#endif


static constexpr double MS_PER_FRAME = 1000.0 / 60.0;

SDL_GameController* findController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

GameWindow::GameWindow(MainWindow* main_window) : main_window(main_window)
#ifdef CHONKYSTATION3_QT_BUILD
    , pause_sema(0)
#endif
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        Helpers::panic("Failed to initialize SDL\n");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

void GameWindow::init() {
    window = SDL_CreateWindow("ChonkyStation3", 100, 100, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        Helpers::panic("Failed to create SDL window: %s\n", SDL_GetError());
    }
    
    context = SDL_GL_CreateContext(window);
    if (context == nullptr)
        Helpers::panic("Failed to create OpenGL context: %s\n", SDL_GetError());
}

void GameWindow::run(PlayStation3* ps3, bool is_rsx_replay) {
    this->ps3 = ps3;
    std::string title = "ChonkyStation3";

#ifdef CHONKYSTATION3_QT_BUILD
    if (ps3->settings.debug.pause_on_start) {
        while (!paused);
        pause(false);
    }
#endif
    
#if defined(CHONKYSTATION3_QT_BUILD) && defined(__APPLE__)
    QMetaObject::invokeMethod(main_window, "createGameWindow", Qt::BlockingQueuedConnection);
#else
    createWindow();
#endif
    
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(0);
    
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        Helpers::panic("OpenGL init failed");
    }

    if (ps3->curr_game.id != "") {
        title_game = ps3->curr_game.title;
    }
    else if (is_rsx_replay) {
        title_game = "RSX Capture Replay";
    }
    else {
        title_game = ps3->elf_path.filename().generic_string();
    }
    title = std::format("ChonkyStation3 | {}", title_game);
#ifndef __APPLE__
    SDL_SetWindowTitle(window, title.c_str());
#endif
    
    printf("\nEXECUTING\n");
    printf("---------\n\n");

    quit = false;
    fullscreen = false;
    vsync_enabled = false;

    frame_count = 0;
    last_time = SDL_GetTicks64() / 1000.0;
    curr_time = 0;
    ppu_usage = 0;

    SDL_GL_SwapWindow(window);

    controller = findController();

    ps3->setFlipHandler(std::bind(&GameWindow::flipHandler, this));
    ps3->rsx.initGL();

    if (!is_rsx_replay) {
        while (!quit) {
            ps3->run();
            //ps3->vblank();
        }
    }
    else {
        RSXCaptureReplayer* capture = new RSXCaptureReplayer(ps3);

        try {
            capture->load(ps3->rsx_capture_path);
        }
        catch (std::runtime_error e) {
            printf("FATAL: %s\n", e.what());
            //exit(0);
        }

        while (!quit) flipHandler();
    }
    
#if defined(CHONKYSTATION3_QT_BUILD) && defined(__APPLE__)
    QMetaObject::invokeMethod(main_window, "destroyGameWindow", Qt::AutoConnection);
#else
    destroyWindow();
#endif
    return;
}

// Will be called on every RSX flip
void GameWindow::flipHandler() {
#ifdef CHONKYSTATION3_QT_BUILD
    // Avoid running this function recursively while we are paused.
    // This will happen if we trigger another RSX flip while single stepping the emulator.
    // We (or well, at least I) don't care about updating the screen while using the PPU debugger, so it's fine
    if (in_pause) {
        return;
    }
    
    if (paused) {
        pause(true);
    }
#endif
    
#if defined(CHONKYSTATION3_QT_BUILD) && defined(__APPLE__)
    QMetaObject::invokeMethod(main_window, "updateGameWindow", Qt::AutoConnection);
    QMetaObject::invokeMethod(main_window, "pollGameWindowInput", Qt::AutoConnection);
#else
    updateWindow();
    pollInput();
#endif

    SDL_GL_SwapWindow(window);
}

#ifdef CHONKYSTATION3_QT_BUILD

void GameWindow::pause(bool handle_pc) {
    if (in_pause) {
        return;
    }
    
    in_pause = true;
    
    // For context: we check if we need to pause the emulator on every RSX flip.
    // Because the flip is triggered directly by a store instruction to the RSX fifo control, we are running this function
    // before the store instruction ends and PC is incremented.
    // We want to begin stepping from the instruction after, so we increment PC.
    // We decrement it when we unpause because it will be incremented back by the PPU after we return from this function.
    // TODO: This breaks if the instruction that triggered the RSX flip isn't a plain store word. If, for example, it's a store
    // word with update, the "update" part will happen *after* we unpause. Meaning that any instructions we step through while paused
    // won't have the correct state.
    // In 99% of cases it's going to be a plain STW so it's fine, but keep that in mind.
    // This whole thing + the PC incrementing/decrementing won't be an issue when I put the RSX on its own thread
    // (but, I'll have to rethink the way I handle pausing entirely...).
    if (handle_pc) ps3->ppu->state.pc += 4;
    
    while (true) {
        // Will be signalled from the Qt thread when we either unpaused or requested to step the emulator
        pause_sema.acquire();
        // Did we unpause?
        if (!paused) {
            in_pause = false;
            break;
        }
        
        // Step the emulator
        ps3->ppu->should_break = true;
        ps3->step();
        // Set the "step completed flag". The Qt thread waits on this to know when the step is done, and clears it afterwards
        stepped = true;
    }
    
    if (handle_pc) ps3->ppu->state.pc -= 4;    // See big comment above
}

void GameWindow::breakpoint() {
    paused = true;
    pause(false);   // For breakpoints the pause does not happen in the middle of an instruction
}

void GameWindow::breakOnNextInstr(u64 addr) {
    ps3->scheduler.push(std::bind(&GameWindow::breakpoint, this), 0, "breakpoint");
    ps3->ppu->should_break = true;
}

void GameWindow::breakOnNextInstrIfExec(u64 addr) {
    if (ps3->ppu->state.pc == addr) breakOnNextInstr(addr);
}

#endif

void GameWindow::createWindow() {
    init();
}

void GameWindow::updateWindow() {
    frame_count++;

    const u64 curr_ticks = SDL_GetTicks64();
    curr_time = curr_ticks / 1000.0;

    std::string title;
    if (curr_time - last_time > 1.0) {
        ppu_usage = std::min(((ps3->scheduler.time - last_timestamp) * 100.0f) / CPU_FREQ, 100.0f);
        title = std::format("ChonkyStation3 | {} | {} FPS | PPU: {:.2f}%", title_game, frame_count, std::ceil(ppu_usage * 100.0f) / 100.0f);
        SDL_SetWindowTitle(window, title.c_str());
        last_time = curr_time;
        frame_count = 0;
    }
    last_timestamp = ps3->scheduler.time;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: {
            quit = true;
            ps3->cycle_count = CPU_FREQ;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            if (e.button.button == SDL_BUTTON_LEFT && e.button.clicks == 2) {
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
                SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
            }
            else if (e.button.button == SDL_BUTTON_RIGHT) {
                vsync_enabled = !vsync_enabled;
                SDL_GL_SetSwapInterval(vsync_enabled ? 1 : 0);
            }
            break;
        }

        case SDL_CONTROLLERDEVICEADDED: {
            if (!controller) {
                controller = SDL_GameControllerOpen(e.cdevice.which);
            }
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            if (controller && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                SDL_GameControllerClose(controller);
                controller = findController();
            }
            break;
        }
        }
    }
}

void GameWindow::destroyWindow() {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
}

void GameWindow::pollInput() {
    ps3->resetButtons();
    if (!controller) {
        const u8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_M])      ps3->pressButton(CELL_PAD_CTRL_START);
        if (keystate[SDL_SCANCODE_K])      ps3->pressButton(CELL_PAD_CTRL_CROSS);
        if (keystate[SDL_SCANCODE_L])      ps3->pressButton(CELL_PAD_CTRL_CIRCLE);
        if (keystate[SDL_SCANCODE_J])      ps3->pressButton(CELL_PAD_CTRL_SQUARE);
        if (keystate[SDL_SCANCODE_I])      ps3->pressButton(CELL_PAD_CTRL_TRIANGLE);
        if (keystate[SDL_SCANCODE_DOWN])   ps3->pressButton(CELL_PAD_CTRL_DOWN);
        if (keystate[SDL_SCANCODE_UP])     ps3->pressButton(CELL_PAD_CTRL_UP);
        if (keystate[SDL_SCANCODE_LEFT])   ps3->pressButton(CELL_PAD_CTRL_LEFT);
        if (keystate[SDL_SCANCODE_RIGHT])  ps3->pressButton(CELL_PAD_CTRL_RIGHT);
        ps3->setLeftStick(0.5f, 0.5f);
        ps3->setRightStick(0.5f, 0.5f);
    }
    else {
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START))             ps3->pressButton(CELL_PAD_CTRL_START);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A))                 ps3->pressButton(CELL_PAD_CTRL_CROSS);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B))                 ps3->pressButton(CELL_PAD_CTRL_CIRCLE);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X))                 ps3->pressButton(CELL_PAD_CTRL_SQUARE);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y))                 ps3->pressButton(CELL_PAD_CTRL_TRIANGLE);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN))         ps3->pressButton(CELL_PAD_CTRL_DOWN);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP))           ps3->pressButton(CELL_PAD_CTRL_UP);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT))         ps3->pressButton(CELL_PAD_CTRL_LEFT);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT))        ps3->pressButton(CELL_PAD_CTRL_RIGHT);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER))      ps3->pressButton(CELL_PAD_CTRL_L1);
        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))     ps3->pressButton(CELL_PAD_CTRL_R1);
        if (SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 20000)     ps3->pressButton(CELL_PAD_CTRL_L2);
        if (SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 20000)    ps3->pressButton(CELL_PAD_CTRL_R2);
        float leftX   = ((float)SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX)  / 32767.0f + 1.0f) / 2.0f;
        float leftY   = ((float)SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY)  / 32767.0f + 1.0f) / 2.0f;
        float rightX  = ((float)SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f + 1.0f) / 2.0f;
        float rightY  = ((float)SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f + 1.0f) / 2.0f;
        // Deadzone
        leftX = (leftX < 0.4f || leftX > 0.6f) ? leftX : 0.5f;
        leftY = (leftY < 0.4f || leftY > 0.6f) ? leftY : 0.5f;
        rightX = (rightX < 0.4f || rightX > 0.6f) ? rightX : 0.5f;
        rightY = (rightY < 0.4f || rightY > 0.6f) ? rightY : 0.5f;
        ps3->setLeftStick(leftX, leftY);
        ps3->setRightStick(rightX, rightY);
    }
}
