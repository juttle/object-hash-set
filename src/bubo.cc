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

    bubo_utils::initialize(ignored);
}

JS_METHOD(Bubo, LookupPoint)
{
    Nan::HandleScope scope;

    if (info.Length() < 3) {
        return Nan::ThrowError("LookupPoint: invalid arguments");
    }

    Local<String> spaceBucket = info[0].As<String>();
    Local<Object> obj = info[1].As<Object>();
    Local<Object> result = info[2].As<Object>();

    Local<String> attrs;
    int error_value = 0;
    int* error = &error_value;
    bool found = cache_.lookup(spaceBucket, obj, attrs, error);

    if (*error) {
        return Nan::ThrowError("point too big");
    }

    static PersistentString founded("found");
    static PersistentString attr_str("attr_str");

    Nan::Set(result, founded, Nan::New<Boolean>(found));
    Nan::Set(result, attr_str, attrs);

    return;
}


JS_METHOD(Bubo, RemovePoint)
{
    Nan::HandleScope scope;

    if (info.Length() < 2) {
        return Nan::ThrowError("RemovePoint: invalid arguments");
    }

    Local<String> spaceBucket = info[0].As<String>();
    Local<Object> obj = info[1].As<Object>();

    cache_.remove(spaceBucket, obj);

    return;
}


JS_METHOD(Bubo, RemoveBucket)
{
    Nan::HandleScope scope;

    if (info.Length() < 1) {
        return Nan::ThrowError("RemoveBucket: invalid arguments");
    }

    Local<String> spaceBucket = info[0].As<String>();
    cache_.remove_bucket(spaceBucket);

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
    Nan::SetPrototypeMethod(tpl, "lookup_point", JS_METHOD_NAME(LookupPoint));
    Nan::SetPrototypeMethod(tpl, "remove_point", JS_METHOD_NAME(RemovePoint));
    Nan::SetPrototypeMethod(tpl, "remove_bucket", JS_METHOD_NAME(RemoveBucket));
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
