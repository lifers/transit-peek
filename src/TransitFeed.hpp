#pragma once
#include <QtQmlIntegration>

class ArrivalData;
class DictionaryStore;

class TransitFeed : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(TransitFeed)
  Q_PROPERTY(QStringList presets WRITE setPresets NOTIFY presetsChanged)

public:
  explicit TransitFeed(QObject *parent = nullptr);

  void setPresets(const QStringList &presets);

Q_SIGNALS:
  void arrivalsUpdated(const QList<ArrivalData> &arrivals);
  void presetsChanged();

private:
  void emitParsedArrivals();
  void fetch();
  void updateStopsFromPresets(const QStringList &presets);

  QTimer m_timer;
  QByteArray m_cachedBytes;
  DictionaryStore *m_dict = nullptr;
  QSet<QString> m_stopSet;
  QStringList m_presets;
};
