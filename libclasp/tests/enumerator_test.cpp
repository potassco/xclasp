// 
// Copyright (c) 2006, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include "test.h"
#include "lpcompare.h"
#include <clasp/solver.h>
#include <clasp/program_builder.h>
#include <clasp/unfounded_check.h>
#include <clasp/minimize_constraint.h>
#include <clasp/model_enumerators.h>
#include <sstream>
using namespace std;
namespace Clasp { namespace Test {
using namespace Clasp::Asp;
class EnumeratorTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(EnumeratorTest);
	CPPUNIT_TEST(testProjectToOutput);
	CPPUNIT_TEST(testProjectExplicit);
	
	CPPUNIT_TEST(testMiniProject);
	CPPUNIT_TEST(testProjectBug);
	CPPUNIT_TEST(testProjectRestart);
	CPPUNIT_TEST(testProjectWithBacktracking);
	CPPUNIT_TEST(testTerminateRemovesWatches);
	CPPUNIT_TEST(testParallelRecord);
	CPPUNIT_TEST(testParallelUpdate);
	CPPUNIT_TEST(testTagLiteral);
	CPPUNIT_TEST(testIgnoreTagLiteralInPath);
	CPPUNIT_TEST(testSplittable);
	CPPUNIT_TEST(testLearnStepLiteral);
	CPPUNIT_TEST(testAssignStepLiteral);
	CPPUNIT_TEST_SUITE_END();	
public:
	void testProjectToOutput() {
		SharedContext ctx;
		Var v1 = ctx.addVar(Var_t::Atom);
		Var v2 = ctx.addVar(Var_t::Atom);
		Var v3 = ctx.addVar(Var_t::Atom);
		ctx.output.add("a", posLit(v1));
		ctx.output.add("_aux", posLit(v2));
		ctx.output.add("b", posLit(v3));
		ctx.startAddConstraints();
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_simple, '_');
		e.init(ctx);
		ctx.endInit();
		CPPUNIT_ASSERT(ctx.output.projectMode() == OutputTable::project_output);
		CPPUNIT_ASSERT(e.project(v1));
		CPPUNIT_ASSERT(e.project(v3));
		
		ctx.detach(0);
	}
	void testProjectExplicit() {
		SharedContext ctx;
		Var v1 = ctx.addVar(Var_t::Atom);
		Var v2 = ctx.addVar(Var_t::Atom);
		Var v3 = ctx.addVar(Var_t::Atom);
		ctx.output.add("a", posLit(v1));
		ctx.output.add("_aux", posLit(v2));
		ctx.output.add("b", posLit(v3));
		ctx.output.addProject(posLit(v2));
		ctx.output.addProject(posLit(v3));
		CPPUNIT_ASSERT(ctx.output.projectMode() == OutputTable::project_explicit);
		
		ctx.startAddConstraints();
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_simple, '_');
		e.init(ctx);
		ctx.endInit();
		CPPUNIT_ASSERT(!e.project(v1));
		CPPUNIT_ASSERT(e.project(v2));
		CPPUNIT_ASSERT(e.project(v3));
		ctx.detach(0);
	}
	void testMiniProject() {
		SharedContext ctx;
		Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()),
			"{x1;x2;x3}.\n"
			"x3 :- not x1.\n"
			"x3 :- not x2.\n"
			"#minimize{x3}.");
		builder.addOutput("a", 1);
		builder.addOutput("b", 2);
		builder.addOutput("_ignore_in_project", 3);
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_simple);
		e.init(ctx);
		ctx.endInit();
		
		solver.assume(builder.getLiteral(1));
		solver.propagate();
		solver.assume(builder.getLiteral(2));
		solver.propagate();
		solver.assume(builder.getLiteral(3));
		solver.propagate();
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		e.commitModel(solver);
		bool ok = e.update(solver) && solver.propagate();
		CPPUNIT_ASSERT(!ok);
		solver.backtrack();
		CPPUNIT_ASSERT(false == solver.propagate() && !solver.resolveConflict());
	}

	void testProjectBug() {
		SharedContext ctx;
		Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()),
			"{x1;x2;x4}.\n"
			"x5 :- x1, x4.\n"
			"x6 :- x2, x4.\n"
			"x3 :- x5, x6.\n");
		Var x = 1, y = 2, z = 3, _p = 4;
		builder.addOutput("x", x).addOutput("y", y).addOutput("z", z).addOutput("_p", _p);
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_full);
		e.init(ctx);
		ctx.endInit();
		
		solver.assume(builder.getLiteral(x));
		solver.propagate();
		solver.assume(builder.getLiteral(y));
		solver.propagate();
		solver.assume(builder.getLiteral(_p));
		solver.propagate();
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, e.commitModel(solver) && e.update(solver));

		solver.undoUntil(0);
		uint32 numT = 0;
		if (solver.value(builder.getLiteral(x).var()) == value_free) {
			solver.assume(builder.getLiteral(x)) && solver.propagate();
			++numT;
		}
		else if (solver.isTrue(builder.getLiteral(x))) {
			++numT;
		}
		if (solver.value(builder.getLiteral(y).var()) == value_free) {
			solver.assume(builder.getLiteral(y)) && solver.propagate();
			++numT;
		}
		else if (solver.isTrue(builder.getLiteral(y))) {
			++numT;
		}
		if (solver.value(builder.getLiteral(_p).var()) == value_free) {
			solver.assume(builder.getLiteral(_p)) && solver.propagate();
		}
		if (solver.isTrue(builder.getLiteral(z))) {
			++numT;
		}
		CPPUNIT_ASSERT(numT < 3);
	}
	
	void testProjectRestart() {
		SharedContext ctx;
		Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()), "{x1;x2;x3}.");
		builder.addOutput("a", 1).addOutput("b", 2);
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_full);
		e.init(ctx);
		ctx.endInit();
		
		
		solver.assume(builder.getLiteral(1));
		solver.propagate();
		solver.assume(builder.getLiteral(2));
		solver.propagate();
		solver.assume(builder.getLiteral(3));
		solver.propagate();
		solver.strategies().restartOnModel = true;
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, e.commitModel(solver));
		CPPUNIT_ASSERT_EQUAL(true, e.update(solver));
		CPPUNIT_ASSERT(solver.isFalse(builder.getLiteral(2)));
	}
	void testProjectWithBacktracking() {
		SharedContext ctx;
		Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()),
			"{x1, x2, x3}."
			"#output a : x1.");
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ModelEnumerator e;
		e.setStrategy(ModelEnumerator::strategy_backtrack, ModelEnumerator::project_enable_full);
		e.init(ctx);
		ctx.endInit();

		solver.assume(builder.getLiteral(2));
		solver.propagate();
		solver.assume(builder.getLiteral(3));
		solver.propagate();
		// x2@1 -x3
		solver.backtrack();
		// x1@1 -x3 a@2
		solver.assume(builder.getLiteral(1));
		solver.propagate();
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, e.commitModel(solver));
		e.update(solver);
		solver.undoUntil(0);
		while (solver.backtrack()) { ; }
		CPPUNIT_ASSERT(solver.isFalse(builder.getLiteral(1)));
	}
	void testTerminateRemovesWatches() {
		SharedContext ctx; Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()), "{x1;x2;x3;x4}.");
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ModelEnumerator e(ModelEnumerator::strategy_record);
		e.init(ctx);
		CPPUNIT_ASSERT_EQUAL(true, ctx.endInit());
		
		
		solver.assume(builder.getLiteral(1)) && solver.propagate();
		solver.assume(builder.getLiteral(2)) && solver.propagate();
		solver.assume(builder.getLiteral(3)) && solver.propagate();
		solver.assume(builder.getLiteral(4)) && solver.propagate();
		CPPUNIT_ASSERT_EQUAL(uint32(0), solver.numFreeVars());
		e.commitModel(solver) && e.update(solver);
		uint32 numW = solver.numWatches(builder.getLiteral(1)) + solver.numWatches(builder.getLiteral(2)) + solver.numWatches(builder.getLiteral(3)) + solver.numWatches(builder.getLiteral(4));
		CPPUNIT_ASSERT(numW > 0);
		ctx.detach(solver);
		numW = solver.numWatches(builder.getLiteral(1)) + solver.numWatches(builder.getLiteral(2)) + solver.numWatches(builder.getLiteral(3)) + solver.numWatches(builder.getLiteral(4));
		CPPUNIT_ASSERT(numW == 0);
	}

	void testParallelRecord() {
		SharedContext ctx; Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()), "{x1;x2;x3;x4}.");
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ctx.setConcurrency(2);
		ModelEnumerator e(ModelEnumerator::strategy_record);
		e.init(ctx);
		Solver& solver2 = ctx.pushSolver();
		ctx.endInit(true);
		
		solver.assume(builder.getLiteral(1)) && solver.propagate();
		solver.assume(builder.getLiteral(2)) && solver.propagate();
		solver.assume(builder.getLiteral(3)) && solver.propagate();
		solver.assume(builder.getLiteral(4)) && solver.propagate();
		CPPUNIT_ASSERT_EQUAL(uint32(0), solver.numFreeVars());
		e.commitModel(solver) && e.update(solver);
		solver.undoUntil(0);
		
		CPPUNIT_ASSERT_EQUAL(true, e.update(solver2));

		solver2.assume(builder.getLiteral(1)) && solver2.propagate();
		solver2.assume(builder.getLiteral(2)) && solver2.propagate();
		solver2.assume(builder.getLiteral(3)) && solver2.propagate();
		CPPUNIT_ASSERT(solver2.isFalse(builder.getLiteral(4)) && solver2.propagate());
		CPPUNIT_ASSERT_EQUAL(uint32(0), solver2.numFreeVars());
		e.commitModel(solver2) && e.update(solver2);
		solver.undoUntil(0);

		CPPUNIT_ASSERT_EQUAL(true, e.update(solver));

		solver.assume(builder.getLiteral(1)) && solver.propagate();
		solver.assume(builder.getLiteral(2)) && solver.propagate();
		CPPUNIT_ASSERT(solver.isFalse(builder.getLiteral(3)));

		ctx.detach(solver2);
		ctx.detach(solver);
		solver2.undoUntil(0);
		ctx.attach(solver2);
		solver2.assume(builder.getLiteral(1)) && solver2.propagate();
		solver2.assume(builder.getLiteral(2)) && solver2.propagate();
		solver2.assume(builder.getLiteral(3)) && solver2.propagate();
		CPPUNIT_ASSERT(solver2.value(builder.getLiteral(4).var()) == value_free);
	}

	void testParallelUpdate() {
		SharedContext ctx; Solver& solver = *ctx.master();
		lpAdd(builder.start(ctx, LogicProgram::AspOptions().noEq().noScc()), "{x1;x2;x3}. #minimize{x2}.");
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram());
		ctx.setConcurrency(2);
		ModelEnumerator e(ModelEnumerator::strategy_record);
		e.init(ctx);
		
		Solver& solver2 = ctx.pushSolver();
		ctx.endInit(true);
		
		// x1
		solver.assume(builder.getLiteral(1));
		solver.pushRootLevel(1);
		solver.propagate();
		// ~x2
		solver2.assume(~builder.getLiteral(1));
		solver2.pushRootLevel(1);
		solver2.propagate();

		// M1: ~x2, x3
		solver.assume(~builder.getLiteral(2));
		solver.propagate();
		solver.assume(builder.getLiteral(3));
		solver.propagate();	
		CPPUNIT_ASSERT_EQUAL(uint32(0), solver.numFreeVars());
		e.commitModel(solver);
		solver.undoUntil(0);
		e.update(solver);
		
		// M2: ~x2, ~x3
		solver2.assume(~builder.getLiteral(2));
		solver2.propagate();
		solver2.assume(~builder.getLiteral(3));
		solver2.propagate();	
		// M2 is NOT VALID!
		CPPUNIT_ASSERT_EQUAL(false, e.update(solver2));
	}

	void testTagLiteral() {
		ModelEnumerator e;
		SharedContext ctx;
		ctx.addVar(Var_t::Atom);
		ctx.addVar(Var_t::Atom);
		ctx.startAddConstraints();
		e.init(ctx);
		ctx.endInit();
		CPPUNIT_ASSERT(2 == ctx.numVars());
		e.start(*ctx.master());
		ctx.master()->pushTagVar(true);
		CPPUNIT_ASSERT(2 == ctx.numVars());
		CPPUNIT_ASSERT(3 == ctx.master()->numVars());
		CPPUNIT_ASSERT(ctx.master()->isTrue(ctx.master()->tagLiteral()));
	}
	void testIgnoreTagLiteralInPath() {
		SharedContext ctx;
		Var a = ctx.addVar(Var_t::Atom);
		ctx.addVar(Var_t::Atom);
		Solver& s1 = ctx.startAddConstraints();
		Solver& s2 = ctx.pushSolver();
		ctx.endInit();
		ctx.attach(s2);
		s1.pushRoot(posLit(a));
		s1.pushTagVar(true);
		CPPUNIT_ASSERT(s1.rootLevel() == 2 && s1.isTrue(s1.tagLiteral()));
		LitVec path;
		s1.copyGuidingPath(path);
		CPPUNIT_ASSERT(path.size() == 1 && path.back() == posLit(a));
	}
	void testSplittable() {
		SharedContext ctx;
		Var a = ctx.addVar(Var_t::Atom);
		Var b = ctx.addVar(Var_t::Atom);
		Var c = ctx.addVar(Var_t::Atom);
		Solver& s = ctx.startAddConstraints();
		ctx.endInit();
		s.pushRoot(posLit(a));
		CPPUNIT_ASSERT(!s.splittable());
		s.assume(posLit(b)) && s.propagate();
		CPPUNIT_ASSERT(s.splittable());
		s.pushTagVar(true);
		CPPUNIT_ASSERT(!s.splittable());
		s.assume(posLit(c)) && s.propagate();
		CPPUNIT_ASSERT(s.splittable());
		s.pushRootLevel();
		Var aux = s.pushAuxVar();
		s.assume(posLit(aux)) && s.propagate();
		CPPUNIT_ASSERT(!s.splittable());
	}
	void testLearnStepLiteral() {
		SharedContext ctx;
		ctx.requestStepVar();
		Var a = ctx.addVar(Var_t::Atom);
		ctx.addVar(Var_t::Atom);
		Solver& s1 = ctx.startAddConstraints();
		ctx.pushSolver();
		ctx.endInit(true);
		ClauseCreator cc(&s1);
		cc.start(Constraint_t::Conflict).add(posLit(a)).add(~ctx.stepLiteral()).end();
		ctx.unfreeze();
		ctx.endInit(true);
		s1.pushRoot(negLit(a));
		CPPUNIT_ASSERT(s1.value(ctx.stepLiteral().var()) == value_free);
	}

	void testAssignStepLiteral() {
		SharedContext ctx;
		ctx.requestStepVar();
		ctx.addVar(Var_t::Atom);
		ctx.addVar(Var_t::Atom);
		Solver& s = ctx.startAddConstraints();
		ctx.endInit();
		CPPUNIT_ASSERT(s.value(ctx.stepLiteral().var()) == value_free);
		ctx.addUnary(ctx.stepLiteral());
		CPPUNIT_ASSERT(s.value(ctx.stepLiteral().var()) != value_free);
		ctx.unfreeze();
		ctx.endInit();
		CPPUNIT_ASSERT(s.value(ctx.stepLiteral().var()) == value_free);
	}
private:
	LogicProgram builder;
	stringstream str;
	Model model;
};
CPPUNIT_TEST_SUITE_REGISTRATION(EnumeratorTest);
 } } 
