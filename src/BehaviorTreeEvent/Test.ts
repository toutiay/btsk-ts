import { MockBehavior } from "./MockBehavior";
import { BehaviorTree } from "./BehaviorTree";
import { Sequence } from "./Sequence";
import { createClass } from "./MockComposite";
import { Default, Status } from "../Enum";
import { CHECK_EQUAL } from "../Utils";

export function testBehaviorTreeEvent() {
    TEST_TaskInitialize();
    TEST_TaskUpdate();
    TEST_TaskTerminate();
    TEST_SequenceOnePassThrough();
    TEST_SequenceTwoFails();
    TEST_SequenceTwoContinues();
}

function TEST_TaskInitialize() {
    let t = new MockBehavior;
    let bt = new BehaviorTree;

    bt.start(t, Default.UNDEFINED);
    CHECK_EQUAL(0, t.m_iInitializeCalled);

    bt.tick();
    CHECK_EQUAL(1, t.m_iInitializeCalled);
};

function TEST_TaskUpdate() {
    let t = new MockBehavior;
    let bt = new BehaviorTree;

    bt.start(t, Default.UNDEFINED);
    bt.tick();
    CHECK_EQUAL(1, t.m_iUpdateCalled);

    t.m_eReturnStatus = Status.BH_SUCCESS;
    bt.tick();
    CHECK_EQUAL(2, t.m_iUpdateCalled);
};

function TEST_TaskTerminate() {
    let t = new MockBehavior;
    let bt = new BehaviorTree;

    bt.start(t, Default.UNDEFINED);
    bt.tick();
    CHECK_EQUAL(0, t.m_iTerminateCalled);

    t.m_eReturnStatus = Status.BH_SUCCESS;
    bt.tick();
    CHECK_EQUAL(1, t.m_iTerminateCalled);
};

function TEST_SequenceOnePassThrough() {
    let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
    for (let i = 0; i < 2; ++i) {
        let bt = new BehaviorTree;
        let MockSequence = createClass("MockSequence", Sequence);
        let seq = new MockSequence(bt, 1);

        bt.start(seq);
        bt.tick();
        CHECK_EQUAL(seq.m_eStatus, Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = status[i];
        bt.tick();
        CHECK_EQUAL(seq.m_eStatus, status[i]);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}

function TEST_SequenceTwoFails() {
    let bt = new BehaviorTree;
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(bt, 2);

    bt.start(seq);
    bt.tick();
    CHECK_EQUAL(seq.m_eStatus, Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    bt.tick();
    CHECK_EQUAL(seq.m_eStatus, Status.BH_FAILURE);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SequenceTwoContinues() {
    let bt = new BehaviorTree;
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(bt, 2);

    bt.start(seq);
    bt.tick();
    CHECK_EQUAL(seq.m_eStatus, Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    bt.tick();
    CHECK_EQUAL(seq.m_eStatus, Status.BH_RUNNING);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}
