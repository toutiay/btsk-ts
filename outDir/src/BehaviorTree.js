"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
class Behavior {
    constructor() {
        this.m_eStatus = Status.BH_INVALID;
    }
    tick() {
        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onInitialize();
        }
        this.m_eStatus = this.update();
        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onTerminate(this.m_eStatus);
        }
        return this.m_eStatus;
    }
    onTerminate(m_eStatus) {
    }
    onInitialize() {
    }
    update() {
        return 0;
    }
    reset() {
        this.m_eStatus = Status.BH_INVALID;
    }
    abort() {
        this.onTerminate(Status.BH_ABORTED);
        this.m_eStatus = Status.BH_ABORTED;
    }
    isTerminated() {
        return this.m_eStatus == Status.BH_SUCCESS || this.m_eStatus == Status.BH_FAILURE;
    }
    isRunning() {
        return this.m_eStatus == Status.BH_RUNNING;
    }
    getStatus() {
        return this.m_eStatus;
    }
}
class Decorator extends Behavior {
    constructor(child) {
        super();
        this.m_pChild = child;
    }
}
class Repeat extends Decorator {
    constructor(child) {
        super(child);
        this.m_iLimit = 0;
        this.m_iCounter = 0;
    }
    setCount(count) {
        this.m_iLimit = count;
    }
    onInitialize() {
        this.m_iCounter = 0;
    }
    update() {
        while (1) {
            this.m_pChild.tick();
            if (this.m_pChild.getStatus() == Status.BH_RUNNING)
                break;
            if (this.m_pChild.getStatus() == Status.BH_FAILURE)
                return Status.BH_FAILURE;
            if (++this.m_iCounter == this.m_iLimit)
                return Status.BH_SUCCESS;
            this.m_pChild.reset();
        }
        return Status.BH_INVALID;
    }
}
class Composite extends Behavior {
    constructor() {
        super(...arguments);
        this.m_Children = [];
    }
    addChild(child) {
        this.m_Children.push(child);
    }
    removeChild(child) {
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
    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = new Behavior;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
    }
    update() {
        // Keep going until a child behavior says it's running.
        while (1) {
            let s = this.m_CurrentChild.tick();
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
    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_Current = new Behavior;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
    }
    update() {
        // Keep going until a child behavior says its running.
        while (1) {
            let s = this.m_Current.tick();
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
    constructor(forSuccess, forFailure) {
        super();
        this.m_eSuccessPolicy = forSuccess;
        this.m_eFailurePolicy = forFailure;
    }
    update() {
        let iSuccessCount = 0, iFailureCount = 0;
        for (let i = 0; i < this.m_Children.length; i++) {
            let b = this.m_Children[i];
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
            let b = this.m_Children[i];
            if (b.isRunning()) {
                b.abort();
            }
        }
    }
}
exports.Parallel = Parallel;
class Monitor extends Parallel {
    constructor() {
        super(Policy.RequireOne, Policy.RequireOne);
    }
    addCondition(condition) {
        this.m_Children.unshift(condition);
    }
    addAction(action) {
        this.m_Children.push(action);
    }
}
class ActiveSelector extends Selector {
    onInitialize() {
        this.m_Current = this.m_Children[this.m_Children.length];
    }
    update() {
        let previous = this.m_Current;
        super.onInitialize();
        let result = super.update();
        if (previous != this.m_Children[this.m_Children.length] && this.m_Current != previous) {
            previous.onTerminate(Status.BH_ABORTED);
        }
        return result;
    }
}
//********************************************需要导出的类 */
class Status {
}
Status.BH_INVALID = 1;
Status.BH_SUCCESS = 2;
Status.BH_FAILURE = 3;
Status.BH_RUNNING = 4;
Status.BH_ABORTED = 5;
exports.Status = Status;
class Policy {
}
Policy.RequireOne = 1;
Policy.RequireAll = 2;
exports.Policy = Policy;
class MockBehavior extends Behavior {
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
    onTerminate(s) {
        ++this.m_iTerminateCalled;
        this.m_eTerminateStatus = s;
    }
    update() {
        ++this.m_iUpdateCalled;
        return this.m_eReturnStatus;
    }
}
exports.MockBehavior = MockBehavior;
class MockSelector extends Selector {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockSelector = MockSelector;
class MockSequence extends Sequence {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockSequence = MockSequence;
class MockActiveSelector extends ActiveSelector {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockActiveSelector = MockActiveSelector;
function createSize(target, size) {
    for (let i = 0; i < size; i++) {
        target.m_Children.push(new MockBehavior);
    }
}
function getMock(target, index) {
    return target.m_Children[index];
}
//# sourceMappingURL=BehaviorTree.js.map