#ifndef MAIN_WND_HPP
#define MAIN_WND_HPP

#include <QMainWindow>
#include "ui_mainwnd.h"

class MainWnd : public QMainWindow
{
    Q_OBJECT

public:
    MainWnd(QWidget *parent=nullptr);

    void paintEvent(QPaintEvent *pe) final;

private:
    QScopedPointer<Ui::MainWnd> Ui;
};

#endif
