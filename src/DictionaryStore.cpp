#include "DictionaryStore.hpp"

#include <csv_reader.hpp>

#include <KIO/StoredTransferJob>
#include <KJob>
#include <KZip>

#include <QQmlEngine>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrentRun>

#include <initializer_list>
#include <sstream>
#include <string_view>
#include <utility>

namespace {
using namespace Qt::StringLiterals;
static constinit QStringView datasetUrl =
    u"https://ckan0.cf.opendata.inter.prod-toronto.ca/dataset/"
    u"bd4809dd-e289-4de8-bbde-c5c00dafbf4f/resource/"
    u"28514055-d011-4ed7-8bb0-97961dfe2b66/download/surfacegtfs.zip";
static constinit QStringView agencyId = u"TTC";
static constinit QStringView readConnectionName = u"dictionary_store_read";
static constinit QStringView buildConnectionName = u"dictionary_store_build";
static constinit QStringView searchConnectionName = u"dictionary_store_search";

QString dbFilePath() {
  return QStandardPaths::writableLocation(
             QStandardPaths::AppLocalDataLocation) +
         u"/transit-peek.sqlite"_s;
}

QSqlDatabase openReadDb() {
  const auto connectionName = QString(readConnectionName);
  auto db = QSqlDatabase::contains(connectionName)
                ? QSqlDatabase::database(connectionName)
                : QSqlDatabase::addDatabase(u"QSQLITE"_s, connectionName);
  // qDebug() << dbFilePath();
  db.setDatabaseName(dbFilePath());
  if (!db.isOpen())
    db.open();
  return db;
}

void closeReadDbConnection() {
  const auto connectionName = QString(readConnectionName);
  if (!QSqlDatabase::contains(connectionName))
    return;

  {
    auto db = QSqlDatabase::database(connectionName);
    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
}

void closeSearchDbConnection() {
  const auto connectionName = QString(searchConnectionName);
  if (!QSqlDatabase::contains(connectionName))
    return;

  {
    auto db = QSqlDatabase::database(connectionName);
    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
}

QStringList normalizeIds(const QStringList &rawIds) {
  QStringList out;
  QSet<QString> dedup;
  for (const auto &raw : rawIds) {
    const auto value = raw.trimmed();
    if (value.isEmpty() || dedup.contains(value))
      continue;
    dedup.insert(value);
    out.push_back(value);
  }
  return out;
}

QString quoteFtsToken(const QString &token) {
  QString quoted = token;
  quoted.replace(QLatin1Char('"'), QStringLiteral(""""));
  return u"\""_s + quoted + u"\""_s;
}

QStringList splitTokens(const QString &query) {
  return query.simplified().split(QRegularExpression(u"\\s+"_s),
                                 Qt::SkipEmptyParts);
}

QString buildMatchExpression(const QString &query) {
  const auto tokens = splitTokens(query);
  if (tokens.isEmpty())
    return {};

  if (tokens.size() == 1)
    return quoteFtsToken(tokens.front());

  QStringList phrases;
  phrases.reserve(tokens.size());
  for (const auto &token : tokens)
    phrases.push_back(quoteFtsToken(token));

  return u"NEAR("_s + phrases.join(u" "_s) + u", 5)"_s;
}

bool tableHasColumns(const QSqlDatabase &db, const QString &tableName,
                     const QStringList &requiredColumns) {
  QSet<QString> foundColumns;
  QSqlQuery query(db);
  if (!query.exec(u"PRAGMA table_info(%1)"_s.arg(tableName)))
    return false;

  while (query.next())
    foundColumns.insert(query.value(1).toString());

  for (const auto &required : requiredColumns) {
    if (!foundColumns.contains(required))
      return false;
  }

  return true;
}

bool hasExpectedSchema() {
  if (!QFile::exists(dbFilePath()))
    return false;

  const auto db = openReadDb();
  if (!db.isOpen())
    return false;

  return tableHasColumns(db, u"metadata"_s,
                         {u"agency_id"_s, u"key"_s, u"value"_s}) &&
         tableHasColumns(db, u"routes"_s,
                         {u"agency_id"_s, u"route_id"_s, u"route_short_name"_s,
                          u"route_long_name"_s, u"route_color"_s,
                          u"route_text_color"_s}) &&
         tableHasColumns(db, u"stops"_s,
                         {u"agency_id"_s, u"stop_id"_s, u"stop_code"_s, u"stop_name"_s,
                          u"stop_lat"_s, u"stop_lon"_s}) &&
         tableHasColumns(db, u"stops_fts"_s,
                         {u"stop_code"_s, u"stop_name"_s}) &&
         tableHasColumns(db, u"trips"_s,
                         {u"agency_id"_s, u"trip_id"_s, u"route_id"_s,
                          u"trip_headsign"_s, u"direction_id"_s});
}

bool buildDb(const QSqlDatabase &db) {
  QSqlQuery query(db);
  if (!query.exec(u"CREATE TABLE IF NOT EXISTS metadata ("
                  "agency_id TEXT NOT NULL,"
                  "key TEXT NOT NULL,"
                  "value TEXT NOT NULL,"
                  "PRIMARY KEY (agency_id, key))"_s))
    return false;

  if (!query.exec(u"CREATE TABLE IF NOT EXISTS routes ("
                  "agency_id TEXT NOT NULL,"
                  "route_id TEXT NOT NULL,"
                  "route_short_name TEXT,"
                  "route_long_name TEXT,"
                  "route_color TEXT,"
                  "route_text_color TEXT,"
                  "PRIMARY KEY (agency_id, route_id))"_s))
    return false;

  if (!query.exec(u"CREATE TABLE IF NOT EXISTS stops ("
                  "agency_id TEXT NOT NULL,"
                  "stop_id TEXT NOT NULL,"
                  "stop_code TEXT,"
                  "stop_name TEXT,"
                  "stop_lat REAL,"
                  "stop_lon REAL,"
                  "PRIMARY KEY (agency_id, stop_id))"_s))
    return false;

  if (!query.exec(u"CREATE TABLE IF NOT EXISTS trips ("
                  "agency_id TEXT NOT NULL,"
                  "trip_id TEXT NOT NULL,"
                  "route_id TEXT,"
                  "trip_headsign TEXT,"
                  "direction_id TEXT,"
                  "PRIMARY KEY (agency_id, trip_id))"_s))
    return false;

  if (!query.exec(
          u"CREATE INDEX IF NOT EXISTS idx_stops_agency_stop_id ON stops (agency_id, stop_id)"_s))
    return false;

  if (!query.exec(
          u"CREATE INDEX IF NOT EXISTS idx_stops_code ON stops (stop_code)"_s))
    return false;

  if (!query.exec(
          u"CREATE INDEX IF NOT EXISTS idx_routes_agency_route ON routes (agency_id, route_id)"_s))
    return false;

  if (!query.exec(
          u"CREATE INDEX IF NOT EXISTS idx_trips_agency_trip ON trips (agency_id, trip_id)"_s))
    return false;

  if (!query.exec(u"CREATE VIRTUAL TABLE IF NOT EXISTS stops_fts USING fts5("
                  "stop_code,"
                  "stop_name,"
                  "content='stops',"
                  "content_rowid='rowid')"_s))
    return false;

  return true;
}

QString csvFieldToString(const csv::CSVRow &row, std::string_view columnName) {
  return QString::fromStdString(row[columnName]);
}

bool execInsert(QSqlQuery &query, const QString &sql,
                const QList<QVariant> &values) {
  query.prepare(sql);
  for (const QVariant &value : values)
    query.addBindValue(value);
  return query.exec();
}
bool buildRoutes(csv::CSVReader &&reader, const QSqlDatabase &db) {
  QSqlQuery query(db);
  for (const auto &row : reader) {
    if (!execInsert(
            query,
            u"INSERT OR REPLACE INTO routes (agency_id, route_id, route_short_name, route_long_name, route_color, route_text_color) VALUES (?, ?, ?, ?, ?, ?)"_s,
            {u"TTC"_s, csvFieldToString(row, "route_id"),
             csvFieldToString(row, "route_short_name"),
             csvFieldToString(row, "route_long_name"),
             csvFieldToString(row, "route_color"),
             csvFieldToString(row, "route_text_color")}))
      return false;
  }

  return true;
}

bool buildStops(csv::CSVReader &&reader, const QSqlDatabase &db) {
  QSqlQuery query(db);
  for (const auto &row : reader) {
    bool latOk = false;
    const double stopLat = csvFieldToString(row, "stop_lat").toDouble(&latOk);
    bool lonOk = false;
    const double stopLon = csvFieldToString(row, "stop_lon").toDouble(&lonOk);

    if (!execInsert(
            query,
            u"INSERT OR REPLACE INTO stops (agency_id, stop_id, stop_code, stop_name, stop_lat, stop_lon) VALUES (?, ?, ?, ?, ?, ?)"_s,
            {u"TTC"_s, csvFieldToString(row, "stop_id"),
             csvFieldToString(row, "stop_code"),
             csvFieldToString(row, "stop_name"),
             latOk ? QVariant(stopLat) : QVariant(),
             lonOk ? QVariant(stopLon) : QVariant()}))
      return false;

    const auto rowId = query.lastInsertId();
    if (!rowId.isValid())
      return false;

    if (!execInsert(
            query,
            u"INSERT OR REPLACE INTO stops_fts (rowid, stop_code, stop_name) VALUES (?, ?, ?)"_s,
            {rowId, csvFieldToString(row, "stop_code"),
             csvFieldToString(row, "stop_name")}))
      return false;
  }

  return true;
}

bool buildTrips(csv::CSVReader &&reader, const QSqlDatabase &db) {
  QSqlQuery query(db);
  for (const auto &row : reader) {
    if (!execInsert(
            query,
            u"INSERT OR REPLACE INTO trips (agency_id, trip_id, route_id, trip_headsign, direction_id) VALUES (?, ?, ?, ?, ?)"_s,
            {u"TTC"_s, csvFieldToString(row, "trip_id"),
             csvFieldToString(row, "route_id"),
             csvFieldToString(row, "trip_headsign"),
             csvFieldToString(row, "direction_id")}))
      return false;
  }

  return true;
}

static constinit std::initializer_list<
    std::pair<QStringView, decltype(&buildRoutes)>>
    buildTargets = {{u"routes.txt", &buildRoutes},
                    {u"stops.txt", &buildStops},
                    {u"trips.txt", &buildTrips}};

bool readZipAndBuildDb(QByteArray &&bytes) {
  const auto sqlitePath = dbFilePath();
  if (QFile::exists(sqlitePath) && !QFile::remove(sqlitePath))
    return false;

  QBuffer buffer(&bytes);
  KZip archive(&buffer);
  if (!archive.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open zip archive: " << archive.errorString();
    return false;
  }
  const auto root = archive.directory();

  bool success = false;
  {
    auto db =
        QSqlDatabase::addDatabase(u"QSQLITE"_s, QString(buildConnectionName));
    db.setDatabaseName(sqlitePath);
    if (!db.open())
      goto archive_fail;

    if (!db.transaction())
      goto dbopen_fail;

    if (!buildDb(db))
      goto rollback_fail;

    for (const auto [filename, buildFn] : buildTargets) {
      const auto file = root->file(QString(filename));
      if (!file) {
        qWarning() << "Entry not found or not a file: " << filename;
        goto rollback_fail;
      }

      const auto data = file->data();
      const std::string str(data.constData(), data.size());
      std::istringstream stream(str);
      if (!buildFn({stream}, db))
        goto rollback_fail;
    }

    success = db.commit();
    db.close();

  rollback_fail:
    if (!success)
      db.rollback();
  dbopen_fail:
    db.close();
  }
  QSqlDatabase::removeDatabase(QString(buildConnectionName));
  if (success)
    return true;
archive_fail:
  archive.close();
  return false;
}
} // namespace

DictionaryStore *DictionaryStore::instance() {
  static DictionaryStore *store = []() {
    auto *instance = new DictionaryStore();
    QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
  }();
  return store;
}

DictionaryStore *DictionaryStore::create(QQmlEngine *engine,
                                         QJSEngine *scriptEngine) {
  Q_UNUSED(engine);
  Q_UNUSED(scriptEngine);
  return instance();
}

QList<DictionaryStore::StopInfo>
DictionaryStore::searchStops(const QString &query) const {
  QList<StopInfo> results;
  if (this->m_rebuilding)
    return results;

  const auto matchExpression = buildMatchExpression(query);
  if (matchExpression.isEmpty())
    return results;

  {
    const auto connectionName = QString(searchConnectionName);
    auto db = QSqlDatabase::contains(connectionName)
                  ? QSqlDatabase::database(connectionName)
                  : QSqlDatabase::addDatabase(u"QSQLITE"_s, connectionName);
    db.setDatabaseName(dbFilePath());
    if (!db.open())
      return results;

    {
      QSqlQuery query(db);
      query.prepare(u"SELECT stop_code, stop_name FROM stops_fts WHERE stops_fts MATCH ? ORDER BY rank LIMIT 20"_s);
      query.addBindValue(matchExpression);
      if (query.exec()) {
        while (query.next())
          results.push_back({query.value(0).toString(), query.value(1).toString()});
      }
    }

    db.close();
  }
  return results;
}

void DictionaryStore::closeReadDb() { closeReadDbConnection(); }

QList<DictionaryStore::StopInfo>
DictionaryStore::allStops() const {
  QList<StopInfo> stops;
  if (this->m_rebuilding)
    return stops;

  const auto db = openReadDb();
  if (!db.isOpen())
    return stops;

  QSqlQuery sqlQuery(db);
  sqlQuery.prepare(u"SELECT stop_code, stop_name FROM stops WHERE agency_id = ? ORDER BY COALESCE(NULLIF(stop_name, ''), stop_code) COLLATE NOCASE, stop_code COLLATE NOCASE"_s);
  sqlQuery.addBindValue(QString(agencyId));

  if (!sqlQuery.exec())
    return stops;

  while (sqlQuery.next()) {
    stops.push_back({sqlQuery.value(0).toString(),
                     sqlQuery.value(1).toString()});
  }

  return stops;
}

QHash<QString, QString>
DictionaryStore::stopNamesByIds(const QStringList &stopIds) const {
  QHash<QString, QString> namesById;
  if (this->m_rebuilding)
    return namesById;
  const auto ids = normalizeIds(stopIds);
  if (ids.isEmpty())
    return namesById;

  const auto db = openReadDb();
  if (!db.isOpen())
    return namesById;

  QStringList placeholders;
  placeholders.reserve(ids.size());
  for (qsizetype i = 0; i < ids.size(); ++i)
    placeholders.push_back(u"?"_s);

  QSqlQuery query(db);
  query.prepare(
      u"SELECT stop_id, stop_name FROM stops WHERE agency_id = ? AND stop_id IN (%1)"_s
          .arg(placeholders.join(u", "_s)));
  query.addBindValue(QString(agencyId));
  for (const auto &id : ids)
    query.addBindValue(id);

  if (!query.exec())
    return namesById;

  while (query.next())
    namesById.insert(query.value(0).toString(), query.value(1).toString());
  return namesById;
}

QHash<QString, QString>
DictionaryStore::stopNamesByCodes(const QStringList &stopCodes) const {
  QHash<QString, QString> namesByCode;
  if (this->m_rebuilding)
    return namesByCode;
  const auto codes = normalizeIds(stopCodes);
  if (codes.isEmpty())
    return namesByCode;

  const auto db = openReadDb();
  if (!db.isOpen())
    return namesByCode;

  QStringList placeholders;
  placeholders.reserve(codes.size());
  for (qsizetype i = 0; i < codes.size(); ++i)
    placeholders.push_back(u"?"_s);

  QSqlQuery query(db);
  query.prepare(
      u"SELECT stop_code, stop_name FROM stops WHERE agency_id = ? AND stop_code IN (%1)"_s
          .arg(placeholders.join(u", "_s)));
  query.addBindValue(QString(agencyId));
  for (const auto &code : codes)
    query.addBindValue(code);

  if (!query.exec())
    return namesByCode;

  while (query.next())
    namesByCode.insert(query.value(0).toString(), query.value(1).toString());
  return namesByCode;
}

QHash<QString, DictionaryStore::TripInfo>
DictionaryStore::tripsByIds(const QStringList &tripIds) const {
  QHash<QString, TripInfo> tripsById;
  if (this->m_rebuilding)
    return tripsById;
  const auto ids = normalizeIds(tripIds);
  if (ids.isEmpty())
    return tripsById;

  const auto db = openReadDb();
  if (!db.isOpen())
    return tripsById;

  QStringList placeholders;
  placeholders.reserve(ids.size());
  for (qsizetype i = 0; i < ids.size(); ++i)
    placeholders.push_back(u"?"_s);

  QSqlQuery query(db);
  query.prepare(
      u"SELECT trip_id, route_id, trip_headsign, direction_id FROM trips WHERE agency_id = ? AND trip_id IN (%1)"_s
          .arg(placeholders.join(u", "_s)));
  query.addBindValue(QString(agencyId));
  for (const auto &id : ids)
    query.addBindValue(id);

  if (!query.exec())
    return tripsById;

  while (query.next()) {
    TripInfo trip;
    trip.routeId = query.value(1).toString();
    trip.tripHeadsign = query.value(2).toString();
    trip.directionId = query.value(3).toString();
    tripsById.insert(query.value(0).toString(), trip);
  }
  return tripsById;
}

QHash<QString, DictionaryStore::RouteInfo>
DictionaryStore::routesByIds(const QStringList &routeIds) const {
  QHash<QString, RouteInfo> routesById;
  if (this->m_rebuilding)
    return routesById;
  const auto ids = normalizeIds(routeIds);
  if (ids.isEmpty())
    return routesById;

  const auto db = openReadDb();
  if (!db.isOpen())
    return routesById;

  QStringList placeholders;
  placeholders.reserve(ids.size());
  for (qsizetype i = 0; i < ids.size(); ++i)
    placeholders.push_back(u"?"_s);

  QSqlQuery query(db);
  query.prepare(
      u"SELECT route_id, route_short_name, route_long_name, route_color, route_text_color FROM routes WHERE agency_id = ? AND route_id IN (%1)"_s
          .arg(placeholders.join(u", "_s)));
  query.addBindValue(QString(agencyId));
  for (const auto &id : ids)
    query.addBindValue(id);

  if (!query.exec())
    return routesById;

  while (query.next()) {
    RouteInfo route;
    route.routeShortName = query.value(1).toString();
    route.routeLongName = query.value(2).toString();
    route.routeColor = query.value(3).toString();
    route.routeTextColor = query.value(4).toString();
    routesById.insert(query.value(0).toString(), route);
  }
  return routesById;
}

QString DictionaryStore::stopIdFromCode(const QString &stopCode) const {
  if (stopCode.isEmpty() || this->m_rebuilding)
    return stopCode;

  const auto db = openReadDb();
  if (!db.isOpen())
    return stopCode;

  QSqlQuery query(db);
  query.prepare(u"SELECT stop_id FROM stops WHERE agency_id = ? AND stop_code = ?"_s);
  query.addBindValue(QString(agencyId));
  query.addBindValue(stopCode);

  if (!query.exec())
    return stopCode;

  if (query.next())
    return query.value(0).toString();

  return stopCode;
}

DictionaryStore::DictionaryStore(QObject *parent) : QObject(parent) {
  this->connect(&this->m_watcher, &QFutureWatcher<bool>::finished, this,
                [this]() {
                  auto result = m_watcher.result();
                  this->m_rebuilding = false;
                  Q_EMIT this->rebuildingChanged();
                  if (!result) {
                    Q_EMIT this->errorOccurred(u"Build db error"_s);
                    return;
                  }

                  Q_EMIT this->ready();
                });

  if (!hasExpectedSchema())
    this->downloadDataset();
}

void DictionaryStore::downloadDataset() {
  closeSearchDbConnection();
  this->closeReadDb();
  this->m_rebuilding = true;
  Q_EMIT this->rebuildingChanged();

  auto job = KIO::storedGet(QUrl(QString(datasetUrl)));
  this->connect(job, &KJob::percentChanged, this,
                [this](auto &&, unsigned long percent) {
                  this->m_downloadProgress = percent;
                  Q_EMIT this->downloadProgressChanged();
                });

  this->connect(job, &KJob::result, this, [this](KJob *job) {
    if (job->error()) {
      Q_EMIT this->errorOccurred(job->errorString());
      this->m_rebuilding = false;
      Q_EMIT this->rebuildingChanged();
      return;
    }

    this->buildDatabaseAsync(static_cast<KIO::StoredTransferJob *>(job));
  });
}

void DictionaryStore::buildDatabaseAsync(KIO::StoredTransferJob *job) {
  if (this->m_watcher.isRunning())
    return;
  auto future = QtConcurrent::run([bytes = job->data()]() mutable {
    return readZipAndBuildDb(std::move(bytes));
  });
  this->m_watcher.setFuture(future);
}
