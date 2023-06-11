#include "Common.h"

std::string getShortValueName(Value *v)
{
    if (v->getName().str().length() > 0)
    {
        return "%" + v->getName().str();
    }
    else if (isa<Instruction>(v))
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        v->print(*strm);
        std::string inst = strm->str();
        size_t idx1 = inst.find("%");
        size_t idx2 = inst.find(" ", idx1);
        if (idx1 != std::string::npos && idx2 != std::string::npos)
        {
            return inst.substr(idx1, idx2 - idx1);
        }
        else
        {
            return "\"" + inst + "\"";
        }
    }
    else if (ConstantInt *cint = dyn_cast<ConstantInt>(v))
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        cint->getValue().print(*strm, true);
        return strm->str();
    }
    else
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        v->print(*strm);
        std::string inst = strm->str();
        return "\"" + inst + "\"";
    }
}

bool isPointerToPointer(Type *T)
{
    return T->isPointerTy() && T->getContainedType(0)->isPointerTy();
}