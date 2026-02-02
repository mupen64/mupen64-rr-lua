#ifndef VIEW_RENDER_GLRENDERWINDOW_HPP_INCLUDED
#define VIEW_RENDER_GLRENDERWINDOW_HPP_INCLUDED

#include <QWindow>
#include <memory>
#include <qopenglcontext.h>
#include <qwidget.h>

class GLRenderWindow : public QWindow {
  Q_OBJECT
public:
  GLRenderWindow(QWidget* parent);

  virtual ~GLRenderWindow();

  QWidget* container() {
    return m_container;
  }

  QOpenGLContext* context() {
    return m_context;
  }

  void moveContextToThread(QThread* thread) {
    m_context->doneCurrent();
    m_context->create();
    m_context->moveToThread(thread);
  }
  
private:
  QWidget* m_container;
  QOpenGLContext* m_context;

};

#endif