"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Utils_1 = require("../Utils");
const MockNode_1 = require("./MockNode");
const Composite_1 = require("./Composite");
function createClass(fname, TASK) {
    let c = class extends Composite_1.Composite {
        constructor(size) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockNode_1.MockNode);
            }
        }
        getOperator(index) {
            Utils_1.ASSERT(index < this.m_Children.length);
            let task = this.m_Children[index].m_pTask;
            Utils_1.ASSERT(task != null);
            return task;
        }
        create() {
            return new TASK(this);
        }
        destroy(t) {
        }
    };
    return c;
}
exports.createClass = createClass;
//# sourceMappingURL=MockComposite.js.map