#ifndef __MACRO_H__
#define __MACRO_H__

#include <string>

/**
 * @class Macro
 * @brief A simple data class to hold the properties of a single macro.
 *
 * This class encapsulates the ID, name, and action sequence string for a macro,
 * making it easy to pass macro data between different parts of the application.
 */
class Macro
{
private:
    int id;               // The unique numeric identifier for the macro.
    std::string name;     // The user-defined name of the macro.
    std::string sequence; // The string defining the macro's actions.
public:
    /** @brief Default constructor. */
    Macro();

    /**
     * @brief Parameterized constructor.
     * @param id The macro's ID.
     * @param name The macro's name.
     * @param sequence The macro's action sequence string.
     */
    Macro(int id, const std::string& name, const std::string& sequence);

    /** @brief Gets the macro's ID. */
    int getId() const;

    /** @brief Gets the macro's name. */
    const std::string& getName() const;

    /** @brief Gets the macro's sequence string. */
    const std::string& getSequence() const;

    /** @brief Sets the macro's ID. */
    void setId(int id);

    /** @brief Sets the macro's name. */
    void setName(const std::string& name);

    /** @brief Sets the macro's sequence string. */
    void setSequence(const std::string& sequence);
};

#endif