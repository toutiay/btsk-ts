import { MockNode } from "./MockNode";
import { Behavior } from "./Behavior";
import { MockTask } from "./MockTask";
import { Selector } from "./Selector";
import { Sequence } from "./Sequence";
import { createClass } from "./MockComposite";
import { Status } from "../Enum";
import { CHECK_EQUAL } from "../Utils";

export function BehaviorTreeShared() {
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

function TEST_TaskInitialize() {
    let n = new MockNode;
    let b = new Behavior(n);

    let t = b.get<MockTask>();

    CHECK_EQUAL(0, t.m_iInitializeCalled);

    b.tick();
    CHECK_EQUAL(1, t.m_iInitializeCalled);
};

function TEST_TaskUpdate() {
    let n = new MockNode;
    let b = new Behavior(n);

    let t = b.get<MockTask>();
    CHECK_EQUAL(0, t.m_iUpdateCalled);

    b.tick();
    CHECK_EQUAL(1, t.m_iUpdateCalled);
};

function TEST_TaskTerminate() {
    let n = new MockNode;
    let b = new Behavior(n);
    b.tick();

    let t = b.get<MockTask>();
    CHECK_EQUAL(0, t.m_iTerminateCalled);

    t.m_eReturnStatus = Status.BH_SUCCESS;
    b.tick();
    CHECK_EQUAL(1, t.m_iTerminateCalled);
};

function TEST_SequenceTwoFails() {
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(2);
    let bh = new Behavior(seq);

    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(bh.tick(), Status.BH_FAILURE);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SequenceTwoContinues() {
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(2);
    let bh = new Behavior(seq);

    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SequenceOnePassThrough() {
    let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
    for (let i = 0; i < 2; i++) {
        let MockSequence = createClass("MockSequence", Sequence);
        let seq = new MockSequence(1);
        let bh = new Behavior(seq);

        CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = status[i];
        CHECK_EQUAL(bh.tick(), status[i]);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}

function TEST_SelectorOnePassThrough() {
    let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
    for (let i = 0; i < 2; ++i) {
        let MockSelector = createClass("MockSelector", Selector);
        let seq = new MockSelector(1);
        let bh = new Behavior(seq);

        CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = status[i];
        CHECK_EQUAL(bh.tick(), status[i]);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}

function TEST_SelectorTwoContinues() {
    let MockSelector = createClass("MockSelector", Selector);
    let seq = new MockSelector(2);
    let bh = new Behavior(seq);

    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SelectorTwoSucceeds() {
    let MockSelector = createClass("MockSelector", Selector);
    let seq = new MockSelector(2);
    let bh = new Behavior(seq);

    CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(bh.tick(), Status.BH_SUCCESS);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}