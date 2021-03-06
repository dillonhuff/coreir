#ifndef RUNGENERATORS_HPP_
#define RUNGENERATORS_HPP_

#include "coreir.h"
// This will recusrively run all the generators and replace module definitions
// For every instance, if it is a generator, it 

namespace CoreIR {
namespace Passes {


class RunGenerators : public NamespacePass {
  public :
    static std::string ID;
    RunGenerators() : NamespacePass(ID,"Runs all generators") {}
    bool runOnNamespace(Namespace* ns);
};

}
}

#endif
