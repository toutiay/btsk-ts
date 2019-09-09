"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const MockNode_1 = require("./MockNode");
const Behavior_1 = require("./Behavior");
const Test_1 = require("../BehaviorTree/Test");
const Selector_1 = require("./Selector");
const Sequence_1 = require("./Sequence");
const MockComposite_1 = require("./MockComposite");
const Enum_1 = require("../Enum");
function BehaviorTreeShared() {
    TEST_TaskInitialize();
    TEST_TaskUpdate();
    TEST_TaskTerminate();
    TEST_SequenceTwoFails();
    TEST_SequenceOnePassThrough();
    TEST_SequenceTwoContinues();
    TEST_SelectorOnePassThrough();
    TEST_SelectorTwoContinues();
    TEST_SelectorTwoSucceeds();
}
exports.BehaviorTreeShared = BehaviorTreeShared;
function TEST_TaskInitialize() {
    let n = new MockNode_1.MockNode;
    let b = new Behavior_1.Behavior(n);
    let t = b.get();
    Test_1.CHECK_EQUAL(0, t.m_iInitializeCalled);
    b.tick();
    Test_1.CHECK_EQUAL(1, t.m_iInitializeCalled);
}
;
function TEST_TaskUpdate() {
    let n = new MockNode_1.MockNode;
    let b = new Behavior_1.Behavior(n);
    let t = b.get();
    Test_1.CHECK_EQUAL(0, t.m_iUpdateCalled);
    b.tick();
    Test_1.CHECK_EQUAL(1, t.m_iUpdateCalled);
}
;
function TEST_TaskTerminate() {
    let n = new MockNode_1.MockNode;
    let b = new Behavior_1.Behavior(n);
    b.tick();
    let t = b.get();
    Test_1.CHECK_EQUAL(0, t.m_iTerminateCalled);
    t.m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    b.tick();
    Test_1.CHECK_EQUAL(1, t.m_iTerminateCalled);
}
;
function TEST_SequenceTwoFails() {
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(2);
    let bh = new Behavior_1.Behavior(seq);
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_FAILURE);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SequenceTwoContinues() {
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(2);
    let bh = new Behavior_1.Behavior(seq);
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SequenceOnePassThrough() {
    let status = [Enum_1.Status.BH_SUCCESS, Enum_1.Status.BH_FAILURE];
    for (let i = 0; i < 2; i++) {
        let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(1);
        let bh = new Behavior_1.Behavior(seq);
        Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
        Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = status[i];
        Test_1.CHECK_EQUAL(bh.tick(), status[i]);
        Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}
function TEST_SelectorOnePassThrough() {
    let status = [Enum_1.Status.BH_SUCCESS, Enum_1.Status.BH_FAILURE];
    for (let i = 0; i < 2; ++i) {
        let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
        let seq = new MockSelector(1);
        let bh = new Behavior_1.Behavior(seq);
        Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
        Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = status[i];
        Test_1.CHECK_EQUAL(bh.tick(), status[i]);
        Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}
function TEST_SelectorTwoContinues() {
    let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
    let seq = new MockSelector(2);
    let bh = new Behavior_1.Behavior(seq);
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SelectorTwoSucceeds() {
    let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
    let seq = new MockSelector(2);
    let bh = new Behavior_1.Behavior(seq);
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Test_1.CHECK_EQUAL(bh.tick(), Enum_1.Status.BH_SUCCESS);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
//# sourceMappingURL=Test.js.map