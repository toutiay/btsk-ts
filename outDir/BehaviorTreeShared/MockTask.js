"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("./../Enum/Status");
const Task_1 = require("./Task");
class MockTask extends Task_1.Task {
    constructor(node) {
        super(node);
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
exports.MockTask = MockTask;
//# sourceMappingURL=MockTask.js.map