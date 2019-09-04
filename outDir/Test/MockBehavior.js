"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("../BehaviorTree/Behavior");
const Status_1 = require("../Enum/Status");
class MockBehavior extends Behavior_1.Behavior {
    constructor() {
        super();
        this.m_iInitializeCalled = 0;
        this.m_iTerminateCalled = 0;
        this.m_iUpdateCalled = 0;
        this.m_eReturnStatus = Status_1.Status.BH_RUNNING;
        this.m_eTerminateStatus = Status_1.Status.BH_INVALID;
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
//# sourceMappingURL=MockBehavior.js.map