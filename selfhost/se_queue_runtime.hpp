#pragma once
#include <deque>
#include <mutex>

struct SEQueueState { std::mutex mutex; std::deque<SEAny> values; };
struct SEQueue {
  std::shared_ptr<SEQueueState> state = std::make_shared<SEQueueState>();
  std::string __se_type_name() const { return "Queue"; }
};
inline SEQueue se_queue_new() { return {}; }
template<class T> void se_queue_put(const SEQueue& queue, T&& value) {
  std::lock_guard<std::mutex> lock(queue.state->mutex);
  queue.state->values.emplace_back(std::forward<T>(value));
}
inline SEAny se_queue_get(const SEQueue& queue) {
  std::lock_guard<std::mutex> lock(queue.state->mutex);
  if (queue.state->values.empty()) throw std::runtime_error("queue.get on empty queue.");
  auto value = queue.state->values.front(); queue.state->values.pop_front(); return value;
}
inline bool se_queue_empty(const SEQueue& queue) {
  std::lock_guard<std::mutex> lock(queue.state->mutex); return queue.state->values.empty();
}
inline std::int64_t se_queue_size(const SEQueue& queue) {
  std::lock_guard<std::mutex> lock(queue.state->mutex); return static_cast<std::int64_t>(queue.state->values.size());
}
inline std::string se_dynamic_type(const SEQueue&) { return "Queue"; }
inline std::ostream& operator<<(std::ostream& out, const SEQueue&) { return out << "<queue>"; }
