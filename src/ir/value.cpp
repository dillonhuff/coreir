#include "coreir/ir/context.h"
#include "coreir/ir/value.h"
#include "coreir/ir/types.h"
#include "coreir/ir/dynamic_bit_vector.h"

using namespace std;

namespace CoreIR {

bool Value::operator==(const Value& r) const {
  assert(0);
  return false;
}

bool operator==(const Values& l, const Values& r) {
  if (l.size() != r.size() ) return false;
  for (auto lpair : l) {
    string field = lpair.first;
    auto rpair = r.find(field);
    if (rpair == r.end()) return false;
    if (!(*(rpair->second) == *(lpair.second))) return false;
  }
  return true;
}


template<typename T>
ConstPtr Const_impl2(ValueType* type,T val) {
  return Underlying2ValueType<T>::make(type,val);
}

template<>
ConstPtr Const_impl<bool>(Context* c,bool val) {
  return ConstBool::make(c->Bool(),val);   
}

template<>
ConstPtr Const_impl<int>(Context* c,int val) {
  return Const_impl2<int>(c->Int(),val);   
}

template<>
ConstPtr Const_impl<BitVector>(Context* c,BitVector val) {
  return Const_impl2<BitVector>(c->BitVector(val.bitLength()),val);   
}

template<>
ConstPtr Const_impl<std::string>(Context* c,std::string val) {
  return Const_impl2<std::string>(c->String(),val);   
}

template<>
ConstPtr Const_impl<Type*>(Context* c,Type* val) {
  return Const_impl2<Type*>(c->CoreIRType(),val);   
}








}

//using namespace CoreIR;
////TODO sketchy because I am overloading a version of unordered_map
//size_t std::hash<Values>::operator() (const Values& values) const {
//  size_t ret = 0;
//  //Need to combine these in an order independent way, so just xor
//  for (auto it : values) {
//    size_t hash = 0;
//    hash_combine(hash,it.first);
//    auto arg = it.second;
//    switch(arg->getKind()) {
//      case Value::VK_Bool : {
//        hash_combine(hash,arg->get<bool>());
//        break;
//      }
//      case Value::VK_Int : {
//        hash_combine(hash,arg->get<int>());
//        break;
//      }
//      case Value::VK_String : {
//        hash_combine(hash,arg->get<string>());
//        break;
//      }
//      case Value::VK_Type : {
//        hash_combine(hash,arg->get<Type*>());
//        break;
//      }
//      default : 
//        assert(false);
//    }
//    ret ^= hash;
//  }
//  return ret;
//}
