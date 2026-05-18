#include "SearchModel.hpp"

SearchModel::SearchModel(QObject *parent) : QAbstractListModel(parent) {
  auto *dictionaryStore = DictionaryStore::instance();
  if (dictionaryStore) {
    this->connect(dictionaryStore, &DictionaryStore::ready, this,
                  &SearchModel::refresh);
    this->connect(dictionaryStore, &DictionaryStore::rebuildingChanged, this,
                  [this, dictionaryStore]() {
                    if (!dictionaryStore->rebuilding())
                      this->refresh();
                  });
  }
}

QVariant SearchModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= m_results.size())
    return QVariant();

  const auto &row = m_results.at(index.row());
  switch (role) {
  case StopCode:
    return row.stopCode;
  case StopName:
    return row.stopName;
  default:
    return QVariant();
  }
}

void SearchModel::setQuery(const QString &query) {
  const auto normalized = query.simplified();
  if (m_query == normalized)
    return;

  m_query = normalized;
  Q_EMIT queryChanged(m_query);
  this->refresh();
}

void SearchModel::refresh() {
  if (m_query.isEmpty()) {
    beginResetModel();
    m_results.clear();
    endResetModel();
    return;
  }

  const auto *dictionaryStore = DictionaryStore::instance();
  const auto results = dictionaryStore ? dictionaryStore->searchStops(m_query)
                                        : QList<DictionaryStore::StopInfo>{};

  beginResetModel();
  m_results = results;
  endResetModel();
}
