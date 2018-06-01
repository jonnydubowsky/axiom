#include "Converter.h"

#include "MaximContext.h"
#include "Num.h"
#include "ComposableModuleClassMethod.h"

using namespace MaximCodegen;

Converter::Converter(MaximContext *ctx, llvm::Module *module, MaximCommon::FormType toType)
    : ComposableModuleClass(ctx, module, "converter." + MaximCommon::formType2String(toType)), _toType(toType) {

    auto numPtr = llvm::PointerType::get(ctx->numType()->get(), 0);
    _callMethod = std::make_unique<ComposableModuleClassMethod>(this, "call",
                                                                ctx->numType()->get(),
                                                                std::vector<llvm::Type *>{numPtr});
}

void Converter::generate() {
    auto undefPos = SourcePos(-1, -1);
    auto &b = _callMethod->builder();
    auto numInput = Num::create(ctx(), _callMethod->arg(0), undefPos, undefPos);
    auto resultNum = Num::create(ctx(), numInput->get(), b, _callMethod->allocaBuilder());

    auto func = _callMethod->get(_callMethod->moduleClass()->module());
    auto numForm = numInput->form(b);
    auto numVec = numInput->vec(b);

    auto defaultBlock = llvm::BasicBlock::Create(ctx()->llvm(), "default", func);

    auto formSwitch = b.CreateSwitch(numForm, defaultBlock, (unsigned int) converters.size());
    for (const auto &pair : converters) {
        auto blockName = "branch." + MaximCommon::formType2String(pair.first);
        auto converterBlock = llvm::BasicBlock::Create(ctx()->llvm(), blockName, func);
        formSwitch->addCase(
            llvm::ConstantInt::get(ctx()->numType()->formType(), (uint64_t) pair.first, false),
            converterBlock
        );
        b.SetInsertPoint(converterBlock);

        auto newVec = pair.second(_callMethod.get(), numVec);
        resultNum->setVec(b, newVec);
        resultNum->setForm(b, _toType);
        b.CreateRet(b.CreateLoad(resultNum->get(), "conv.deref"));
    }

    b.SetInsertPoint(defaultBlock);
    resultNum->setForm(b, _toType);
    b.CreateRet(b.CreateLoad(resultNum->get(), "conv.deref"));

    complete();
}

std::unique_ptr<Num> Converter::call(ComposableModuleClassMethod *method, std::unique_ptr<Num> value,
                                     SourcePos startPos, SourcePos endPos) {
    // hot path if there aren't any converters, just return the input with the target form
    if (converters.empty()) {
        value->setForm(method->builder(), _toType);
        return std::move(value);
    }

    // call the generated function
    auto entryIndex = method->moduleClass()->addEntry(this);
    auto result = method->callInto(entryIndex, {value->get()}, _callMethod.get(), "convertresult");
    auto newNum = Num::create(ctx(), method->allocaBuilder());
    method->builder().CreateStore(result, newNum->get());
    return std::move(newNum);
}
