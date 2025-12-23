#ifndef VIEW_RENDER_RENDERWINDOW_HPP_INCLUDED
#define VIEW_RENDER_RENDERWINDOW_HPP_INCLUDED

#include <QWindow>

class RenderWindow : public QWindow
{
    Q_OBJECT
  public:
    RenderWindow(QWindow *parent = nullptr) : QWindow(parent) {}

    virtual QWidget& container() = 0;

  private:
};

#endif