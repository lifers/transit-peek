#include "PresetModel.hpp"

#include "DictionaryStore.hpp"
#include <QJsonArray>
#include <QJsonDocument>

namespace {
using namespace Qt::StringLiterals;

PresetModel::PresetData presetFromJson(const QJsonObject &object,
                                       int fallbackIndex) {
  PresetModel::PresetData preset;
  preset.title = object.value(u"title").toString();
  if (preset.title.trimmed().isEmpty())
    preset.title = u"Preset %1"_s.arg(fallbackIndex + 1);

  const auto dict = DictionaryStore::instance();
  for (const auto &stopValue : object.value(u"stops").toArray()) {
    const auto stopCode = stopValue.toString().trimmed();
    if (stopCode.isEmpty())
      continue;

    QString displayName = stopCode;
    if (dict) {
      const auto nameMap = dict->stopNamesByCodes({stopCode});
      const auto name = nameMap.value(stopCode).trimmed();
      if (!name.isEmpty())
        displayName = name + u" (" + stopCode + u")";
    }
    preset.stops.append(displayName);
  }
  return preset;
}

// Extract code from "Name (CODE)" format, or return as-is if no parens
QString extractStopCode(const QString &displayString) {
  const auto lastParen = displayString.lastIndexOf(u'(');
  if (lastParen >= 0) {
    const auto closeParen = displayString.indexOf(u')', lastParen);
    if (closeParen > lastParen) {
      return displayString.mid(lastParen + 1, closeParen - lastParen - 1)
          .trimmed();
    }
  }
  return displayString.trimmed();
}

QJsonObject presetToJson(const PresetModel::PresetData &preset) {
  QJsonObject object;
  object.insert(u"title", preset.title);

  QJsonArray stops;
  for (const auto &stop : preset.stops) {
    const auto stopCode = extractStopCode(stop);
    if (!stopCode.isEmpty())
      stops.append(stopCode);
  }
  object.insert(u"stops", stops);
  return object;
}

} // namespace

QVariant PresetModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() < 0 || index.row() >= this->m_presets.size())
    return QVariant();

  auto &row = this->m_presets.at(index.row());
  switch (role) {
  case Title:
    return row.title;
  case Stops:
    return row.stops;
  default:
    return QVariant();
  }
}

QString PresetModel::makeDefaultTitle() const {
  for (int nextPresetNumber = 1;; ++nextPresetNumber) {
    const auto candidate = u"Preset %1"_s.arg(nextPresetNumber);
    bool alreadyUsed = false;
    for (const auto &preset : this->m_presets) {
      if (preset.title == candidate) {
        alreadyUsed = true;
        break;
      }
    }
    if (!alreadyUsed)
      return candidate;
  }
}

void PresetModel::syncJsonPresets() {
  QStringList jsonPresets;
  jsonPresets.reserve(this->m_presets.size());
  for (const auto &preset : this->m_presets) {
    const auto document = QJsonDocument(presetToJson(preset));
    jsonPresets.append(
        QString::fromUtf8(document.toJson(QJsonDocument::Compact)));
  }

  this->m_jsonPresets = jsonPresets;
  Q_EMIT this->changedJsonPresets(this->m_jsonPresets);
  Q_EMIT this->changedPresets(this->m_presets);
}

void PresetModel::setJsonPresets(const QStringList &sl) {
  this->beginResetModel();
  this->m_jsonPresets = sl;
  this->m_presets.clear();

  for (int index = 0; index < sl.size(); ++index) {
    const auto document = QJsonDocument::fromJson(sl.at(index).toUtf8());
    PresetData presetData;
    if (document.isObject()) {
      presetData = presetFromJson(document.object(), index);
    } else {
      presetData.title = sl.at(index).trimmed();
      if (presetData.title.isEmpty())
        presetData.title = u"Preset %1"_s.arg(index + 1);
    }
    this->m_presets.append(presetData);
  }

  this->endResetModel();
  Q_EMIT this->changedJsonPresets(m_jsonPresets);
  Q_EMIT this->changedPresets(m_presets);
}

void PresetModel::setPresets(const QList<PresetData> &presets) {
  this->beginResetModel();
  this->m_presets = presets;
  this->syncJsonPresets();
  this->endResetModel();
}

void PresetModel::addPreset(const QString &title) {
  PresetData preset;
  preset.title = title.trimmed();
  if (preset.title.isEmpty())
    preset.title = this->makeDefaultTitle();

  const auto insertRow = this->m_presets.count();
  this->beginInsertRows(QModelIndex(), insertRow, insertRow);
  this->m_presets.append(preset);
  this->endInsertRows();
  this->syncJsonPresets();
}

void PresetModel::renamePreset(int index, const QString &title) {
  if (index < 0 || index >= this->m_presets.size())
    return;

  auto &preset = this->m_presets[index];
  preset.title = title.trimmed();
  if (preset.title.isEmpty())
    preset.title = this->makeDefaultTitle();

  const auto modelIndex = QAbstractListModel::index(index, 0);
  Q_EMIT this->dataChanged(modelIndex, modelIndex, {Title});
  this->syncJsonPresets();
}

void PresetModel::removePreset(int index) {
  if (index < 0 || index >= this->m_presets.size())
    return;

  this->beginRemoveRows(QModelIndex(), index, index);
  this->m_presets.removeAt(index);
  this->endRemoveRows();
  this->syncJsonPresets();
}

void PresetModel::addStop(int presetIndex, const QString &stopLabel) {
  if (presetIndex < 0 || presetIndex >= this->m_presets.size())
    return;

  auto &preset = this->m_presets[presetIndex];
  const auto stopCode = stopLabel.trimmed();
  if (stopCode.isEmpty())
    return;

  // Look up display name from DictionaryStore
  const auto dict = DictionaryStore::instance();
  QString displayName = stopCode;
  if (dict) {
    const auto nameMap = dict->stopNamesByCodes({stopCode});
    const auto name = nameMap.value(stopCode).trimmed();
    if (!name.isEmpty())
      displayName = name + u" (" + stopCode + u")";
  }

  preset.stops.append(displayName);
  const auto modelIndex = QAbstractListModel::index(presetIndex, 0);
  Q_EMIT this->dataChanged(modelIndex, modelIndex, {Stops});
  this->syncJsonPresets();
}

void PresetModel::removeStop(int presetIndex, int stopIndex) {
  if (presetIndex < 0 || presetIndex >= this->m_presets.size())
    return;

  auto &preset = this->m_presets[presetIndex];
  if (stopIndex < 0 || stopIndex >= preset.stops.size())
    return;

  preset.stops.removeAt(stopIndex);
  const auto modelIndex = QAbstractListModel::index(presetIndex, 0);
  Q_EMIT this->dataChanged(modelIndex, modelIndex, {Stops});
  this->syncJsonPresets();
}