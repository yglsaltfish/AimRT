

#include "context.h"

namespace aimrt::plugins::sm_plugin {

/**
 * @brief Construct a new Context object
 *
 */
Context::Context() {}

/**
 * @brief Destroy the Context object
 *
 */
Context::~Context() {}

/**
 * @brief Construct a new Context object from another Context object
 *
 * @param other The other Context object to copy from
 */
Context::Context(const Context& other)
    : seq_(other.seq_),
      sender_id_(other.sender_id_),
      channel_id_(other.channel_id_),
      timestamp_(other.timestamp_) {}

/**
 * @brief Copy the values of another Context object to this Context object
 *
 * @param other The other Context object to copy from
 * @return Context& This Context object
 */
Context& Context::operator=(const Context& other) {
  if (this != &other) {
    seq_ = other.seq_;
    sender_id_ = other.sender_id_;
    channel_id_ = other.channel_id_;
    timestamp_ = other.timestamp_;
  }
  return *this;
}

/**
 * @brief Check if this Context object is equal to another Context object
 *
 * @param other The other Context object to compare with
 * @return true If the two Context objects are equal
 * @return false If the two Context objects are not equal
 */
bool Context::operator==(const Context& other) const {
  return seq_ == other.seq_ && sender_id_ == other.sender_id_ &&
         channel_id_ == other.channel_id_ && timestamp_ == other.timestamp_;
}

/**
 * @brief Check if this Context object is not equal to another Context object
 *
 * @param other The other Context object to compare with
 * @return true If the two Context objects are not equal
 * @return false If the two Context objects are equal
 */
bool Context::operator!=(const Context& other) const {
  return !(*this == other);
}

/**
 * @brief Set the sequence number of this Context object
 *
 * @param seq The sequence number to set
 */
void Context::SetSeq(uint64_t seq) { seq_ = seq; }

/**
 * @brief Set the sender ID of this Context object
 *
 * @param sender_id The sender ID to set
 */
void Context::SetSenderId(uint64_t sender_id) { sender_id_ = sender_id; }

/**
 * @brief Set the channel ID of this Context object
 *
 * @param channel_id The channel ID to set
 */
void Context::SetChannelId(uint64_t channel_id) { channel_id_ = channel_id; }

/**
 * @brief Set the timestamp of this Context object
 *
 * @param timestamp The timestamp to set
 */
void Context::SetTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

/**
 * @brief Get the sequence number of this Context object
 *
 * @return uint64_t The sequence number
 */
uint64_t Context::seq() const { return seq_; }

/**
 * @brief Get the sender ID of this Context object
 *
 * @return uint64_t The sender ID
 */
uint64_t Context::sender_id() const { return sender_id_; }

/**
 * @brief Get the channel ID of this Context object
 *
 * @return uint64_t The channel ID
 */
uint64_t Context::channel_id() const { return channel_id_; }

/**
 * @brief Get the timestamp of this Context object
 *
 * @return uint64_t The timestamp
 */
uint64_t Context::timestamp() const { return timestamp_; }

}  // namespace aimrt::plugins::sm_plugin
