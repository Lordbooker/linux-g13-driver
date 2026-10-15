#include "G13Keys.h"

#include <QHash>
#include <QSet>

namespace G13Keys {

namespace {

const QHash<QString, QString> &fileKeys()
{
    static const QHash<QString, QString> map = [] {
        QHash<QString, QString> m;
        for (int n = 1; n <= 22; ++n)
            m.insert(QString("G%1").arg(n), QString("G%1").arg(n - 1));
        m.insert("BD",          "G24");
        m.insert("M1",          "G29");
        m.insert("M2",          "G30");
        m.insert("M3",          "G31");
        m.insert("MR",          "G32");
        m.insert("LEFT",        "G33");
        m.insert("DOWN",        "G34");
        m.insert("TOP",         "G35");
        m.insert("STICK_UP",    "G36");
        m.insert("STICK_LEFT",  "G37");
        m.insert("STICK_RIGHT", "G38");
        m.insert("STICK_DOWN",  "G39");
        return m;
    }();
    return map;
}

const QSet<QString> &locked()
{
    // L1-L4 schalten im Treiber die vier Profile um (bindings-0..3).
    static const QSet<QString> set = {"L1", "L2", "L3", "L4"};
    return set;
}

}

const QStringList &all()
{
    static const QStringList keys = [] {
        QStringList k;
        for (int n = 1; n <= 22; ++n)
            k << QString("G%1").arg(n);
        k << "L1" << "L2" << "L3" << "L4" << "BD"
          << "M1" << "M2" << "M3" << "MR"
          << "LEFT" << "DOWN" << "TOP"
          << "STICK_UP" << "STICK_LEFT" << "STICK_RIGHT" << "STICK_DOWN";
        return k;
    }();
    return keys;
}

QString fileKey(const QString &keyName)
{
    return fileKeys().value(keyName);
}

bool isConfigurable(const QString &keyName)
{
    return fileKeys().contains(keyName) && !locked().contains(keyName);
}

QString displayName(const QString &keyName)
{
    static const QHash<QString, QString> names = {
        {"LEFT",        "Daumentaste links"},
        {"DOWN",        "Daumentaste unten"},
        {"TOP",         "Joystick drücken"},
        {"STICK_UP",    "Joystick ↑"},
        {"STICK_LEFT",  "Joystick ←"},
        {"STICK_RIGHT", "Joystick →"},
        {"STICK_DOWN",  "Joystick ↓"},
    };
    return names.value(keyName, keyName);
}

}
