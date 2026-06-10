#include "SmsCommand.h"
#include <cctype>

namespace {

std::string trim(const std::string& s) {
  const size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos) return "";
  const size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

std::string upper(std::string s) {
  for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

bool has(const std::string& haystack, const char* needle) {
  return haystack.find(needle) != std::string::npos;
}

} // namespace

namespace SmsCommand {

bool parseCmtHeader(const std::string& line, std::string& sender) {
  const size_t q1 = line.find('"');
  if (q1 == std::string::npos) return false;
  const size_t q2 = line.find('"', q1 + 1);
  if (q2 == std::string::npos) return false;
  sender = line.substr(q1 + 1, q2 - q1 - 1);
  return !sender.empty();
}

bool toEvent(const std::string& body, EventType& ev) {
  const std::string b = upper(trim(body));
  if (has(b, "PANICO") || has(b, "EMERGENCIA") || has(b, "AUXILIO") ||
      has(b, "ALARMA") || has(b, "SOS")) {
    ev = EventType::SMS_IN;
    return true;
  }
  if (has(b, "SILENC") || has(b, "APAGAR") || b == "OK") {
    ev = EventType::SILENCE;
    return true;
  }
  return false;
}

bool isAllowedSender(const std::string& sender, const std::string& allowList) {
  const std::string s = trim(sender);
  if (s.empty()) return false;
  size_t start = 0;
  for (;;) {
    const size_t comma = allowList.find(',', start);
    const std::string token = trim(allowList.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start));
    if (!token.empty() && token == s) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

} // namespace SmsCommand
