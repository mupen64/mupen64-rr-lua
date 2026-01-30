#include "GLRenderWindow.hpp"
#include <qopenglcontext.h>
#include <qwidget.h>

GLRenderWindow::GLRenderWindow(QWidget* parent) {
  m_container = QWidget::createWindowContainer(this, parent);
  m_owns_container = true;

  setSurfaceType(QWindow::OpenGLSurface);
  m_context = new QOpenGLContext();
}

GLRenderWindow::~GLRenderWindow() {
  if (m_owns_container)
    m_container->deleteLater();
  m_context->deleteLater();
}