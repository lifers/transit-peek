#pragma once
#include <QtQmlIntegration>

#include "DictionaryStore.hpp"

class SearchModel : public QAbstractListModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(SearchModel)
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
  explicit SearchModel(QObject *parent = nullptr);

  enum Roles {
    StopCode = Qt::UserRole,
    StopName,
  };
  Q_ENUM(Roles)

  QHash<int, QByteArray> roleNames() const override {
    return {{StopCode, "stopCode"}, {StopName, "stopName"}};
  }

  int rowCount(const QModelIndex &) const override {
    return m_results.count();
  }

  QVariant data(const QModelIndex &index, int role) const override;

  QString query() const { return m_query; }
  void setQuery(const QString &query);

Q_SIGNALS:
  void queryChanged(const QString &query);

private:
  void refresh();

  QString m_query;
  QList<DictionaryStore::StopInfo> m_results;
};