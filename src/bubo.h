#pragma once

#include "node.h"
#include "nan.h"
#include "js-method.h"
#include "attrs-table.h"
#include "strings-table.h"

#include <unordered_set>


class Bubo : public Nan::ObjectWrap {
public:
    static void Init(v8::Handle<v8::Object> exports);
    static NAN_METHOD(New);
    static Nan::Persistent<v8::Function> constructor;

private:
    explicit Bubo();
    ~Bubo();

    NAN_METHOD(Initialize);

    JS_METHOD_DECL(Add);
    JS_METHOD_DECL(Contains);
    JS_METHOD_DECL(Delete);
    JS_METHOD_DECL(Stats);
    JS_METHOD_DECL(Test);

    AttributesTable* attrs_table_;
    StringsTable* strings_table_;
    std::vector<std::string> ignored_attributes_;
};
