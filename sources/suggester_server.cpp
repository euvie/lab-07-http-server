// Copyright 2020 Your Name <your_email>

#include "suggester_server.hpp"

std::shared_mutex suggester_server::_collection_mutex;
std::unique_ptr<nlohmann::json> suggester_server::_collection = nullptr;

const char suggestions_str[] = "suggestions";
const char input_str[] = "input";
const char position_str[] = "position";
const char name_str[] = "name";
const char id_str[] = "id";
const char cost_str[] = "cost";
const char text_str[] = "text";

std::string suggester_server::parse_request(const std::string &request) const {
  nlohmann::json req;
  try {
    req = nlohmann::json::parse(request);
  } catch (const nlohmann::detail::parse_error& e) {
    throw std::runtime_error("Not json input");
  }
  if (req.contains(input_str)){
    return req[input_str];
  } else {
    throw std::runtime_error("Invalid fields in json input");
  }
}

std::string suggester_server::suggest(const std::string &input) const {
  try {
    nlohmann::json suggestion;
    suggestion[suggestions_str] = nlohmann::json::array();
    _collection_mutex.lock_shared();
    for (const auto& elem : *_collection){
      if (elem[id_str] == input){
        suggestion[suggestions_str].push_back(nlohmann::json{
            {text_str, elem[name_str]},
            {cost_str, elem[cost_str]}});
      }
    }
    _collection_mutex.unlock_shared();
    std::sort(suggestion[suggestions_str].begin(),
              suggestion[suggestions_str].end(),
              [](const nlohmann::json& a, const nlohmann::json& b) -> bool
              {
                return a[cost_str] < b[cost_str];
              });
    size_t position = 0
    for (auto& elem : suggestion[suggestions_str]){
      elem[position_str] = position;
      elem.erase(cost_str);
      ++position;
    }
    return suggestion.dump(4);
  } catch (const nlohmann::detail::parse_error &e) {
    throw std::runtime_error("Internal json error");
  }
}

[[noreturn]] void update_collection(const std::string &filename_json)
{
  const size_t minutes_time = 15;
  std::ifstream file_json;
  while (true){
    suggester_server::_collection_mutex.lock();
    suggester_server::_collection = nullptr;
    suggester_server::_collection =
        std::make_unique<nlohmann::json>(nlohmann::json());
    file_json.open(filename_json);
    file_json >> *(suggester_server::_collection);
    file_json.close();
    suggester_server::_collection_mutex.unlock();
    std::this_thread::sleep_for(std::chrono::minutes(minutes_time));
  }
}
