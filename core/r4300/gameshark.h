#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Gameshark {
/**
 * \brief Represents a gameshark script
 * \warning Thread-unsafe
 */
class Script {
public:
  /**
   * \brief Executes the script code
   */
  void execute();

  /**
   * \brief Sets the resumed state
   * \param resumed Whether the script is allowed to execute code
   */
  void set_resumed(bool resumed) { m_resumed = resumed; }

  /**
   * \brief Sets the script's name
   * \param name A name describing the script's functionality
   */
  void set_name(std::wstring name) { m_name = name; }

  /**
   * \brief Gets the resumed state
   */
  bool resumed() const { return m_resumed; }

  /**
   * \brief Gets the script's name
   */
  std::wstring name() const { return m_name; }

  /**
   * \brief Gets the script's code
   */
  std::wstring code() const { return m_code; }

  /**
   * \brief Compiles a script from a string
   * \param code The script code
   * \return The compiled script, or none if the script is invalid
   */
  static std::optional<std::shared_ptr<Script>>
  compile(const std::wstring &code);

private:
  // Whether the script code gets executed
  bool m_resumed = true;

  // Pair 1st element tells us whether instruction is a conditional. That's
  // required for special treatment of buggy kaze blj anywhere code
  std::vector<std::tuple<bool, std::function<bool()>>> m_instructions;

  // The script's name. FIXME: This should be read from the script
  std::wstring m_name = L"Unnamed Script";

  // The script's code. Can't be mutated after creation.
  std::wstring m_code;
};

/**
 * \brief The gameshark execution list's scripts
 */
extern std::vector<std::shared_ptr<Script>> scripts;

/**
 * \brief Executes the gameshark code
 */
void execute();
} // namespace Gameshark
