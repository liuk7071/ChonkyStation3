#include "GameWindow.hpp"


static constexpr double MS_PER_FRAME = 1000.0 / 60.0;

SDL_GameController* findController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

GameWindow::GameWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        Helpers::panic("Failed to initialize SDL\n");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

void GameWindow::run(PlayStation3* ps3) {
    this->ps3 = ps3;

    std::string title = "ChonkyStation3";
    window = SDL_CreateWindow(title.c_str(), 100, 100, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(0);

    if (context == nullptr)
        Helpers::panic("Failed to create OpenGL context\n");

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        Helpers::panic("OpenGL init failed");
    }

    if (ps3->curr_game.id != "") {
        title_game = ps3->curr_game.title;
    }
    else {
        title_game = ps3->elf_path.filename().generic_string();
    }
    title = std::format("ChonkyStation3 | {}", title_game);
    SDL_SetWindowTitle(window, title.c_str());

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
    while (!quit) {
        ps3->run();
        //ps3->vblank();
    }
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    return;
}

// Will be called on every RSX flip
void GameWindow::flipHandler() {
    frame_count++;

    const u64 curr_ticks = SDL_GetTicks64();
    curr_time = curr_ticks / 1000.0;

    std::string title;
    if (curr_time - last_time > 1.0) {
        //ppu_usage = ((CPU_FREQ - ps3->skipped_cycles) * 100.0f) / CPU_FREQ;
        ppu_usage = std::min(((ps3->scheduler.time - last_timestamp) * 100.0f) / CPU_FREQ, 100.0f);
        title = std::format("ChonkyStation3 | {} | {} FPS | PPU: {:.2f}%", title_game, frame_count, std::ceil(ppu_usage * 100.0f) / 100.0f);
        SDL_SetWindowTitle(window, title.c_str());
        last_time = curr_time;
        frame_count = 0;
    }
    last_timestamp = ps3->scheduler.time;

    ps3->resetButtons();

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

    SDL_GL_SwapWindow(window);
}