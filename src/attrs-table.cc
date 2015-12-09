#include <assert.h>
#include <string.h>
#include <cstring>
#include "attrs-table.h"
#include "utils.h"
#include "strings-table.h"
#include "persistent-string.h"

static EntryToken* entryTokens[100];
static const int MAX_BUFFER_SIZE = 16 << 10;

AttributesTable::AttributesTable(StringsTable* strings_table,
                                 std::vector<std::string> ignored_attributes)
    : attributes_hash_set_(),
      strings_table_(strings_table),
      ignored_attributes_(ignored_attributes)
{
    for (int i = 0; i < 100; i++) {
      entryTokens[i] = new EntryToken();
    }
}


bool AttributesTable::add(const v8::Local<v8::Object>& pt, bool should_get_attr_str,
                             v8::Local<v8::String>& attr_str, int* error) {
    int entrylen = 0;

    prepare_entry_buffer(pt, &entrylen, should_get_attr_str, attr_str, error);

    if (*error) {
        return false;
    }

    return !attributes_hash_set_.insert(entry_buf_, entrylen);
}

bool AttributesTable::contains(const v8::Local<v8::Object>& pt, int* error) {
    int entrylen = 0;
    v8::Local<v8::String> dummy;

    prepare_entry_buffer(pt, &entrylen, false, dummy, error);

    if (*error) {
        return false;
    }

    return attributes_hash_set_.contains(entry_buf_, entrylen);
}

void AttributesTable::remove(const v8::Local<v8::Object>& pt) {

    int entrylen = 0;
    v8::Local<v8::String> dummy;
    int* dummyInt = nullptr;
    prepare_entry_buffer(pt, &entrylen, false, dummy, dummyInt);

    attributes_hash_set_.erase(entry_buf_);
}

AttributesTable::~AttributesTable() {
    attributes_hash_set_.clear();
}

char* mystrcat( char* dest, const char* src, int* total_buffer_size );

// http://www.joelonsoftware.com/articles/fog0000000319.html
char* mystrcat( char* dest, const char* src, int* total_buffer_size ) {
     while (*dest) dest++;
     while ((*dest++ = *src++) && (*total_buffer_size)++ < MAX_BUFFER_SIZE);
     return --dest;
}

inline bool _contains(std::vector<std::string> vector, std::string string) {
    return std::find(vector.begin(), vector.end(), string) != vector.end();
}

/* Returns true if all the tags and tag-names are found in the internal maps */
bool AttributesTable::prepare_entry_buffer(const v8::Local<v8::Object>& pt,
                                           int* entry_len,
                                           bool get_attr_str,
                                           v8::Local<v8::String>& attr_str,
                                           int* error) {
    int total_buffer_size = 0;
    static char g_attrstr_buf[MAX_BUFFER_SIZE] __attribute__ ((aligned (8)));
    g_attrstr_buf[0] = '\0';

    v8::Local<v8::Array> keys = Nan::GetOwnPropertyNames(pt).ToLocalChecked();
    std::vector<EntryToken*> tokens;
    bool all_found = true;
    uint32_t length = keys->Length();

    for (u_int32_t i = 0; i < length; ++i) {
        v8::Local<v8::Value> key = Nan::Get(keys, i).ToLocalChecked();
        v8::Local<v8::String> key_str(key->ToString());
        v8::String::Utf8Value tag(key);
        std::string tag_str(*tag);

        if (!ignored_attributes_.empty() && _contains(ignored_attributes_, tag_str)) {
            continue;
        }

        v8::String::Utf8Value val(Nan::Get(pt, key_str).ToLocalChecked());

        EntryToken* et = entryTokens[i];
        all_found = strings_table_->check_and_add(*tag, *val, et) && all_found;
        assert(et->tag_seq_no_ > 0 && et->val_seq_no_ > 0);
        tokens.push_back(et);
    }

    std::sort(tokens.begin(), tokens.end(), bubo_utils::cmp_entry_token);

    BYTE* entry_buf_ptr = entry_buf_;
    char* attr_buff_ptr = g_attrstr_buf;

    int encoded_len = 0;

    u_int32_t tags_count = tokens.size();
    bubo_utils::encode_packed(tags_count, entry_buf_ptr, &encoded_len);
    entry_buf_ptr += encoded_len;

    for (size_t i = 0; i < tags_count; i++) {

        EntryToken* et = tokens.at(i);

        encoded_len = 0;
        bubo_utils::encode_packed(et->tag_seq_no_, entry_buf_ptr, &encoded_len);
        entry_buf_ptr += encoded_len;

        encoded_len = 0;
        bubo_utils::encode_packed(et->val_seq_no_, entry_buf_ptr, &encoded_len);
        entry_buf_ptr += encoded_len;

        if (get_attr_str) {
            if (i != 0) {
                attr_buff_ptr = mystrcat(attr_buff_ptr, ",", &total_buffer_size);
            }
            attr_buff_ptr = mystrcat(attr_buff_ptr, et->tag_, &total_buffer_size);
            attr_buff_ptr = mystrcat(attr_buff_ptr, "=", &total_buffer_size);
            attr_buff_ptr = mystrcat(attr_buff_ptr, et->val_, &total_buffer_size);
        }
    }

    if (total_buffer_size >= MAX_BUFFER_SIZE) {
        *error = 1;
        return false;
    }

    if (get_attr_str) {
        attr_str = Nan::New<v8::String>(g_attrstr_buf).ToLocalChecked();
    }

    *entry_len = entry_buf_ptr - entry_buf_;

    // NOTE: "all_found == true" doesn't necessarily mean we have this entry.
    // This just means that each tag & tagname is known. But the order in which
    // they appear can vary within an entry.
    return all_found;
}


void AttributesTable::stats(v8::Local<v8::Object>& stats) const {

    static PersistentString attr_entries("attr_entries");

    static PersistentString blob_allocated_bytes("blob_allocated_bytes");
    static PersistentString blob_used_bytes("blob_used_bytes");

    static PersistentString ht_spine_len("ht_spine_len");
    static PersistentString ht_spine_use("ht_spine_use");
    static PersistentString ht_entries("ht_entries");
    static PersistentString ht_bytes("ht_bytes");
    static PersistentString ht_collision_slots("ht_collision_slots");
    static PersistentString ht_total_chain_len("ht_total_chain_len");
    static PersistentString ht_max_chain_len("ht_max_chain_len");
    static PersistentString ht_1_2("ht_dist_1_2");
    static PersistentString ht_3_5("ht_dist_3_5");
    static PersistentString ht_6_9("ht_dist_6_9");
    static PersistentString ht_10_("ht_dist_10_");
    static PersistentString ht_avg_chain_len("ht_avg_chain_len");

    static PersistentString ht_total_bytes("ht_total_bytes");

    Nan::Set(stats, attr_entries, Nan::New<v8::Number>(attributes_hash_set_.size()));

    BuboHashStat bhs;
    memset(&bhs, 0, sizeof(BuboHashStat));

    attributes_hash_set_.get_stats(&bhs);

    Nan::Set(stats, ht_spine_len, Nan::New<v8::Number>(bhs.spine_len));
    Nan::Set(stats, ht_spine_use, Nan::New<v8::Number>(bhs.spine_use));
    Nan::Set(stats, ht_entries, Nan::New<v8::Number>(bhs.entries));
    Nan::Set(stats, ht_bytes, Nan::New<v8::Number>(bhs.ht_bytes));
    Nan::Set(stats, ht_collision_slots, Nan::New<v8::Number>(bhs.collision_slots));
    Nan::Set(stats, ht_total_chain_len, Nan::New<v8::Number>(bhs.total_chain_len));
    Nan::Set(stats, ht_max_chain_len, Nan::New<v8::Number>(bhs.max_chain_len));
    Nan::Set(stats, ht_1_2, Nan::New<v8::Number>(bhs.dist_1_2));
    Nan::Set(stats, ht_3_5, Nan::New<v8::Number>(bhs.dist_3_5));
    Nan::Set(stats, ht_6_9, Nan::New<v8::Number>(bhs.dist_6_9));
    Nan::Set(stats, ht_10_, Nan::New<v8::Number>(bhs.dist_10_));
    Nan::Set(stats, ht_avg_chain_len, Nan::New<v8::Number>(bhs.avg_chain_len));

    Nan::Set(stats, blob_allocated_bytes, Nan::New<v8::Number>(bhs.blob_allocated_bytes));
    Nan::Set(stats, blob_used_bytes, Nan::New<v8::Number>(bhs.blob_used_bytes));

    Nan::Set(stats, ht_total_bytes, Nan::New<v8::Number>(bhs.bytes));

}
