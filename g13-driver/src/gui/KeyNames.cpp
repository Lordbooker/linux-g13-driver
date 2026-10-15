#include "KeyNames.h"

#include <QHash>

namespace KeyNames {

const QList<QPair<QString, int>> &all()
{
    static const QList<QPair<QString, int>> keys = {
        {"ESC",       1},  {"1",        2},  {"2",       3},  {"3",       4},
        {"4",         5},  {"5",        6},  {"6",       7},  {"7",       8},
        {"8",         9},  {"9",       10},  {"0",      11},  {"Backspace",14},
        {"Tab",      15},  {"Q",       16},  {"W",      17},  {"E",      18},
        {"R",        19},  {"T",       20},  {"Y",      21},  {"U",      22},
        {"I",        23},  {"O",       24},  {"P",      25},  {"Enter",  28},
        {"LCtrl",    29},  {"A",       30},  {"S",      31},  {"D",      32},
        {"F",        33},  {"G",       34},  {"H",      35},  {"J",      36},
        {"K",        37},  {"L",       38},  {"LShift", 42},  {"Z",      44},
        {"X",        45},  {"C",       46},  {"V",      47},  {"B",      48},
        {"N",        49},  {"M",       50},  {"RShift", 54},  {"LAlt",   56},
        {"Space",    57},  {"F1",      59},  {"F2",     60},  {"F3",     61},
        {"F4",       62},  {"F5",      63},  {"F6",     64},  {"F7",     65},
        {"F8",       66},  {"F9",      67},  {"F10",    68},  {"F11",    87},
        {"F12",      88},  {"RCtrl",   97},  {"RAlt",  100},
        {"Home",    102},  {"Up",     103},  {"PgUp",  104},  {"Left",  105},
        {"Right",   106},  {"End",    107},  {"Down",  108},  {"PgDn",  109},
        {"Insert",  110},  {"Delete", 111},
        {"Mute",    113},  {"VolDn",  114},  {"VolUp", 115},
        {"Num0",     82},  {"Num1",    79},  {"Num2",   80},  {"Num3",   81},
        {"Num4",     75},  {"Num5",    76},  {"Num6",   77},  {"Num7",   71},
        {"Num8",     72},  {"Num9",    73},  {"Num.",   83},  {"Num+",   78},
        {"Num-",     74},  {"Num*",    55},  {"Num/",   98},  {"NumEnt", 96},
    };
    return keys;
}

QString nameFor(int code)
{
    static const QHash<int, QString> byCode = [] {
        QHash<int, QString> map;
        for (const auto &[name, keyCode] : all())
            map.insert(keyCode, name);
        return map;
    }();
    return byCode.value(code, QString("Taste %1").arg(code));
}

}
