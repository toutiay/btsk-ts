"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
const Enum_1 = require("../Enum");
class MockBehavior extends Behavior_1.Behavior {
    constructor() {
        super();
        this.m_iInitializeCalled = 0;
        this.m_iTerminateCalled = 0;
        this.m_iUpdateCalled = 0;
        this.m_eReturnStatus = Enum_1.Status.BH_RUNNING;
        this.m_eTerminateStatus = Enum_1.Status.BH_INVALID;
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