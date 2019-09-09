"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Selector_1 = require("./Selector");
const ActiveSelector_1 = require("./ActiveSelector");
const Parallel_1 = require("./Parallel");
const MockBehavior_1 = require("./MockBehavior");
const MockComposite_1 = require("./MockComposite");
const Sequence_1 = require("./Sequence");
const Enum_1 = require("../Enum");
const Utils_1 = require("../Utils");
function testBehaviorTree() {
    TEST_TaskInitialize();
    TEST_TaskUpdate();
    TEST_TaskTerminate();
    TEST_SequenceTwoChildrenFails();
    TEST_SequenceTwoChildrenContinues();
    TEST_SequenceOneChildPassThrough();
    TEST_SelectorTwoChildrenContinues();
    TEST_SelectorTwoChildrenSucceeds();
    TEST_SelectorOneChildPassThrough();
    TEST_ParallelSucceedRequireAll();
    TEST_ParallelSucceedRequireOne();
    TEST_ParallelFailureRequireAll();
    TEST_ParallelFailureRequireOne();
    TEST_ActiveBinarySelector();
}
exports.testBehaviorTree = testBehaviorTree;
function TEST_TaskInitialize() {
    let t = new MockBehavior_1.MockBehavior();
    Utils_1.CHECK_EQUAL(0, t.m_iInitializeCalled);
    t.tick();
    Utils_1.CHECK_EQUAL(1, t.m_iInitializeCalled);
}
function TEST_TaskUpdate() {
    let t = new MockBehavior_1.MockBehavior();
    Utils_1.CHECK_EQUAL(0, t.m_iUpdateCalled);
    t.tick();
    Utils_1.CHECK_EQUAL(1, t.m_iUpdateCalled);
}
function TEST_TaskTerminate() {
    let t = new MockBehavior_1.MockBehavior();
    t.tick();
    Utils_1.CHECK_EQUAL(0, t.m_iTerminateCalled);
    t.m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    t.tick();
    Utils_1.CHECK_EQUAL(1, t.m_iTerminateCalled);
}
function TEST_SequenceTwoChildrenFails() {
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(2);
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_FAILURE);
    Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
}
function TEST_SequenceTwoChildrenContinues() {
    let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
    let seq = new MockSequence(2);
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
}
function TEST_SequenceOneChildPassThrough() {
    let status = [Enum_1.Status.BH_SUCCESS, Enum_1.Status.BH_FAILURE];
    for (let i = 0; i < status.length; i++) {
        let MockSequence = MockComposite_1.createClass("MockSequence", Sequence_1.Sequence);
        let seq = new MockSequence(1);
        Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
        Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = status[i];
        Utils_1.CHECK_EQUAL(seq.tick(), status[i]);
        Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}
function TEST_SelectorTwoChildrenContinues() {
    let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
    let seq = new MockSelector(2);
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SelectorTwoChildrenSucceeds() {
    let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
    let seq = new MockSelector(2);
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    seq.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_SUCCESS);
    Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
function TEST_SelectorOneChildPassThrough() {
    let status = [Enum_1.Status.BH_SUCCESS, Enum_1.Status.BH_FAILURE];
    for (let i = 0; i < status.length; i++) {
        {
            let MockSelector = MockComposite_1.createClass("MockSelector", Selector_1.Selector);
            let seq = new MockSelector(1);
            Utils_1.CHECK_EQUAL(seq.tick(), Enum_1.Status.BH_RUNNING);
            Utils_1.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
            seq.getOperator(0).m_eReturnStatus = status[i];
            Utils_1.CHECK_EQUAL(seq.tick(), status[i]);
            Utils_1.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
}
function TEST_ParallelSucceedRequireAll() {
    let parallel = new Parallel_1.Parallel(Enum_1.Policy.RequireAll, Enum_1.Policy.RequireOne);
    let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[1].m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_SUCCESS, parallel.tick());
}
function TEST_ParallelSucceedRequireOne() {
    let parallel = new Parallel_1.Parallel(Enum_1.Policy.RequireOne, Enum_1.Policy.RequireAll);
    let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_SUCCESS, parallel.tick());
}
function TEST_ParallelFailureRequireAll() {
    let parallel = new Parallel_1.Parallel(Enum_1.Policy.RequireOne, Enum_1.Policy.RequireAll);
    let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[1].m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_FAILURE, parallel.tick());
}
function TEST_ParallelFailureRequireOne() {
    let parallel = new Parallel_1.Parallel(Enum_1.Policy.RequireAll, Enum_1.Policy.RequireOne);
    let children = [new MockBehavior_1.MockBehavior(), new MockBehavior_1.MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    Utils_1.CHECK_EQUAL(Enum_1.Status.BH_FAILURE, parallel.tick());
}
function TEST_ActiveBinarySelector() {
    let MockActiveSelector = MockComposite_1.createClass("MockActiveSelector", ActiveSelector_1.ActiveSelector);
    let sel = new MockActiveSelector(2);
    sel.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_FAILURE;
    sel.getOperator(1).m_eReturnStatus = Enum_1.Status.BH_RUNNING;
    Utils_1.CHECK_EQUAL(sel.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(sel.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
    sel.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_RUNNING;
    Utils_1.CHECK_EQUAL(sel.tick(), Enum_1.Status.BH_RUNNING);
    Utils_1.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
    sel.getOperator(0).m_eReturnStatus = Enum_1.Status.BH_SUCCESS;
    Utils_1.CHECK_EQUAL(sel.tick(), Enum_1.Status.BH_SUCCESS);
    Utils_1.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    Utils_1.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
}
//# sourceMappingURL=Test.js.map