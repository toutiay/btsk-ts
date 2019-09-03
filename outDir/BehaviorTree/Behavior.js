"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("./Status");
class Behavior {
    constructor() {
        this.m_eStatus = Status_1.Status.BH_INVALID;
    }
    tick() {
        if (this.m_eStatus != Status_1.Status.BH_RUNNING) {
            this.onInitialize();
        }
        this.m_eStatus = this.update();
        if (this.m_eStatus != Status_1.Status.BH_RUNNING) {
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
        this.m_eStatus = Status_1.Status.BH_INVALID;
    }
    abort() {
        this.onTerminate(Status_1.Status.BH_ABORTED);
        this.m_eStatus = Status_1.Status.BH_ABORTED;
    }
    isTerminated() {
        return this.m_eStatus == Status_1.Status.BH_SUCCESS || this.m_eStatus == Status_1.Status.BH_FAILURE;
    }
    isRunning() {
        return this.m_eStatus == Status_1.Status.BH_RUNNING;
    }
    getStatus() {
        return this.m_eStatus;
    }
}
exports.Behavior = Behavior;
//# sourceMappingURL=Behavior.js.map