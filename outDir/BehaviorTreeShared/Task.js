"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("./../Enum/Status");
class Task {
    constructor(node) {
        this.m_pNode = node;
    }
    update() {
        return Status_1.Status.BH_INVALID;
    }
    onInitialize() { }
    onTerminate(s) { }
}
exports.Task = Task;
//# sourceMappingURL=Task.js.map