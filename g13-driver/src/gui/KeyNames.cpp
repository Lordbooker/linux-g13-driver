#include "KeyNames.h"

#include <QCoreApplication>
#include <QHash>
#include <QKeyEvent>
#include <QSettings>

namespace KeyNames {

namespace {

// Names by US key position (linux/input-event-codes.h).
const QHash<int, QString> &baseNames()
{
    static const QHash<int, QString> names = {
        {1, "Esc"},        {2, "1"},          {3, "2"},          {4, "3"},
        {5, "4"},          {6, "5"},          {7, "6"},          {8, "7"},
        {9, "8"},          {10, "9"},         {11, "0"},         {12, "-"},
        {13, "="},         {14, "Backspace"}, {15, "Tab"},       {16, "Q"},
        {17, "W"},         {18, "E"},         {19, "R"},         {20, "T"},
        {21, "Y"},         {22, "U"},         {23, "I"},         {24, "O"},
        {25, "P"},         {26, "["},         {27, "]"},         {28, "Enter"},
        {29, "LCtrl"},     {30, "A"},         {31, "S"},         {32, "D"},
        {33, "F"},         {34, "G"},         {35, "H"},         {36, "J"},
        {37, "K"},         {38, "L"},         {39, ";"},         {40, "'"},
        {41, "`"},         {42, "LShift"},    {43, "\\"},        {44, "Z"},
        {45, "X"},         {46, "C"},         {47, "V"},         {48, "B"},
        {49, "N"},         {50, "M"},         {51, ","},         {52, "."},
        {53, "/"},         {54, "RShift"},    {55, "Num*"},      {56, "LAlt"},
        {57, "Space"},     {58, "CapsLock"},  {59, "F1"},        {60, "F2"},
        {61, "F3"},        {62, "F4"},        {63, "F5"},        {64, "F6"},
        {65, "F7"},        {66, "F8"},        {67, "F9"},        {68, "F10"},
        {69, "NumLock"},   {70, "ScrollLock"},{71, "Num7"},      {72, "Num8"},
        {73, "Num9"},      {74, "Num-"},      {75, "Num4"},      {76, "Num5"},
        {77, "Num6"},      {78, "Num+"},      {79, "Num1"},      {80, "Num2"},
        {81, "Num3"},      {82, "Num0"},      {83, "Num."},      {86, "<>"},
        {87, "F11"},       {88, "F12"},       {96, "NumEnter"},  {97, "RCtrl"},
        {98, "Num/"},      {99, "Print"},     {100, "RAlt"},     {102, "Home"},
        {103, "Up"},       {104, "PgUp"},     {105, "Left"},     {106, "Right"},
        {107, "End"},      {108, "Down"},     {109, "PgDn"},     {110, "Insert"},
        {111, "Delete"},   {113, "Mute"},     {114, "VolDown"},  {115, "VolUp"},
        {117, "Num="},     {119, "Pause"},    {125, "LSuper"},   {126, "RSuper"},
        {127, "Menu"},     {163, "NextSong"}, {164, "PlayPause"},{165, "PrevSong"},
        {166, "StopCD"},   {183, "F13"},      {184, "F14"},      {185, "F15"},
        {186, "F16"},      {187, "F17"},      {188, "F18"},      {189, "F19"},
        {190, "F20"},      {191, "F21"},      {192, "F22"},      {193, "F23"},
        {194, "F24"},
    };
    return names;
}

constexpr const char *SettingsGroup = "KeyLabels";

// Labels of the user's own layout (keycode -> text), loaded from the settings.
QHash<int, QString> &learnedLabels()
{
    static QHash<int, QString> labels = [] {
        QHash<int, QString> map;
        QSettings settings;
        settings.beginGroup(SettingsGroup);
        for (const QString &key : settings.childKeys()) {
            bool ok = false;
            const int code = key.toInt(&ok);
            const QString label = settings.value(key).toString();
            if (ok && code > 0 && !label.isEmpty())
                map.insert(code, label);
        }
        return map;
    }();
    return labels;
}

// Labels for dead keys (they produce no text on their own, e.g. ^ and ´).
QString deadKeyLabel(int qtKey)
{
    switch (qtKey) {
    case Qt::Key_Dead_Grave:      return QStringLiteral("`");
    case Qt::Key_Dead_Acute:      return QStringLiteral("´");
    case Qt::Key_Dead_Circumflex: return QStringLiteral("^");
    case Qt::Key_Dead_Tilde:      return QStringLiteral("~");
    case Qt::Key_Dead_Diaeresis:  return QStringLiteral("¨");
    default:                      return {};
    }
}

}

QString nameFor(int code)
{
    const QHash<int, QString> &learned = learnedLabels();
    const auto itLearned = learned.constFind(code);
    if (itLearned != learned.constEnd())
        return itLearned.value();
    const auto itBase = baseNames().constFind(code);
    if (itBase != baseNames().constEnd())
        return itBase.value();
    return QCoreApplication::translate("KeyNames", "Key %1").arg(code);
}

void learnLabel(int code, const QKeyEvent *event)
{
    if (code <= 0 || !event)
        return;
    // With modifiers (Shift, AltGr, ...) the key produces a different character;
    // keypad keys (KeypadModifier) keep their names (Num1, ...).
    if (event->modifiers() != Qt::NoModifier)
        return;

    QString label = deadKeyLabel(event->key());
    if (label.isEmpty()) {
        const QString text = event->text();
        if (text.size() != 1 || !text.at(0).isPrint() || text.at(0).isSpace())
            return;
        label = QString(text.at(0).toUpper());
    }

    QHash<int, QString> &labels = learnedLabels();
    if (labels.value(code) == label)
        return;
    labels.insert(code, label);

    QSettings settings;
    settings.beginGroup(SettingsGroup);
    settings.setValue(QString::number(code), label);
}

}
