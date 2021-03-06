#include "namespace.hpp"
#include "typegen.hpp"

using namespace std;

namespace CoreIR {

bool operator==(const NamedCacheParams& l,const NamedCacheParams& r) {
  return (l.name==r.name) && (l.args==r.args);
}

size_t NamedCacheParamsHasher::operator()(const NamedCacheParams& ncp) const {
  size_t hash = 0;
  hash_combine(hash,ncp.name);
  hash_combine(hash,ncp.args);
  return hash;
}
  
Namespace::~Namespace() {
  for (auto m : moduleList) delete m.second;
  for (auto g : generatorList) delete g.second;
  for (auto n : namedTypeList) delete n.second;
  for (auto n : namedTypeGenCache) delete n.second;
  for (auto tg : typeGenList) delete tg.second;
}

NamedType* Namespace::newNamedType(string name, string nameFlip, Type* raw) {
  //Make sure the name and its flip are different
  assert(name != nameFlip);
  //Verify this name and the flipped name do not exist yet
  assert(!typeGenList.count(name) && !typeGenList.count(nameFlip));
  assert(!namedTypeList.count(name) && !namedTypeList.count(nameFlip) );
  
  //Add name to namedTypeNameMap
  namedTypeNameMap[name] = nameFlip;

  //Create two new NamedTypes
  NamedType* named = new NamedType(c,this,name,raw);
  NamedType* namedFlip = new NamedType(c,this,nameFlip,raw->getFlipped());
  named->setFlipped(namedFlip);
  namedFlip->setFlipped(named);
  namedTypeList[name] = named;
  namedTypeList[nameFlip] = namedFlip;
  return named;
}

//TODO
void Namespace::newNominalTypeGen(string name, string nameFlip, Params genparams, TypeGenFun fun) {
  //Make sure the name and its flip are different
  assert(name != nameFlip);
  //Verify this name and the flipped name do not exist yet
  assert(!typeGenList.count(name) && !typeGenList.count(nameFlip));
  assert(!namedTypeList.count(name) && !namedTypeList.count(nameFlip) );
 
  //Add name to typeGenNameMap
  typeGenNameMap[name] = nameFlip;

  //Create the TypeGens
  TypeGen* typegen = new TypeGenFromFun(this,name,genparams,fun,false);
  TypeGen* typegenFlip = new TypeGenFromFun(this,nameFlip,genparams,fun,true);
  
  //Add typegens into list
  typeGenList[name] = typegen;
  typeGenList[nameFlip] = typegenFlip;
}

bool Namespace::hasNamedType(string name) {
  return namedTypeList.count(name) > 0;
}

//This has to be found. Error if not found
NamedType* Namespace::getNamedType(string name) {
  auto found = namedTypeList.find(name);
  ASSERT(found != namedTypeList.end(),"Cannot find " + name);
  return found->second;
}

//Check if cached in namedTypeGenCache
//Make sure the name is found in the typeGenCache. Error otherwise
//Then create a new entry in NamedCache if it does not exist
NamedType* Namespace::getNamedType(string name, Args genargs) {
  NamedCacheParams ncp(name,genargs);
  auto namedFound = namedTypeGenCache.find(ncp);
  if (namedFound != namedTypeGenCache.end() ) {
    return namedFound->second;
  }
  
  //Not found. Verify that name exists in TypeGenList
  //TODO deal with the 'at' error possiblities
  if (typeGenList.count(name)==0 || typeGenNameMap.count(name)==0) {
    Error e;
    e.message("Could not Named Type!");
    e.message("  Namespace: " + this->name);
    e.message("  name: " + name);
    e.fatal();
    c->error(e);
  }
  ASSERT(typeGenList.count(name),"Missing " + name);
  TypeGen* tgen = typeGenList.at(name);
  ASSERT(typeGenNameMap.count(name),"Missing " + name);
  string nameFlip = typeGenNameMap.at(name);
  ASSERT(typeGenList.count(nameFlip),"Missing " + name);
  TypeGen* tgenFlip = typeGenList.at(nameFlip);
  NamedCacheParams ncpFlip(nameFlip,genargs);

  //Create two new named entries
  NamedType* named = new NamedType(c,this,name,tgen,genargs);
  NamedType* namedFlip = new NamedType(c,this,nameFlip,tgenFlip,genargs);
  named->setFlipped(namedFlip);
  namedFlip->setFlipped(named);
  namedTypeGenCache[ncp] = named;
  namedTypeGenCache[ncpFlip] = namedFlip;

  return named;
}
TypeGen* Namespace::newTypeGen(string name, Params genparams, TypeGenFun fun) {
  assert(namedTypeList.count(name)==0);
  assert(typeGenList.count(name)==0);
  
  TypeGen* typegen = new TypeGenFromFun(this,name,genparams,fun);
  
  //Add name to typeGenNameMap
  typeGenNameMap[name] = "";
  
  typeGenList[name] = typegen;
  return typegen;
}

//TODO deal with at errors
TypeGen* Namespace::getTypeGen(string name) {
  assert(typeGenList.count(name)>0);
  TypeGen* ret = typeGenList.at(name);
  assert(ret->getName()==name);
  return ret;
}



Generator* Namespace::newGeneratorDecl(string name,TypeGen* typegen, Params genparams, Params configparams) {
  //Make sure module does not already exist as a module or generator
  assert(moduleList.count(name)==0);
  assert(generatorList.count(name)==0);
  
  Generator* g = new Generator(this,name,typegen,genparams,configparams);
  generatorList.emplace(name,g);
  return g;
}

Module* Namespace::newModuleDecl(string name, Type* t, Params configparams) {
  //Make sure module does not already exist as a module or generator
  assert(moduleList.count(name)==0);
  assert(generatorList.count(name)==0);
  Module* m = new Module(this,name,t, configparams);
  moduleList[name] = m;
  return m;
}

void Namespace::addModule(Module* m) {
  ASSERT(m->getLinkageKind()==Instantiable::LK_Generated,"Cannot add Namespace module to another namespace!");
  string name = m->getName();
  assert(moduleList.count(name)==0);
  assert(generatorList.count(name)==0);
  m->setNamespace(this);
  m->setLinkageKind(Instantiable::LK_Namespace);
  moduleList[name] = m;
}

Generator* Namespace::getGenerator(string gname) {
  auto it = generatorList.find(gname);
  if (it != generatorList.end()) return it->second;
  Error e;
  e.message("Could not find Generator in namespace!");
  e.message("  Generator: " + gname);
  e.message("  Namespace: " + name);
  e.fatal();
  c->error(e);
  return nullptr;
}
Module* Namespace::getModule(string mname) {
  auto it = moduleList.find(mname);
  if (it != moduleList.end()) return it->second;
  Error e;
  e.message("Could not find Module in namespace!");
  e.message("  Module: " + mname);
  e.message("  Namespace: " + name);
  e.fatal();
  c->error(e);
  return nullptr;
}

Instantiable* Namespace::getInstantiable(string iname) {
  if (moduleList.count(iname) > 0) return moduleList.at(iname);
  if (generatorList.count(iname) > 0) return generatorList.at(iname);
  Error e;
  e.message("Could not find Instance in library!");
  e.message("  Instance: " + iname);
  e.message("  Namespace: " + name);
  e.fatal();
  c->error(e);
  return nullptr;
}

//TODO Update this
void Namespace::print() {
  cout << "Namespace: " << name << endl;
  cout << "  Generators:" << endl;
  for (auto it : generatorList) it.second->print();
  for (auto it : moduleList) it.second->print();
  cout << endl;
}

}//CoreIR namespace
