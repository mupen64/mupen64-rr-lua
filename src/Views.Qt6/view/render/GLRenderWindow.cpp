#include "GLRenderWindow.hpp"
#include <qcolor.h>
#include <qnamespace.h>
#include <qopenglcontext.h>
#include <QPalette>
#include <QWidget>
#include <qpalette.h>

#include "moc_GLRenderWindow.cpp"

GLRenderWindow::GLRenderWindow(QWidget* parent) {
  m_container = QWidget::createWindowContainer(this, parent);

  {
    QPalette palette {};
    palette.setColor(QPalette::Window, Qt::black);
    m_container->setAutoFillBackground(true);
    m_container->setPalette(palette);
  }

  setSurfaceType(QWindow::OpenGLSurface);
  m_context = new QOpenGLContext();
}

GLRenderWindow::~GLRenderWindow() {
  m_context->deleteLater();
}