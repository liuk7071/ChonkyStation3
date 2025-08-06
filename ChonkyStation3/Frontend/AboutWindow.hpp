#pragma once

#include <common.hpp>

#include "Frontend/UI/ui_about.h"
#include <QtWidgets>


class AboutWindow : public QWidget {
    Q_OBJECT
    
public:
    AboutWindow(QWidget* parent = nullptr);
    Ui::About ui;
};
