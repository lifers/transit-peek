#include "gtfs-reader.hpp"
#include "gtfs-realtime.pb.h"

#include <QObject>
#include <iostream>

GTFSReader *GTFSReader::create(QQmlEngine *, QJSEngine *) {
  static GTFSReader instance;
  QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
  return &instance;
}

void GTFSReader::request() const {
    std::cerr << "request called" << std::endl;
}

GTFSReader::GTFSReader() : QObject(nullptr) {}