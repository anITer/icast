/*
 * MIT License
 *
 * Copyright (c) 2020 Andy Young
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef OBJECT_CACHER_H
#define OBJECT_CACHER_H

#include "cacheable.h"
#include <map>
#include <mutex>

template <typename T = Cacheable, typename A = Cacheable::Attributes>
class ObjectCacher {
public:
  ObjectCacher() : lock_(), cached_objects_(), type_counts_() {}

  virtual ~ObjectCacher() {
    purge_cache();
  }

  inline T* fetch_object(int width, int height, A* attributes) {
    T* object_from_cache = nullptr;
    std::string lookup_hash = Cacheable::get_hash(width, height, attributes);
    int number_of_matching_objects = 0;

    {
      std::lock_guard<std::mutex> lck(lock_);
      if (type_counts_.find(lookup_hash) != type_counts_.end()) {
        number_of_matching_objects = type_counts_[lookup_hash];
      }
      if (number_of_matching_objects < 1) {
        object_from_cache = T::create(width, height, attributes);
      } else {
        std::string object_hash = str_format("%s-%ld", lookup_hash.c_str(), --number_of_matching_objects);
        object_from_cache = cached_objects_[object_hash];
        cached_objects_.erase(object_hash);
        type_counts_[lookup_hash] = number_of_matching_objects;
      }
    }
    return object_from_cache;
  }

  inline bool return_object(T* object) {
    if (object == nullptr) return false;
    if (!object->need_be_cached()) {
      delete object;
      return false;
    }
    std::string lookup_hash = Cacheable::get_hash(object->get_width(), object->get_height(),
        object->get_attributes());
    int number_of_matching_objects = 0;

    {
      std::lock_guard<std::mutex> lck(lock_);
      if (type_counts_.find(lookup_hash) != type_counts_.end()) {
        number_of_matching_objects = type_counts_[lookup_hash];
      }
      std::string object_hash = str_format("%s-%ld", lookup_hash.c_str(), number_of_matching_objects++);
      cached_objects_[object_hash] = object;
      type_counts_[lookup_hash] = number_of_matching_objects;
    }
    return true;
  }

  inline bool purge_cache() {
    std::lock_guard<std::mutex> lck(lock_);
    for (const auto kvp : cached_objects_) {
      delete kvp.second;
    }
    cached_objects_.clear();
    type_counts_.clear();
    return true;
  }

protected:

  mutable std::mutex lock_;
  std::map<std::string, T*> cached_objects_;
  std::map<std::string, int> type_counts_;
};

#endif // OBJECT_CACHER_H
