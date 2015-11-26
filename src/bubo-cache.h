#pragma once


#include <stdlib.h>
#include <unordered_map>
#include <string>
#include "bubo-types.h"

class AttributesTable;
class StringsTable;

class BuboCache {
public:
    BuboCache();
    virtual ~BuboCache();

    void initialize(v8::Local<v8::Object> ignoredAttrs);

    bool add(const v8::Local<v8::String>& bucket,
                const v8::Local<v8::Object>& pt,
                bool should_get_attr_str,
                v8::Local<v8::String>& attr_str,
                int* error);

    bool contains(const v8::Local<v8::String>& bucket,
                const v8::Local<v8::Object>& pt,
                int* error);

    void remove(const v8::Local<v8::String>& bucket,
                const v8::Local<v8::Object>& pt);

    void delete_bucket(const v8::Local<v8::String>& bucket);

    v8::Local<v8::Array> get_buckets();

    void stats(v8::Local<v8::Object>& stats) const;

protected:
    typedef std::unordered_map<std::string, AttributesTable*> bubo_cache_t;
    bubo_cache_t bubo_cache_;
    StringsTable* strings_table_;
    Nan::Persistent<v8::Object> ignored_attributes_;
};
