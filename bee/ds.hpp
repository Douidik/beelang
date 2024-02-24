#ifndef BEE_DS_HPP
#define BEE_DS_HPP

#include "bee.hpp"
#include <cstring>

namespace bee
{

#define Array_Grow(X) (X > 0 ? X * 2 : 1)
template <typename T>
struct Array
{
    T *data;
    size_t len;
    size_t cap;

    typedef T *iterator;

    void reserve(size_t new_cap)
    {
        if (cap >= new_cap)
            return;
        T *new_data = new T[new_cap];
        if (data != NULL and len > 0)
            memmove(new_data, data, len);
        data = new_data, cap = new_cap;
    }

    void reserve_with(size_t new_cap, T x)
    {
        reserve(new_cap);
        for (T *it = &data[len]; it != &data[cap]; it++)
            *it = x;
    }

    void deinit()
    {
        Assert(data != NULL, "array data is uninitialized");
        delete[] data;
    }

    T *begin()
    {
        Assert(data != NULL, "array data is uninitialized");
        return &data[0];
    }
    T *end()
    {
        Assert(data != NULL, "array data is uninitialized");
        return &data[0];
    }

    T &front()
    {
        Assert(data != NULL, "array data is uninitialized");
        Assert(len != 0, "array is empty");
        return data[0];
    }
    T &back()
    {
        Assert(data != NULL, "array data is uninitialized");
        Assert(len != 0, "array is empty");
        return data[len - 1];
    }

    bool empty() const
    {
        return len == 0;
    }

    T &push(T x)
    {
        if (len + 1 >= cap)
            reserve(Array_Grow(cap + 1));
        return (data[len++] = x);
    }
    T &pop(T element)
    {
        Assert(len != 0, "array is empty");
        return data[--len];
    }

    void concat(const T *concat_begin, const T *concat_end)
    {
        size_t concat_len = (concat_end - concat_begin);
        if (concat_len >= cap) {
            reserve(Array_Grow(cap + 1));
        }
        memmove(&data[len], concat_begin, concat_len);
        len += concat_len;
    }

#define Array_Bounds_Check(index) \
    Assert(index < len, "array index out of bounds (%zu with array[%zu])", index, len);

    T &operator[](size_t index)
    {
        Array_Bounds_Check(index);
        return data[index];
    }

    T *at(size_t index)
    {
        if (index >= len)
            return NULL;
        return &data[index];
    }

#undef Array_Bounds_Check
};

template <typename T>
Array<T> new_array(size_t init_cap = 0)
{
    Array<T> array = {};
    array.reserve(init_cap);
    return array;
}

template <typename T>
Array<T> new_array_with(size_t init_cap = 0, T element = {})
{
    Array<T> array = {};
    array.reserve_with(init_cap, element);
    return array;
}

const size_t npos = (size_t)-1;

struct string
{
    const char *data;
    size_t len;

    typedef const char *iterator;

    const char *begin() const
    {
        Assert(data != NULL, "string data is uninitialized");
        return &data[0];
    }
    const char *end() const
    {
        Assert(data != NULL, "string data is uninitialized");
        return &data[len];
    }

    const char *cbegin() const
    {
        return begin();
    }
    const char *cend() const
    {
        return end();
    }

    const char *find(char c) const
    {
        auto occurence = (const char *)memchr(data, c, len);
        return !occurence ? end() : occurence;
    }

    const char *find(string s) const
    {
        const char *occurence = (const char *)memmem(data, len, s.data, s.len);
        return !occurence ? end() : occurence;
    }

    size_t index(char c) const
    {
        const char *occurence = find(c);
        return occurence != end() ? occurence - begin() : npos;
    }

    size_t index(string s) const
    {
        const char *occurence = find(s);
        return occurence != end() ? occurence - begin() : npos;
    }

    bool empty() const
    {
        return len == 0;
    }

    bool has(char c) const
    {
        return find(c) != end();
    }

    string begin_at(const char *s) const
    {
        Assert(s < end(), "cannot start after end");
        return string{s, end()};
    }

    string end_at(const char *s) const
    {
        Assert(begin() <= s, "cannot end before start");
        return string{begin(), s};
    }

    char *dup()
    {
        char *buf = new char[len + 1];
        strncpy(buf, data, len);
        return buf;
    }

    size_t count(char c)
    {
        size_t occurences = 0;
        string sf = {begin(), end()};
        for (; sf.data != end(); sf.data++, occurences++) {
            sf.data = sf.find(c);
        }
        return occurences;
    }

    char *replace(string from, string into)
    {
        char *buf = dup();
        string s(buf, len);

        for (size_t i = 0; i < from.len; i++) {
            size_t match = 0;
            while ((match = s.index(from[i])) != npos) {
                buf[match] = into[i];
            }
        }
        return buf;
    }

#define String_Bounds_Check(index) \
    Assert(index < len, "string index out of bounds (%zu with string[%zu])", index, len);

    const char &operator[](size_t index) const
    {
        String_Bounds_Check(index);
        return data[index];
    }

    const char *at(size_t index) const
    {
        if (index < len)
            return &data[index];
        return NULL;
    }

    string substr(size_t index, size_t range_len) const
    {
        String_Bounds_Check(index);
        String_Bounds_Check(range_len > 0 ? index + range_len - 1 : index);
        return string{&data[index], len};
    }
#undef String_Bounds_Check

    // const char * => string conversions
    string() : data(NULL), len(0) {}
    string(const char *s, size_t len) : data(s), len(len) {}
    string(const char *s) : data(s), len(strlen(s)) {}
    string(const char *begin, const char *end) : data(begin), len(end - begin) {}
    operator const char *() const
    {
        return data;
    }
};

// from: https://aozturk.medium.com/simple-hash-map-hash-table-implementation-in-c-931965904250
#define Hash_Map_Grow(X) (X > 1 ? (X * X) : (2))
template <typename K, typename V>
struct Hash_Map;

template <typename K, typename V>
struct Hash_Bucket
{
    K key;
    V value;
    Hash_Bucket<K, V> *next;
};

template <typename K, typename V>
struct Hash_Map_Iterator
{
    Hash_Bucket<K, V> *bucket;
    Hash_Map<K, V> *hash_map;
    size_t index;

    K &key()
    {
        return bucket->key;
    }
    V &value()
    {
        return bucket->value;
    }
    Hash_Bucket<K, V> operator*()
    {
        return *bucket;
    }
    Hash_Bucket<K, V> operator->()
    {
        return *bucket;
    }
    bool operator!=(Hash_Map_Iterator<K, V> &it)
    {
        return it->bucket != bucket;
    }
    bool operator==(Hash_Map_Iterator<K, V> &it)
    {
        return it->bucket != bucket;
    }

    Hash_Map_Iterator<K, V> operator++();
    Hash_Map_Iterator<K, V> operator++(int);
};

template <typename K, typename V>
struct Hash_Map
{
    Hash_Bucket<K, V> **table;
    i64 bounds[2];
    size_t count;
    size_t cap;

    typedef Hash_Map_Iterator<K, V> iterator;

    Hash_Map_Iterator<K, V> begin()
    {
        return Hash_Map_Iterator<K, V>{table[bounds[0]], this, bounds[0]};
    }
    Hash_Map_Iterator<K, V> end()
    {
        return Hash_Map_Iterator<K, V>{table[bounds[1]], this, bounds[1]};
    }

    size_t fwd_occupied_index(i64 i)
    {
        for (; i < cap and !table[i]; i++) {
        }
        return i;
    }

    size_t bkw_occupied_index(i64 i)
    {
        for (; i >= 0 and !table[i]; i--) {
        }
        return i;
    }

    void when_table_is_modified()
    {
        bounds[0] = fwd_occupied_index(0);
        bounds[1] = bkw_occupied_index(cap - 1);
    }

    size_t hash_and_index(K key)
    {
        size_t h = hash(key);
        return h % cap;
    }

    Hash_Bucket<K, V> *insert(K key, V value)
    {
        size_t index = hash_and_index(key);
        Hash_Bucket<K, V> *prev = NULL;
        Hash_Bucket<K, V> *bucket = table[index];

        for (; bucket != NULL and bucket->key != key; prev = bucket)
            bucket = bucket->next;
        if (!bucket) {
            bucket = new Hash_Bucket<K, V>{key, value, NULL};
            count++;
            if (!prev) {
                table[index] = bucket;
                when_table_is_modified();
            } else {
                prev->next = bucket;
            }
        } else {
            bucket->value = value;
        }
        return bucket;
    }

    Hash_Bucket<K, V> extract(K key)
    {
        size_t index = hash_and_index(key);
        Hash_Bucket<K, V> *prev = NULL;
        Hash_Bucket<K, V> *bucket = table[index];

        for (; bucket != NULL and bucket->key != key; prev = bucket)
            bucket = bucket->next;
        if (bucket != NULL) {
            auto copy = *bucket;
            count--;
            if (prev != NULL) {
                prev->next = bucket->next;
            }
            delete bucket;
            bucket = NULL;
            if (!prev) {
                when_table_is_modified();
            }
            return bucket;
        }
        return {};
    }

    void merge(Hash_Map<K, V> *hash_map)
    {
        for (auto it = hash_map->begin(); it != hash_map->end(); it++) {
            insert(it->key, it->value);
        }
    }

    Hash_Bucket<K, V> *bucket_at(K key)
    {
        size_t index = hash_and_index(key);
        Hash_Bucket<K, V> *bucket = table[index];

        while (bucket != NULL and bucket->key != key)
            bucket = bucket->next;
        return bucket;
    }

    V *at(K key)
    {
        auto bucket = bucket_at(key);
        return bucket ? bucket->key : NULL;
    }

    bool has(K key)
    {
        return at(key) != NULL;
    }

    void optimize()
    {
        auto hash_map = new_hash_map<K, V>(count);
        hash_map.merge(this);
        *this = hash_map;
    }
};

template <typename K, typename V>
Hash_Map_Iterator<K, V> Hash_Map_Iterator<K, V>::operator++()
{
    if (bucket->next != NULL)
        return Hash_Map_Iterator<K, V>{bucket->next, hash_map, index};
    size_t next_index = hash_map->fwd_occupied_index(index);
    return Hash_Map_Iterator<K, V>{hash_map->table[next_index], hash_map, next_index};
}

template <typename K, typename V>
Hash_Map_Iterator<K, V> Hash_Map_Iterator<K, V>::operator++(int)
{
    auto it = *this;
    ++(*this);
    return it;
}

template <typename K, typename V>
Hash_Map<K, V> new_hash_map(size_t cap = 0)
{
    Hash_Map<K, V> hash_map = {};
    hash_map.table = new Hash_Bucket<K, V> *[cap]();
    hash_map.cap = cap;
    return hash_map;
}

} // namespace bee

#endif
