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
    Local<Object> instance = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    info.GetReturnValue().Set(instance);
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
    if (info[0]->IsUndefined()) {
        return;
    }

    Local<Object> opts = info[0].As<Object>();

    Local<String> ignoredAttributes = Nan::New("ignoredAttributes").ToLocalChecked();
    if (! Nan::Has(opts, ignoredAttributes).FromJust()) {
        return;
    }

    Local<Value> ignoredVal = Nan::Get(opts, ignoredAttributes).ToLocalChecked();
    Local<Object> ignored = Nan::To<Object>(ignoredVal).ToLocalChecked();

    cache_.initialize(ignored);
}

JS_METHOD(Bubo, Add)
{
    Nan::HandleScope scope;
    int num_arguments = info.Length();

    if (num_arguments < 2) {
        return Nan::ThrowError("Add: invalid arguments");
    }

    Local<String> bucket = info[0].As<String>();
    Local<Object> obj = info[1].As<Object>();

    Local<String> attrs;
    int error_value = 0;
    int* error = &error_value;
    bool should_get_attr_str = num_arguments >= 3;
    bool found = cache_.add(bucket, obj, should_get_attr_str, attrs, error);

    if (*error) {
        return Nan::ThrowError("point too big");
    }

    static PersistentString attr_str("attr_str");

    if (should_get_attr_str) {
        Local<Object> result = info[2].As<Object>();
        Nan::Set(result, attr_str, attrs);
    }

    info.GetReturnValue().Set(found);
}

JS_METHOD(Bubo, Contains)
{
    Nan::HandleScope scope;

    if (info.Length() < 2) {
        return Nan::ThrowError("Contains: invalid arguments");
    }

    Local<String> bucket = info[0].As<String>();
    Local<Object> obj = info[1].As<Object>();

    int error_value = 0;
    int* error = &error_value;
    bool found = cache_.contains(bucket, obj, error);
    if (*error) {
        return Nan::ThrowError("point too big");
    }

    info.GetReturnValue().Set(found);
}


JS_METHOD(Bubo, Delete)
{
    Nan::HandleScope scope;

    if (info.Length() < 2) {
        return Nan::ThrowError("Delete: invalid arguments");
    }

    Local<String> bucket = info[0].As<String>();
    Local<Object> obj = info[1].As<Object>();

    cache_.remove(bucket, obj);

    return;
}


JS_METHOD(Bubo, DeleteBucket)
{
    Nan::HandleScope scope;

    if (info.Length() < 1) {
        return Nan::ThrowError("DeleteBucket: invalid arguments");
    }

    Local<String> bucket = info[0].As<String>();
    cache_.delete_bucket(bucket);

    return;
}

JS_METHOD(Bubo, GetBuckets) {
    Nan::HandleScope scope;

    v8::Local<v8::Array> buckets = cache_.get_buckets();

    info.GetReturnValue().Set(buckets);
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
    cache_.stats(stats);

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
    Nan::SetPrototypeMethod(tpl, "delete_bucket", JS_METHOD_NAME(DeleteBucket));
    Nan::SetPrototypeMethod(tpl, "get_buckets", JS_METHOD_NAME(GetBuckets));
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
