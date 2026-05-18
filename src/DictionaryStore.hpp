#pragma once
#include <QtQmlIntegration>

class QJSEngine;
class QQmlEngine;

namespace KIO {
class StoredTransferJob;
}

class DictionaryStore : public QObject {
  Q_OBJECT
  Q_PROPERTY(unsigned long downloadProgress READ downloadProgress NOTIFY
                 downloadProgressChanged)
  Q_PROPERTY(bool rebuilding READ rebuilding NOTIFY rebuildingChanged)

public:
  struct StopInfo {
    QString stopCode;
    QString stopName;
  };

  struct TripInfo {
    QString routeId;
    QString tripHeadsign;
    QString directionId;
  };

  struct RouteInfo {
    QString routeShortName;
    QString routeLongName;
    QString routeColor;
    QString routeTextColor;
  };

  explicit DictionaryStore(QObject *parent = nullptr);
  static DictionaryStore *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static DictionaryStore *instance();
  unsigned long downloadProgress() const { return this->m_downloadProgress; }
  bool rebuilding() const { return this->m_rebuilding; }
  QList<StopInfo> allStops() const;
  QList<StopInfo> searchStops(const QString &query) const;
  QHash<QString, QString> stopNamesByIds(const QStringList &stopIds) const;
  QHash<QString, QString> stopNamesByCodes(const QStringList &stopCodes) const;
  QHash<QString, TripInfo> tripsByIds(const QStringList &tripIds) const;
  QHash<QString, RouteInfo> routesByIds(const QStringList &routeIds) const;
  QString stopIdFromCode(const QString &stopCode) const;

Q_SIGNALS:
  void downloadProgressChanged();
  void rebuildingChanged();
  void ready();
  void errorOccurred(const QString &message);

private:
  void closeReadDb();
  void downloadDataset();
  void buildDatabaseAsync(KIO::StoredTransferJob *job);

  unsigned long m_downloadProgress = 0;
  QFutureWatcher<bool> m_watcher;
  bool m_rebuilding = false;
};
