#ifndef __MACRO_H__
#define __MACRO_H__

#pragma once

#include <string>

class Macro {
private:
    int _id = 0;
    std::string _name;
    std::string _sequence;
public:
    Macro() = default;

    int getId() const;
    const std::string& getName() const;
    const std::string& getSequence() const;

    void setId(int id);
    void setName(const std::string& name);
    void setSequence(const std::string& sequence);
};

#endif // __MACRO_H__