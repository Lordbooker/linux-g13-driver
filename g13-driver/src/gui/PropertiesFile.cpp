#include "PropertiesFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

namespace {

// Splits "key=value" (trimmed); false for empty lines, comments and lines without '='.
bool splitLine(const QString &line, QString &key, QString &value)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#'))
        return false;
    const auto eq = trimmed.indexOf('=');
    if (eq <= 0)
        return false;
    key = trimmed.left(eq).trimmed();
    value = trimmed.mid(eq + 1).trimmed();
    return true;
}

QStringList readLines(const QString &path, bool *exists)
{
    QStringList lines;
    QFile file(path);
    *exists = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (*exists) {
        QTextStream in(&file);
        while (!in.atEnd())
            lines << in.readLine();
    }
    return lines;
}

bool writeLines(const QString &path, const QStringList &lines)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    for (const QString &line : lines)
        out << line << '\n';
    out.flush();
    return file.commit();
}

} // namespace

namespace PropertiesFile {

QString configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/g13";
}

QString bindingsPath(int profile)
{
    return configDir() + QString("/bindings-%1.properties").arg(profile);
}

QString macroPath(int macroId)
{
    return configDir() + QString("/macro-%1.properties").arg(macroId);
}

QString settingsPath()
{
    return configDir() + "/settings.properties";
}

QMap<QString, QString> read(const QString &path)
{
    QMap<QString, QString> values;
    bool exists = false;
    QString key;
    QString value;
    for (const QString &line : readLines(path, &exists)) {
        if (splitLine(line, key, value))
            values[key] = value;   // the last entry wins, as in the driver
    }
    return values;
}

bool update(const QString &path, const QMap<QString, std::optional<QString>> &changes,
            const QString &header)
{
    bool exists = false;
    const QStringList oldLines = readLines(path, &exists);
    QStringList lines;
    if (!exists)
        lines << header;

    QMap<QString, std::optional<QString>> pending = changes;
    QString key;
    QString value;
    for (const QString &line : oldLines) {
        if (!splitLine(line, key, value) || !changes.contains(key)) {
            lines << line;
            continue;
        }
        // Keep the position of the first entry, drop duplicates and removed keys.
        const auto it = pending.find(key);
        if (it != pending.end()) {
            const std::optional<QString> newValue = it.value();
            if (newValue)
                lines << key + '=' + *newValue;
            pending.erase(it);
        }
    }
    for (auto it = pending.cbegin(); it != pending.cend(); ++it) {
        const std::optional<QString> &newValue = it.value();
        if (newValue)
            lines << it.key() + '=' + *newValue;
    }
    return writeLines(path, lines);
}

bool write(const QString &path, const QString &header, const QStringList &lines)
{
    return writeLines(path, QStringList{header} + lines);
}

} // namespace PropertiesFile
