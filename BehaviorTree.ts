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

class Test {
    static CHECK(X: number) {
        if (!X) {
            console.log(X + " == ");
        }
    }

    static CHECK_EQUAL(X: number, Y: number) {
        if (X != Y) {
            console.log("(" + X + " == " + Y + ") ");
            console.log("expected:" + (X) + " actual:" + (Y) + " ");
        }
    }

    static test_MockBehavior_TaskInitialize() {
        let t = new MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iInitializeCalled);

        t.tick();
        Test.CHECK_EQUAL(1, t.m_iInitializeCalled);
    }

    static test_MockBehavior_TaskUpdate() {
        let t = new MockBehavior();
        Test.CHECK_EQUAL(0, t.m_iUpdateCalled);

        t.tick();
        Test.CHECK_EQUAL(1, t.m_iUpdateCalled);
    }

    static test_MockBehavior_TaskTerminate() {
        let t = new MockBehavior();
        t.tick();
        Test.CHECK_EQUAL(0, t.m_iTerminateCalled);

        t.m_eReturnStatus = Status.BH_SUCCESS;
        t.tick();
        Test.CHECK_EQUAL(1, t.m_iTerminateCalled);
    }
}

class Behavior {
    m_eStatus: number;

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

class MockBehavior extends Behavior {
    m_iInitializeCalled: number;
    m_iTerminateCalled: number;
    m_iUpdateCalled: number;
    m_eReturnStatus: number;
    m_eTerminateStatus: number;

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

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
    }

    update() {
        // Keep going until a child behavior says it's running.
        while (1) {
            let s: number = (this.m_CurrentChild).tick();

            // If the child fails, or keeps running, do the same.
            if (s != Status.BH_SUCCESS) {
                return s;
            }

            // Hit the end of the array, job done!
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.m_Children.length - 1) {
                return Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
        }
    }
}

class Selector extends Composite {
    m_CurrentIndex: number;
    m_Current: Behavior;
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
    }

    update(): number {
        // Keep going until a child behavior says its running.
        while (1) {
            let s: number = (this.m_Current).tick();

            // If the child succeeds, or keeps running, do the same.
            if (s != Status.BH_FAILURE) {
                return s;
            }

            this.m_CurrentIndex++;
            // Hit the end of the array, it didn't end well...
            if (this.m_CurrentIndex == this.m_Children.length - 1) {
                return Status.BH_FAILURE;
            }
            this.m_Current = this.m_Children[this.m_CurrentIndex];
        }
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
        this.m_Current = this.m_Children[this.m_Children.length - 1];
    }

    update() {
        let previous: Behavior = this.m_Current;

        super.onInitialize();

        let result: number = super.update();

        if (previous != this.m_Children[this.m_Children.length - 1] && this.m_Current != previous) {
            previous.onTerminate(Status.BH_ABORTED);
        }
        return result;
    }
}