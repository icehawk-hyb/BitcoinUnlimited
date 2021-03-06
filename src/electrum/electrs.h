#ifndef ELECTRUM_ELECTRS_H
#define ELECTRUM_ELECTRS_H

#include <string>
#include <vector>
#include <map>

/// Electrs specific code goes in here. Separating generic electrum code allows
/// us to support multiple, or swap electrum implementation in the future.

namespace electrum {

std::string electrs_path();
std::vector<std::string> electrs_args(int rpcport, const std::string& network);

std::map<std::string, int> fetch_electrs_info();

} // ns electrum

#endif
