#include "Macro.h"

/**
 * @brief Default constructor.
 * Creates an empty Macro object with ID 0.
 */
Macro::Macro() : id(0) {
    // name and sequence are default-constructed to empty strings.
}

/**
 * @brief Parameterized constructor.
 * @param id The numeric ID of the macro.
 * @param name The human-readable name of the macro.
 * @param sequence The string representing the macro's actions.
 */
Macro::Macro(int id, const std::string& name, const std::string& sequence)
    : id(id), name(name), sequence(sequence) {
}

/**
 * @brief Gets the macro's ID.
 * @return The integer ID.
 */
int Macro::getId() const
{
    return this->id;
}

/**
 * @brief Gets the macro's name.
 * @return A const reference to the name string.
 */
const std::string& Macro::getName() const
{
    return this->name;
}

/**
 * @brief Gets the macro's sequence string.
 * @return A const reference to the sequence string.
 */
const std::string& Macro::getSequence() const
{
    return this->sequence;
}

/**
 * @brief Sets the macro's ID.
 * @param id The new integer ID.
 */
void Macro::setId(int id)
{
    this->id = id;
}

/**
 * @brief Sets the macro's name.
 * @param name The new name string.
 */
void Macro::setName(const std::string& name)
{
    this->name = name;
}

/**
 * @brief Sets the macro's sequence string.
 * @param sequence The new sequence string.
 */
void Macro::setSequence(const std::string& sequence)
{
    this->sequence = sequence;
}