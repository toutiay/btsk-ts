"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("./../Enum/Status");
const Test_1 = require("../Test/Test");
const Default_1 = require("../Enum/Default");
class Behavior {
    constructor(node) {
        this.m_pTask = Default_1.Default.UNDEFINED;
        this.m_pNode = Default_1.Default.UNDEFINED;
        this.m_eStatus = Status_1.Status.BH_INVALID;
        node && this.setup(node);
    }
    setup(node) {
        this.teardown();
        this.m_pNode = node;
        this.m_pTask = node.create();
    }
    teardown() {
        if (this.m_pTask == null) {
            return;
        }
        Test_1.ASSERT(this.m_eStatus != Status_1.Status.BH_RUNNING);
        this.m_pNode.destroy(this.m_pTask);
        this.m_pTask = Default_1.Default.UNDEFINED;
    }
    tick() {
        if (this.m_eStatus == Status_1.Status.BH_INVALID) {
            this.m_pTask.onInitialize();
        }
        this.m_eStatus = this.m_pTask.update();
        if (this.m_eStatus != Status_1.Status.BH_RUNNING) {
            this.m_pTask.onTerminate(this.m_eStatus);
        }
        return this.m_eStatus;
    }
    get() {
        return this.m_pTask;
    }
}
exports.Behavior = Behavior;
//# sourceMappingURL=Behavior.js.map