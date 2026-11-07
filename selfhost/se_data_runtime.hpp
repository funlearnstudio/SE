#pragma once

template<class T, class V> void se_data_append(SEList<T>& values, V&& value) {
  values.push_back(static_cast<T>(std::forward<V>(value)));
}
template<class T, class U> void se_data_extend(SEList<T>& values, const SEList<U>& source) {
  SEList<T> copy;
  for (const auto& item : source) copy.push_back(static_cast<T>(item));
  for (const auto& item : copy) values.push_back(item);
}
template<class T, class V> void se_data_insert(SEList<T>& values, std::int64_t index, V&& value) {
  if (index < 0) index += static_cast<std::int64_t>(values.size());
  index = std::clamp<std::int64_t>(index, 0, static_cast<std::int64_t>(values.size()));
  values.emplace_back();
  for (std::size_t i = values.size() - 1; i > static_cast<std::size_t>(index); --i) values[i] = std::move(values[i - 1]);
  values[static_cast<std::size_t>(index)] = static_cast<T>(std::forward<V>(value));
}
template<class T> T se_data_pop(SEList<T>& values, std::int64_t index = -1) {
  if (values.empty()) throw std::runtime_error("data.pop cannot pop an empty List.");
  if (index < 0) index += static_cast<std::int64_t>(values.size());
  if (index < 0 || static_cast<std::uint64_t>(index) >= values.size()) throw std::runtime_error("data.pop index is out of bounds.");
  auto i = static_cast<std::size_t>(index);
  T value = values[i];
  values.erase(values.begin() + static_cast<std::ptrdiff_t>(i));
  return value;
}
template<class T> void se_data_clear(SEList<T>& values) { values.clear(); }
template<class T> void se_data_clear(SEMap<T>& values) { values.items.clear(); }
template<class T> void se_data_clear(SESet<T>& values) { values.items.clear(); }
template<class T> auto se_data_copy(const T& value) { return value.clone(); }
template<class T> SEAny se_data_get(const SEMap<T>& values, const std::string& key) {
  for (const auto& item : values.items) if (item.first == key) return SEAny(item.second);
  return SEAny{};
}
template<class T, class V> SEAny se_data_get(const SEMap<T>& values, const std::string& key, V&& fallback) {
  for (const auto& item : values.items) if (item.first == key) return SEAny(item.second);
  return SEAny(std::forward<V>(fallback));
}
template<class T, class V> void se_data_set(SEMap<T>& values, const std::string& key, V&& value) {
  values.put(key, static_cast<T>(std::forward<V>(value)));
}
template<class T, class U> void se_data_update(SEMap<T>& values, const SEMap<U>& source) {
  for (const auto& item : source.items) values.put(item.first, static_cast<T>(item.second));
}
template<class T> bool se_data_delete(SEMap<T>& values, const std::string& key) {
  for (auto it = values.items.begin(); it != values.items.end(); ++it) if (it->first == key) { values.items.erase(it); return true; }
  return false;
}
template<class T> bool se_data_has(const SEList<T>& values, const T& item) { return se_contains(values, item); }
template<class T> bool se_data_has(const SEMap<T>& values, const std::string& key) {
  return std::any_of(values.items.begin(), values.items.end(), [&](const auto& item) { return item.first == key; });
}
template<class T> SEList<std::string> se_data_keys(const SEMap<T>& values) {
  SEList<std::string> out; for (const auto& item : values.items) out.push_back(item.first); return out;
}
template<class T> SEList<T> se_data_values(const SEMap<T>& values) {
  SEList<T> out; for (const auto& item : values.items) out.push_back(item.second); return out;
}
template<class T> SEList<SEList<SEAny>> se_data_items(const SEMap<T>& values) {
  SEList<SEList<SEAny>> out;
  for (const auto& item : values.items) out.push_back(SEList<SEAny>{SEAny(item.first), SEAny(item.second)});
  return out;
}
