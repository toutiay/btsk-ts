"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const BehaviorTree_1 = require("./BehaviorTree");
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
        let t = new BehaviorTree_1.MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    }
    static TEST_TaskUpdate() {
        let t = new BehaviorTree_1.MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    }
    static TEST_TaskTerminate() {
        let t = new BehaviorTree_1.MockBehavior();
        t.tick();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);
        t.m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
    static TEST_SequenceTwoChildrenFails() {
        let seq = new BehaviorTree_1.MockSequence(2);
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_FAILURE);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceTwoChildrenContinues() {
        let seq = new BehaviorTree_1.MockSequence(2);
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
        seq.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, seq.getOperator(1).m_iInitializeCalled);
    }
    static TEST_SequenceOneChildPassThrough() {
        let status = [BehaviorTree_1.Status.BH_SUCCESS, BehaviorTree_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            let seq = new BehaviorTree_1.MockSequence(1);
            Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
            Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
            seq.getOperator(0).m_eReturnStatus = status[i];
            Test.CHECK_EQUAL(seq.tick(), status[i]);
            Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }
    static TEST_SelectorTwoChildrenContinues() {
        let seq = new BehaviorTree_1.MockSelector(2);
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorTwoChildrenSucceeds() {
        let seq = new BehaviorTree_1.MockSelector(2);
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
        seq.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_SUCCESS);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }
    static TEST_SelectorOneChildPassThrough() {
        let status = [BehaviorTree_1.Status.BH_SUCCESS, BehaviorTree_1.Status.BH_FAILURE];
        for (let i = 0; i < status.length; i++) {
            {
                let seq = new BehaviorTree_1.MockSelector(1);
                Test.CHECK_EQUAL(seq.tick(), BehaviorTree_1.Status.BH_RUNNING);
                Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);
                seq.getOperator(0).m_eReturnStatus = status[i];
                Test.CHECK_EQUAL(seq.tick(), status[i]);
                Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
            }
        }
    }
    static TEST_ParallelSucceedRequireAll() {
        let parallel = new BehaviorTree_1.MockParallel(BehaviorTree_1.Policy.RequireAll, BehaviorTree_1.Policy.RequireOne);
        let children = [new BehaviorTree_1.MockBehavior(), new BehaviorTree_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelSucceedRequireOne() {
        let parallel = new BehaviorTree_1.MockParallel(BehaviorTree_1.Policy.RequireOne, BehaviorTree_1.Policy.RequireAll);
        let children = [new BehaviorTree_1.MockBehavior(), new BehaviorTree_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_SUCCESS, parallel.tick());
    }
    static TEST_ParallelFailureRequireAll() {
        let parallel = new BehaviorTree_1.MockParallel(BehaviorTree_1.Policy.RequireOne, BehaviorTree_1.Policy.RequireAll);
        let children = [new BehaviorTree_1.MockBehavior(), new BehaviorTree_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[1].m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ParallelFailureRequireOne() {
        let parallel = new BehaviorTree_1.MockParallel(BehaviorTree_1.Policy.RequireAll, BehaviorTree_1.Policy.RequireOne);
        let children = [new BehaviorTree_1.MockBehavior(), new BehaviorTree_1.MockBehavior()];
        parallel.addChild(children[0]);
        parallel.addChild(children[1]);
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_RUNNING, parallel.tick());
        children[0].m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        Test.CHECK_EQUAL(BehaviorTree_1.Status.BH_FAILURE, parallel.tick());
    }
    static TEST_ActiveBinarySelector() {
        let sel = new BehaviorTree_1.MockActiveSelector(2);
        sel.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_FAILURE;
        sel.getOperator(1).m_eReturnStatus = BehaviorTree_1.Status.BH_RUNNING;
        Test.CHECK_EQUAL(sel.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        Test.CHECK_EQUAL(sel.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(0, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_RUNNING;
        Test.CHECK_EQUAL(sel.tick(), BehaviorTree_1.Status.BH_RUNNING);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(2, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
        sel.getOperator(0).m_eReturnStatus = BehaviorTree_1.Status.BH_SUCCESS;
        Test.CHECK_EQUAL(sel.tick(), BehaviorTree_1.Status.BH_SUCCESS);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iInitializeCalled);
        Test.CHECK_EQUAL(3, sel.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iInitializeCalled);
        Test.CHECK_EQUAL(1, sel.getOperator(1).m_iTerminateCalled);
    }
}
exports.default = Test;
//# sourceMappingURL=Test.js.map