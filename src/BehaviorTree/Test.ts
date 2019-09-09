import { Selector } from './Selector';
import { ActiveSelector } from './ActiveSelector';
import { Parallel } from "./Parallel";
import { MockBehavior } from "./MockBehavior";
import { createClass } from './MockComposite';
import { Sequence } from './Sequence';
import { Status, Policy } from '../Enum';

export function ASSERT(bool: boolean) {
    if (!bool) {
        console.log("ASSERT.bool == " + bool);
    }
}

export function CHECK(X: number) {
    if (!X) {
        console.log("CHECK.X == " + X);
    }
}

export function CHECK_EQUAL(X: number, Y: number) {
    if (X != Y) {
        console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
        console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
    }
}

export function testBehaviorTree() {
    TEST_TaskInitialize()

    TEST_TaskUpdate()

    TEST_TaskTerminate()

    TEST_SequenceTwoChildrenFails()

    TEST_SequenceTwoChildrenContinues()

    TEST_SequenceOneChildPassThrough()

    TEST_SelectorTwoChildrenContinues()

    TEST_SelectorTwoChildrenSucceeds()

    TEST_SelectorOneChildPassThrough()

    TEST_ParallelSucceedRequireAll()

    TEST_ParallelSucceedRequireOne()

    TEST_ParallelFailureRequireAll()

    TEST_ParallelFailureRequireOne()

    TEST_ActiveBinarySelector()
}

function TEST_TaskInitialize() {
    let t = new MockBehavior();
    CHECK_EQUAL(0, t.m_iInitializeCalled);

    t.tick();
    CHECK_EQUAL(1, t.m_iInitializeCalled);
}

function TEST_TaskUpdate() {
    let t = new MockBehavior();
    CHECK_EQUAL(0, t.m_iUpdateCalled);

    t.tick();
    CHECK_EQUAL(1, t.m_iUpdateCalled);
}

function TEST_TaskTerminate() {
    let t = new MockBehavior();
    t.tick();
    CHECK_EQUAL(0, t.m_iTerminateCalled);

    t.m_eReturnStatus = Status.BH_SUCCESS;
    t.tick();
    CHECK_EQUAL(1, t.m_iTerminateCalled);
}

function TEST_SequenceTwoChildrenFails() {
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(2);

    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(seq.tick(), Status.BH_FAILURE);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
}

function TEST_SequenceTwoChildrenContinues() {
    let MockSequence = createClass("MockSequence", Sequence);
    let seq = new MockSequence(2);

    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
}

function TEST_SequenceOneChildPassThrough() {
    let status: number[] = [Status.BH_SUCCESS, Status.BH_FAILURE];
    for (let i = 0; i < status.length; i++) {
        let MockSequence = createClass("MockSequence", Sequence);
        let seq = new MockSequence(1);
        CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = status[i];
        CHECK_EQUAL(seq.tick(), status[i]);
        CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
}

function TEST_SelectorTwoChildrenContinues() {
    let MockSelector = createClass("MockSelector", Selector);
    let seq = new MockSelector(2);

    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SelectorTwoChildrenSucceeds() {
    let MockSelector = createClass("MockSelector", Selector);
    let seq = new MockSelector(2);

    CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

    seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(seq.tick(), Status.BH_SUCCESS);
    CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
}

function TEST_SelectorOneChildPassThrough() {
    let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
    for (let i = 0; i < status.length; i++) {
        {
            let MockSelector = createClass("MockSelector", Selector);
            let seq = new MockSelector(1);

            CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
            CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

            seq.getOperator(0).m_eReturnStatus = status[i];
            CHECK_EQUAL(seq.tick(), status[i]);
            CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
}

function TEST_ParallelSucceedRequireAll() {
    let parallel = new Parallel(Policy.RequireAll, Policy.RequireOne);
    let children = [new MockBehavior(), new MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);

    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[1].m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(Status.BH_SUCCESS, parallel.tick());
}

function TEST_ParallelSucceedRequireOne() {
    let parallel = new Parallel(Policy.RequireOne, Policy.RequireAll);
    let children = [new MockBehavior(), new MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);

    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(Status.BH_SUCCESS, parallel.tick());
}

function TEST_ParallelFailureRequireAll() {
    let parallel = new Parallel(Policy.RequireOne, Policy.RequireAll);
    let children = [new MockBehavior(), new MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);

    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[1].m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(Status.BH_FAILURE, parallel.tick());
}

function TEST_ParallelFailureRequireOne() {
    let parallel = new Parallel(Policy.RequireAll, Policy.RequireOne);
    let children = [new MockBehavior(), new MockBehavior()];
    parallel.addChild(children[0]);
    parallel.addChild(children[1]);

    CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
    children[0].m_eReturnStatus = Status.BH_FAILURE;
    CHECK_EQUAL(Status.BH_FAILURE, parallel.tick());
}

function TEST_ActiveBinarySelector() {
    let MockActiveSelector = createClass("MockActiveSelector", ActiveSelector);
    let sel = new MockActiveSelector(2);

    sel.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
    sel.getOperator(1).m_eReturnStatus = Status.BH_RUNNING;

    CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
    CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);

    CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
    CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);

    sel.getOperator(0).m_eReturnStatus = Status.BH_RUNNING;
    CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
    CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
    CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);

    sel.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
    CHECK_EQUAL(sel.tick(), Status.BH_SUCCESS);
    CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
    CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
    CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
}