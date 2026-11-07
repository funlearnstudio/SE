#pragma once

struct SEOption {
  bool some = false;
  SEAny value{};
  std::string __se_type_name() const { return "Option"; }
};
struct SEResult {
  bool ok = false;
  SEAny value{};
  std::string error;
  std::string __se_type_name() const { return "Result"; }
};
inline std::string se_dynamic_type(const SEOption&) { return "Option"; }
inline std::string se_dynamic_type(const SEResult&) { return "Result"; }
inline SEOption se_option_none() { return {}; }
template<class T> SEOption se_option_some(T&& value) { return {true, SEAny(std::forward<T>(value))}; }
inline bool se_option_is_some(const SEOption& value) { return value.some; }
inline bool se_option_is_none(const SEOption& value) { return !value.some; }
inline SEAny se_option_value(const SEOption& value) {
  if (!value.some) throw SEKindError("OptionError", "Option has no value.");
  return value.value;
}
template<class T> SEAny se_option_or(const SEOption& value, T&& fallback) {
  return value.some ? value.value : SEAny(std::forward<T>(fallback));
}
template<class T> SEResult se_result_ok(T&& value) { return {true, SEAny(std::forward<T>(value)), {}}; }
inline SEResult se_result_err(const std::string& message) { return {false, SEAny{}, message}; }
inline bool se_result_is_ok(const SEResult& value) { return value.ok; }
inline bool se_result_is_err(const SEResult& value) { return !value.ok; }
inline SEAny se_result_value(const SEResult& value) {
  if (!value.ok) throw SEKindError("ResultError", value.error);
  return value.value;
}
inline std::string se_result_error(const SEResult& value) { return value.error; }
template<class T> SEAny se_result_or(const SEResult& value, T&& fallback) {
  return value.ok ? value.value : SEAny(std::forward<T>(fallback));
}
inline std::ostream& operator<<(std::ostream& out, const SEOption&) { return out << "<Option>"; }
inline std::ostream& operator<<(std::ostream& out, const SEResult&) { return out << "<Result>"; }

template<class T> T se_unbox_callback_arg(const SEAny& value) {
  if constexpr (std::is_same_v<T, SEAny>) return value;
  else return se_any_cast<T>(value);
}
template<class T> struct SECallbackSignature;
template<class R, class... A> struct SECallbackSignature<R(A...)> {
  static constexpr std::size_t arity = sizeof...(A);
  template<class F, std::size_t... I>
  static SEAny invoke(F&& fn, const SEList<SEAny>& args, std::index_sequence<I...>) {
    if (args.size() != sizeof...(A)) throw std::runtime_error("Wrong callback arity.");
    return SEAny(std::invoke(std::forward<F>(fn), se_unbox_callback_arg<std::remove_cvref_t<A>>(args[I])...));
  }
};
template<class R, class... A> struct SECallbackSignature<R(*)(A...)> : SECallbackSignature<R(A...)> {};
template<class F> struct SECallbackSignature<std::reference_wrapper<F>> : SECallbackSignature<F> {};
template<class F> SEAny se_invoke_callback(F&& fn, const SEList<SEAny>& args) {
  return SECallbackSignature<std::remove_cvref_t<F>>::invoke(std::forward<F>(fn), args,
      std::make_index_sequence<SECallbackSignature<std::remove_cvref_t<F>>::arity>{});
}
template<class F, class N> SEAny se_match_option(const SEOption& value, F&& some, N&& none) {
  return value.some ? se_invoke_callback(std::forward<F>(some), SEList<SEAny>{value.value})
                    : se_invoke_callback(std::forward<N>(none), {});
}
template<class F, class E> SEAny se_match_result(const SEResult& value, F&& ok, E&& error) {
  return value.ok ? se_invoke_callback(std::forward<F>(ok), SEList<SEAny>{value.value})
                  : se_invoke_callback(std::forward<E>(error), SEList<SEAny>{SEAny(value.error)});
}
template<class V, class F> SEAny se_match_value(const V& value, F&& fallback) {
  return se_invoke_callback(std::forward<F>(fallback), SEList<SEAny>{SEAny(value)});
}
template<class V, class P, class F, class... Rest>
SEAny se_match_value(const V& value, const P& pattern, F&& handler, Rest&&... rest) {
  if (SEAny(value) == SEAny(pattern))
    return se_invoke_callback(std::forward<F>(handler), SEList<SEAny>{SEAny(value)});
  return se_match_value(value, std::forward<Rest>(rest)...);
}
