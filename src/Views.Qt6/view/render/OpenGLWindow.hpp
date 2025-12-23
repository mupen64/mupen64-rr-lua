#ifndef VIEW_RENDER_OPENGLWINDOW_HPP_INCLUDED
#define VIEW_RENDER_OPENGLWINDOW_HPP_INCLUDED

#include <memory>

#include <QOpenGLContext>
#include <QWindow>
#include <QWidget>

#include "RenderWindow.hpp"

class OpenGLWindow : public RenderWindow
{
    Q_OBJECT
  public:
    OpenGLWindow(QWindow *parent = nullptr);

    virtual QWidget &container() override { return *m_container; }

    QOpenGLContext &context() { return *m_context; }
  private:
    std::unique_ptr<QWidget> m_container;
    std::unique_ptr<QOpenGLContext> m_context;
};

#endif