#ifndef BIPBUFFER_H
#define BIPBUFFER_H

#ifdef __cplusplus
#include <atomic>   // for `atomic_*` types in C++
#include <cstdint>  // for `std::uint32_t` and `std::uint8_t` in C++
#include <memory>   // for `std::unique_ptr` in C++
#include <optional> // for `std::optional` in C++
// This is required for C++ since all `atomic_*` type aliases are in namespace
// `std`, as in `std::atomic_bool`, for instance, instead of just `atomic_bool`!
// using all of namespace std is overkill; just do this instead:
using atomic_int32_t = std::atomic_int32_t;
using atomic_uint32_t = std::atomic_uint32_t;
using atomic_bool_t = std::atomic_bool;
#else
#include <stdatomic.h> // for `atomic_*` types in C
#endif

template <size_t N> class BBBuffer {

  std::unique_ptr<std::uint32_t[]> buf;

  atomic_int32_t write;
  atomic_int32_t read;
  atomic_int32_t last;
  atomic_int32_t reserve;

  atomic_bool_t read_in_progress;
  atomic_bool_t write_in_progress;
  atomic_bool_t already_split;

  unsigned long int size;

  /* region A */
  unsigned int a_start, a_end;

  /* region B */
  unsigned int b_end;

  /* is B inuse? */
  int b_inuse;

  unsigned char data[];

public:
  struct Producer {};
  struct Consumer {};

  std::optional<std::pair<Producer, Consumer>> try_split() {
    bool expected = false;
    if (!already_split.compare_exchange_strong(expected, true)) {
      return std::nullopt;
    }

    // Explicitly zero the data to avoid undefined behavior.
    std::memset(buf.get(), 0, N);

    return std::make_pair(Producer{}, Consumer{});
  }
};

#endif /* BIPBUFFER_H */