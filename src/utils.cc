#include "utils.h"
#include "persistent-string.h"
#include "strings-table.h"

using namespace v8;

namespace bubo_utils {

Nan::Persistent<v8::Object> ignoredAttrs_;

void initialize(Local<Object> ignoredAttrs) {
    ignoredAttrs_.Reset(ignoredAttrs);
}

void hex_out(const BYTE* data, int len, const char* hint) {
    printf("%s [%p][%d]\n", (hint ? hint : ""), data, len);

    for (int i = 0; i < len; i++) {
        if (i != 0 && i % 8 == 0) printf("\n");
        printf("%02X ", data[i]);
    }
    printf("\n\n");
}

bool is_ignored_attribute(const Local<String>& attr)
{
    if (ignoredAttrs_.IsEmpty()) {
        return false;
    }
    return Nan::HasOwnProperty(Nan::New(ignoredAttrs_), attr).FromJust();
}
}
