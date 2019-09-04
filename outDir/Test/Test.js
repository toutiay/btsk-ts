"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const ActiveSelector_1 = require("./../BehaviorTree/ActiveSelector");
const Parallel_1 = require("../BehaviorTree/Parallel");
const Status_1 = require("../Enum/Status");
const Policy_1 = require("../Enum/Policy");
const MockBehavior_1 = require("./MockBehavior");
// import { MockSequence, MockSelector, MockActiveSelector } from "./MockComposite";
const MockNode_1 = require("../BehaviorTreeShared/MockNode");
const Behavior_1 = require("../BehaviorTreeShared/Behavior");
const MockComposite_1 = require("./MockComposite");
const Sequence_1 = require("../BehaviorTree/Sequence");
const Selector_1 = require("../BehaviorTree/Selector");
//  测试相关
class Test {
    static ASSERT(bool) {
        if (!bool) {
            console.log("ASSERT.bool == " + bool);
        }
    }
    static CHECK(X) {
        if (!X) {
            console.log("CHECK.X == " + X);
        }
    }
    static CHECK_EQUAL(X, Y) {
        if (X != Y) {
            console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
            console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
        }
    }
    static TEST_TaskInitialize() {
        let t = new MockBehavior_1.MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    }
    static TEST_TaskUpdate() {
        let t = new MockBehavior_1.MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    }
    static TEST_TaskTerminate() {
        let t = new MockBehavior_1.MockBehavior();
        t.tick();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);
        t.m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
    static TEST_SequenceTwoChildrenFails() {
        let MockSequence = MockComposite_1.createInstance("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(2);
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_FAILURE);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceTwoChildrenContinues() {
        let MockSequence = MockComposite_1.createInstance("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(2);
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceOneChildPassThrough() {
        let status = [Status_1.Status.BH_SUCCESS, Status_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            let MockSequence = MockComposite_1.createInstance("MockSequence", Sequence_1.Sequence);
            let seq = new MockSequence(1);
            Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
            Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
            seq.getOperator(0).m_eReturnStatus = status[i];
            Test.CHECK_EQUAL(seq.tick(), status[i]);
            Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
    static TEST_SelectorTwoChildrenContinues() {
        let MockSelector = MockComposite_1.createInstance("MockSelector", Selector_1.Selector);
        let seq = new MockSelector(2);
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorTwoChildrenSucceeds() {
        let MockSelector = MockComposite_1.createInstance("MockSelector", Selector_1.Selector);
        let seq = new MockSelector(2);
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_SUCCESS);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorOneChildPassThrough() {
        let status = [Status_1.Status.BH_SUCCESS, Status_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            {
                let MockSelector = MockComposite_1.createInstance("MockSelector", Selector_1.Selector);
                let seq = new MockSelector(1);
                Test.CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
                Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
                seq.getOperator(0).m_eReturnStatus = status[i];
                Test.CHECK_EQUAL(seq.tick(), status[i]);
                Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
            }
        }
    }
    static TEST_ParallelSucceedRequireAll() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireAll, Policy_1.Policy.RequireOne);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelSucceedRequireOne() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireOne, Policy_1.Policy.RequireAll);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelFailureRequireAll() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireOne, Policy_1.Policy.RequireAll);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ParallelFailureRequireOne() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireAll, Policy_1.Policy.RequireOne);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ActiveBinarySelector() {
        let MockActiveSelector = MockComposite_1.createInstance("MockActiveSelector", ActiveSelector_1.ActiveSelector);
        let sel = new MockActiveSelector(2);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        sel.getOperator(1).m_eReturnStatus = Status_1.Status.BH_RUNNING;
        Test.CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        Test.CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_RUNNING;
        Test.CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(sel.tick(), Status_1.Status.BH_SUCCESS);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
    }
    TEST_TaskInitialize() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        let t = b.get();
        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);
        b.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    }
    ;
    TEST_TaskUpdate() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        let t = b.get();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);
        b.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    }
    ;
    TEST_TaskTerminate() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        b.tick();
        let t = b.get();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);
        t.m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        b.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
    ;
}
exports.default = Test;
//# sourceMappingURL=Test.js.map