class Status {
    static BH_INVALID: number = 1;
    static BH_SUCCESS: number = 2;
    static BH_FAILURE: number = 3;
    static BH_RUNNING: number = 4;
    static BH_ABORTED: number = 5;
}

class Policy {
    static RequireOne: number = 1;
    static RequireAll: number = 2;
}

class Behavior {
    m_eStatus: number;
    constructor() {
        this.m_eStatus = Status.BH_INVALID;
    }

    tick(): number {
        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onInitialize();
        }

        this.m_eStatus = this.update();

        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onTerminate(this.m_eStatus);
        }
        return this.m_eStatus;
    }

    onTerminate(m_eStatus: number) {

    }

    onInitialize() {

    }

    update(): number {
        return 0;
    }

    reset() {
        this.m_eStatus = Status.BH_INVALID;
    }

    abort() {
        this.onTerminate(Status.BH_ABORTED);
        this.m_eStatus = Status.BH_ABORTED;
    }

    isTerminated(): boolean {
        return this.m_eStatus == Status.BH_SUCCESS || this.m_eStatus == Status.BH_FAILURE;
    }

    isRunning(): boolean {
        return this.m_eStatus == Status.BH_RUNNING;
    }

    getStatus(): number {
        return this.m_eStatus;
    }
}

class Decorator extends Behavior {
    m_pChild: Behavior;
    constructor(child: Behavior) {
        super();
        this.m_pChild = child;
    }
}

class Repeat extends Decorator {
    m_iLimit: number;
    m_iCounter: number;

    constructor(child: Behavior) {
        super(child);
        this.m_iLimit = 0;
        this.m_iCounter = 0;
    }

    setCount(count: number) {
        this.m_iLimit = count;
    }

    onInitialize() {
        this.m_iCounter = 0;
    }

    update() {
        while (1) {
            this.m_pChild.tick();
            if (this.m_pChild.getStatus() == Status.BH_RUNNING) break;
            if (this.m_pChild.getStatus() == Status.BH_FAILURE) return Status.BH_FAILURE;
            if (++this.m_iCounter == this.m_iLimit) return Status.BH_SUCCESS;
            this.m_pChild.reset();
        }
        return Status.BH_INVALID;
    }
}

class Composite extends Behavior {
    m_Children: Array<Behavior> = [];

    addChild(child: Behavior) {
        this.m_Children.push(child);
    }

    removeChild(child: Behavior) {
        let index = this.m_Children.indexOf(child);
        if (index != -1) {
            this.m_Children.splice(index, 1);
        }
    }

    clearChildren() {
        this.m_Children.length = 0;
    }
}

class Sequence extends Composite {
    m_CurrentIndex: number;
    m_CurrentChild: Behavior;

    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = new Behavior;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
    }

    update(): number {
        // Keep going until a child behavior says it's running.
        while (1) {
            let s: number = this.m_CurrentChild.tick();

            // If the child fails, or keeps running, do the same.
            if (s != Status.BH_SUCCESS) {
                return s;
            }

            // Hit the end of the array, job done!
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
        }
        return Status.BH_FAILURE;
    }
}

class Selector extends Composite {
    m_CurrentIndex: number;
    m_Current: Behavior;

    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_Current = new Behavior;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
    }

    update(): number {
        // Keep going until a child behavior says its running.
        while (1) {
            let s: number = this.m_Current.tick();

            // If the child succeeds, or keeps running, do the same.
            if (s != Status.BH_FAILURE) {
                return s;
            }

            this.m_CurrentIndex++;
            // Hit the end of the array, it didn't end well...
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status.BH_FAILURE;
            }
            this.m_Current = this.m_Children[this.m_CurrentIndex];
        }
        return Status.BH_FAILURE;
    }
}

class Parallel extends Composite {
    m_eSuccessPolicy: number;
    m_eFailurePolicy: number;

    constructor(forSuccess: number, forFailure: number) {
        super();
        this.m_eSuccessPolicy = forSuccess;
        this.m_eFailurePolicy = forFailure;
    }

    update(): number {
        let iSuccessCount = 0, iFailureCount = 0;

        for (let i = 0; i < this.m_Children.length; i++) {
            let b: Behavior = this.m_Children[i];

            if (!b.isTerminated()) {
                b.tick();
            }
            if (b.getStatus() == Status.BH_SUCCESS) {
                ++iSuccessCount;
                if (this.m_eSuccessPolicy == Policy.RequireOne) {
                    return Status.BH_SUCCESS;
                }
            }

            if (b.getStatus() == Status.BH_FAILURE) {
                ++iFailureCount;
                if (this.m_eFailurePolicy == Policy.RequireOne) {
                    return Status.BH_FAILURE;
                }
            }
        }

        if (this.m_eFailurePolicy == Policy.RequireAll && iFailureCount == this.m_Children.length) {
            return Status.BH_FAILURE;
        }

        if (this.m_eSuccessPolicy == Policy.RequireAll && iSuccessCount == this.m_Children.length) {
            return Status.BH_SUCCESS;
        }

        return Status.BH_RUNNING;
    }

    onTerminate() {
        for (let i = 0; i < this.m_Children.length; i++) {
            let b: Behavior = this.m_Children[i];
            if (b.isRunning()) {
                b.abort();
            }
        }
    }
}

class Monitor extends Parallel {
    constructor() {
        super(Policy.RequireOne, Policy.RequireOne);
    }

    addCondition(condition: Behavior) {
        this.m_Children.unshift(condition);
    }

    addAction(action: Behavior) {
        this.m_Children.push(action);
    }
}

class ActiveSelector extends Selector {
    onInitialize() {
        this.m_Current = this.m_Children[this.m_Children.length];
    }

    update() {
        let previous: Behavior = this.m_Current;

        super.onInitialize();

        let result: number = super.update();

        if (previous != this.m_Children[this.m_Children.length] && this.m_Current != previous) {
            previous.onTerminate(Status.BH_ABORTED);
        }
        return result;
    }
}

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
        let seq: MockSequence = new MockSequence(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_FAILURE);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        Test.CHECK_EQUAL(0, seq.getOperator(1).m_iInitializeCalled);
    }

    static TEST_SequenceTwoChildrenContinues() {
        let seq: MockSequence = new MockSequence(2);

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
            let seq: MockSequence = new MockSequence(1);
            Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
            Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

            seq.getOperator(0).m_eReturnStatus = status[i];
            Test.CHECK_EQUAL(seq.tick(), status[i]);
            Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
        }
    }

    static TEST_SelectorTwoChildrenContinues() {
        let seq: MockSelector = new MockSelector(2);

        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(0, seq.getOperator(0).m_iTerminateCalled);

        seq.getOperator(0).m_eReturnStatus = Status.BH_FAILURE;
        Test.CHECK_EQUAL(seq.tick(), Status.BH_RUNNING);
        Test.CHECK_EQUAL(1, seq.getOperator(0).m_iTerminateCalled);
    }

    static TEST_SelectorTwoChildrenSucceeds() {
        let seq: MockSelector = new MockSelector(2);

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
                let seq: MockSelector = new MockSelector(1);

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
        let sel: MockActiveSelector = new MockActiveSelector(2);

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
}

class MockBehavior extends Behavior {
    m_iInitializeCalled: number;
    m_iTerminateCalled: number;
    m_iUpdateCalled: number;
    m_eReturnStatus: number;
    m_eTerminateStatus: number;

    constructor() {
        super();
        this.m_iInitializeCalled = 0;
        this.m_iTerminateCalled = 0;
        this.m_iUpdateCalled = 0;
        this.m_eReturnStatus = Status.BH_RUNNING;
        this.m_eTerminateStatus = Status.BH_INVALID;
    }

    onInitialize() {
        ++this.m_iInitializeCalled;
    }

    onTerminate(s: number) {
        ++this.m_iTerminateCalled;
        this.m_eTerminateStatus = s;
    }

    update() {
        ++this.m_iUpdateCalled;
        return this.m_eReturnStatus;
    }
}

function createInstance<T>(c: new () => T): T {
    return new c();
}

function createSize(target: Composite, size: number) {
    for (let i = 0; i < size; i++) {
        target.m_Children.push(new MockBehavior);
    }
}

function getMock(target: Composite, index: number): MockBehavior {
    Test.ASSERT(index < target.m_Children.length);
    return target.m_Children[index] as MockBehavior;
}

class MockSelector extends Selector {
    constructor(size: number) {
        super();
        createSize(this, size);
    }

    getOperator(index: number): MockBehavior {
        return getMock(this, index);
    }
}

class MockSequence extends Sequence {
    constructor(size: number) {
        super();
        createSize(this, size);
    }

    getOperator(index: number): MockBehavior {
        return getMock(this, index);
    }
}

class MockActiveSelector extends ActiveSelector {
    constructor(size: number) {
        super();
        createSize(this, size);
    }

    getOperator(index: number): MockBehavior {
        return getMock(this, index);
    }
}