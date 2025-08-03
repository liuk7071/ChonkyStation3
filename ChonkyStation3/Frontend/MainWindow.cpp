#include "MainWindow.hpp"

MainWindow::MainWindow() : QMainWindow() {
    ps3 = new PlayStation3();
    game_loader = new GameLoader(ps3);
    game_window = new GameWindow(this);
    settings = new SettingsWidget(ps3);
    thread_debugger = new ThreadDebuggerWidget(ps3);
    ppu_debugger = new PPUDebuggerWidget(ps3, game_window);
    
    // Qt6 UI
    ui.setupUi(this);
    
    // Setup menubar buttons
    connect(ui.actionLaunch_Disc_Game, &QAction::triggered, this, &MainWindow::launchDiscGame);
    connect(ui.actionOpen_ELF, &QAction::triggered, this, &MainWindow::launchELF);
    
    connect(ui.actionSystem, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(0);
        settings->show();
    });

    connect(ui.actionFilesystem, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(1);
        settings->show();
    });

    connect(ui.actionLLE, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(2);
        settings->show();
        });

    connect(ui.actionPPU, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(3);
        settings->show();
    });

    connect(ui.actionSPU, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(4);
        settings->show();
    });

    connect(ui.actionRSX, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(5);
        settings->show();
    });

    connect(ui.actionAudio, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(6);
        settings->show();
    });
    
    connect(ui.actionDebug, &QAction::triggered, this, [this]() {
        settings->ui.tabWidget->setCurrentIndex(7);
        settings->show();
    });

    connect(ui.actionThread_Debugger, &QAction::triggered, this, [this]() {
        thread_debugger->show();
    });

    connect(ui.actionPPU_Debugger, &QAction::triggered, this, [this]() {
        ppu_debugger->show();
    });
    
    connect(ui.actionReplay_RSX_Capture, &QAction::triggered, this, &MainWindow::replayRSXCapture);
    
    // Pause / Resume
    connect(ui.pauseButton, &QPushButton::clicked, this, [this]() {
        pause();
    });
    
    // Timer to periodically check if some event other than user input caused the emulator to pause (i.e. a breakpoint)
    connect(&timer, &QTimer::timeout, this, [this]() {
        if (game_window->in_pause && !is_paused) {
            is_paused = true;
            ui.pauseButton->setText(tr("Resume"));
            enableWidgets();
            timer.stop();
        }
    });
    timer.setInterval(100);
    timer.start();
    
    int row = 0;
    int column = 0;

    // Setup the table widget
    ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui.tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    ui.tableWidget->setRowCount(game_loader->games.size());
    ui.tableWidget->setColumnCount(4);
    for (int i = 0; i < ui.tableWidget->rowCount(); i++)
        ui.tableWidget->setRowHeight(i, 60);
    ui.tableWidget->setColumnWidth(0, 100);
    ui.tableWidget->setColumnWidth(1, 3000);
    ui.tableWidget->setColumnWidth(2, 100);
    ui.tableWidget->setColumnWidth(3, 100);
    ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui.tableWidget->setHorizontalHeaderLabels(QStringList({ tr("Icon"), tr("Title"), "ID", tr("Version")}));

    connect(ui.tableWidget, &QTableWidget::cellClicked, this, [this](int row, int column) {
        curr_selection = row;
        updateBackgroundImage();
    });
    
    connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        curr_selection = row;
        loadAndLaunchGame(row);
    });

    // Populate table
    for (int i = 0; i < game_loader->games.size(); i++) {
        setListIcon(i, ps3->fs.guestPathToHost(game_loader->games[i].content_path / "ICON0.PNG"));
        setListItem(i, 1, game_loader->games[i].title);
        setListItem(i, 2, game_loader->games[i].id);
        if (game_loader->games[i].sfo.strings.contains("VERSION"))
            setListItem(i, 3, (char*)game_loader->games[i].sfo.strings["VERSION"].c_str());
        else
            setListItem(i, 3, tr("Unknown").toStdString());
    }

    QPalette palette;
    palette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
    setPalette(palette);

    // Show a dialog if the config file was broken
    if (ps3->settings.detected_broken_config) {
        QMessageBox* dialog = new QMessageBox();
        dialog->setText(tr("Detected outdated or broken config file, a new one was created.\n"));
        dialog->exec();
    }

    resize(1280, 720);
    setWindowTitle("ChonkyStation3");
    show();
}

void MainWindow::setListItem(int row, int column, std::string str) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(widget);

    layout->setAlignment(Qt::AlignVCenter);
    label->setStyleSheet("color: white; font-size: 16px;");

    label->setText(QString::fromStdString(str));
    layout->addWidget(label);
    widget->setLayout(layout);

    ui.tableWidget->setItem(row, column, item);
    ui.tableWidget->setCellWidget(row, column, widget);
}

void MainWindow::setListIcon(int row, fs::path icon) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(widget);
    
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);
    if (fs::exists(icon)) {
        QImage image = QImage(QString::fromStdString(icon.generic_string())).scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label->setPixmap(QPixmap::fromImage(image));
    }
    else {
        label->setText("No icon");
    }

    layout->addWidget(label);
    widget->setLayout(layout);

    ui.tableWidget->setItem(row, 0, item);
    ui.tableWidget->setCellWidget(row, 0, widget);
}

// Enable widgets that have a DisabledWidgetOverlay
void MainWindow::enableWidgets() {
    ppu_debugger->enable();
};

// Disable widgets that have a DisabledWidgetOverlay
void MainWindow::disableWidgets() {
    ppu_debugger->disable();
};

bool MainWindow::ensureGameNotRunning() {
    if (is_game_running) {
        QMessageBox* dialog = new QMessageBox();
        dialog->setText(tr("Close the current game before launching a new one!"));
        dialog->exec();
    }
    return is_game_running;
}

void MainWindow::launchDiscGame() {
    if (ensureGameNotRunning()) return;

    const fs::path path = QFileDialog::getExistingDirectory(this, tr("Select a PlayStation3 Disc Game"), ".").toStdString();
    if (!path.empty()) {
        ps3->fs.mount(Filesystem::Device::DEV_BDVD, path);
        if (!game_loader->isDiscGameOK()) {
            ps3->fs.umount(Filesystem::Device::DEV_BDVD);

            QMessageBox* dialog = new QMessageBox();
            dialog->setText("The Disc Game is invalid");
            dialog->exec();
            return;
        }

        ps3->loadGame(game_loader->getDiscGame());
        launchGame();
    }
}

void MainWindow::launchELF() {
    if (ensureGameNotRunning()) return;

    const fs::path path = QFileDialog::getOpenFileName(this, "Select a PlayStation3 ELF", "", "ELF File (*.elf)").toStdString();
    if (!path.empty()) {
        ps3->elf_path = path;
        launchGame();
    }
}

void MainWindow::replayRSXCapture() {
    if (ensureGameNotRunning()) return;

    const fs::path path = QFileDialog::getExistingDirectory(this, "Select a ChonkyStation3 RSX Capture folder", ".").toStdString();
    if (!path.empty()) {
        ps3->rsx_capture_path = path;
        launchGame();
    }
}

void MainWindow::loadAndLaunchGame(int idx) {
    if (ensureGameNotRunning()) return;
    Helpers::debugAssert(idx >= 0 && idx < game_loader->games.size(), "MainWindow::loadAndLaunchGame: invalid idx");

    ps3->loadGame(game_loader->games[idx]);
    launchGame();
}

void MainWindow::updateBackgroundImage() {
    if (!game_loader) return;
    if (curr_selection < 0 || curr_selection >= game_loader->games.size()) return;

    fs::path local_content_path = ps3->fs.guestPathToHost(game_loader->games[curr_selection].content_path);
    QPalette palette;
    if (fs::exists(local_content_path / "PIC1.PNG")) {
        fs::path image_path = local_content_path / "PIC1.PNG";
        QImage image = QImage(QString::fromStdString(image_path.generic_string()));
        QPixmap pix = QPixmap::fromImage(image).scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPixmap final_pix = QPixmap(size());
        final_pix.fill(Qt::transparent);
        QPainter painter = QPainter(&final_pix);
        painter.setOpacity(0.3);
        painter.drawPixmap(0, 0, pix);
        palette.setBrush(QPalette::Base, QBrush(final_pix));
    }

    palette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
    setPalette(palette);
}

void MainWindow::launchGame() {
    ps3->init();
    game_thread = std::thread(&MainWindow::gameThread, this);
    game_thread.detach();
    is_game_running = true;
    if (ps3->settings.debug.pause_on_start)
        pause();
}

void MainWindow::gameThread() {
    game_window->run(ps3, !ps3->rsx_capture_path.empty());

    is_game_running = false;
    delete ps3;
    ps3 = new PlayStation3();
}

void MainWindow::pause() {
    if (is_game_running) {
        timer.stop();
        
        if (!is_paused) {
            // The game thread will check the atomic variable below on every RSX flip to know if we requested to pause the emulator.
            // We can't assume the game thread will pause instantly, we also can't assume it will ever pause if no RSX flip happens at all
            game_window->paused = true;
            // Wait until the game flips & pauses, or timeout if no flip happens (meaning we failed to pause)
            // We wait 100ms for 100 times (== 10s timeout)
            int cnt = 0;
            bool timeout = false;
            while (!game_window->in_pause) {
                if (cnt == 100) {
                    timeout = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cnt++;
            }
            
            if (!timeout) {
                is_paused = true;
                ui.pauseButton->setText(tr("Resume"));
                enableWidgets();
            } else {    // Did we timeout?
                game_window->paused = false;
                game_window->pause_sema.release();
                timer.start();
            };
        } else {
            game_window->paused = false;
            game_window->pause_sema.release();
            is_paused = false;
            ui.pauseButton->setText(tr("Pause"));
            disableWidgets();
            timer.start();
        }
    }
}

void MainWindow::createGameWindow() {
    game_window->createWindow();
}

void MainWindow::updateGameWindow() {
    game_window->updateWindow();
}

void MainWindow::destroyGameWindow() {
    game_window->destroyWindow();
}

void MainWindow::pollGameWindowInput() {
    game_window->pollInput();
}

void MainWindow::onExit() {}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateBackgroundImage();
}
