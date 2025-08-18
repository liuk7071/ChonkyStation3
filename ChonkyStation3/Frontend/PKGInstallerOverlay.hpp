#pragma once

#include <common.hpp>

#include "PlayStation3.hpp"
#include <Loaders/PKG/PKGInstaller.hpp>

#include <QtWidgets>


class PKGInstallerOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double opacity READ getOpacity WRITE setOpacity)

public:
    PKGInstallerOverlay(PlayStation3* ps3, QWidget* parent = nullptr);
    PlayStation3* ps3;
    fs::path pkg_path;

    double getOpacity() const { return opacity; }
    void setOpacity(double o) {
        o = qBound<double>(0.0, o, 1.0);
        if (qFuzzyCompare(opacity + 1.0, o + 1.0)) return;
        opacity = o;
        update();
    }

    void install(fs::path pkg_path, std::function<void(int)> on_complete);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void showPopup();
    void onGameLoaded();

private:
    PKGInstaller* pkg;
    std::function<void(int)> on_complete;
    bool done = false;
    double opacity = 0.0;
    
    QPropertyAnimation* anim = nullptr;
    QWidget* loading = nullptr;
    QLabel* title_label;
    QLabel* info_label;
    
    QPropertyAnimation* moveAnim(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration);
    QPropertyAnimation* fadeIn(QWidget* obj, int duration);
    QPropertyAnimation* fadeOut(QWidget* obj, int duration);
    QParallelAnimationGroup* moveAndFadeIn(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration, QEasingCurve move_curve = QEasingCurve::OutCubic);
    QParallelAnimationGroup* moveAndFadeOut(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration);
};
