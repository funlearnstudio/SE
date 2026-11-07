#pragma once
// Tilemap parsing with the same width and bounds guarantees as SE expansion.
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

inline SEList<std::string> se_tilemap_parse(const std::string& source) {
  SEList<std::string> rows;
  std::istringstream input(source);
  std::string line;
  std::size_t width=0;
  while(std::getline(input,line)){
    if(!line.empty() && line.back()=='\r')line.pop_back();
    if(rows.empty())width=line.size();
    if(line.size()!=width)
      throw std::runtime_error("Tilemap rows must have equal width.");
    rows.push_back(line);
  }
  if(rows.empty()||width==0)
    throw std::runtime_error("Tilemap cannot be empty.");
  return rows;
}
inline std::string se_tilemap_at(const SEList<std::string>& rows,
                                 std::int64_t x,std::int64_t y) {
  if(y<0||static_cast<std::size_t>(y)>=rows.size())
    throw std::runtime_error("Tile coordinate out of range.");
  const auto& row=rows[static_cast<std::size_t>(y)];
  if(x<0||static_cast<std::size_t>(x)>=row.size())
    throw std::runtime_error("Tile coordinate out of range.");
  return row.substr(static_cast<std::size_t>(x),1);
}
inline SEList<std::int64_t> se_tilemap_size(const SEList<std::string>& rows) {
  if(rows.empty())throw std::runtime_error("Tilemap cannot be empty.");
  return {static_cast<std::int64_t>(rows[0].size()),
          static_cast<std::int64_t>(rows.size())};
}
