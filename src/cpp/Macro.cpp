#include "Macro.h"

int Macro::getId() const {
    return _id;
}

const std::string& Macro::getName() const {
    return _name;
}

const std::string& Macro::getSequence() const {
    return _sequence;
}

void Macro::setId(int id) {
    _id = id;
}

void Macro::setName(const std::string& name) {
    _name = name;
}

void Macro::setSequence(const std::string& sequence) {
    _sequence = sequence;
}