"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const ActiveSelector_1 = require("./../BehaviorTree/ActiveSelector");
const Parallel_1 = require("../BehaviorTree/Parallel");
const Status_1 = require("../Enum/Status");
const Policy_1 = require("../Enum/Policy");
const MockBehavior_1 = require("./MockBehavior");
const MockNode_1 = require("../BehaviorTreeShared/MockNode");
const MockComposite_1 = require("./MockComposite");
const Sequence_1 = require("../BehaviorTree/Sequence");
const Selector_1 = require("../BehaviorTree/Selector");
const Sequence_2 = require("./../BehaviorTreeShared/Sequence");
const Behavior_1 = require("../BehaviorTreeShared/Behavior");
function ASSERT(bool) {
    if (!bool) {
        console.log("ASSERT.bool == " + bool);
    }
}
exports.ASSERT = ASSERT;
function CHECK(X) {
    if (!X) {
        console.log("CHECK.X == " + X);
    }
}
exports.CHECK = CHECK;
function CHECK_EQUAL(X, Y) {
    if (X != Y) {
        console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
        console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
    }
}
exports.CHECK_EQUAL = CHECK_EQUAL;
function testAll() {
    // Test.TEST_TaskInitialize()
    // Test.TEST_TaskUpdate()
    // Test.TEST_TaskTerminate()
    // Test.TEST_SequenceTwoChildrenFails()
    // Test.TEST_SequenceTwoChildrenContinues()
    // Test.TEST_SequenceOneChildPassThrough()
    // Test.TEST_SelectorTwoChildrenContinues()
    // Test.TEST_SelectorTwoChildrenSucceeds()
    // Test.TEST_SelectorOneChildPassThrough()
    // Test.TEST_ParallelSucceedRequireAll()
    // Test.TEST_ParallelSucceedRequireOne()
    // Test.TEST_ParallelFailureRequireAll()
    // Test.TEST_ParallelFailureRequireOne()
    // Test.TEST_ActiveBinarySelector()
    Test.TEST_TaskInitialize_Node();
    Test.TEST_TaskUpdate_Node();
    Test.TEST_TaskTerminate_Node();
    Test.TEST_SequenceTwoFails();
    Test.TEST_SequenceTwoContinues();
    Test.TEST_SequenceOnePassThrough();
}
exports.testAll = testAll;
//  测试相关
class Test {
    static TEST_TaskInitialize() {
        let t = new MockBehavior_1.MockBehavior();
        CHECK_EQUAL(0, t.m_iInitializeCalled);
        t.tick();
        CHECK_EQUAL(1, t.m_iInitializeCalled);
    }
    static TEST_TaskUpdate() {
        let t = new MockBehavior_1.MockBehavior();
        CHECK_EQUAL(0, t.m_iUpdateCalled);
        t.tick();
        CHECK_EQUAL(1, t.m_iUpdateCalled);
    }
    static TEST_TaskTerminate() {
        let t = new MockBehavior_1.MockBehavior();
        t.tick();
        CHECK_EQUAL(0, t.m_iTerminateCalled);
        t.m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        t.tick();
        CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
    static TEST_SequenceTwoChildrenFails() {
        let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(2);
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_FAILURE);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceTwoChildrenContinues() {
        let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(2);
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceOneChildPassThrough() {
        let status = [Status_1.Status.BH_SUCCESS, Status_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
            let seq = new MockSequence(1);
            CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
            CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
            seq.getOperator(0).m_eReturnStatus = status[i];
            CHECK_EQUAL(seq.tick(), status[i]);
            CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
    static TEST_SelectorTwoChildrenContinues() {
        let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
        let seq = new MockSelector(2);
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorTwoChildrenSucceeds() {
        let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
        let seq = new MockSelector(2);
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(seq.tick(), Status_1.Status.BH_SUCCESS);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorOneChildPassThrough() {
        let status = [Status_1.Status.BH_SUCCESS, Status_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            {
                let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
                let seq = new MockSelector(1);
                CHECK_EQUAL(seq.tick(), Status_1.Status.BH_RUNNING);
                CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
                seq.getOperator(0).m_eReturnStatus = status[i];
                CHECK_EQUAL(seq.tick(), status[i]);
                CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
            }
        }
    }
    static TEST_ParallelSucceedRequireAll() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireAll, Policy_1.Policy.RequireOne);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(Status_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelSucceedRequireOne() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireOne, Policy_1.Policy.RequireAll);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(Status_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelFailureRequireAll() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireOne, Policy_1.Policy.RequireAll);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(Status_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ParallelFailureRequireOne() {
        let parallel = new Parallel_1.Parallel(Policy_1.Policy.RequireAll, Policy_1.Policy.RequireOne);
        let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        CHECK_EQUAL(Status_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(Status_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ActiveBinarySelector() {
        let MockActiveSelector = MockComposite_1.createClass("MockActiveSelector", ActiveSelector_1.ActiveSelector);
        let sel = new MockActiveSelector(2);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        sel.getOperator(1).m_eReturnStatus = Status_1.Status.BH_RUNNING;
        CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
        CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
        CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_RUNNING;
        CHECK_EQUAL(sel.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(sel.tick(), Status_1.Status.BH_SUCCESS);
        CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
    }
    static TEST_TaskInitialize_Node() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        let t = b.get();
        CHECK_EQUAL(0, t.m_iInitializeCalled);
        b.tick();
        CHECK_EQUAL(1, t.m_iInitializeCalled);
    }
    ;
    static TEST_TaskUpdate_Node() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        let t = b.get();
        CHECK_EQUAL(0, t.m_iUpdateCalled);
        b.tick();
        CHECK_EQUAL(1, t.m_iUpdateCalled);
    }
    ;
    static TEST_TaskTerminate_Node() {
        let n = new MockNode_1.MockNode;
        let b = new Behavior_1.Behavior(n);
        b.tick();
        let t = b.get();
        CHECK_EQUAL(0, t.m_iTerminateCalled);
        t.m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        b.tick();
        CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
    ;
    static TEST_SequenceTwoFails() {
        let MockSequenceShared = MockComposite_1.createClass1("MockSequence", Sequence_2.Sequence);
        let seq = new MockSequenceShared(2);
        let bh = new Behavior_1.Behavior(seq);
        CHECK_EQUAL(bh.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_FAILURE;
        CHECK_EQUAL(bh.tick(), Status_1.Status.BH_FAILURE);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SequenceTwoContinues() {
        let MockSequenceShared = MockComposite_1.createClass1("MockSequence", Sequence_2.Sequence);
        let seq = new MockSequenceShared(2);
        let bh = new Behavior_1.Behavior(seq);
        CHECK_EQUAL(bh.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = Status_1.Status.BH_SUCCESS;
        CHECK_EQUAL(bh.tick(), Status_1.Status.BH_RUNNING);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SequenceOnePassThrough() {
        let status = [Status_1.Status.BH_SUCCESS, Status_1.Status.BH_FAILURE];
        for (let i = 0; i < 2; i++) {
            let MockSequenceShared = MockComposite_1.createClass1("MockSequence", Sequence_2.Sequence);
            let seq = new MockSequenceShared(1);
            let bh = new Behavior_1.Behavior(seq);
            CHECK_EQUAL(bh.tick(), Status_1.Status.BH_RUNNING);
            CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
            seq.getOperator(0).m_eReturnStatus = status[i];
            CHECK_EQUAL(bh.tick(), status[i]);
            CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
}
exports.default = Test;
//# sourceMappingURL=Test.js.map