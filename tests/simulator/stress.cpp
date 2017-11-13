#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <fstream>
#include <string>
#include <iostream>

#include "fuzzing.hpp"

#include "coreir.h"
#include "coreir/passes/analysis/pass_sim.h"
#include "coreir/passes/transform/rungenerators.h"
#include "coreir/simulator/interpreter.h"

#include "../src/simulator/output.hpp"
#include "../src/simulator/sim.hpp"
#include "../src/simulator/utils.hpp"

#include <iostream>

using namespace CoreIR;
using namespace CoreIR::Passes;
using namespace std;

namespace CoreIR {

  set<vdisc> connectedComponent(const vdisc v, NGraph& gr) {
    set<vdisc> cc;

    vector<vdisc> rem{v};

    while (rem.size() > 0) {
      vdisc nextNode = rem.back();
      rem.pop_back();

      cc.insert(nextNode);

      for (auto& ed : gr.inEdges(nextNode)) {
        vdisc other = gr.source(ed);
        vdisc thisN = gr.target(ed);

        assert(thisN == nextNode);

        if (cc.find(other) == end(cc)) {
          WireNode wd = gr.getNode(other);
          if (!isGraphInput(wd)) {
            rem.push_back(other);
          }
        }
      }

      for (auto& ed : gr.outEdges(nextNode)) {
        vdisc other = gr.target(ed);
        if (cc.find(other) == end(cc)) {
          WireNode wd = gr.getNode(other);
          if (!isGraphInput(wd)) {
            rem.push_back(other);
          }
          //rem.push_back(other);
        }
      }
    
    }

    return cc;
  }

  void balancedComponentsParallel(NGraph& gr) {
    set<vdisc> nodes;
    for (auto& vd : gr.getVerts()) {
      nodes.insert(vd);
    }

    int numComponents = 0;

    vector<set<vdisc> > ccs;
    for (auto& vd : gr.getVerts()) {

      WireNode wd = gr.getNode(vd);

      if (!isGraphInput(wd)) {
        if (nodes.find(vd) != end(nodes)) {
          set<vdisc> ccNodes =
            connectedComponent(vd, gr);

          //cout << "CC size = " << ccNodes.size() << endl;

          for (auto& ccNode : ccNodes) {
            nodes.erase(ccNode);

            WireNode w = gr.getNode(ccNode);
            w.setThreadNo(numComponents);
            gr.addVertLabel(ccNode, w);
          }
      

          ccs.push_back(ccNodes);
          numComponents++;
        }
      }
    }

    cout << "# of connected components = " << numComponents << endl;

    // Now balance the components
    //int nThreads = 2;
    int i = 0;
    for (auto& cc : ccs) {
      for (auto& vd : cc) {
        WireNode w = gr.getNode(vd);
        //w.setThreadNo((i % 2) + 1);
        w.setThreadNo((i % 2) + 1); 
        gr.addVertLabel(vd, w);
      }
      i++;
    }
  }
  
  void setThreadNumbers(NGraph& gr) {
    for (auto& v : gr.getVerts()) {
      WireNode w = gr.getNode(v);

      if (isGraphInput(w)) {
	cout << "Input = " << w.getWire()->toString() << endl;
      	w.setThreadNo(0);
      } else {
      	w.setThreadNo(13);
      }

      //w.setThreadNo(13);

      //cout << "Thread number before setting = " << gr.getNode(v).getThreadNo() << endl;
      gr.addVertLabel(v, w);

      //cout << "Thread number after setting  = " << gr.getNode(v).getThreadNo() << endl;
    }
  }

  Module* buildModule(const uint numOutputs,
                      Context* c,
                      Namespace* g,
                      const std::string& name) {
    uint n = 16;
    uint numInputs = numOutputs*2;
    
    Generator* and2 = c->getGenerator("coreir.and");
    Generator* or2 = c->getGenerator("coreir.or");

    // Define Add4 Module
    RecordParams opParams = {
      {"clk", c->Named("coreir.clkIn")}};

    cout << "Creating recordparams" << endl;
    for (uint i = 0; i < numInputs; i++) {
      opParams.push_back({"in_" + to_string(i), c->Array(n,c->BitIn())});
    }

    for (uint i = 0; i < numOutputs; i++) {
      opParams.push_back({"out_" + to_string(i), c->Array(n, c->Bit())});
    }

    cout << "Creating module" << endl;

    Type* manyOpsType = c->Record(opParams);
    Module* manyOps = g->newModuleDecl(name, manyOpsType);
    ModuleDef* def = manyOps->newModuleDef();
    Wireable* self = def->sel("self");

      
    cout << "Wiring up inputs" << endl;

    vector<Wireable*> ops;
    vector<Wireable*> regs;
    for (uint i = 0; i < numOutputs; i++) {
      Wireable* op;
      if ((i % 2) == 0) {
        op =
          def->addInstance("and_" + to_string(i), and2, {{"width", Const::make(c,n)}});
      } else {
        op =
          def->addInstance("or_" + to_string(i), or2, {{"width", Const::make(c,n)}});
      }

      auto reg = def->addInstance("reg_" + to_string(i),
                                  "coreir.reg",
                                  {{"width", Const::make(c, n)}});

      ops.push_back(op);
      regs.push_back(reg);

    }

    cout << "Created ALL instances" << endl;

    cout << "Creating dummy selects" << endl;

    for (uint i = 0; i < numOutputs; i++) {

      auto op = ops[i];
      auto reg = regs[i];

      auto s1 = self->sel("in_" + to_string(2*i));
      auto s2 = op->sel("in0");
      auto s3 = self->sel("in_" + to_string(2*i + 1));
      auto s4 = op->sel("in1");

      auto s5 = op->sel("out");
      auto s6 = reg->sel("in");
      auto s7 = self->sel("clk");
      auto s8 = reg->sel("clk");

      auto s9 = reg->sel("out");
      auto s10 = self->sel("out_" + to_string(i));

      if ((i % 1000) == 0) {
        cout << "Selects " << i << "!!!" << endl;
      }

    }

    cout << "Done with selects" << endl;

    for (uint i = 0; i < numOutputs; i++) {

      auto op = ops[i];
      auto reg = regs[i];

      def->connect(self->sel("in_" + to_string(2*i)), op->sel("in0"));
      def->connect(self->sel("in_" + to_string(2*i + 1)), op->sel("in1"));

      def->connect(op->sel("out"), reg->sel("in"));
      def->connect(self->sel("clk"), reg->sel("clk"));

      def->connect(reg->sel("out"), self->sel("out_" + to_string(i)));

      if ((i % 1000) == 0) {
        cout << "Wired up inputs " << i << endl;
      }
    }
      
    cout << "Setting definition" << endl;

    //assert(false);

    manyOps->setDef(def);

    cout << "Running passes" << endl;

    c->runPasses({"rungenerators"}); //, "flattentypes"}); //, "flatten"});

    return manyOps;
  }

  TEST_CASE("Large circuits for testing") {
    Context* c = newContext();
    Namespace* g = c->getGlobal();


    SECTION("Many logical operations in parallel") {
      //uint numOutputs = 100000;
      //uint numOutputs = 10000;
  

      // cout << "Writing to json" << endl;
      // if (!saveToFile(g, "many_ops.json", manyOps)) {
      //   cout << "Could not save to json!!" << endl;
      //   c->die();
      // }

      // Simulation code

      SECTION("Interpreting code") {
        // cout << "Starting passes" << endl;
        // cout << "Done with passes" << endl;
        // cout << "Setting up interpreter" << endl;
        uint numOutputs = 10;

        while (numOutputs <= 1e5) {

          auto manyOps = buildModule(numOutputs, c, g, "manyOps_" + to_string(numOutputs));

          clock_t start, end;

          start = clock();

          SimulatorState state(manyOps);

          end = clock();

          std::cout << "Time to startup with " << state.getCircuitGraph().getVerts().size() << " nodes : " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

          state.setClock("self.clk", 0, 1);

          uint numInputs = numOutputs*2;

          for (uint i = 0; i < numInputs; i++) {
            state.setValue("self.in_" + to_string(i), BitVector(16, 1)); //.push_back({"in_" + to_string(i), c->Array(n,c->BitIn())});
          }

          uint nCycles = 10;

          start = clock();
          for (uint i = 0; i < nCycles; i++) {
            state.execute();
          }
          end = clock();

          double totalTime = 
            (end - start) / (double)(CLOCKS_PER_SEC / 1000);
          double timePerCycle =
            totalTime / (double) nCycles;

          cout << "Time per cycle with " << state.getCircuitGraph().getVerts().size() << " nodes : " << timePerCycle << " ms" << endl;
          
          numOutputs = numOutputs * 10;

          
        }

      }
      
      // NGraph gr;
      // buildOrderedGraph(manyOps, gr);

      // balancedComponentsParallel(gr);

      // cout << "Starting manyOpsological sort" << endl;

      // // Delete inputs from the order, since they do not need to
      // // be printed out
      // deque<vdisc> topoOrder = topologicalSort(gr);

      // string codePath = "./gencode/";
      // string codeFile = manyOps->getName() + "_sim.cpp";
      // string hFile = manyOps->getName() + "_sim.h";

      // writeBitVectorLib(codePath + "bit_vector.h");

      // cout << "Writing out files" << endl;

      // writeFiles(topoOrder, gr, manyOps, codePath, codeFile, hFile);
      

      // SECTION("Compiling code") {
      //   c->runPasses({"rungenerators", "flattentypes"});

      // 	cout << "About to build graph" << endl;

      // 	NGraph gr;
      // 	buildOrderedGraph(manyOps, gr);

      //   SECTION("3 topological levels") {
      //     vector<vector<vdisc>> topoLevels =
      //       topologicalLevels(gr);

      //     REQUIRE(topoLevels.size() == 3);
      //   }

      // 	setThreadNumbers(gr);

      // 	cout << "Built ordered graph" << endl;
      // 	deque<vdisc> topoOrder = topologicalSort(gr);

      // 	cout << "Topologically sorted" << endl;

      // 	string randIns =
      // 	  randomSimInputHarness(manyOps);

      // 	cout << "Generating harness" << endl;

      // 	int s =
      // 	  generateHarnessAndRun(topoOrder, gr, manyOps,
      // 				"./gencode/",
      // 				"many_ops",
      // 				"./gencode/auto_harness_many_ops.cpp");

      // 	REQUIRE(s == 0);
      // }

    }

    deleteContext(c);
  }

}
