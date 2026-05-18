#pragma once
#include <QtQmlIntegration>

class PresetModel : public QAbstractListModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(PresetModel)
  Q_PROPERTY(QStringList jsonPresets READ jsonPresets WRITE setJsonPresets
                 NOTIFY changedJsonPresets)
  Q_PROPERTY(QList<PresetData> presets READ presets WRITE setPresets NOTIFY
                 changedPresets)

public:
  struct PresetData {
    QString title;
    QStringList stops;
  };

  enum Roles {
    Title = Qt::UserRole,
    Stops,
  };
  Q_ENUM(Roles)

  QHash<int, QByteArray> roleNames() const override {
    return {{Title, "title"}, {Stops, "stops"}};
  }
  int rowCount(const QModelIndex &) const override {
    return m_presets.count();
  }
  QVariant data(const QModelIndex &index, int role) const override;

  QStringList jsonPresets() { return this->m_jsonPresets; }
  void setJsonPresets(const QStringList &sl);
  QList<PresetData> presets() { return this->m_presets; }
  void setPresets(const QList<PresetData> &presets);

  Q_INVOKABLE void addPreset(const QString &title);
  Q_INVOKABLE void renamePreset(int index, const QString &title);
  Q_INVOKABLE void removePreset(int index);
  Q_INVOKABLE void addStop(int presetIndex, const QString &stopLabel);
  Q_INVOKABLE void removeStop(int presetIndex, int stopIndex);

Q_SIGNALS:
  void changedJsonPresets(const QStringList &sl);
  void changedPresets(const QList<PresetData> &presets);

private:
  QString makeDefaultTitle() const;
  void syncJsonPresets();

  QStringList m_jsonPresets;
  QList<PresetData> m_presets;
};