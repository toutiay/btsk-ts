import { ActiveSelector } from './../BehaviorTree/ActiveSelector';
import { MockTask } from './../BehaviorTreeShared/MockTask';
import { Parallel } from "../BehaviorTree/Parallel";
import { Status } from "../Enum/Status";
import { Policy } from "../Enum/Policy";
import { MockBehavior } from "./MockBehavior";
import { MockNode } from "../BehaviorTreeShared/MockNode";
import { createClass, createClass1 } from './MockComposite';
import { Sequence } from '../BehaviorTree/Sequence';
import { Selector } from '../BehaviorTree/Selector';
import { Sequence as SequenceShared } from './../BehaviorTreeShared/Sequence';
import { Behavior as BehaviorShared } from "../BehaviorTreeShared/Behavior";
import { Behavior } from '../BehaviorTree/Behavior';

//  测试相关
export default class Test {
    static ASSERT(bool: boolean) {
        if (!bool) {
            console.log("ASSERT.bool == " + bool);
        }
    }

    static CHECK(X: number) {
        if (!X) {
            console.log("CHECK.X == " + X);
        }
    }

    static CHECK_EQUAL(X: number, Y: number) {
        if (X != Y) {
            console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
            console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
        }
    }

    static TEST_TaskInitialize() {
        let t = new MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);

        t.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    }

    static TEST_TaskUpdate() {
        let t = new MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);

        t.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    }

    static TEST_TaskTerminate() {
        let t = new MockBehavior();
        t.tick();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);

        t.m_eReturnStatus = Status.BH_SUCCESS;
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    }

    static TEST_SequenceTwoChildrenFails() {
        let MockSequence = createClass("MockSequence", Sequence);
        let seq = new MockSequence(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_FAILURE);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    }

    static TEST_SequenceTwoChildrenContinues() {
        let MockSequence = createClass("MockSequence", Sequence);
        let seq = new MockSequence(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
    }

    static TEST_SequenceOneChildPassThrough() {
        let status: number[] = [Status.BH_SUCCESS, Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            let MockSequence = createClass("MockSequence", Sequence);
            let seq = new MockSequence(1);
            Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
            Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

            seq.getOperator(0).m_eReturnStatus = status[i];
            Test.CHECK_EQUAL(seq.tick(), status[i]);
            Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }

    static TEST_SelectorTwoChildrenContinues() {
        let MockSelector = createClass("MockSelector", Selector);
        let seq = new MockSelector(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }

    static TEST_SelectorTwoChildrenSucceeds() {
        let MockSelector = createClass("MockSelector", Selector);
        let seq = new MockSelector(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_SUCCESS);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }

    static TEST_SelectorOneChildPassThrough() {
        let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            {
                let MockSelector = createClass("MockSelector", Selector);
                let seq = new MockSelector(1);

                Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
                Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

                seq.getOperator(0).m_eReturnStatus = status[i];
                Test.CHECK_EQUAL(seq.tick(), status[i]);
                Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
            }
        }
    }

    static TEST_ParallelSucceedRequireAll() {
        let parallel = new Parallel(Policy.RequireAll, Policy.RequireOne);
        let children = [new MockBehavior(), new MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);

        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status.BH_SUCCESS, parallel.tick());
    }

    static TEST_ParallelSucceedRequireOne() {
        let parallel = new Parallel(Policy.RequireOne, Policy.RequireAll);
        let children = [new MockBehavior(), new MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);

        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(Status.BH_SUCCESS, parallel.tick());
    }

    static TEST_ParallelFailureRequireAll() {
        let parallel = new Parallel(Policy.RequireOne, Policy.RequireAll);
        let children = [new MockBehavior(), new MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);

        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status.BH_FAILURE, parallel.tick());
    }

    static TEST_ParallelFailureRequireOne() {
        let parallel = new Parallel(Policy.RequireAll, Policy.RequireOne);
        let children = [new MockBehavior(), new MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);

        Test.CHECK_EQUAL(Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(Status.BH_FAILURE, parallel.tick());
    }

    static TEST_ActiveBinarySelector() {
        let MockActiveSelector = createClass("MockActiveSelector", ActiveSelector);
        let sel = new MockActiveSelector(2);

        sel.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        sel.getOperator(1).m_eReturnStatus = Status.BH_RUNNING;

        Test.CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);

        Test.CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);

        sel.getOperator(0).m_eReturnStatus = Status.BH_RUNNING;
        Test.CHECK_EQUAL(sel.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);

        sel.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(sel.tick(), Status.BH_SUCCESS);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
    }


    static TEST_TaskInitialize_Node() {
        let n = new MockNode;
        let b = new BehaviorShared(n);

        let t = b.get<MockTask>();

        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);

        b.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    };

    static TEST_TaskUpdate_Node() {
        let n = new MockNode;
        let b = new BehaviorShared(n);

        let t = b.get<MockTask>();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);

        b.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    };

    static TEST_TaskTerminate_Node() {
        let n = new MockNode;
        let b = new BehaviorShared(n);
        b.tick();

        let t = b.get<MockTask>();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);

        t.m_eReturnStatus = Status.BH_SUCCESS;
        b.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    };

    static TEST_SequenceTwoFails() {
        let MockSequenceShared = createClass1("MockSequence", SequenceShared);
        let seq = new MockSequenceShared(2);
        let bh = new BehaviorShared(seq);

        Test.CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(bh.tick(), Status.BH_FAILURE);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }

    static TEST_SequenceTwoContinues() {
        let MockSequenceShared = createClass1("MockSequence", SequenceShared);
        let seq = new MockSequenceShared(2);
        let bh = new BehaviorShared(seq);

        Test.CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_SUCCESS;
        Test.CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }

    static TEST_SequenceOnePassThrough() {
        let status = [Status.BH_SUCCESS, Status.BH_FAILURE];
        for (let i = 0; i < 2; i++) {
            let MockSequenceShared = createClass1("MockSequence", SequenceShared);
            let seq = new MockSequenceShared(1);
            let bh = new BehaviorShared(seq);

            Test.CHECK_EQUAL(bh.tick(), Status.BH_RUNNING);
            Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

            seq.getOperator(0).m_eReturnStatus = status[i];
            Test.CHECK_EQUAL(bh.tick(), status[i]);
            Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
}