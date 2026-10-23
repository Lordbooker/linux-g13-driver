#ifndef G13_GUI_PROPERTIESFILE_H
#define G13_GUI_PROPERTIESFILE_H

#include <QMap>
#include <QString>

#include <optional>

// Access to the configuration files of the driver in ~/.config/g13
// (bindings-<profile>.properties, macro-<id>.properties, settings.properties).
//
// Format: one "key=value" per line, lines starting with '#' are comments. The
// driver reads the same files; writing goes through QSaveFile, so the driver
// never sees an empty or half-written file.
namespace PropertiesFile {

QString configDir();
QString bindingsPath(int profile);
QString macroPath(int macroId);
QString settingsPath();

// All "key=value" pairs; empty if the file does not exist.
QMap<QString, QString> read(const QString &path);

// Sets (value) or removes (std::nullopt) keys. All other lines, including
// comments, stay as they are. A new file starts with the comment `header`.
bool update(const QString &path, const QMap<QString, std::optional<QString>> &changes,
            const QString &header);

// Replaces the whole file with `header` and the given lines.
bool write(const QString &path, const QString &header, const QStringList &lines);

} // namespace PropertiesFile

#endif // G13_GUI_PROPERTIESFILE_H
