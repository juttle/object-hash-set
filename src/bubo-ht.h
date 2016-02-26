#pragma once

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <boost/dynamic_bitset.hpp>

#include "bubo-types.h"
#include "blob-store.h"
#include "utils.h"


#define DEFAULT_INIT_HASH_TABLE_SZ (4 << 10)
#define DEFAULT_MAX_HASH_TABLE_SZ (512 << 20)

#define RESIZE_THRESHOLD_PCT 70

/*
  BuboHashSet is a simple hash set in which one can insert any BYTE pointer except NULL, and do lookups.

  Internally, it is an array of BYTE* pointers. Whenever there is a collision, we go to the next bucket.
  our bit_set_ tells us which buckets are full

  Only disallowed value in the Bubo Hash Set is a NULL value for the BYTE pointer.
 */


struct BuboHashStat {
    uint64_t spine_len;         // Essentially, the size of the spine array.
    uint64_t entries;           // Total number of hash set entries that have been added.
    uint64_t ht_bytes;          // Current bytes used by the bubo hash set.

    uint64_t blob_allocated_bytes; //blobstore allocated
    uint64_t blob_used_bytes;      //blobstore used

    uint64_t bit_set_bytes;

    uint64_t bytes;             // Total bytes of hash set plus blobstore.
};


template<typename H, typename E>
class BuboHashSet {

public:
    BuboHashSet() : BuboHashSet(DEFAULT_INIT_HASH_TABLE_SZ, DEFAULT_MAX_HASH_TABLE_SZ) {}

    BuboHashSet(uint32_t table_size, uint32_t max_table_size) : table_size_(table_size),
                                                                max_table_size_(max_table_size),
                                                                num_entries_(0),
                                                                table_(new BYTE*[table_size_]()),
                                                                blob_store_(new BlobStore()),
                                                                bit_set_(table_size) {}

    ~BuboHashSet() {
        clear();
        delete blob_store_;
        delete [] table_;
    }

    // Returns true if inserted val is a new entry. Else false.
    inline bool insert(const BYTE* entry_buf, int entry_len) {
        assert(entry_buf);
        uint32_t idx = hash(entry_buf, entry_len) % table_size_;

        bool found = find_at(idx, entry_buf, entry_len);

        if (!found) {
            BYTE* blob_ptr = blob_store_->add(entry_buf, entry_len);
            insert_value_into_table_at_index(blob_ptr, table_, idx);
        }

        maybe_resize();

        return !found;
    }

    inline bool contains(const BYTE* entry_buf, int entry_len) {
        assert(entry_buf);
        uint32_t idx = hash(entry_buf, entry_len) % table_size_;

        return find_at(idx, entry_buf, entry_len);
    }

    inline void erase(BYTE* val) {
        int len = bubo_utils::get_entry_len(val);
        uint32_t index = hash(val, len) % table_size_;

        bool found = find_at(index, val, len);
        if (found) {
            BYTE** entry_to_erase = &table_[index];
            *entry_to_erase = NULL;
            num_entries_--;
        }
    }


    inline void clear(uint32_t size) {
        // clear() does not deallocate the spine.
        for (uint32_t idx = 0; idx < size; idx++) {
            table_[idx] = NULL;
        }
    }

    inline void clear() {
        clear(table_size_);
    }

    inline uint64_t size() const {
        return num_entries_;
    }


    void get_stats(BuboHashStat* stat) const {

        stat->spine_len = table_size_;
        stat->entries = num_entries_;

        stat->ht_bytes = table_size_ * sizeof(BYTE*);

        for (uint32_t idx = 0; idx < table_size_; idx++) {
            stat->ht_bytes += sizeof(BYTE*);
        }

        uint64_t allocated_bytes = 0, used_bytes = 0;
        blob_store_->stats(&allocated_bytes, &used_bytes);
        stat->blob_allocated_bytes = allocated_bytes;
        stat->blob_used_bytes = used_bytes;
        stat->bit_set_bytes = bit_set_.size() / 8;

        stat->bytes = stat->ht_bytes + stat->blob_allocated_bytes + stat->bit_set_bytes;
    }

protected:
    uint32_t table_size_;
    uint32_t max_table_size_;

    uint64_t num_entries_;
    BYTE** table_;

    BlobStore* blob_store_;
    boost::dynamic_bitset<> bit_set_;

    H hash;
    E equals;

    void insert_value_into_table_at_index(BYTE* value, BYTE** table, uint32_t index) {
        while (bit_set_[index]) {
            index = (index + 1) % table_size_;
        }

        bit_set_[index] = 1;

        table[index] = value;

        num_entries_ ++;
    }

    /*
     * Checks if the val is present in the chain at spine_entry.
     * Returns true if found.
     */
    inline bool find_at(uint32_t index, const BYTE* val, int len) {
        while (bit_set_[index]) {
            BYTE** p = &table_[index];
            if (equals(*p, val, len)) {
                return true;
            }
            index = (index + 1) % table_size_;
        }

        return false;
    }

    inline void maybe_resize() {
        uint32_t occupancy = 100 * num_entries_ / table_size_;
        if (occupancy == 100) {
            printf("Hash set is full :(\n");
            assert(false);
        }
        if (occupancy > RESIZE_THRESHOLD_PCT && table_size_ < max_table_size_) {
            uint32_t old_size = table_size_;
            table_size_ *= 2;
            bit_set_.clear();
            bit_set_.resize(table_size_);

            num_entries_ = 0;

            BYTE** new_table = new BYTE*[table_size_]();

            for (uint32_t idx = 0; idx < old_size; idx++) {
                BYTE** p = &table_[idx];
                if (*p) {
                    int len = bubo_utils::get_entry_len(*p);
                    uint32_t new_idx = hash(*p, len) % table_size_;
                    insert_value_into_table_at_index(*p, new_table, new_idx);
                }
            }

            clear(old_size); //clear() operates on table_
            BYTE** tmp = table_;
            table_ = new_table;

            delete [] tmp;
        }
    }
};
