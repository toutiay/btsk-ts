"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Node_1 = require("./Node");
const MockTask_1 = require("./MockTask");
const Default_1 = require("../Enum/Default");
class MockNode extends Node_1.Node {
    constructor() {
        super();
        this.m_pTask = Default_1.Default.UNDEFINED;
    }
    create() {
        this.m_pTask = new MockTask_1.MockTask(this);
        return this.m_pTask;
    }
    destroy() {
    }
}
exports.MockNode = MockNode;
//# sourceMappingURL=MockNode.js.map