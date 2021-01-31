#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <set>
#include <chrono>

using namespace std;

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id, std::ostream& dst_stream = std::cerr)
        : id_(id)
        , dst_stream_(dst_stream) {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        dst_stream_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& dst_stream_;
};

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex),
            ref_to_value(bucket.map[key])
        {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        return { key, GetBucket(key) };
    }

    void erase(const Key& key) {
        Bucket& bucket = GetBucket(key);
        std::lock_guard guard(bucket.mutex);
        bucket.map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;

    Bucket& GetBucket(const Key& key) {
        return buckets_[static_cast<uint64_t>(key) % buckets_.size()];
    }
};

namespace TestRunnerPrivate {
    template <
        class Map
    >
        std::ostream& PrintMap(std::ostream& os, const Map& m) {
        os << "{";
        bool first = true;
        for (const auto& kv : m) {
            if (!first) {
                os << ", ";
            }
            first = false;
            os << kv.first << ": " << kv.second;
        }
        return os << "}";
    }
}

template <class T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& s) {
    os << "{";
    bool first = true;
    for (const auto& x : s) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class T>
std::ostream& operator << (std::ostream& os, const std::set<T>& s) {
    os << "{";
    bool first = true;
    for (const auto& x : s) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class K, class V, class C>
std::ostream& operator << (std::ostream& os, const std::map<K, V, C>& m) {
    return TestRunnerPrivate::PrintMap(os, m);
}

template <class K, class V>
std::ostream& operator << (std::ostream& os, const std::unordered_map<K, V>& m) {
    return TestRunnerPrivate::PrintMap(os, m);
}

template<class T, class U>
void AssertEqual(const T& t, const U& u, const std::string& hint = {}) {
    if (!(t == u)) {
        std::ostringstream os;
        os << "Assertion failed: " << t << " != " << u;
        if (!hint.empty()) {
            os << " hint: " << hint;
        }
        throw std::runtime_error(os.str());
    }
}

inline void Assert(bool b, const std::string& hint) {
    AssertEqual(b, true, hint);
}

class TestRunner {
public:
    template <class TestFunc>
    void RunTest(TestFunc func, const std::string& test_name) {
        try {
            func();
            std::cerr << test_name << " OK" << std::endl;
        }
        catch (std::exception& e) {
            ++fail_count;
            std::cerr << test_name << " fail: " << e.what() << std::endl;
        }
        catch (...) {
            ++fail_count;
            std::cerr << "Unknown exception caught" << std::endl;
        }
    }

    ~TestRunner() {
        std::cerr.flush();
        if (fail_count > 0) {
            std::cerr << fail_count << " unit tests failed. Terminate" << std::endl;
            exit(1);
        }
    }

private:
    int fail_count = 0;
};

#ifndef FILE_NAME
#define FILE_NAME __FILE__
#endif

#define ASSERT_EQUAL(x, y) {                          \
  std::ostringstream __assert_equal_private_os;       \
  __assert_equal_private_os                           \
    << #x << " != " << #y << ", "                     \
    << FILE_NAME << ":" << __LINE__;                  \
  AssertEqual(x, y, __assert_equal_private_os.str()); \
}

#define ASSERT(x) {                           \
  std::ostringstream __assert_private_os;     \
  __assert_private_os << #x << " is false, "  \
    << FILE_NAME << ":" << __LINE__;          \
  Assert(static_cast<bool>(x), __assert_private_os.str());       \
}

#define RUN_TEST(tr, func) \
  tr.RunTest(func, #func)

#define ASSERT_THROWS(expr, expected_exception) {                                           \
  bool __assert_private_flag = true;                                                        \
  try {                                                                                     \
    expr;                                                                                   \
    __assert_private_flag = false;                                                          \
  } catch (expected_exception&) {                                                           \
  } catch (...) {                                                                           \
    std::ostringstream __assert_private_os;                                                 \
    __assert_private_os << "Expression " #expr " threw an unexpected exception"             \
      " " FILE_NAME ":" << __LINE__;                                                        \
    Assert(false, __assert_private_os.str());                                               \
  }                                                                                         \
  if (!__assert_private_flag){                                                              \
    std::ostringstream __assert_private_os;                                                 \
    __assert_private_os << "Expression " #expr " is expected to throw " #expected_exception \
      " " FILE_NAME ":" << __LINE__;                                                        \
    Assert(false, __assert_private_os.str());                                               \
  }                                                                                         \
}

#define ASSERT_DOESNT_THROW(expr)                                           \
  try {                                                                     \
    expr;                                                                   \
  } catch (...) {                                                           \
    std::ostringstream __assert_private_os;                                 \
    __assert_private_os << "Expression " #expr " threw an unexpected exception" \
      " " FILE_NAME ":" << __LINE__;                                        \
    Assert(false, __assert_private_os.str());                               \
  }

void RunConcurrentUpdates(
    ConcurrentMap<int, int>& cm, size_t thread_count, int key_count
) {
    auto kernel = [&cm, key_count](int seed) {
        vector<int> updates(key_count);
        iota(begin(updates), end(updates), -key_count / 2);
        shuffle(begin(updates), end(updates), mt19937(seed));

        for (int i = 0; i < 2; ++i) {
            for (auto key : updates) {
                ++cm[key].ref_to_value;
            }
        }
    };

    vector<future<void>> futures;
    for (size_t i = 0; i < thread_count; ++i) {
        futures.push_back(async(kernel, i));
    }
}

void TestConcurrentUpdate() {
    constexpr size_t THREAD_COUNT = 3;
    constexpr size_t KEY_COUNT = 50000;

    ConcurrentMap<int, int> cm(THREAD_COUNT);
    RunConcurrentUpdates(cm, THREAD_COUNT, KEY_COUNT);

    const auto result = cm.BuildOrdinaryMap();
    ASSERT_EQUAL(result.size(), KEY_COUNT);
    for (auto& [k, v] : result) {
        AssertEqual(v, 6, "Key = " + to_string(k));
    }
}

void TestReadAndWrite() {
    ConcurrentMap<size_t, string> cm(5);

    auto updater = [&cm] {
        for (size_t i = 0; i < 50000; ++i) {
            cm[i].ref_to_value.push_back('a');
        }
    };
    auto reader = [&cm] {
        vector<string> result(50000);
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = cm[i].ref_to_value;
        }
        return result;
    };

    auto u1 = async(updater);
    auto r1 = async(reader);
    auto u2 = async(updater);
    auto r2 = async(reader);

    u1.get();
    u2.get();

    for (auto f : { &r1, &r2 }) {
        auto result = f->get();
        ASSERT(all_of(result.begin(), result.end(), [](const string& s) {
            return s.empty() || s == "a" || s == "aa";
            }));
    }
}

void TestSpeedup() {
    {
        ConcurrentMap<int, int> single_lock(1);

        LOG_DURATION("Single lock");
        RunConcurrentUpdates(single_lock, 4, 50000);
    }
    {
        ConcurrentMap<int, int> many_locks(100);

        LOG_DURATION("100 locks");
        RunConcurrentUpdates(many_locks, 4, 50000);
    }
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestConcurrentUpdate);
    RUN_TEST(tr, TestReadAndWrite);
    RUN_TEST(tr, TestSpeedup);
}