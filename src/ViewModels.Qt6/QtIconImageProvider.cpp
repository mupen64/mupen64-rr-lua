#include <ViewModels.Qt6/QtIconImageProvider.hpp>

#include <QIcon>

QPixmap QtIconImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
    auto icon = QIcon::fromTheme(id);

    QSize actual_size = requestedSize.isValid() ? requestedSize : QSize(64, 64);
    *size = actual_size;
    return icon.pixmap(actual_size);
}