#include "bubo-cache.h"

#include "strings-table.h"
#include "blob-store.h"
#include "attrs-table.h"
#include "persistent-string.h"

std::string stdString(const v8::Local<v8::String>& bucket) {
    const v8::String::Utf8Value s(bucket);
    return std::string(*s);
}

BuboCache::BuboCache()
    : bubo_cache_(),
      strings_table_(new StringsTable()) {}


BuboCache::~BuboCache() {
    for (bubo_cache_t::iterator it = bubo_cache_.begin(); it != bubo_cache_.end(); it++) {
        delete it->second;
    }
    bubo_cache_.clear();
    delete strings_table_;
}


bool BuboCache::add(const v8::Local<v8::String>& bucket,
                       const v8::Local<v8::Object>& pt,
                       v8::Local<v8::String>& attr_str,
                       int* error) {
    std::string key = stdString(bucket);
    AttributesTable* at = NULL;
    bubo_cache_t::iterator it = bubo_cache_.find(key);
    if (it == bubo_cache_.end()) {
        at = new AttributesTable(strings_table_);
        bubo_cache_.insert(std::make_pair(key, at));
    } else {
        at = it->second;
    }
    return at->add(pt, attr_str, error);
}

bool BuboCache::contains(const v8::Local<v8::String>& bucket,
                       const v8::Local<v8::Object>& pt,
                       int* error) {
    std::string key = stdString(bucket);
    bubo_cache_t::iterator it = bubo_cache_.find(key);
    if (it == bubo_cache_.end()) {
        return false;
    } else {
        AttributesTable* at = it->second;
        return at->contains(pt, error);
    }
}


void BuboCache::remove(const v8::Local<v8::String>& bucket,
                       const v8::Local<v8::Object>& pt) {
    std::string key = stdString(bucket);

    bubo_cache_t::iterator it = bubo_cache_.find(key);
    if (it != bubo_cache_.end()) {
        it->second->remove(pt);
    }
}

void BuboCache::delete_bucket(const v8::Local<v8::String>& bucket) {
    const v8::String::Utf8Value s(bucket);
    std::string str(*s);
    bubo_cache_.erase(str);
}

void BuboCache::stats(v8::Local<v8::Object>& stats) const {

    static PersistentString strings_table("strings_table");

    v8::Local<v8::Object> strings_stats = Nan::New<v8::Object>();
    strings_table_->stats(strings_stats);

    Nan::Set(stats, strings_table, strings_stats);

    for (bubo_cache_t::const_iterator it = bubo_cache_.begin(); it != bubo_cache_.end(); it++) {
        v8::Local<v8::Object> attr_stats = Nan::New<v8::Object>();

        it->second->stats(attr_stats);
        Nan::Set(stats, Nan::New(it->first.c_str()).ToLocalChecked(), attr_stats);
    }
}
