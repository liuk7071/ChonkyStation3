#include "SettingsWidget.hpp"


SettingsWidget::SettingsWidget(PlayStation3* ps3, QWidget* parent) : QWidget(parent, Qt::Window), ps3(ps3) {
    ui.setupUi(this);

    // Initialize settings
    
    // System
    ui.nickname->setText(QString::fromStdString(ps3->settings.system.nickname));

    // Filesystem
    ui.dev_hdd0_mountpoint->setText(QString::fromStdString(ps3->settings.filesystem.dev_hdd0_mountpoint));
    ui.dev_hdd1_mountpoint->setText(QString::fromStdString(ps3->settings.filesystem.dev_hdd1_mountpoint));
    ui.dev_flash_mountpoint->setText(QString::fromStdString(ps3->settings.filesystem.dev_flash_mountpoint));
    ui.dev_usb000_mountpoint->setText(QString::fromStdString(ps3->settings.filesystem.dev_usb000_mountpoint));

    // LLE
    ui.partialLv2->setChecked(ps3->settings.lle.partialLv2LLE);
    ui.sys_fs->setChecked(ps3->settings.lle.sys_fs);
    ui.cellResc->setChecked(ps3->settings.lle.cellResc);
    ui.cellPngDec->setChecked(ps3->settings.lle.cellPngDec);
    ui.cellJpgDec->setChecked(ps3->settings.lle.cellJpgDec);
    ui.cellSpurs->setChecked(ps3->settings.lle.cellSpurs);
    ui.cellSpursJq->setChecked(ps3->settings.lle.cellSpursJq);
    
    // Audio
    ui.audioBackend->setCurrentIndex(ui.audioBackend->findData(ps3->settings.audio.backend.c_str(), Qt::DisplayRole));

    // Debug
    ui.pauseOnStart->setChecked(ps3->settings.debug.pause_on_start);
    ui.disableSPU->setChecked(ps3->settings.debug.disable_spu);
    ui.enableSPUAfterPC->setText(QString::fromStdString(ps3->settings.debug.enable_spu_after_pc));
    ui.spuThreadToEnable->setText(QString::fromStdString(ps3->settings.debug.spu_thread_to_enable));
    ui.dontStepCellAudioPortReadIndex->setChecked(ps3->settings.debug.dont_step_cellaudio_port_read_idx);

    // Setup events
    connect(ui.apply, &QPushButton::clicked, this, [this, ps3]() {
        ps3->settings.system.nickname   = ui.nickname->text().toStdString();

        ps3->settings.filesystem.dev_hdd0_mountpoint   = ui.dev_hdd0_mountpoint->text().toStdString();
        ps3->settings.filesystem.dev_hdd1_mountpoint   = ui.dev_hdd1_mountpoint->text().toStdString();
        ps3->settings.filesystem.dev_flash_mountpoint  = ui.dev_flash_mountpoint->text().toStdString();
        ps3->settings.filesystem.dev_usb000_mountpoint = ui.dev_usb000_mountpoint->text().toStdString();

        ps3->settings.lle.partialLv2LLE = ui.partialLv2->isChecked();
        ps3->settings.lle.sys_fs        = ui.sys_fs->isChecked();
        ps3->settings.lle.cellResc      = ui.cellResc->isChecked();
        ps3->settings.lle.cellPngDec    = ui.cellPngDec->isChecked();
        ps3->settings.lle.cellJpgDec    = ui.cellJpgDec->isChecked();
        ps3->settings.lle.cellSpurs     = ui.cellSpurs->isChecked();
        ps3->settings.lle.cellSpursJq   = ui.cellSpursJq->isChecked();
        
        ps3->settings.audio.backend = ui.audioBackend->currentText().toStdString();

        ps3->settings.debug.pause_on_start                      = ui.pauseOnStart->isChecked();
        ps3->settings.debug.disable_spu                         = ui.disableSPU->isChecked();
        ps3->settings.debug.enable_spu_after_pc                 = ui.enableSPUAfterPC->text().toStdString();
        ps3->settings.debug.spu_thread_to_enable                = ui.spuThreadToEnable->text().toStdString();
        ps3->settings.debug.dont_step_cellaudio_port_read_idx   = ui.dontStepCellAudioPortReadIndex->isChecked();

        ps3->settings.save();
        
        // Update the backends
        ps3->createAudioDevice();
    });

    setWindowTitle("Settings");
    hide();
}
