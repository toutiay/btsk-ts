"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Enum_1 = require("../Enum");
class Task {
    constructor(node) {
        this.m_pNode = node;
    }
    update() {
        return Enum_1.Status.BH_INVALID;
    }
    onInitialize() { }
    onTerminate(s) { }
}
exports.Task = Task;
//# sourceMappingURL=Task.js.map