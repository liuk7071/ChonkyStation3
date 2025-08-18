#include "PKGInstallerOverlay.hpp"


class LoadingBar : public QWidget {
public:
    LoadingBar(QWidget* parent = nullptr) : QWidget(parent), anim(new QVariantAnimation(this)) {
        setMinimumHeight(8);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        track_color = QColor(255,255,255,40);
        bar_color   = QColor(255,255,255);

        anim->setEasingCurve(QEasingCurve::OutCubic);

        connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v){
            display = v.toReal();
            update();
        });
    }

    void setProgress(int p) {
        p = qBound(0, p, 100);
        if (p == target_progress) return;
        target_progress = p;

        double start = display;
        double target = target_progress;
        if (qFuzzyCompare(start, target)) return;

        if (anim->state() == QAbstractAnimation::Running)
            anim->stop();

        double delta = std::abs(target - start);
        int duration = std::max<int>(60, (int)(delta * ms_per_percent));

        anim->setStartValue(start);
        anim->setEndValue(target);
        anim->setDuration(duration);
        anim->start();
    }

    int progress() { return target_progress; }
    void setBarColor(const QColor &c)   { bar_color = c;        update();   }
    void setTrackColor(const QColor &c) { track_color = c;      update();   }
    void setSpeedPerPercent(int ms)     { ms_per_percent = ms;              }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int w = width();
        const int h = height();
        const double r = h / 2.0;

        QRectF track = QRectF(0.5, 0.5, w - 1.0, h - 1.0);
        p.setPen(Qt::NoPen);
        p.setBrush(track_color);
        p.drawRoundedRect(track, r, r);

        double frac = qBound<double>(0.0, display / 100.0, 1.0);
        double bar_w = frac * (w - 1.0);
        if (bar_w > 0.0) {
            QRectF barRect(track.left(), track.top(), bar_w, track.height());
            p.setBrush(bar_color);
            p.drawRoundedRect(barRect, r, r);
        }
    }

private:
    double display = 0.0;
    int target_progress = 0;
    int ms_per_percent = 10;
    QColor track_color;
    QColor bar_color;
    QVariantAnimation* anim;
};

PKGInstallerOverlay::PKGInstallerOverlay(PlayStation3* ps3, QWidget* parent) : ps3(ps3), QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    opacity = 0.0;
    hide();
}

void PKGInstallerOverlay::install(fs::path pkg_path, std::function<void(int)> on_complete) {
    this->pkg_path = pkg_path;
    this->on_complete = on_complete;
    
    show();
    raise();
    
    constexpr float duration = 500.0f;
    anim = new QPropertyAnimation(this, "opacity", this);
    anim->setDuration(duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QTimer::singleShot(duration * 0.60f, this, &PKGInstallerOverlay::showPopup);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void PKGInstallerOverlay::paintEvent(QPaintEvent* event) {
    if (opacity <= 0.0) return;
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, qRound(opacity * 255.0)));
}

void PKGInstallerOverlay::showPopup() {
    if (!loading) {
        loading = new QWidget(this);
        loading->setFixedSize(100, 100);
        loading->setStyleSheet("background-color: rgba(255, 255, 255, 80); border-radius: 20px;");
        loading->setAttribute(Qt::WA_TranslucentBackground);

        QLabel* img_label = new QLabel(loading);
        img_label->setAlignment(Qt::AlignCenter);

        QVBoxLayout* layout = new QVBoxLayout(loading);
        layout->addWidget(img_label);
        layout->setContentsMargins(0, 0, 0, 0);

        QPixmap img = QPixmap(":/Resources/Icons/loading.png").scaled(loading->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        img_label->setPixmap(img);
        
        QVariantAnimation* rot_anim = new QVariantAnimation();
        rot_anim->setStartValue(0.0f);
        rot_anim->setEndValue(360.0f);
        rot_anim->setDuration(500);
        connect(rot_anim, &QVariantAnimation::valueChanged, [=, this](const QVariant& val) {
            if (done) {
                rot_anim->stop();
                return;
            }
            QTransform t;
            t.rotate(val.toReal());
            img_label->setPixmap(img.transformed(t));
        });
        rot_anim->setLoopCount(-1); // Loop forever
        rot_anim->start();

        loading->setGraphicsEffect(new QGraphicsOpacityEffect(loading));
        loading->graphicsEffect()->setEnabled(true);
    }

    // Bring up loading icon
    QPoint start_pos = QPoint((width() - loading->width()) / 2, (height() - loading->height()) / 2 + 50);
    QPoint end_pos = QPoint((width() - loading->width()) / 2, (height() - loading->height()) / 2);
    loading->show();
    connect(moveAndFadeIn(loading, start_pos, end_pos, 400), &QAbstractAnimation::finished, [this]() {
        // Setup pkg installer
        if (!pkg) pkg = new PKGInstaller(ps3);
        pkg->load(pkg_path);
        
        // Move spinning icon down
        QPoint start_pos = loading->pos();
        QPoint end_pos = QPoint((width() - loading->width()) / 2, (height() - loading->height()) / 2 + 60);
        moveAnim(loading, start_pos, end_pos, 1000);
        
        // Display game title
        title_label = new QLabel(this);
        title_label->setStyleSheet("font-size: 24px; font-weight: bold;");
        title_label->setGraphicsEffect(new QGraphicsOpacityEffect(title_label));
        title_label->graphicsEffect()->setEnabled(true);
        title_label->setText(QString::fromStdString(pkg->title));
        title_label->adjustSize();
        title_label->setAlignment(Qt::AlignCenter);
        title_label->show();
        start_pos = QPoint((width() - title_label->width()) / 2, (height() - title_label->height()) / 2);
        end_pos = QPoint((width() - title_label->width()) / 2, (height() - title_label->height()) / 2 - 60);
        moveAndFadeIn(title_label, start_pos, end_pos, 500);
        
        // Display title ID and size
        
        // Get size string
        const float size_in_mb = pkg->size_in_bytes / 1024.0f / 1024.0f;
        std::string size_str;
        if (size_in_mb > 1024.0f)
            size_str = std::format("{:.2f} GB", size_in_mb / 1024.0f);
        else
            size_str = std::format("{:.2f} MB", size_in_mb);
        
        info_label = new QLabel(this);
        info_label->setStyleSheet("color: rgba(128, 128, 128, 255); font-size: 14px; font-weight: bold;");
        info_label->setGraphicsEffect(new QGraphicsOpacityEffect(title_label));
        info_label->graphicsEffect()->setEnabled(true);
        info_label->setText(QString::fromStdString(pkg->title_id + "\n" + size_str));
        info_label->adjustSize();
        info_label->setAlignment(Qt::AlignCenter);
        info_label->show();
        start_pos = QPoint((width() - info_label->width()) / 2, (height() - info_label->height()) / 2);
        end_pos = QPoint((width() - info_label->width()) / 2, (height() - info_label->height()) / 2 - 30);
        moveAndFadeIn(info_label, start_pos, end_pos, 500);
        
        // Get ICON0.PNG from the package and dump it to /dev_hdd1/pkg_installer
        pkg->getFileAsync("ICON0.PNG", "/dev_hdd1/pkg_installer", [&](bool ok) {
            if (!ok) {
                Helpers::panic("Could not get ICON0.PNG");
            }
            
            // Display game icon
            QMetaObject::invokeMethod(this, &PKGInstallerOverlay::onGameLoaded, Qt::AutoConnection);
        });
    });
}
    
void PKGInstallerOverlay::onGameLoaded() {
    QLabel* icon_label = new QLabel(this);
    icon_label->setAlignment(Qt::AlignCenter);
    icon_label->setFixedSize(250, 250);
    icon_label->setGraphicsEffect(new QGraphicsOpacityEffect(icon_label));
    icon_label->graphicsEffect()->setEnabled(true);
    
    // Show game icon
    const auto icon_path = ps3->fs.guestPathToHost("/dev_hdd1/pkg_installer/ICON0.PNG");
    QPixmap img = QPixmap(QString::fromStdString(icon_path.generic_string())).scaled(icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    icon_label->setPixmap(img);
    icon_label->show();
    QPoint start_pos = QPoint((width() - icon_label->width()) / 2, (height() - icon_label->height()) / 2);
    QPoint end_pos = QPoint((width() - icon_label->width()) / 2, (height() - icon_label->height()) / 2 - 175);
    moveAndFadeIn(icon_label, start_pos, end_pos, 750, QEasingCurve::OutBack);
    
    // Fade the loading icon out
    connect(fadeOut(loading, 500), &QAbstractAnimation::finished, [=, this]() {
        // Show confirmation text
        QLabel* text_label = new QLabel(this);
        text_label->setStyleSheet("font-size: 18px;");
        text_label->setGraphicsEffect(new QGraphicsOpacityEffect(text_label));
        text_label->graphicsEffect()->setEnabled(true);
        text_label->setText("Do you want to install this package?");
        text_label->adjustSize();
        text_label->setAlignment(Qt::AlignCenter);
        
        QPushButton* yes_btn = new QPushButton(this);
        yes_btn->setStyleSheet("font-size: 18px; font-weight: bold;");
        yes_btn->setGraphicsEffect(new QGraphicsOpacityEffect(yes_btn));
        yes_btn->graphicsEffect()->setEnabled(true);
        yes_btn->setText("Yes");
        yes_btn->adjustSize();
        QPushButton* no_btn = new QPushButton(this);
        no_btn->setStyleSheet("font-size: 18px; font-weight: bold;");
        no_btn->setGraphicsEffect(new QGraphicsOpacityEffect(no_btn));
        no_btn->graphicsEffect()->setEnabled(true);
        no_btn->setText("No");
        no_btn->adjustSize();
        
        text_label->show();
        QPoint start_pos = QPoint((width() - text_label->width()) / 2, (height() - text_label->height()) / 2 + 120);
        QPoint end_pos = QPoint((width() - text_label->width()) / 2, (height() - text_label->height()) / 2 + 60);
        connect(moveAndFadeIn(text_label, start_pos, end_pos, 400, QEasingCurve::OutBack), &QAbstractAnimation::finished, [=, this]() {
            yes_btn->show();
            QPoint start_pos = QPoint((width() - yes_btn->width()) / 2, (height() - yes_btn->height()) / 2 + 200);
            QPoint end_pos = QPoint((width() - yes_btn->width()) / 2, (height() - yes_btn->height()) / 2 + 100);
            connect(moveAndFadeIn(yes_btn, start_pos, end_pos, 350, QEasingCurve::OutBack), &QAbstractAnimation::finished, [=, this]() {
                no_btn->show();
                QPoint start_pos = QPoint((width() - no_btn->width()) / 2, (height() - no_btn->height()) / 2 + 200);
                QPoint end_pos = QPoint((width() - no_btn->width()) / 2, (height() - no_btn->height()) / 2 + 140);
                moveAndFadeIn(no_btn, start_pos, end_pos, 350, QEasingCurve::OutBack);
            });
        });
        
        connect(yes_btn, &QPushButton::clicked, [=, this]() {
            disconnect(yes_btn, &QPushButton::clicked, nullptr, nullptr);
            disconnect(no_btn, &QPushButton::clicked, nullptr, nullptr);
            // Fade out confirmation text and buttons
            fadeOut(text_label, 250);
            fadeOut(yes_btn,    250);
            fadeOut(no_btn,     250);
            
            LoadingBar* bar = new LoadingBar(this);
            bar->resize(150, 12);
            bar->setFixedHeight(12);
            bar->setTrackColor(QColor(255,255,255,50));
            bar->setBarColor(QColor(255,255,255));
            bar->setSpeedPerPercent(5);
            bar->setProgress(0);
            bar->setGraphicsEffect(new QGraphicsOpacityEffect(bar));
            bar->graphicsEffect()->setEnabled(true);
            bar->show();
            QPoint start_pos = QPoint((width() - bar->width()) / 2, (height() - bar->height()) / 2 + 100);
            QPoint end_pos = QPoint((width() - bar->width()) / 2, (height() - bar->height()) / 2 + 50);
            moveAndFadeIn(bar, start_pos, end_pos, 500);
            
            pkg->installAsync([=, this](bool ok) {
                if (!ok)
                    Helpers::panic("Failed to install package\n");
                
                QMetaObject::invokeMethod(this, [=, this] {
                    fadeOut(bar, 500);
                    QLabel* text_label = new QLabel(this);
                    text_label->setStyleSheet("font-size: 24px; font-weight: bold;");
                    text_label->setGraphicsEffect(new QGraphicsOpacityEffect(text_label));
                    text_label->graphicsEffect()->setEnabled(true);
                    text_label->setText("Installed successfully!");
                    text_label->adjustSize();
                    text_label->setAlignment(Qt::AlignCenter);
                    text_label->show();
                    QPoint start_pos = QPoint((width() - text_label->width()) / 2, (height() - text_label->height()) / 2 + 100);
                    QPoint end_pos = QPoint((width() - text_label->width()) / 2, (height() - text_label->height()) / 2 + 50);
                    moveAndFadeIn(text_label, start_pos, end_pos, 500);
                    
                    QPushButton* ok_btn = new QPushButton(this);
                    ok_btn->setStyleSheet("font-size: 18px; font-weight: bold;");
                    ok_btn->setGraphicsEffect(new QGraphicsOpacityEffect(yes_btn));
                    ok_btn->graphicsEffect()->setEnabled(true);
                    ok_btn->setText("Ok");
                    ok_btn->adjustSize();
                    ok_btn->show();
                    start_pos = QPoint((width() - ok_btn->width()) / 2, (height() - ok_btn->height()) / 2 + 200);
                    end_pos = QPoint((width() - ok_btn->width()) / 2, (height() - ok_btn->height()) / 2 + 100);
                    moveAndFadeIn(ok_btn, start_pos, end_pos, 550);
                    connect(ok_btn, &QPushButton::clicked, [=, this]() {
                        // Fade out everything
                        anim = new QPropertyAnimation(this, "opacity", this);
                        anim->setDuration(500);
                        anim->setStartValue(1.0);
                        anim->setEndValue(0.0);
                        anim->setEasingCurve(QEasingCurve::OutCubic);
                        anim->start(QAbstractAnimation::DeleteWhenStopped);
                        fadeOut(text_label, 250);
                        fadeOut(ok_btn, 250);
                        fadeOut(icon_label, 250);
                        fadeOut(title_label, 250);
                        fadeOut(info_label, 250);
                        
                        on_complete(0);
                        
                        connect(anim, &QAbstractAnimation::finished, [&]() {
                            hide();
                        });
                    });
                    
                    // Cleanup
                    done = true;
                    fs::remove(ps3->fs.guestPathToHost("/dev_hdd1/pkg_installer/ICON0.PNG"));
                    //deleteLater();
                }, Qt::AutoConnection);
            },
            [=, this](float progress) {
                QMetaObject::invokeMethod(this, [=] {
                    bar->setProgress(progress);
                }, Qt::AutoConnection);
            });
        });
        
        connect(no_btn, &QPushButton::clicked, [=, this]() {
            disconnect(no_btn, &QPushButton::clicked, nullptr, nullptr);
            disconnect(yes_btn, &QPushButton::clicked, nullptr, nullptr);
            // Fade out everything
            fadeOut(text_label,     250);
            fadeOut(yes_btn,        250);
            fadeOut(no_btn,         250);
            fadeOut(icon_label,     250);
            fadeOut(title_label,    250);
            fadeOut(info_label,     250);
            anim = new QPropertyAnimation(this, "opacity", this);
            anim->setDuration(500);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            
            on_complete(1);
            
            connect(anim, &QAbstractAnimation::finished, [&]() {
                hide();
            });
        });
    });
}
 
QPropertyAnimation* PKGInstallerOverlay::moveAnim(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration) {
    QPropertyAnimation* move_anim = new QPropertyAnimation(obj, "pos");
    move_anim->setStartValue(start_pos);
    move_anim->setEndValue(end_pos);
    move_anim->setDuration(duration);
    move_anim->setEasingCurve(QEasingCurve::OutCubic);
    move_anim->start(QAbstractAnimation::DeleteWhenStopped);
    return move_anim;
}
    
QPropertyAnimation* PKGInstallerOverlay::fadeIn(QWidget* obj, int duration) {
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(obj->graphicsEffect());
    QPropertyAnimation* fade_anim = new QPropertyAnimation(effect, "opacity");
    fade_anim->setStartValue(0.0);
    fade_anim->setEndValue(1.0);
    fade_anim->setDuration(duration);
    fade_anim->setEasingCurve(QEasingCurve::OutCubic);
    fade_anim->start(QAbstractAnimation::DeleteWhenStopped);
    return fade_anim;
}
    
QPropertyAnimation* PKGInstallerOverlay::fadeOut(QWidget* obj, int duration) {
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(obj->graphicsEffect());
    QPropertyAnimation* fade_anim = new QPropertyAnimation(effect, "opacity");
    fade_anim->setStartValue(1.0);
    fade_anim->setEndValue(0.0);
    fade_anim->setDuration(duration);
    fade_anim->setEasingCurve(QEasingCurve::OutCubic);
    fade_anim->start(QAbstractAnimation::DeleteWhenStopped);
    return fade_anim;
}
    
QParallelAnimationGroup* PKGInstallerOverlay::moveAndFadeIn(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration, QEasingCurve move_curve) {
    QPropertyAnimation* move_anim = new QPropertyAnimation(obj, "pos");
    move_anim->setStartValue(start_pos);
    move_anim->setEndValue(end_pos);
    move_anim->setDuration(duration);
    move_anim->setEasingCurve(move_curve);
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(obj->graphicsEffect());
    QPropertyAnimation* fade_anim = new QPropertyAnimation(effect, "opacity");
    fade_anim->setStartValue(0.0);
    fade_anim->setEndValue(1.0);
    fade_anim->setDuration(duration);
    fade_anim->setEasingCurve(QEasingCurve::OutCubic);
    QParallelAnimationGroup* group = new QParallelAnimationGroup(obj);
    group->addAnimation(move_anim);
    group->addAnimation(fade_anim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
    return group;
}
    
QParallelAnimationGroup* PKGInstallerOverlay::moveAndFadeOut(QWidget* obj, QPoint start_pos, QPoint end_pos, int duration) {
    QPropertyAnimation* move_anim = new QPropertyAnimation(obj, "pos");
    move_anim->setStartValue(start_pos);
    move_anim->setEndValue(end_pos);
    move_anim->setDuration(500);
    move_anim->setEasingCurve(QEasingCurve::OutCubic);
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(obj->graphicsEffect());
    QPropertyAnimation* fade_anim = new QPropertyAnimation(effect, "opacity");
    fade_anim->setStartValue(1.0);
    fade_anim->setEndValue(0.0);
    fade_anim->setDuration(500);
    fade_anim->setEasingCurve(QEasingCurve::OutCubic);
    QParallelAnimationGroup* group = new QParallelAnimationGroup(obj);
    group->addAnimation(move_anim);
    group->addAnimation(fade_anim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
    return group;
}
