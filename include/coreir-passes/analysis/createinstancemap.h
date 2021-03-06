#ifndef CREATEINSTANCEMAP_HPP_
#define CREATEINSTANCEMAP_HPP_

#include "coreir.h"

namespace CoreIR {
namespace Passes {

class CreateInstanceMap : public ModulePass {
  public:
    //Map from Instantiables to a list of instances
    typedef unordered_map<Instantiable*,unordered_set<Instance*>> InstanceMapType;
  private:
    unordered_map<Module*,InstanceMapType> modInstanceMap;
  public :
    static std::string ID;
    CreateInstanceMap() : ModulePass(ID,"Create Instance Map",true) {}
    bool runOnModule(Module* ns) override;
    void releaseMemory() override {
      modInstanceMap.clear();
    }
    InstanceMapType getInstanceMap(Module* m) {
      ASSERT(modInstanceMap.count(m),"Missing Module!" + m->getName());
      return modInstanceMap[m];
    }
};

}
}
#endif
