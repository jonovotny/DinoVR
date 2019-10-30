#pragma once

#include <unordered_map>
#include <string>

/*** Config represents a configuration file reader.
 *
 *   For the sake of laziness it only stores string : string configuration states (converting to int/double/vec is on you)
 *   Format read is strings with key being everything on the line left of the first =, value the rest.
 *
 */
class Config{
public:
  /**
   * Creates a Config and immediately reads the provided file
   */
  Config(std::string fileName);
  /**
   * Creates an empty Config which can be added to later.
   */
  Config();
  /**
   * Adds a file to the Config.
   */
  void ReadFile(std::string fileName);

  /**
   * Gets a value from the config.
   * If the value is not found it returns either the supplied fallback or empty string by default.
   **/
  std::string GetValue(std::string key, std::string fallback = "");
  /**
   * Gets a value from the config.
   * If the value is not found it returns either the supplied fallback or empty string by default.
   **/
  std::string GetValueWithDefault(std::string key, std::string fallback);

  /**
   * Adds an entry to the config.
   * @param key Key of the entry to be added.
   * @param value Value of the entry to be added.
   */
  void AddEntry(std::string key, std::string value);

  /**
   * Prints the keys and values stored in the config.
   * Added for debug purposes.
   **/
  void Print();

private:
  std::unordered_map<std::string, std::string> entries;


};
