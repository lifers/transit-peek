#include "TransitFeed.hpp"

#include <KIO/StoredTransferJob>

#include <algorithm>
#include <execution>

#include <QJsonDocument>

#include "ArrivalData.hpp"
#include "DictionaryStore.hpp"
#include "gtfs-realtime.pb.h"

namespace {
using namespace Qt::StringLiterals;
static constinit QStringView fetchUrl = u"https://bustime.ttc.ca/gtfsrt/trips";

struct ParsedTripArrival {
  QString stopId;
  QString tripId;
  QString fallbackRouteId;
  qint64 arrivalEpoch = 0;
};

struct ParsedTripHeadsign {
  QString name;
  QString headsign;
};

struct ArrivalForSort {
  ArrivalData arrival;
  qint64 arrivalEpoch = 0;
};

QColor toCssColor(const QString &value, const QColor &fallback) {
  const auto text = value.trimmed();
  if (text.size() == 6) {
    const QColor color(u"#" + text);
    return color.isValid() ? color : fallback;
  }

  if (text.size() == 7 && text.startsWith(u'#')) {
    const QColor color(text);
    return color.isValid() ? color : fallback;
  }

  return fallback;
}

ParsedTripHeadsign parseTripHeadsign(const QString &tripHeadsign) {
  if (tripHeadsign.isEmpty())
    return {u"?"_s, u"Unknown route"_s};

  if (tripHeadsign == u"Not In Service")
    return {u"🛇"_s, u"Not In Service"_s};

  static const QRegularExpression pattern(
      u"^(North|South|East|West)\\s-\\s(\\S+)\\s(.+)$"_s);
  const auto match = pattern.match(tripHeadsign);
  if (!match.hasMatch())
    return {u"🛇"_s, tripHeadsign};

  const auto direction = match.captured(1);
  const auto routeWord = match.captured(2);
  const auto sentence = match.captured(3);
  if (direction.isEmpty() || routeWord.isEmpty() || sentence.isEmpty())
    return {u"🛇"_s, tripHeadsign};

  static const QHash<QStringView, QStringView> arrowByDirection = {
      {u"North", u"⮝"},
      {u"South", u"⮟"},
      {u"East", u"⮞"},
      {u"West", u"⮜"},
  };

  return {routeWord,
          arrowByDirection.value(direction, u"🛇"_s) + u" "_s + sentence};
}

QString formatArrival(qint64 unixSeconds) {
  if (unixSeconds <= 0)
    return u"--"_s;

  const auto nowEpoch = QDateTime::currentSecsSinceEpoch();
  const auto secondsUntilArrival = unixSeconds - nowEpoch;
  if (secondsUntilArrival <= 0)
    return u"due"_s;
  if (secondsUntilArrival <= 60)
    return u"now"_s;
  if (secondsUntilArrival <= 15 * 60) {
    const auto minutesUntilArrival = (secondsUntilArrival + 59) / 60;
    return u"in %1 mins"_s.arg(minutesUntilArrival);
  }

  return QDateTime::fromSecsSinceEpoch(unixSeconds).toString(u"hh:mm"_s);
}

QList<ArrivalData> parseGTFS(const QByteArray &bytes,
                             const QSet<QString> &stopSet,
                             DictionaryStore *dictionaryStore) {
  transit_realtime::FeedMessage feed;
  if (!feed.ParseFromArray(bytes.constData(), bytes.size())) {
    return {};
  }

  QList<ParsedTripArrival> parsedArrivals;
  QStringList tripIds;
  QStringList stopIds;

  for (const auto &entity : feed.entity()) {
    if (!entity.has_trip_update())
      continue;

    const auto &tripUpdate = entity.trip_update();
    const auto &trip = tripUpdate.trip();

    for (const auto &stopUpdate : tripUpdate.stop_time_update()) {
      const auto stopId =
          QString::fromStdString(stopUpdate.stop_id()).trimmed();
      if (!stopSet.contains(stopId))
        continue;

      const auto tripId = QString::fromStdString(trip.trip_id()).trimmed();
      const auto fallbackRouteId =
          QString::fromStdString(trip.route_id()).trimmed();
      const auto arrivalEpoch =
          stopUpdate.has_arrival() ? stopUpdate.arrival().time() : 0;

      parsedArrivals.push_back({.stopId = stopId,
                                .tripId = tripId,
                                .fallbackRouteId = fallbackRouteId,
                                .arrivalEpoch = arrivalEpoch});
      tripIds.push_back(tripId);
      stopIds.push_back(stopId);
    }
  }

  const auto stopNamesById = dictionaryStore
                                 ? dictionaryStore->stopNamesByIds(stopIds)
                                 : QHash<QString, QString>{};
  const auto tripsById = dictionaryStore
                             ? dictionaryStore->tripsByIds(tripIds)
                             : QHash<QString, DictionaryStore::TripInfo>{};

  QStringList routeIds;
  routeIds.reserve(parsedArrivals.size());
  for (const auto &parsed : parsedArrivals) {
    const auto tripIt = tripsById.constFind(parsed.tripId);
    const auto routeId =
        (tripIt != tripsById.cend() && !tripIt->routeId.trimmed().isEmpty())
            ? tripIt->routeId.trimmed()
            : parsed.fallbackRouteId;
    if (!routeId.isEmpty())
      routeIds.push_back(routeId);
  }

  const auto routesById = dictionaryStore
                              ? dictionaryStore->routesByIds(routeIds)
                              : QHash<QString, DictionaryStore::RouteInfo>{};

  QList<ArrivalForSort> arrivals;
  arrivals.reserve(parsedArrivals.size());
  for (const auto &parsed : parsedArrivals) {
    const auto tripIt = tripsById.constFind(parsed.tripId);
    const auto routeId =
        (tripIt != tripsById.cend() && !tripIt->routeId.trimmed().isEmpty())
            ? tripIt->routeId.trimmed()
            : parsed.fallbackRouteId;

    const auto routeIt = routesById.constFind(routeId);

    const auto staticTripHeadsign =
        tripIt != tripsById.cend() ? tripIt->tripHeadsign.trimmed() : QString{};
    const auto parsedTripHeadsign = staticTripHeadsign.isEmpty()
                                        ? ParsedTripHeadsign{}
                                        : parseTripHeadsign(staticTripHeadsign);

    auto routeName = routeIt != routesById.cend()
                         ? routeIt->routeShortName.trimmed()
                         : QString{};
    if (routeName.isEmpty())
      routeName = QString(parsedTripHeadsign.name.trimmed());
    if (routeName.isEmpty())
      routeName = parsed.fallbackRouteId;
    if (routeName.isEmpty())
      routeName = u"?"_s;

    auto headsign = parsedTripHeadsign.headsign.trimmed();
    if (headsign.isEmpty())
      headsign = staticTripHeadsign;
    if (headsign.isEmpty() && routeIt != routesById.cend())
      headsign = routeIt->routeLongName.trimmed();
    if (headsign.isEmpty())
      headsign = parsed.fallbackRouteId;
    if (headsign.isEmpty())
      headsign = u"Unknown route"_s;

    const auto stopName = stopNamesById.value(parsed.stopId).trimmed().isEmpty()
                  ? u"Stop %1"_s.arg(parsed.stopId)
                  : stopNamesById.value(parsed.stopId).trimmed();

    const auto iconBg = toCssColor(
        routeIt != routesById.cend() ? routeIt->routeColor : QString{},
        Qt::transparent);
    const auto iconFg = toCssColor(
        routeIt != routesById.cend() ? routeIt->routeTextColor : QString{},
        Qt::black);

    arrivals.push_back({.arrival = {.text = formatArrival(parsed.arrivalEpoch),
                                    .headsign = headsign,
                                    .stopName = stopName,
                                    .routeName = routeName,
                                    .iconFG = iconFg,
                                    .iconBG = iconBg},
                        .arrivalEpoch = parsed.arrivalEpoch});
  }

  std::sort(std::execution::unseq, arrivals.begin(), arrivals.end(),
            [](const ArrivalForSort &left, const ArrivalForSort &right) {
              const auto leftTime = left.arrivalEpoch > 0
                                        ? left.arrivalEpoch
                                        : std::numeric_limits<qint64>::max();
              const auto rightTime = right.arrivalEpoch > 0
                                         ? right.arrivalEpoch
                                         : std::numeric_limits<qint64>::max();
              return leftTime < rightTime;
            });

  QList<ArrivalData> result;
  result.reserve(arrivals.size());
  for (const auto &arrival : arrivals)
    result.push_back(arrival.arrival);

  return result;
}
} // namespace

TransitFeed::TransitFeed(QObject *parent) : QObject(parent) {
  this->m_timer.setInterval(60'000);
  this->m_timer.setSingleShot(false);
  this->connect(&m_timer, &QTimer::timeout, this, &TransitFeed::fetch);
  this->m_dict = DictionaryStore::instance();
  this->connect(this->m_dict, &DictionaryStore::rebuildingChanged, this,
                [this]() {
                  if (this->m_dict->rebuilding())
                    return;
                  this->emitParsedArrivals();
                });
  this->fetch();
  this->m_timer.start();
}

void TransitFeed::setPresets(const QStringList &presets) {
  if (this->m_presets == presets)
    return;

  this->m_presets = presets;
  this->updateStopsFromPresets(this->m_presets);
  Q_EMIT this->presetsChanged();
  this->emitParsedArrivals();
}

void TransitFeed::emitParsedArrivals() {
  if (this->m_cachedBytes.isEmpty())
    return;

  Q_EMIT this->arrivalsUpdated(
      parseGTFS(this->m_cachedBytes, this->m_stopSet, this->m_dict));
}

void TransitFeed::updateStopsFromPresets(const QStringList &presets) {
  this->m_stopSet.clear();

  for (const auto &presetJson : presets) {
    const auto doc = QJsonDocument::fromJson(presetJson.toUtf8());
    if (!doc.isObject())
      continue;
    for (const auto &v : doc.object().value(u"stops").toArray()) {
      const auto stopCode = v.toString().trimmed();
      if (!stopCode.isEmpty()) {
        const auto stopId = this->m_dict->stopIdFromCode(stopCode);
        this->m_stopSet.insert(stopId);
      }
    }
  }
}

void TransitFeed::fetch() {
  auto job = KIO::storedGet(QUrl(QString(fetchUrl)));
  this->connect(job, &KJob::result, this, [this, job]() {
    if (job->error()) {
      // TODO: handle error
      Q_EMIT this->arrivalsUpdated({});
      return;
    }

    this->m_cachedBytes = job->data();
    this->emitParsedArrivals();
  });
}
