#pragma once

template<class T> SEMap<std::int64_t> se_enum_make(const SEList<T>& names) {
  SEMap<std::int64_t> result;
  for (std::size_t i = 0; i < names.size(); ++i) result.put(se_string(names[i]), static_cast<std::int64_t>(i));
  return result;
}
template<class T> std::string se_enum_name(const SEMap<T>& values, const T& value) {
  for (const auto& item : values.items) if (item.second == value) return item.first;
  throw std::runtime_error("enum.name could not find that value.");
}
template<class T> T se_enum_value(const SEMap<T>& values, const std::string& name) { return values.at(name); }
template<class T> bool se_enum_has(const SEMap<T>& values, const std::string& name) {
  return std::any_of(values.items.begin(), values.items.end(), [&](const auto& item) { return item.first == name; });
}
template<class T> std::string se_typing_type_of(const T& value) { return se_dynamic_type(value); }
template<class T> bool se_typing_is(const T& value, const std::string& type) { return se_lower(se_dynamic_type(value)) == se_lower(type); }
template<class T> T se_typing_cast(const T& value, const std::string& type) {
  if (!se_typing_is(value, type)) throw std::runtime_error("typing.cast expected " + type + " but got " + se_dynamic_type(value) + ".");
  return value;
}
