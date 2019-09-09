"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const MockBehavior_1 = require("./MockBehavior");
const BehaviorTree_1 = require("./BehaviorTree");
const Test_1 = require("../BehaviorTree/Test");
const Sequence_1 = require("./Sequence");
const MockComposite_1 = require("./MockComposite");
const Enum_1 = require("../Enum");
function testBehaviorTreeEvent() {
    TEST_TaskInitialize();
    TEST_TaskUpdate();
    TEST_TaskTerminate();
    TEST_SequenceOnePassThrough();
    TEST_SequenceTwoFails();
    TEST_SequenceTwoContinues();
}
exports.testBehaviorTreeEvent = testBehaviorTreeEvent;
function TEST_TaskInitialize() {
    let t = new MockBehavior_1.MockBehavior;
    let bt = new BehaviorTree_1.BehaviorTree;
    bt.start(t, Enum_1.Default.UNDEFINED);
    Test_1.CHECK_EQUAL(0, t.m_iInitializeCalled);
    bt.tick();
    Test_1.CHECK_EQUAL(1, t.m_iInitializeCalled);
}
;
function TEST_TaskUpdate() {
    let t = new MockBehavior_1.MockBehavior;
    let bt = new BehaviorTree_1.BehaviorTree;
    bt.start(t, Enum_1.Default.UNDEFINED);
    bt.tick();
    Test_1.CHECK_EQUAL(1, t.m_iUpdateCalled);
    t.m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    bt.tick();
    Test_1.CHECK_EQUAL(2, t.m_iUpdateCalled);
}
;
function TEST_TaskTerminate() {
    let t = new MockBehavior_1.MockBehavior;
    let bt = new BehaviorTree_1.BehaviorTree;
    bt.start(t, Enum_1.Default.UNDEFINED);
    bt.tick();
    Test_1.CHECK_EQUAL(0, t.m_iTerminateCalled);
    t.m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    bt.tick();
    Test_1.CHECK_EQUAL(1, t.m_iTerminateCalled);
}
;
function TEST_SequenceOnePassThrough() {
    let status = [Enum_1.Status.BH_SUCCESS, Enum_1.Status.BH_FAILURE];
    for (let i = 0; i < 2; ++i) {
        let bt = new BehaviorTree_1.BehaviorTree;
        let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(bt, 1);
        bt.start(seq);
        bt.tick();
        Test_1.CHECK_EQUAL(seq.m_eStatus, Enum_1.Status.BH_RUNNING);
        Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = status[i];
        bt.tick();
        Test_1.CHECK_EQUAL(seq.m_eStatus, status[i]);
        Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}
function TEST_SequenceTwoFails() {
    let bt = new BehaviorTree_1.BehaviorTree;
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(bt, 2);
    bt.start(seq);
    bt.tick();
    Test_1.CHECK_EQUAL(seq.m_eStatus, Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    bt.tick();
    Test_1.CHECK_EQUAL(seq.m_eStatus, Enum_1.Status.BH_FAILURE);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SequenceTwoContinues() {
    let bt = new BehaviorTree_1.BehaviorTree;
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(bt, 2);
    bt.start(seq);
    bt.tick();
    Test_1.CHECK_EQUAL(seq.m_eStatus, Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    bt.tick();
    Test_1.CHECK_EQUAL(seq.m_eStatus, Enum_1.Status.BH_RUNNING);
    Test_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
//# sourceMappingURL=Test.js.map