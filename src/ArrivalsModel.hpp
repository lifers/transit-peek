#pragma once
#include "ArrivalData.hpp"

#include <QAbstractListModel>

#include <qcolor.h>
#include <qhashfunctions.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qvariant.h>

#include "TransitFeed.hpp"

class ArrivalsModel : public QAbstractListModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(ArrivalsModel)
  Q_PROPERTY(TransitFeed *feed READ feed WRITE connectFeed)

public:
  enum Roles {
    ArrivalText = Qt::UserRole,
    Headsign,
    StopName,
    RouteName,
    IconFg,
    IconBg,
  };
  Q_ENUM(Roles)

  QHash<int, QByteArray> roleNames() const override {
    return {{ArrivalText, "arrivalText"},  {Headsign, "headsign"},
            {StopName, "stopName"}, {RouteName, "routeName"},
            {IconFg, "iconFG"},     {IconBg, "iconBG"}};
  }

  int rowCount(const QModelIndex &) const override {
    return m_arrivals.count();
  }

  QVariant data(const QModelIndex &index, int role) const override {
    if (!index.isValid())
      return QVariant();

    if (index.row() >= m_arrivals.size())
      return QVariant();

    auto &row = m_arrivals.at(index.row());
    switch (role) {
    case ArrivalText:
      return row.text;
    case Headsign:
      return row.headsign;
    case StopName:
      return row.stopName;
    case RouteName:
      return row.routeName;
    case IconFg:
      return row.iconFG;
    case IconBg:
      return row.iconBG;
    default:
      return QVariant();
    }
  }

  void connectFeed(TransitFeed *feed) const {
    this->connect(feed, &TransitFeed::arrivalsUpdated, this,
                  &ArrivalsModel::onArrivalsUpdated);
  }

  TransitFeed *feed() { return nullptr; }

private Q_SLOTS:
  void onArrivalsUpdated(const QList<ArrivalData> &arrivals) {
    this->beginResetModel();
    this->m_arrivals = arrivals;
    this->endResetModel();
  }

private:
  QList<ArrivalData> m_arrivals{};
};
