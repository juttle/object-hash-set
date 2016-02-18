#include <stdlib.h>

#include "bubo.h"
#include "utils.h"
#include "persistent-string.h"
#include "test.h"


using namespace v8;

Nan::Persistent<Function> Bubo::constructor;

NAN_METHOD(NewInstance) {

    const unsigned argc = 1;
    Local<Value> argv[argc] = {info[0]};
    Local<Function> cons = Nan::New<Function>(Bubo::constructor);
    Nan::MaybeLocal<Object> instance = Nan::NewInstance(cons, argc, argv);

    if (! instance.IsEmpty()) {
        info.GetReturnValue().Set(instance.ToLocalChecked());
    }
}

NAN_METHOD(Bubo::New)
{
    if (!info.IsConstructCall()) {
        return Nan::ThrowError("non-constructor invocation not supported");
    }

    Bubo* obj = new Bubo();
    obj->Wrap(info.This());

    obj->Initialize(info);
    info.GetReturnValue().Set(info.This());
}

Bubo::Bubo()
{
}

Bubo::~Bubo()
{
}

NAN_METHOD(Bubo::Initialize)
{
    strings_table_ = new StringsTable();
    attrs_table_ = new AttributesTable(strings_table_);
    if (info[0]->IsUndefined()) {
        return;
    }

    Local<Object> opts = info[0].As<Object>();

    Local<String> ignoredAttributes = Nan::New("ignoredAttributes").ToLocalChecked();
    if (! Nan::Has(opts, ignoredAttributes).FromJust()) {
        return;
    }

    Local<Value> ignored_value = Nan::Get(opts, ignoredAttributes).ToLocalChecked();
    if (! ignored_value->IsArray()) {
        return Nan::ThrowError("ignoredAttributes must be an array");
    }
    Local<Array> ignored_array = Nan::To<Object>(ignored_value).ToLocalChecked().As<Array>();
    for (uint32_t i = 0; i < ignored_array->Length(); ++i) {
        v8::Local<v8::Value> ignored_value_string = Nan::Get(ignored_array, i).ToLocalChecked();
        v8::Local<v8::String> ignored_string(ignored_value_string->ToString());
        v8::String::Utf8Value ignored_utf8_value(ignored_string);
        std::string ignored_std_str(*ignored_utf8_value);

        ignored_attributes_.push_back(ignored_std_str);
    }

    attrs_table_->set_ignored_attributes(&ignored_attributes_);
}

JS_METHOD(Bubo, Add)
{
    Nan::HandleScope scope;
    int num_arguments = info.Length();

    if (num_arguments < 1) {
        return Nan::ThrowError("Add: invalid arguments");
    }

    Local<Object> point = info[0].As<Object>();

    Local<String> attrs;
    int error_value = 0;
    int* error = &error_value;
    bool should_get_attr_str = (num_arguments >= 2);

    bool found = attrs_table_->add(point, should_get_attr_str, attrs, error);

    if (*error) {
        return Nan::ThrowError("point too big");
    }

    static PersistentString attr_str("attr_str");

    if (should_get_attr_str) {
        Local<Object> result = info[1].As<Object>();
        Nan::Set(result, attr_str, attrs);
    }

    info.GetReturnValue().Set(!found);
}

JS_METHOD(Bubo, Contains)
{
    Nan::HandleScope scope;

    if (info.Length() < 1) {
        return Nan::ThrowError("Contains: invalid arguments");
    }

    Local<Object> point = info[0].As<Object>();

    int error_value = 0;
    int* error = &error_value;
    bool found = attrs_table_->contains(point, error);
    if (*error) {
        return Nan::ThrowError("point too big");
    }

    info.GetReturnValue().Set(found);
}


JS_METHOD(Bubo, Delete)
{
    Nan::HandleScope scope;

    if (info.Length() < 1) {
        return Nan::ThrowError("Delete: invalid arguments");
    }

    Local<Object> point = info[0].As<Object>();

    attrs_table_->remove(point);

    return;
}

JS_METHOD(Bubo, Test)
{
    Nan::HandleScope scope;

    testall();

    return;
}

JS_METHOD(Bubo, Stats)
{
    Nan::HandleScope scope;

    if (info.Length() < 1) {
        return Nan::ThrowError("Stats: invalid arguments");
    }

    Local<Object> stats = info[0].As<Object>();
    static PersistentString strings_table("strings_table");

    v8::Local<v8::Object> strings_stats = Nan::New<v8::Object>();
    strings_table_->stats(strings_stats);
    Nan::Set(stats, strings_table, strings_stats);

    static PersistentString attrs_table("attrs_table");

    v8::Local<v8::Object> attr_stats = Nan::New<v8::Object>();
    attrs_table_->stats(attr_stats);
    Nan::Set(stats, attrs_table, attr_stats);

    return;
}

void
Bubo::Init(Handle<Object> exports)
{
    Nan::HandleScope scope;

    // Prepare constructor template
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Bubo::New);
    tpl->SetClassName(Nan::New("Bubo").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    Nan::SetPrototypeMethod(tpl, "add", JS_METHOD_NAME(Add));
    Nan::SetPrototypeMethod(tpl, "contains", JS_METHOD_NAME(Contains));
    Nan::SetPrototypeMethod(tpl, "delete", JS_METHOD_NAME(Delete));
    Nan::SetPrototypeMethod(tpl, "test", JS_METHOD_NAME(Test));
    Nan::SetPrototypeMethod(tpl, "stats", JS_METHOD_NAME(Stats));

    constructor.Reset(tpl->GetFunction());

    Nan::Set(exports, Nan::New("Bubo").ToLocalChecked(),
        Nan::GetFunction(Nan::New<FunctionTemplate>(NewInstance)).ToLocalChecked());
}

void
InitModule(Handle<Object> exports)
{
    Bubo::Init(exports);
}

NODE_MODULE(bubo, InitModule)
